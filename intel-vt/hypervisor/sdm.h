#pragma once

#include "ia32.h"

#define VMM_STACK_SIZE 0x6000
#define VMM_TAG 'VMM'

typedef struct _vmexit_guest_registers_t
{
    unsigned __int64 rax;
    unsigned __int64 rcx;
    unsigned __int64 rdx;
    unsigned __int64 rbx;
    unsigned __int64 rsp;
    unsigned __int64 rbp;
    unsigned __int64 rsi;
    unsigned __int64 rdi;
    unsigned __int64 r8;
    unsigned __int64 r9;
    unsigned __int64 r10;
    unsigned __int64 r11;
    unsigned __int64 r12;
    unsigned __int64 r13;
    unsigned __int64 r14;
    unsigned __int64 r15;
} vmexit_guest_registers_t;

typedef struct _vcpu_t
{
	vmcs* vmcs;
	unsigned __int64 vmcs_physical;

	vmxon* vmxon;
	unsigned __int64 vmxon_physical;

	vmx_msr_bitmap* msr_bitmap;
	unsigned __int64 msr_bitmap_physical;

	void* vmm_stack;
} vcpu_t;

typedef struct _vmm_context_t
{
	vcpu_t** vcpu_table;

	unsigned __int32 processor_count;
} vmm_context_t;