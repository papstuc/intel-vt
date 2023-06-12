#include "vmexit.h"
#include "vmm.h"
#include "vmx_utilities.h"
#include "log.h"
#include "ia32.h"
#include "interrupt.h"

#include <intrin.h>
#include <ntddk.h>

static void adjust_rip()
{
    unsigned __int64 rip, instruction_length = 0;

    __vmx_vmread(VMCS_GUEST_RIP, &rip);
    __vmx_vmread(VMCS_VMEXIT_INSTRUCTION_LENGTH, &instruction_length);
    __vmx_vmwrite(VMCS_GUEST_RIP, rip + instruction_length);
}

static void handle_cpuid(vmexit_guest_registers_t* guest_registers)
{
    __int32 regs[4];
    __cpuidex(regs, guest_registers->eax, guest_registers->ecx);

    guest_registers->rax = regs[0];
    guest_registers->rbx = regs[1];
    guest_registers->rcx = regs[2];
    guest_registers->rdx = regs[3];
}

static NTSTATUS handle_xsetbv(vmexit_guest_registers_t* guest_registers)
{
    unsigned __int64 msr_value = ((unsigned __int64)guest_registers->edx << 32) | guest_registers->eax;

    __try
    {
        _xsetbv(guest_registers->ecx, msr_value);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        inject_interrupt(HARDWARE_EXCEPTION, GENERAL_PROTECTION, 0);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS handle_rdmsr(vmexit_guest_registers_t* guest_registers)
{
    __try
    {
        unsigned __int64 msr_value = __readmsr(guest_registers->ecx);

        guest_registers->rax = msr_value & 0xFFFFFFFF;
        guest_registers->rdx = msr_value >> 32;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        inject_interrupt(HARDWARE_EXCEPTION, GENERAL_PROTECTION, 0);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS handle_wrmsr(vmexit_guest_registers_t* guest_registers)
{
    unsigned __int64 msr_value = ((unsigned __int64)guest_registers->edx << 32) | guest_registers->eax;

    __try
    {
        __writemsr(guest_registers->ecx, msr_value);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        inject_interrupt(HARDWARE_EXCEPTION, GENERAL_PROTECTION, 0);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

void vmexit_handler(vmexit_guest_registers_t* guest_registers)
{
    unsigned __int64 vmexit_reason = 0;
    __vmx_vmread(VMCS_EXIT_REASON, &vmexit_reason);

    switch (vmexit_reason)
    {
        case VMX_EXIT_REASON_EXECUTE_CPUID:
            handle_cpuid(guest_registers);

            adjust_rip();
            break;
        case VMX_EXIT_REASON_EXECUTE_INVD:
            __wbinvd();

            adjust_rip();
            break;

        case VMX_EXIT_REASON_EXECUTE_XSETBV:
            if (NT_SUCCESS(handle_xsetbv(guest_registers)))
            {
                adjust_rip();
            }

            break;
        
        case VMX_EXIT_REASON_EXECUTE_RDMSR:
            if (NT_SUCCESS(handle_rdmsr(guest_registers)))
            {
                adjust_rip();
            }

            break;

        case VMX_EXIT_REASON_EXECUTE_WRMSR:
            if (NT_SUCCESS(handle_wrmsr(guest_registers)))
            {
                adjust_rip();
            }

            break;

        case VMX_EXIT_REASON_EXECUTE_RDTSCP:
            break;

        case VMX_EXIT_REASON_EXECUTE_INVPCID:
            break;

        case VMX_EXIT_REASON_EXECUTE_XSAVES:
            break;

        case VMX_EXIT_REASON_EXECUTE_XRSTORS:
            break;

        case VMX_EXIT_REASON_EXECUTE_INVEPT:
        case VMX_EXIT_REASON_EXECUTE_INVVPID:
        case VMX_EXIT_REASON_EXECUTE_VMFUNC:
        case VMX_EXIT_REASON_EXECUTE_VMCALL:
        case VMX_EXIT_REASON_EXECUTE_VMCLEAR:
        case VMX_EXIT_REASON_EXECUTE_VMLAUNCH:
        case VMX_EXIT_REASON_EXECUTE_VMPTRLD:
        case VMX_EXIT_REASON_EXECUTE_VMPTRST:
        case VMX_EXIT_REASON_EXECUTE_VMREAD:
        case VMX_EXIT_REASON_EXECUTE_VMRESUME:
        case VMX_EXIT_REASON_EXECUTE_VMWRITE:
        case VMX_EXIT_REASON_EXECUTE_VMXOFF:
        case VMX_EXIT_REASON_EXECUTE_VMXON:
            inject_interrupt(HARDWARE_EXCEPTION, INVALID_OPCODE, 0);
            break;
        default:
            break;
    }
}