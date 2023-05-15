#include "vmx_utilities.h"
#include "vmx_intrin.h"
#include "ia32.h"

#include <intrin.h>

unsigned __int32 read_segment_access_rights(unsigned __int16 segment)
{
    segment_selector selector = { 0 };
    vmx_segment_access_rights vmx_access_rights = { 0 };

    selector.flags = segment;

    if (selector.table == 0 && selector.index == 0)
    {
        vmx_access_rights.flags = 0;
        vmx_access_rights.unusable = 1;
        return vmx_access_rights.flags;
    }

    vmx_access_rights.flags = (__load_ar(segment) >> 8);
    vmx_access_rights.unusable = 0;
    vmx_access_rights.reserved1 = 0;
    vmx_access_rights.reserved2 = 0;

    return vmx_access_rights.flags;
}

unsigned __int64 read_segment_base(unsigned __int64 gdt_base, unsigned __int16 segment)
{
    segment_descriptor_64* segment_descriptor;

    segment_descriptor = (segment_descriptor_64*)(gdt_base + (segment & ~0x7));

    unsigned __int64 segment_base = segment_descriptor->base_address_low | segment_descriptor->base_address_middle << 16 | segment_descriptor->base_address_high << 24;

    if (segment_descriptor->descriptor_type == 0)
    {
        segment_base = (segment_base & 0xFFFFFFFF) | (unsigned __int64)segment_descriptor->base_address_upper << 32;
    }

    return segment_base;
}

unsigned __int64 vmread(unsigned __int64 vmcs_field)
{
    unsigned __int64 value = 0;
    __vmx_vmread(vmcs_field, &value);
    return value;
}