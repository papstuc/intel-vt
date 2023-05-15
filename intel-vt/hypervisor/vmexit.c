#include "vmexit.h"

#include <ntddk.h>

unsigned __int8 vmexit_handler(vmexit_guest_registers_t* registers)
{
	if (!registers)
	{
		return 1;
	}

	DbgBreakPoint();

	return 1;
}