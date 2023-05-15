#include "log.h"

#include <ntddk.h>
#include <stdarg.h>

void log_debug(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vDbgPrintExWithPrefix("[*] ", DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, fmt, args);
	va_end(args);
}

void log_success(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vDbgPrintExWithPrefix("[+] ", DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, fmt, args);
	va_end(args);
}

void log_error(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vDbgPrintExWithPrefix("[-] ", DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, fmt, args);
	va_end(args);
}