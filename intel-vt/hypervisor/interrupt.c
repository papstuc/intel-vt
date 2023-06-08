#include "interrupt.h"

void inject_interrupt(interruption_type type, exception_vector vector, unsigned __int32 error_code)
{
	vmentry_interrupt_information interrupt = { 0 };
	interrupt.interruption_type = type;
	interrupt.vector = vector;
	interrupt.valid = 1;
	interrupt.deliver_error_code = 1;

	__vmx_vmwrite(VMCS_VMEXIT_INTERRUPTION_ERROR_CODE, error_code);
	__vmx_vmwrite(VMCS_CTRL_VMENTRY_INTERRUPTION_INFORMATION_FIELD, interrupt.flags);
}