#include "vmexit.h"
#include "vmm.h"
#include "vmx_utilities.h"
#include "log.h"
#include "ia32.h"

#include <intrin.h>
#include <ntddk.h>

static void adjust_rip(vcpu_t* cpu)
{
	unsigned __int64 instruction_length = vmread(VMCS_VMEXIT_INSTRUCTION_LENGTH);
	cpu->vmexit.guest_rip += instruction_length;
	__vmx_vmwrite(VMCS_GUEST_RIP, cpu->vmexit.guest_rip);
}

unsigned __int8 vmexit_handler(vmexit_guest_registers_t* guest_registers)
{
	UNREFERENCED_PARAMETER(guest_registers);

	__debugbreak();

	//vcpu_t* vcpu = vmm_find_vcpu(KeGetCurrentProcessorNumber());

	return 1;
}