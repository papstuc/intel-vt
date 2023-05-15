#include <ntddk.h>

#include "dispatch.h"
#include "vmx.h"
#include "vmm.h"
#include "log.h"

static PDEVICE_OBJECT device_object = { 0 };
static UNICODE_STRING dev, dos = { 0 };

static void unload(PDRIVER_OBJECT driver_object)
{
	terminate_vmm();

	IoDeleteSymbolicLink(&dos);
	IoDeleteDevice(driver_object->DeviceObject);
}

NTSTATUS driver_entry(PDRIVER_OBJECT driver_object, PUNICODE_STRING registry_path)
{
	UNREFERENCED_PARAMETER(registry_path);

	RtlInitUnicodeString(&dev, L"\\Device\\hypervisor");
	RtlInitUnicodeString(&dos, L"\\DosDevices\\hypervisor");

	NTSTATUS status = IoCreateDevice(driver_object, 0, &dev, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &device_object);

	if (!NT_SUCCESS(status))
	{
		log_error("failed to create device! NTSTATUS: %ld\n", status);
		return status;
	}

	status = IoCreateSymbolicLink(&dos, &dev);

	if (!NT_SUCCESS(status))
	{
		log_error("failed to create symbolic link! NTSTATUS: %ld\n", status);
		return status;
	}

	driver_object->MajorFunction[IRP_MJ_CREATE] = create_call;
	driver_object->MajorFunction[IRP_MJ_CLOSE] = close_call;
	driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL] = handle_call;
	driver_object->DriverUnload = unload;

	status = vmx_locked_in_bios();

	if (!NT_SUCCESS(status))
	{
		log_error("virtual machine extensions not enabled in bios!\n");
		return status;
	}

	status = vmx_supported();

	if (!NT_SUCCESS(status))
	{
		log_error("virtual machine extensions not supported!\n");
		return status;
	}

	status = initialize_vmm();

	if (!NT_SUCCESS(status))
	{
		log_error("failed to initialize vmm!\n");
		return status;
	}

	return status;
}