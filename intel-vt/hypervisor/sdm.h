#pragma once

#include "ia32.h"
#include <xmmintrin.h>

#if defined(_MSC_EXTENSIONS)
#pragma warning(push)
#pragma warning(disable: 4201)
#endif

typedef struct _vmexit_guest_registers_t
{
    __m128 xmm0;
    __m128 xmm1;
    __m128 xmm2;
    __m128 xmm3;
    __m128 xmm4;
    __m128 xmm5;
    __m128 xmm6;
    __m128 xmm7;
    __m128 xmm8;
    __m128 xmm9;
    __m128 xmm10;
    __m128 xmm11;
    __m128 xmm12;
    __m128 xmm13;
    __m128 xmm14;
    __m128 xmm15;

    unsigned __int64 padding;

    union
    {
        unsigned __int64 r15;
        unsigned __int32 r15d;
        unsigned __int16 r15w;
        unsigned __int8 r15b;
    };

    union
    {
        unsigned __int64 r14;
        unsigned __int32 r14d;
        unsigned __int16 r14w;
        unsigned __int8 r14b;
    };

    union
    {
        unsigned __int64 r13;
        unsigned __int32 r13d;
        unsigned __int16 r13w;
        unsigned __int8 r13b;
    };

    union
    {
        unsigned __int64 r12;
        unsigned __int32 r12d;
        unsigned __int16 r12w;
        unsigned __int8 r12b;
    };

    union
    {
        unsigned __int64 r11;
        unsigned __int32 r11d;
        unsigned __int16 r11w;
        unsigned __int8 r11b;
    };

    union
    {
        unsigned __int64 r10;
        unsigned __int32 r10d;
        unsigned __int16 r10w;
        unsigned __int8 r10b;
    };

    union
    {
        unsigned __int64 r9;
        unsigned __int32 r9d;
        unsigned __int16 r9w;
        unsigned __int8 r9b;
    };

    union
    {
        unsigned __int64 r8;
        unsigned __int32 r8d;
        unsigned __int16 r8w;
        unsigned __int8 r8b;
    };

    union
    {
        unsigned __int64 rbp;
        unsigned __int32 ebp;
        unsigned __int16 bp;
        unsigned __int8 bpl;
    };

    union
    {
        unsigned __int64 rdi;
        unsigned __int32 edi;
        unsigned __int16 di;
        unsigned __int8 dil;
    };

    union
    {
        unsigned __int64 rsi;
        unsigned __int32 esi;
        unsigned __int16 si;
        unsigned __int8 sil;
    };

    union
    {
        unsigned __int64 rdx;
        unsigned __int32 edx;
        unsigned __int16 dx;
        unsigned __int8 dl;
    };

    union
    {
        unsigned __int64 rcx;
        unsigned __int32 ecx;
        unsigned __int16 cx;
        unsigned __int8 cl;
    };

    union
    {
        unsigned __int64 rbx;
        unsigned __int32 ebx;
        unsigned __int16 bx;
        unsigned __int8 bl;
    };

    union
    {
        unsigned __int64 rax;
        unsigned __int32 eax;
        unsigned __int16 ax;
        unsigned __int8 al;
    };
} vmexit_guest_registers_t;

#if defined(_MSC_EXTENSIONS)
#pragma warning(pop)
#endif

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