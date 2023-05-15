#include "dispatch.h"

NTSTATUS handle_call(PDEVICE_OBJECT device_object, PIRP irp)
{
	UNREFERENCED_PARAMETER(device_object);

	NTSTATUS status = STATUS_INVALID_PARAMETER;

	unsigned __int64 bytes_io = 0;

	PIO_STACK_LOCATION irp_stack = IoGetCurrentIrpStackLocation(irp);

	unsigned __int64 ioctl = irp_stack->Parameters.DeviceIoControl.IoControlCode;

	if (ioctl == HV_START)
	{

	}
	else if (ioctl == HV_STOP)
	{

	}
	else if (ioctl == HV_STATUS)
	{

	}
	else
	{
		status = STATUS_INVALID_PARAMETER;
		bytes_io = 0;
	}

	irp->IoStatus.Status = status;
	irp->IoStatus.Information = bytes_io;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return status;
}

NTSTATUS create_call(PDEVICE_OBJECT device_object, PIRP irp)
{
	UNREFERENCED_PARAMETER(device_object);

	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS close_call(PDEVICE_OBJECT device_object, PIRP irp)
{
	UNREFERENCED_PARAMETER(device_object);

	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}