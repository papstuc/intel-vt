#pragma once

#include <ntddk.h>

void NTAPI KeGenericCallDpc(_In_ PKDEFERRED_ROUTINE Routine, PVOID Context);
void NTAPI KeSignalCallDpcDone(_In_ PVOID SystemArgument1);
BOOLEAN NTAPI KeSignalCallDpcSynchronize(_In_ PVOID SystemArgument2);

typedef struct _kprocess_t
{
    unsigned char pad1[0x28];
    unsigned __int64 directory_table_base;
} kprocess_t;

typedef struct _eprocess_t
{
    kprocess_t pcb;
} eprocess_t;
