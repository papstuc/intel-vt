#include "vmx.h"
#include "ia32.h"

#include <intrin.h>
#include <ntstatus.h>

NTSTATUS vmx_locked_in_bios()
{
	ia32_feature_control_register feature_msr = { 0 };
	feature_msr.flags = __readmsr(IA32_FEATURE_CONTROL);

	return feature_msr.lock_bit ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

NTSTATUS vmx_supported()
{
	cpuid_eax_01 cpu_info = { 0 };
	__cpuid((__int32*)&cpu_info, 1);
	return cpu_info.cpuid_feature_information_ecx.virtual_machine_extensions ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

void vmx_enable()
{
	typedef union
	{
		unsigned __int64 all;
		struct
		{
			unsigned long low;
			long high;
		} split;
		struct
		{
			unsigned long low;
			long high;
		} u;
	} __cr_fixed;

	cr4 cr4;
	cr0 cr0;
	__cr_fixed cr_fixed;

	cr_fixed.all = __readmsr(IA32_VMX_CR0_FIXED0);
	cr0.flags = __readcr0();
	cr0.flags |= cr_fixed.split.low;
	cr_fixed.all = __readmsr(IA32_VMX_CR0_FIXED1);
	cr0.flags &= cr_fixed.split.low;
	__writecr0(cr0.flags);
	cr_fixed.all = __readmsr(IA32_VMX_CR4_FIXED0);
	cr4.flags = __readcr4();
	cr4.flags |= cr_fixed.split.low;
	cr_fixed.all = __readmsr(IA32_VMX_CR4_FIXED1);
	cr4.flags &= cr_fixed.split.low;
	__writecr4(cr4.flags);
}

void vmx_disable()
{
	cr4 cr4 = { 0 };
	ia32_feature_control_register feature_msr = { 0 };

	cr4.flags = __readcr4();
	cr4.vmx_enable = 0;
	__writecr4(cr4.flags);

	feature_msr.flags = __readmsr(IA32_FEATURE_CONTROL);
	feature_msr.enable_vmx_outside_smx = 1;
	__writemsr(IA32_FEATURE_CONTROL, feature_msr.flags);
}