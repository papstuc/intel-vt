#pragma once

#include <ntdef.h>

NTSTATUS vmx_locked_in_bios();
NTSTATUS vmx_supported();
void vmx_enable();
void vmx_disable();