#pragma once

#include "vmm.h"
#include "vmx.h"
#include "sdm.h"
#include "vmexit.h"

void vmx_entrypoint();
void vmx_save_state();
void vmx_restore_state();