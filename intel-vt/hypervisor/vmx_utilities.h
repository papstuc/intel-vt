#pragma once

#include "ia32.h"
#include "sdm.h"

segment_information_t find_segment_information(segment_descriptor_register_64* gdtr, unsigned __int16 segment);
unsigned __int32 adjust_control_value(unsigned __int32 vmx_capability, unsigned __int64 value);