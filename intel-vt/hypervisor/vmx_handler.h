#pragma once

#include "vmm.h"
#include "vmx.h"
#include "sdm.h"
#include "vmexit.h"

void vmx_entrypoint();
unsigned __int32 vmx_launch_cpu();