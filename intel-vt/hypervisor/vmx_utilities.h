#pragma once

unsigned __int32 read_segment_access_rights(unsigned __int16 segment);
unsigned __int64 read_segment_base(unsigned __int64 gdt_base, unsigned __int16 segment);

unsigned __int64 vmread(unsigned __int64 vmcs_field);
