#pragma once

#include <ntddk.h>

typedef struct _kprocess_t
{
    unsigned char pad1[0x28];
    unsigned __int64 directory_table_base;
} kprocess_t;

typedef struct _eprocess_t
{
    kprocess_t pcb;
} eprocess_t;
