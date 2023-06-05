#include "vmcs.h"
#include "vmx_intrin.h"
#include "vmx_handler.h"
#include "vmx_utilities.h"
#include "log.h"
#include "ntapi.h"

#include <ntddk.h>
#include <intrin.h>

static NTSTATUS load_vmcs(vcpu_t* vcpu)
{
	unsigned __int64 vmcs_physical = MmGetPhysicalAddress(&vcpu->vmcs).QuadPart;
	if (__vmx_vmclear(&vmcs_physical) != 0)
	{
		return STATUS_UNSUCCESSFUL;
	}

	if (__vmx_vmptrld(&vmcs_physical) != 0)
	{
		return STATUS_UNSUCCESSFUL;
	}

	return STATUS_SUCCESS;
}

static NTSTATUS setup_host(segment_descriptor_register_64* gdtr, segment_descriptor_register_64* idtr, unsigned __int64 host_rsp, unsigned __int64 host_rip)
{
	__vmx_vmwrite(VMCS_HOST_CR0, __readcr0());
	__vmx_vmwrite(VMCS_HOST_CR3, ((eprocess_t*)PsInitialSystemProcess)->pcb.directory_table_base);
	__vmx_vmwrite(VMCS_HOST_CR4, __readcr4());

	__vmx_vmwrite(VMCS_HOST_RSP, host_rsp);
	__vmx_vmwrite(VMCS_HOST_RIP, host_rip);

	segment_information_t es = find_segment_information(gdtr, __read_es());
	segment_information_t cs = find_segment_information(gdtr, __read_cs());
	segment_information_t ss = find_segment_information(gdtr, __read_ss());
	segment_information_t ds = find_segment_information(gdtr, __read_ds());
	segment_information_t fs = find_segment_information(gdtr, __read_fs());
	segment_information_t gs = find_segment_information(gdtr, __read_gs());
	segment_information_t tr = find_segment_information(gdtr, __read_tr());

	__vmx_vmwrite(VMCS_HOST_FS_BASE, __readmsr(IA32_FS_BASE));
	__vmx_vmwrite(VMCS_HOST_GS_BASE, __readmsr(IA32_GS_BASE));
	__vmx_vmwrite(VMCS_HOST_TR_BASE, tr.base_address);
	__vmx_vmwrite(VMCS_HOST_GDTR_BASE, gdtr->base_address);
	__vmx_vmwrite(VMCS_HOST_IDTR_BASE, idtr->base_address);

	__vmx_vmwrite(VMCS_HOST_SYSENTER_CS, __readmsr(IA32_SYSENTER_CS));
	__vmx_vmwrite(VMCS_HOST_SYSENTER_ESP, __readmsr(IA32_SYSENTER_ESP));
	__vmx_vmwrite(VMCS_HOST_SYSENTER_EIP, __readmsr(IA32_SYSENTER_EIP));

	__vmx_vmwrite(VMCS_HOST_ES_SELECTOR, es.selector.flags & 0xF8);
	__vmx_vmwrite(VMCS_HOST_CS_SELECTOR, cs.selector.flags & 0xF8);
	__vmx_vmwrite(VMCS_HOST_SS_SELECTOR, ss.selector.flags & 0xF8);
	__vmx_vmwrite(VMCS_HOST_DS_SELECTOR, ds.selector.flags & 0xF8);
	__vmx_vmwrite(VMCS_HOST_FS_SELECTOR, fs.selector.flags & 0xF8);
	__vmx_vmwrite(VMCS_HOST_GS_SELECTOR, gs.selector.flags & 0xF8);
	__vmx_vmwrite(VMCS_HOST_TR_SELECTOR, tr.selector.flags & 0xF8);

	unsigned __int64 error_code = 0;
	__vmx_vmread(VMCS_VM_INSTRUCTION_ERROR, &error_code);
	return (error_code == 0) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

static NTSTATUS setup_guest(segment_descriptor_register_64* gdtr, segment_descriptor_register_64* idtr)
{
	UNREFERENCED_PARAMETER(idtr);
	__vmx_vmwrite(VMCS_GUEST_CR0, __readcr0());
	__vmx_vmwrite(VMCS_GUEST_CR3, __readcr3());
	__vmx_vmwrite(VMCS_GUEST_CR4, __readcr4());
	__vmx_vmwrite(VMCS_GUEST_DR7, __readdr(7));

	__vmx_vmwrite(VMCS_GUEST_RSP, 0);
	__vmx_vmwrite(VMCS_GUEST_RIP, 0);

	__vmx_vmwrite(VMCS_GUEST_RFLAGS, __readeflags());

	segment_information_t es = find_segment_information(gdtr, __read_es());
	segment_information_t cs = find_segment_information(gdtr, __read_cs());
	segment_information_t ss = find_segment_information(gdtr, __read_ss());
	segment_information_t ds = find_segment_information(gdtr, __read_ds());
	segment_information_t fs = find_segment_information(gdtr, __read_fs());
	segment_information_t gs = find_segment_information(gdtr, __read_gs());
	segment_information_t tr = find_segment_information(gdtr, __read_tr());
	segment_information_t ldtr = find_segment_information(gdtr, __read_ldtr());

	__vmx_vmwrite(VMCS_GUEST_ES_SELECTOR, es.selector.flags);
	__vmx_vmwrite(VMCS_GUEST_CS_SELECTOR, cs.selector.flags);
	__vmx_vmwrite(VMCS_GUEST_SS_SELECTOR, ss.selector.flags);
	__vmx_vmwrite(VMCS_GUEST_DS_SELECTOR, ds.selector.flags);
	__vmx_vmwrite(VMCS_GUEST_FS_SELECTOR, fs.selector.flags);
	__vmx_vmwrite(VMCS_GUEST_GS_SELECTOR, gs.selector.flags);
	__vmx_vmwrite(VMCS_GUEST_TR_SELECTOR,tr.selector.flags);
	__vmx_vmwrite(VMCS_GUEST_LDTR_SELECTOR, ldtr.selector.flags);

	__vmx_vmwrite(VMCS_GUEST_ES_LIMIT, es.limit);
	__vmx_vmwrite(VMCS_GUEST_CS_LIMIT, cs.limit);
	__vmx_vmwrite(VMCS_GUEST_SS_LIMIT, ss.limit);
	__vmx_vmwrite(VMCS_GUEST_DS_LIMIT, ds.limit);
	__vmx_vmwrite(VMCS_GUEST_FS_LIMIT, fs.limit);
	__vmx_vmwrite(VMCS_GUEST_GS_LIMIT, gs.limit);
	__vmx_vmwrite(VMCS_GUEST_TR_LIMIT, tr.limit);
	__vmx_vmwrite(VMCS_GUEST_LDTR_LIMIT, ldtr.limit);
	__vmx_vmwrite(VMCS_GUEST_GDTR_LIMIT, gdtr->limit);
	__vmx_vmwrite(VMCS_GUEST_IDTR_LIMIT, idtr->limit);

	__vmx_vmwrite(VMCS_GUEST_ES_ACCESS_RIGHTS, es.access_rights.flags);
	__vmx_vmwrite(VMCS_GUEST_CS_ACCESS_RIGHTS, cs.access_rights.flags);
	__vmx_vmwrite(VMCS_GUEST_SS_ACCESS_RIGHTS, ss.access_rights.flags);
	__vmx_vmwrite(VMCS_GUEST_DS_ACCESS_RIGHTS, ds.access_rights.flags);
	__vmx_vmwrite(VMCS_GUEST_FS_ACCESS_RIGHTS, fs.access_rights.flags);
	__vmx_vmwrite(VMCS_GUEST_GS_ACCESS_RIGHTS, gs.access_rights.flags);
	__vmx_vmwrite(VMCS_GUEST_TR_ACCESS_RIGHTS, tr.access_rights.flags);
	__vmx_vmwrite(VMCS_GUEST_LDTR_ACCESS_RIGHTS, ldtr.access_rights.flags);

	__vmx_vmwrite(VMCS_GUEST_ES_BASE, es.base_address);
	__vmx_vmwrite(VMCS_GUEST_CS_BASE, cs.base_address);
	__vmx_vmwrite(VMCS_GUEST_SS_BASE, ss.base_address);
	__vmx_vmwrite(VMCS_GUEST_DS_BASE, ds.base_address);
	__vmx_vmwrite(VMCS_GUEST_FS_BASE, __readmsr(IA32_FS_BASE));
	__vmx_vmwrite(VMCS_GUEST_GS_BASE, __readmsr(IA32_GS_BASE));
	__vmx_vmwrite(VMCS_GUEST_TR_BASE, tr.base_address);
	__vmx_vmwrite(VMCS_GUEST_LDTR_BASE, ldtr.base_address);
	__vmx_vmwrite(VMCS_GUEST_GDTR_BASE, gdtr->base_address);
	__vmx_vmwrite(VMCS_GUEST_IDTR_BASE, idtr->base_address);

	__vmx_vmwrite(VMCS_GUEST_SYSENTER_CS, __readmsr(IA32_SYSENTER_CS));
	__vmx_vmwrite(VMCS_GUEST_SYSENTER_ESP, __readmsr(IA32_SYSENTER_ESP));
	__vmx_vmwrite(VMCS_GUEST_SYSENTER_EIP, __readmsr(IA32_SYSENTER_EIP));
	__vmx_vmwrite(VMCS_GUEST_DEBUGCTL, __readmsr(IA32_DEBUGCTL));

	__vmx_vmwrite(VMCS_GUEST_VMCS_LINK_POINTER, MAXULONG64);
	__vmx_vmwrite(VMCS_GUEST_INTERRUPTIBILITY_STATE, 0);
	__vmx_vmwrite(VMCS_GUEST_ACTIVITY_STATE, 0);
	__vmx_vmwrite(VMCS_GUEST_PENDING_DEBUG_EXCEPTIONS, 0);

	unsigned __int64 error_code = 0;
	__vmx_vmread(VMCS_VM_INSTRUCTION_ERROR, &error_code);
	return (error_code == 0) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

static NTSTATUS setup_controls()
{
	ia32_vmx_basic_register basic_msr = { 0 };
	basic_msr.flags = __readmsr(IA32_VMX_BASIC);

	ia32_vmx_pinbased_ctls_register pinbased_ctls = { 0 };
	pinbased_ctls.flags = adjust_control_value(basic_msr.vmx_controls ? IA32_VMX_TRUE_PINBASED_CTLS : IA32_VMX_PINBASED_CTLS, pinbased_ctls.flags);

	ia32_vmx_entry_ctls_register entry_ctls = { 0 };
	entry_ctls.ia32e_mode_guest = 1;
	entry_ctls.flags = adjust_control_value(basic_msr.vmx_controls ? IA32_VMX_TRUE_ENTRY_CTLS : IA32_VMX_ENTRY_CTLS, entry_ctls.flags);

	ia32_vmx_exit_ctls_register exit_ctls = { 0 };
	exit_ctls.host_address_space_size = 1;
	exit_ctls.flags = adjust_control_value(basic_msr.vmx_controls ? IA32_VMX_TRUE_EXIT_CTLS : IA32_VMX_EXIT_CTLS, exit_ctls.flags);

	ia32_vmx_procbased_ctls_register primary_ctls = { 0 };
	primary_ctls.use_msr_bitmaps = 1;
	primary_ctls.activate_secondary_controls = 1;
	primary_ctls.hlt_exiting = 1;
	primary_ctls.flags = adjust_control_value(basic_msr.vmx_controls ? IA32_VMX_TRUE_PROCBASED_CTLS : IA32_VMX_PROCBASED_CTLS, primary_ctls.flags);

	ia32_vmx_procbased_ctls2_register secondary_ctls = { 0 };
	secondary_ctls.flags = 0;
	secondary_ctls.enable_rdtscp = 1;
	secondary_ctls.enable_invpcid = 1;
	secondary_ctls.enable_xsaves = 1;
	//secondary_ctls.enable_ept = 1;
	secondary_ctls.flags = adjust_control_value(IA32_VMX_PROCBASED_CTLS2, secondary_ctls.flags);

	__vmx_vmwrite(VMCS_CTRL_PIN_BASED_VM_EXECUTION_CONTROLS, pinbased_ctls.flags);
	__vmx_vmwrite(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, primary_ctls.flags);
	__vmx_vmwrite(VMCS_CTRL_SECONDARY_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, secondary_ctls.flags);
	__vmx_vmwrite(VMCS_CTRL_VMENTRY_CONTROLS, entry_ctls.flags);
	__vmx_vmwrite(VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS, exit_ctls.flags);
	__vmx_vmwrite(VMCS_CTRL_MSR_BITMAP_ADDRESS, 0);
	__vmx_vmwrite(VMCS_CTRL_VMENTRY_MSR_LOAD_COUNT, 0);
	//__vmx_vmwrite(VMCS_CTRL_EPT_POINTER, 0);

	unsigned __int64 error_code = 0;
	__vmx_vmread(VMCS_VM_INSTRUCTION_ERROR, &error_code);
	return (error_code == 0) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

NTSTATUS setup_vmcs(vcpu_t* vcpu)
{
	NTSTATUS status = load_vmcs(vcpu);

	if (!NT_SUCCESS(status))
	{
		log_error("failed to load vmcs for vcpu\n");
		return status;
	}

	segment_descriptor_register_64 gdtr = { 0 };
	segment_descriptor_register_64 idtr = { 0 };

	_sgdt(&gdtr);
	__sidt(&idtr);

	status = setup_host(&gdtr, &idtr, (unsigned __int64)(vcpu->host_stack) + 0x6000, (unsigned __int64)vmx_entrypoint);

	if (!NT_SUCCESS(status))
	{
		log_error("failed to setup host vmcs fields\n");
		return status;
	}

	status = setup_guest(&gdtr, &idtr);

	if (!NT_SUCCESS(status))
	{
		log_error("failed to setup guest vmcs fields\n");
		return status;
	}

	status = setup_controls();

	if (!NT_SUCCESS(status))
	{
		log_error("failed to setup control vmcs fields\n");
		return status;
	}

	return status;
}