#pragma once

#include <ntdef.h>

#include "sdm.h"

NTSTATUS initialize_vmm();
void terminate_vmm();
void initialize_logical_processor(void* guest_rsp);

unsigned __int64 vmxoff_find_rsp();
unsigned __int64 vmxoff_find_rip();
vcpu_t* vmm_find_vcpu(unsigned __int32 processor_number);