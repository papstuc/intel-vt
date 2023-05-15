#pragma once

#include "ia32.h"
#include <xmmintrin.h>

#define VMM_STACK_SIZE 0x6000
#define VMM_TAG 'VMM'

#define RST_CNT_IO_PORT 0xCF9

#if defined(_MSC_EXTENSIONS)
#pragma warning(push)
#pragma warning(disable: 4201)
#endif

typedef union _reset_control_register_t
{
    unsigned __int8 flags;
    struct
    {
        unsigned __int8 reserved0 : 1;
        unsigned __int8 system_reset : 1;
        unsigned __int8 reset_cpu : 1;
        unsigned __int8 full_reset : 1;
        unsigned __int8 reserved1 : 4;
    };
} reset_control_register_t;

typedef struct _vmexit_guest_registers_t
{
    __m128 xmm[6];
    unsigned __int64 r15;
    unsigned __int64 r14;
    unsigned __int64 r13;
    unsigned __int64 r12;
    unsigned __int64 r11;
    unsigned __int64 r10;
    unsigned __int64 r9;
    unsigned __int64 r8;
    unsigned __int64 rdi;
    unsigned __int64 rsi;
    unsigned __int64 rbp;
    unsigned __int64 rsp;
    unsigned __int64 rbx;
    unsigned __int64 rdx;
    unsigned __int64 rcx;
    unsigned __int64 rax;
} vmexit_guest_registers_t;

typedef struct _cpuid_t
{
    unsigned __int64 rax;
    unsigned __int64 rbx;
    unsigned __int64 rcx;
    unsigned __int64 rdx;
} cpuid_t;

#if defined(_MSC_EXTENSIONS)
#pragma warning(pop)
#endif

typedef struct _vmexit_t
{
    vmexit_guest_registers_t* guest_registers;
    unsigned __int64 guest_rip;

    rflags guest_rflags;

    unsigned __int64 instruction_length;
    unsigned __int64 reason;
    unsigned __int64 qualification;
    unsigned __int64 instruction_information;
} vmexit_t;

typedef struct vmxoff_t
{
    unsigned __int8 vmx_off_executed;
    unsigned __int64 guest_rip;
    unsigned __int64 guest_rsp;
} vmxoff_t;

typedef struct _vmstatus_t
{
    unsigned __int8 vmx_on;
    unsigned __int8 vmm_launched;
} vmstatus_t;

typedef struct _vcpu_t
{
	vmcs* vmcs;
	unsigned __int64 vmcs_physical;

	vmxon* vmxon;
	unsigned __int64 vmxon_physical;

	vmx_msr_bitmap* msr_bitmap;
	unsigned __int64 msr_bitmap_physical;

	void* vmm_stack;

    vmstatus_t vmstatus;
    vmxoff_t vmxoff;
    vmexit_t vmexit;
} vcpu_t;

typedef struct _vmm_context_t
{
	vcpu_t** vcpu_table;

	unsigned __int32 processor_count;
} vmm_context_t;