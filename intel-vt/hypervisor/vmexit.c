#include "vmexit.h"
#include "vmm.h"
#include "vmx_utilities.h"
#include "log.h"
#include "ia32.h"

#include <intrin.h>
#include <ntddk.h>

unsigned __int8 vmexit_handler(vmexit_guest_registers_t* guest_registers)
{
    DbgBreakPoint();
    UNREFERENCED_PARAMETER(guest_registers);

    unsigned __int64 ExitReason = 0;
    __vmx_vmread(VMCS_EXIT_REASON, &ExitReason);

    unsigned __int64 ExitQualification = 0;
    __vmx_vmread(VMCS_EXIT_QUALIFICATION, &ExitQualification);

    DbgPrintEx(0, 0, "VM_EXIT_REASION 0x%x\n", ExitReason & 0xffff);
    DbgPrintEx(0, 0, "EXIT_QUALIFICATION 0x%x\n", ExitQualification);

    return 1;
}