#pragma once

#include <ntdef.h>

#include "sdm.h"

NTSTATUS initialize_vmm();
void terminate_vmm();
void initialize_logical_processor(void* guest_rsp);