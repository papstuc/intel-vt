#include "vmm.h"
#include "vmx.h"
#include "vmcs.h"
#include "vmx_handler.h"
#include "sdm.h"
#include "log.h"
#include "ntapi.h"

#include <ntddk.h>

static vmm_context_t* vmm_context = 0;

static NTSTATUS initialize_context()
{
	vmm_context = ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(vmm_context_t), VMM_TAG);

	if (!vmm_context)
	{
		return STATUS_UNSUCCESSFUL;
	}

	RtlSecureZeroMemory(vmm_context, sizeof(vmm_context_t));

	vmm_context->processor_count = KeQueryActiveProcessorCount(0);
	vmm_context->vcpu_table = (vcpu_t**)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(vcpu_t*) * vmm_context->processor_count, VMM_TAG);

	if (!vmm_context->vcpu_table)
	{
		return STATUS_UNSUCCESSFUL;
	}

	RtlSecureZeroMemory(vmm_context->vcpu_table, sizeof(vcpu_t*) * vmm_context->processor_count);

	return STATUS_SUCCESS;
}

static NTSTATUS initialize_vcpu(vcpu_t** vcpu)
{
	vcpu_t* cpu = ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(vcpu_t), VMM_TAG);

	if (!cpu)
	{
		return STATUS_UNSUCCESSFUL;
	}

	RtlSecureZeroMemory(cpu, sizeof(vmm_context_t));

	cpu->vmm_stack = ExAllocatePool2(POOL_FLAG_NON_PAGED, VMM_STACK_SIZE, VMM_TAG);

	if (!cpu->vmm_stack)
	{
		return STATUS_UNSUCCESSFUL;
	}

	memset(cpu->vmm_stack, 0xCC, VMM_STACK_SIZE);

	cpu->msr_bitmap = ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(vmx_msr_bitmap), VMM_TAG);

	if (!cpu->msr_bitmap)
	{
		return STATUS_UNSUCCESSFUL;
	}

	RtlSecureZeroMemory(cpu->msr_bitmap, sizeof(vmx_msr_bitmap));

	cpu->msr_bitmap_physical = MmGetPhysicalAddress(cpu->msr_bitmap).QuadPart;

	if (!cpu->msr_bitmap_physical)
	{
		return STATUS_UNSUCCESSFUL;
	}

	*vcpu = cpu;

	return STATUS_SUCCESS;
}

static NTSTATUS initialize_vmcs(vcpu_t* vcpu)
{
	if (!vcpu)
	{
		return STATUS_UNSUCCESSFUL;
	}

	ia32_vmx_basic_register vmx_basic = { 0 };
	vmx_basic.flags = __readmsr(IA32_VMX_BASIC);

	PHYSICAL_ADDRESS physical_max = { 0 };
	physical_max.QuadPart = ~0ULL;

	vcpu->vmcs = MmAllocateContiguousMemory(PAGE_SIZE * 2, physical_max);

	if (!vcpu->vmcs)
	{
		return STATUS_UNSUCCESSFUL;
	}

	vcpu->vmcs_physical = MmGetPhysicalAddress(vcpu->vmcs).QuadPart;

	if (!vcpu->vmcs_physical)
	{
		return STATUS_UNSUCCESSFUL;
	}

	RtlSecureZeroMemory(vcpu->vmcs, PAGE_SIZE * 2);

	vcpu->vmcs->revision_id = (unsigned __int32)vmx_basic.vmcs_revision_id;
	vcpu->vmcs->shadow_vmcs_indicator = 0;

	return STATUS_SUCCESS;
}

static NTSTATUS initialize_vmxon(vcpu_t* vcpu)
{
	if (!vcpu)
	{
		return STATUS_UNSUCCESSFUL;
	}

	ia32_vmx_basic_register vmx_basic = { 0 };
	vmx_basic.flags = __readmsr(IA32_VMX_BASIC);

	PHYSICAL_ADDRESS physical_max = { 0 };
	physical_max.QuadPart = ~0ULL;

	vcpu->vmxon = MmAllocateContiguousMemory(PAGE_SIZE * 2, physical_max);

	if (!vcpu->vmxon)
	{
		return STATUS_UNSUCCESSFUL;
	}

	RtlSecureZeroMemory(vcpu->vmxon, PAGE_SIZE * 2);

	vcpu->vmxon_physical = MmGetPhysicalAddress(vcpu->vmxon).QuadPart;

	if (!vcpu->vmxon_physical)
	{
		return STATUS_UNSUCCESSFUL;
	}

	vcpu->vmxon->revision_id = (unsigned __int32)vmx_basic.vmcs_revision_id;

	return STATUS_SUCCESS;
}

static void dpc_broadcast_initialize_vmm(KDPC* dpc, void* deferred_context, void* sys_arg1, void* sys_arg2)
{
	UNREFERENCED_PARAMETER(dpc);
	UNREFERENCED_PARAMETER(deferred_context);

	vmx_save_state();

	KeSignalCallDpcSynchronize(sys_arg2);
	KeSignalCallDpcDone(sys_arg1);
}

NTSTATUS initialize_vmm()
{
	NTSTATUS status = initialize_context();

	if (!NT_SUCCESS(status))
	{
		log_error("failed to allocate vmm_context\n");
		terminate_vmm();
		return status;
	}

	log_success("vmm_context allocated at %llX\n", vmm_context);

	for (unsigned __int32 i = 0; i < vmm_context->processor_count; i++)
	{
		status = initialize_vcpu((vcpu_t**)&(vmm_context->vcpu_table[i]));

		if (!NT_SUCCESS(status))
		{
			log_error("failed to allocate vcpu for processor %d\n", i);
			terminate_vmm();
			return status;
		}

		log_success("vcpu %d allocated at %llX\n", i,vmm_context->vcpu_table[i]);

		log_success("msr bitmap %d allocated at %llX\n", i, vmm_context->vcpu_table[i]->msr_bitmap);

		status = initialize_vmcs(vmm_context->vcpu_table[i]);

		if (!NT_SUCCESS(status))
		{
			log_error("failed to allocate vmcs for processor %d\n", i);
			terminate_vmm();
			return status;
		}

		log_success("vmcs %d allocated at %llX\n", i, vmm_context->vcpu_table[i]);

		status = initialize_vmxon(vmm_context->vcpu_table[i]);

		if (!NT_SUCCESS(status))
		{
			log_error("failed to allocate vmxon for processor %d\n", i);
			terminate_vmm();
			return status;
		}

		log_success("vmxon %d allocated at %llX\n", i, vmm_context->vcpu_table[i]);
	}

	KeGenericCallDpc(dpc_broadcast_initialize_vmm, 0);

	return STATUS_SUCCESS;
}

void terminate_vmm()
{
	if (!vmm_context)
	{
		return;
	}

	if (!vmm_context->vcpu_table)
	{
		return;
	}

	for (unsigned __int32 i = 0; i < vmm_context->processor_count; i++)
	{
		if (vmm_context->vcpu_table[i] != 0)
		{
			if (vmm_context->vcpu_table[i]->vmxon != 0)
			{
				MmFreeContiguousMemory(vmm_context->vcpu_table[i]->vmxon);
			}

			if (vmm_context->vcpu_table[i]->vmcs != 0)
			{
				MmFreeContiguousMemory(vmm_context->vcpu_table[i]->vmcs);
			}

			if (vmm_context->vcpu_table[i]->msr_bitmap_physical != 0)
			{
				ExFreePoolWithTag(vmm_context->vcpu_table[i]->msr_bitmap, VMM_TAG);
			}

			if (vmm_context->vcpu_table[i]->vmm_stack != 0)
			{
				ExFreePoolWithTag(vmm_context->vcpu_table[i]->vmm_stack, VMM_TAG);
			}

			ExFreePoolWithTag(vmm_context->vcpu_table[i], VMM_TAG);

			vmm_context->vcpu_table[i] = 0;
		}
	}

	ExFreePoolWithTag(vmm_context->vcpu_table, VMM_TAG);
	ExFreePoolWithTag(vmm_context, VMM_TAG);

	vmm_context = 0;
}

void initialize_logical_processor(void* guest_rsp)
{
	unsigned __int64 processor_number = KeGetCurrentProcessorNumber();
	vcpu_t* vcpu = vmm_context->vcpu_table[processor_number];

	vmx_enable();

	if (__vmx_on(&vcpu->vmxon_physical) != 0)
	{
		log_error("failed to enable vmx_on\n");
		return;
	}

	if (!NT_SUCCESS(setup_vmcs(vcpu, guest_rsp)))
	{
		log_error("failed to setup vmcs\n");
		return;
	}

	if (__vmx_vmlaunch() != 0)
	{
		UINT64 FailureCode;
		__vmx_vmread(VMCS_VM_INSTRUCTION_ERROR, &FailureCode);
		DbgPrintEx(0, 0, "vm launch: 0x%llx\n", FailureCode);
	}


	__vmx_off();
}