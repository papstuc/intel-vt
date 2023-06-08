#include "vmm.h"
#include "vmx.h"
#include "vmcs.h"
#include "vmx_handler.h"
#include "sdm.h"
#include "log.h"
#include "ntapi.h"

#include <intrin.h>
#include <ntddk.h>

#define VMM_TAG 'VMM'

static vmm_context_t vmm_context = { 0 };

static void initialize_logical_processor()
{
	unsigned __int64 processor_number = KeGetCurrentProcessorNumber();
	vcpu_t* vcpu = &vmm_context.vcpu_table[processor_number];

	vmx_enable();

	unsigned __int64 vmxon_physical = MmGetPhysicalAddress(&vcpu->vmxon).QuadPart;
	if (__vmx_on(&vmxon_physical) != 0)
	{
		log_error("failed to enable vmx_on\n");
		return;
	}

	if (!NT_SUCCESS(setup_vmcs(vcpu)))
	{
		log_error("failed to setup vmcs\n");
		return;
	}

	if (vmx_launch_cpu() != 0xDEADBEEF)
	{
		unsigned __int64 vmxerror;
		__vmx_vmread(VMCS_VM_INSTRUCTION_ERROR, &vmxerror);

		log_error("vmerror: %d\n", vmxerror);
		__vmx_off();

		vmx_disable();
	}
}

static void initialize_vmxon(vcpu_t* vcpu)
{
	if (!vcpu)
	{
		return;
	}

	ia32_vmx_basic_register vmx_basic = { 0 };
	vmx_basic.flags = __readmsr(IA32_VMX_BASIC);

	vcpu->vmxon.revision_id = (unsigned __int32)vmx_basic.vmcs_revision_id;
	vcpu->vmxon.must_be_zero = 0;
}

static void initialize_vmcs(vcpu_t* vcpu)
{
	if (!vcpu)
	{
		return;
	}

	ia32_vmx_basic_register vmx_basic = { 0 };
	vmx_basic.flags = __readmsr(IA32_VMX_BASIC);

	vcpu->vmcs.revision_id = (unsigned __int32)vmx_basic.vmcs_revision_id;
	vcpu->vmcs.shadow_vmcs_indicator = 0;
}

NTSTATUS initialize_vmm()
{
	vmm_context.processor_count = KeQueryActiveProcessorCount(0);

	unsigned __int32 array_size = sizeof(vcpu_t) * vmm_context.processor_count;

	vmm_context.vcpu_table = (vcpu_t*)ExAllocatePool2(POOL_FLAG_NON_PAGED, array_size, VMM_TAG);

	if (!vmm_context.vcpu_table)
	{
		return STATUS_UNSUCCESSFUL;
	}

	RtlZeroMemory(vmm_context.vcpu_table, array_size);

	for (unsigned __int32 i = 0; i < vmm_context.processor_count; i++)
	{
		vcpu_t* vcpu = &vmm_context.vcpu_table[i];
		initialize_vmxon(vcpu);
		initialize_vmcs(vcpu);
	}

	log_success("allocated %u vcpus with %u bytes at 0x%x", vmm_context.processor_count, array_size, vmm_context.vcpu_table);
	KeIpiGenericCall((PKIPI_BROADCAST_WORKER)&initialize_logical_processor, 0);

	return STATUS_SUCCESS;
}

void terminate_vmm()
{
	if (vmm_context.vcpu_table)
	{
		ExFreePoolWithTag(vmm_context.vcpu_table, VMM_TAG);
	}

	vmm_context.vcpu_table = 0;
	vmm_context.processor_count = 0;
}