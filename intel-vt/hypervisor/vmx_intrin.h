#pragma once

#include "ia32.h"

#include <intrin.h>

void _sgdt(segment_descriptor_register_64* gdtr);
void _lgdt(segment_descriptor_register_64* gdtr);

unsigned __int16 __read_ldtr(void);
unsigned __int16 __read_tr(void);
unsigned __int16 __read_cs(void);
unsigned __int16 __read_ss(void);
unsigned __int16 __read_ds(void);
unsigned __int16 __read_es(void);
unsigned __int16 __read_fs(void);
unsigned __int16 __read_gs(void);
unsigned __int32 __load_ar(unsigned __int16);