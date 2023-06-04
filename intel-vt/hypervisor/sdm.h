#pragma once

#include "ia32.h"
#include <xmmintrin.h>

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

typedef struct _segment_information_t
{
    segment_selector selector;
    vmx_segment_access_rights access_rights;
    unsigned __int64 limit;
    unsigned __int64 base_address;
} segment_information_t;

typedef struct _vcpu_t
{
    __declspec(align(0x1000)) vmxon vmxon;
    __declspec(align(0x1000)) vmcs vmcs;
    __declspec(align(0x1000)) unsigned __int8 host_stack[0x6000];
    __declspec(align(0x1000)) segment_descriptor_interrupt_gate_64 host_idt[256];
    __declspec(align(0x1000)) segment_descriptor_32 host_gdt[4];
} vcpu_t;

typedef struct _vmm_context_t
{
	vcpu_t* vcpu_table;
	unsigned __int32 processor_count;
} vmm_context_t;