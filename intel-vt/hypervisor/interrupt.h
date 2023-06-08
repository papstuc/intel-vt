#pragma once

#include "ia32.h"

void inject_interrupt(interruption_type type, exception_vector vector, unsigned __int32 error_code);