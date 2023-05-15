#include "vmcs.h"
#include "vmx_intrin.h"
#include "vmx_handler.h"
#include "vmx_utilities.h"
#include "log.h"
#include "ntapi.h"

#include <ntddk.h>
#include <intrin.h>

static unsigned __int64 adjust_cv(unsigned __int32 capability_msr, unsigned __int64 value)
{
	ia32_vmx_true_ctls_register cap;

	unsigned __int64 actual;

	cap.flags = __readmsr(capability_msr);
	actual = value;

	actual |= cap.allowed_0_settings;
	actual &= cap.allowed_1_settings;

	return actual;
}

static void adjust_entry_controls(ia32_vmx_entry_ctls_register* entry_controls)
{
	unsigned __int32 capability_msr;

	ia32_vmx_basic_register vmx_basic = { 0 };
	vmx_basic.flags = __readmsr(IA32_VMX_BASIC);

	capability_msr = (vmx_basic.vmx_controls != FALSE) ? IA32_VMX_TRUE_ENTRY_CTLS : IA32_VMX_ENTRY_CTLS;

	entry_controls->flags = adjust_cv(capability_msr, entry_controls->flags);
}

static void adjust_exit_controls(ia32_vmx_exit_ctls_register* exit_controls)
{
	unsigned __int32 capability_msr;

	ia32_vmx_basic_register vmx_basic = { 0 };
	vmx_basic.flags = __readmsr(IA32_VMX_BASIC);

	capability_msr = (vmx_basic.vmx_controls != FALSE) ? IA32_VMX_TRUE_EXIT_CTLS : IA32_VMX_EXIT_CTLS;
	exit_controls->flags = adjust_cv(capability_msr, exit_controls->flags);
}

static void adjust_pinbased_controls(ia32_vmx_pinbased_ctls_register* pinbased_controls)
{
	unsigned __int32 capability_msr;

	ia32_vmx_basic_register vmx_basic = { 0 };
	vmx_basic.flags = __readmsr(IA32_VMX_BASIC);

	capability_msr = (vmx_basic.vmx_controls != FALSE) ? IA32_VMX_TRUE_PINBASED_CTLS : IA32_VMX_PINBASED_CTLS;
	pinbased_controls->flags = adjust_cv(capability_msr, pinbased_controls->flags);
}

static void adjust_primary_controls(ia32_vmx_procbased_ctls_register* primary_controls)
{
	unsigned __int32 capability_msr;

	ia32_vmx_basic_register vmx_basic = { 0 };
	vmx_basic.flags = __readmsr(IA32_VMX_BASIC);

	capability_msr = (vmx_basic.vmx_controls != FALSE) ? IA32_VMX_TRUE_PROCBASED_CTLS : IA32_VMX_PROCBASED_CTLS;
	primary_controls->flags = adjust_cv(capability_msr, primary_controls->flags);
}

static void adjust_secondary_controls(ia32_vmx_procbased_ctls2_register* secondary_controls)
{
	unsigned __int32 capability_msr;

	ia32_vmx_basic_register vmx_basic = { 0 };
	vmx_basic.flags = __readmsr(IA32_VMX_BASIC);

	capability_msr = IA32_VMX_PROCBASED_CTLS2;
	secondary_controls->flags = adjust_cv(capability_msr, secondary_controls->flags);
}

static NTSTATUS load_vmcs(vcpu_t* vcpu)
{
	if (__vmx_vmclear(&vcpu->vmcs_physical) != 0)
	{
		return STATUS_UNSUCCESSFUL;
	}

	if (__vmx_vmptrld(&vcpu->vmcs_physical) != 0)
	{
		return STATUS_UNSUCCESSFUL;
	}

	return STATUS_SUCCESS;
}

NTSTATUS setup_vmcs(vcpu_t* vcpu, void* guest_rsp)
{
	NTSTATUS status = load_vmcs(vcpu);

	if (!NT_SUCCESS(status))
	{
		log_error("failed to load vmcs for vcpu\n");
		return status;
	}

	segment_descriptor_register_64 gdtr = { 0 };
	segment_descriptor_register_64 idtr = { 0 };

	__sgdt(&gdtr);
	__sidt(&idtr);

	__vmx_vmwrite(VMCS_GUEST_GDTR_LIMIT, gdtr.limit);
	__vmx_vmwrite(VMCS_GUEST_IDTR_LIMIT, idtr.limit);
	__vmx_vmwrite(VMCS_GUEST_GDTR_BASE, gdtr.base_address);
	__vmx_vmwrite(VMCS_GUEST_IDTR_BASE, idtr.base_address);
	__vmx_vmwrite(VMCS_GUEST_LDTR_BASE, read_segment_base(gdtr.base_address, __read_ldtr()));
	__vmx_vmwrite(VMCS_GUEST_TR_BASE, read_segment_base(gdtr.base_address, __read_tr()));

	__vmx_vmwrite(VMCS_HOST_TR_BASE, read_segment_base(gdtr.base_address, __read_tr()));
	__vmx_vmwrite(VMCS_HOST_GDTR_BASE, gdtr.base_address);
	__vmx_vmwrite(VMCS_HOST_IDTR_BASE, idtr.base_address);
	__vmx_vmwrite(VMCS_HOST_FS_BASE, __readmsr(IA32_FS_BASE));
	__vmx_vmwrite(VMCS_HOST_GS_BASE, __readmsr(IA32_GS_BASE));

	__vmx_vmwrite(VMCS_HOST_CR0, __readcr0());
	__vmx_vmwrite(VMCS_HOST_CR4, __readcr4());

	__vmx_vmwrite(VMCS_GUEST_CR0, __readcr0());
	__vmx_vmwrite(VMCS_GUEST_CR3, __readcr3());
	__vmx_vmwrite(VMCS_GUEST_CR4, __readcr4());

	__vmx_vmwrite(VMCS_GUEST_DR7, __readdr(7));

	__vmx_vmwrite(VMCS_GUEST_RSP, (unsigned __int64)guest_rsp);
	__vmx_vmwrite(VMCS_GUEST_RIP, (unsigned __int64)vmx_restore_state);
	__vmx_vmwrite(VMCS_HOST_RSP, (unsigned __int64)vcpu->vmm_stack + VMM_STACK_SIZE);
	__vmx_vmwrite(VMCS_HOST_RIP, (unsigned __int64)vmx_entrypoint);

	__vmx_vmwrite(VMCS_GUEST_RFLAGS, __readeflags());
	__vmx_vmwrite(VMCS_GUEST_DEBUGCTL, __readmsr(IA32_DEBUGCTL));
	__vmx_vmwrite(VMCS_GUEST_SYSENTER_ESP, __readmsr(IA32_SYSENTER_ESP));
	__vmx_vmwrite(VMCS_GUEST_SYSENTER_EIP, __readmsr(IA32_SYSENTER_EIP));
	__vmx_vmwrite(VMCS_GUEST_SYSENTER_CS, __readmsr(IA32_SYSENTER_CS));
	__vmx_vmwrite(VMCS_GUEST_VMCS_LINK_POINTER, MAXUINT64);
	__vmx_vmwrite(VMCS_GUEST_FS_BASE, __readmsr(IA32_FS_BASE));
	__vmx_vmwrite(VMCS_GUEST_GS_BASE, __readmsr(IA32_GS_BASE));

	__vmx_vmwrite(VMCS_GUEST_EFER, __readmsr(IA32_EFER));
	__vmx_vmwrite(VMCS_HOST_SYSENTER_CS, __readmsr(IA32_SYSENTER_CS));
	__vmx_vmwrite(VMCS_HOST_SYSENTER_ESP, __readmsr(IA32_SYSENTER_ESP));
	__vmx_vmwrite(VMCS_HOST_SYSENTER_EIP, __readmsr(IA32_SYSENTER_EIP));
	__vmx_vmwrite(VMCS_HOST_EFER, __readmsr(IA32_EFER));

	__vmx_vmwrite(VMCS_CTRL_CR0_READ_SHADOW, __readcr0());
	__vmx_vmwrite(VMCS_CTRL_CR4_READ_SHADOW, __readcr4() & ~0x2000);
	__vmx_vmwrite(VMCS_CTRL_CR4_GUEST_HOST_MASK, 0x2000);

	__vmx_vmwrite(VMCS_GUEST_CS_SELECTOR, __read_cs());
	__vmx_vmwrite(VMCS_GUEST_SS_SELECTOR, __read_ss());
	__vmx_vmwrite(VMCS_GUEST_DS_SELECTOR, __read_ds());
	__vmx_vmwrite(VMCS_GUEST_ES_SELECTOR, __read_es());
	__vmx_vmwrite(VMCS_GUEST_FS_SELECTOR, __read_fs());
	__vmx_vmwrite(VMCS_GUEST_GS_SELECTOR, __read_fs());
	__vmx_vmwrite(VMCS_GUEST_LDTR_SELECTOR, __read_ldtr());
	__vmx_vmwrite(VMCS_GUEST_TR_SELECTOR, __read_tr());

	__vmx_vmwrite(VMCS_GUEST_CS_LIMIT, __segmentlimit(__read_cs()));
	__vmx_vmwrite(VMCS_GUEST_SS_LIMIT, __segmentlimit(__read_ss()));
	__vmx_vmwrite(VMCS_GUEST_DS_LIMIT, __segmentlimit(__read_ds()));
	__vmx_vmwrite(VMCS_GUEST_ES_LIMIT, __segmentlimit(__read_es()));
	__vmx_vmwrite(VMCS_GUEST_FS_LIMIT, __segmentlimit(__read_fs()));
	__vmx_vmwrite(VMCS_GUEST_GS_LIMIT, __segmentlimit(__read_fs()));
	__vmx_vmwrite(VMCS_GUEST_LDTR_LIMIT, __segmentlimit(__read_ldtr()));
	__vmx_vmwrite(VMCS_GUEST_TR_LIMIT, __segmentlimit(__read_tr()));

	__vmx_vmwrite(VMCS_GUEST_CS_ACCESS_RIGHTS, read_segment_access_rights(__read_cs()));
	__vmx_vmwrite(VMCS_GUEST_SS_ACCESS_RIGHTS, read_segment_access_rights(__read_ss()));
	__vmx_vmwrite(VMCS_GUEST_DS_ACCESS_RIGHTS, read_segment_access_rights(__read_ds()));
	__vmx_vmwrite(VMCS_GUEST_ES_ACCESS_RIGHTS, read_segment_access_rights(__read_es()));
	__vmx_vmwrite(VMCS_GUEST_FS_ACCESS_RIGHTS, read_segment_access_rights(__read_fs()));
	__vmx_vmwrite(VMCS_GUEST_GS_ACCESS_RIGHTS, read_segment_access_rights(__read_gs()));
	__vmx_vmwrite(VMCS_GUEST_LDTR_ACCESS_RIGHTS, read_segment_access_rights(__read_ldtr()));
	__vmx_vmwrite(VMCS_GUEST_TR_ACCESS_RIGHTS, read_segment_access_rights(__read_tr()));

	__vmx_vmwrite(VMCS_HOST_CS_SELECTOR, __read_cs() & 0xF8);
	__vmx_vmwrite(VMCS_HOST_SS_SELECTOR, __read_ss() & 0xF8);
	__vmx_vmwrite(VMCS_HOST_DS_SELECTOR, __read_ds() & 0xF8);
	__vmx_vmwrite(VMCS_HOST_ES_SELECTOR, __read_es() & 0xF8);
	__vmx_vmwrite(VMCS_HOST_FS_SELECTOR, __read_fs() & 0xF8);
	__vmx_vmwrite(VMCS_HOST_GS_SELECTOR, __read_gs() & 0xF8);
	__vmx_vmwrite(VMCS_HOST_TR_SELECTOR, __read_tr() & 0xF8);

	__vmx_vmwrite(VMCS_HOST_CR3, ((eprocess_t*)PsInitialSystemProcess)->pcb.directory_table_base);
	__vmx_vmwrite(VMCS_CTRL_CR3_TARGET_COUNT, 0);

	__vmx_vmwrite(VMCS_GUEST_VMCS_LINK_POINTER, ~0ULL);

	ia32_vmx_entry_ctls_register entry_controls = { 0 };
	entry_controls.ia32e_mode_guest = 1;
	adjust_entry_controls(&entry_controls);

	ia32_vmx_exit_ctls_register exit_controls = { 0 };
	exit_controls.host_address_space_size = 1;
	adjust_exit_controls(&exit_controls);

	ia32_vmx_pinbased_ctls_register pinbased_controls = { 0 };
	adjust_pinbased_controls(&pinbased_controls);

	ia32_vmx_procbased_ctls_register primary_controls = { 0 };
	primary_controls.use_msr_bitmaps = 1;
	primary_controls.activate_secondary_controls = 1;
	adjust_primary_controls(&primary_controls);

	ia32_vmx_procbased_ctls2_register secondary_controls = { 0 };
	secondary_controls.enable_rdtscp = 1;
	secondary_controls.enable_xsaves = 1;
	secondary_controls.enable_invpcid = 1;
	adjust_secondary_controls(&secondary_controls);

	__vmx_vmwrite(VMCS_CTRL_PIN_BASED_VM_EXECUTION_CONTROLS, pinbased_controls.flags);
	__vmx_vmwrite(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, primary_controls.flags);
	__vmx_vmwrite(VMCS_CTRL_SECONDARY_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, secondary_controls.flags);
	__vmx_vmwrite(VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS, exit_controls.flags);
	__vmx_vmwrite(VMCS_CTRL_VMENTRY_CONTROLS, entry_controls.flags);
	__vmx_vmwrite(VMCS_CTRL_MSR_BITMAP_ADDRESS, vcpu->msr_bitmap_physical);
	__vmx_vmwrite(VMCS_CTRL_VMENTRY_MSR_LOAD_COUNT, 0);

	__vmx_vmwrite(VMCS_CTRL_VIRTUAL_PROCESSOR_IDENTIFIER, 1);

	unsigned __int64 error_code = 0;
	__vmx_vmread(VMCS_VM_INSTRUCTION_ERROR, &error_code);
	
	status = error_code == 0 ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;

	return status;
}