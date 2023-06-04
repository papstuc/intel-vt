#include "vmx_utilities.h"
#include "vmx_intrin.h"
#include "ia32.h"

#include <intrin.h>

static vmx_segment_access_rights find_segment_access_rights(segment_selector segment_selector)
{
	vmx_segment_access_rights vmx_access_rights = { 0 };

	if (segment_selector.table == 0 && segment_selector.index == 0)
	{
		vmx_access_rights.flags = 0;
		vmx_access_rights.unusable = 1;
		return vmx_access_rights;
	}

	vmx_access_rights.flags = (__load_ar(segment_selector.flags) >> 8);
	vmx_access_rights.unusable = 0;
	vmx_access_rights.reserved1 = 0;
	vmx_access_rights.reserved2 = 0;

	return vmx_access_rights;
}

static unsigned __int64 find_segment_base(segment_descriptor_register_64* gdtr, segment_selector segment_selector)
{
	if (segment_selector.index == 0)
	{
		return 0;
	}

	segment_descriptor_64* descriptor = (segment_descriptor_64*)(gdtr->base_address + (unsigned __int64)(segment_selector.index) * 8);

	unsigned __int64 base_address = (unsigned __int64)descriptor->base_address_low | ((unsigned __int64)descriptor->base_address_middle << 16) | ((unsigned __int64)descriptor->base_address_high << 24);

	if (descriptor->descriptor_type == SEGMENT_DESCRIPTOR_TYPE_SYSTEM)
	{
		base_address |= (unsigned __int64)descriptor->base_address_upper << 32;
	}

	return base_address;
}

segment_information_t find_segment_information(segment_descriptor_register_64* gdtr, unsigned __int16 segment)
{
    segment_information_t segment_information = { 0 };

	segment_selector segment_selector = { segment };

	segment_information.selector = segment_selector;
	segment_information.access_rights = find_segment_access_rights(segment_selector);
	segment_information.limit = __segmentlimit(segment_selector.flags);
	segment_information.base_address = find_segment_base(gdtr, segment_selector);

    return segment_information;
}

unsigned __int32 adjust_control_value(unsigned __int32 vmx_capability, unsigned __int64 value)
{
	ia32_vmx_true_ctls_register capabilities = { 0 };
	capabilities.flags = __readmsr(vmx_capability);

	unsigned __int32 effective_value = 0;

	effective_value = (unsigned __int32)value;

	effective_value |= capabilities.allowed_0_settings;
	effective_value &= capabilities.allowed_1_settings;

	return effective_value;
}