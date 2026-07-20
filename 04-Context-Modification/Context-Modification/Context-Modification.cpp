#include <iostream>
#include "windows.h"

LONG WINAPI handler(struct _EXCEPTION_POINTERS* ExceptionInfo)
{
	// Ignore exceptions that do not belong to this demonstration.
	if (ExceptionInfo->ExceptionRecord->ExceptionCode != STATUS_ACCESS_VIOLATION)
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}
	
	printf("RCX value before modification: 0x%llx\n", ExceptionInfo->ContextRecord->Rcx);

	// Modify the context of the exception
	// Change the value of RCX register to 0x1337
	ExceptionInfo->ContextRecord->Rcx = 0x1337; 
	printf("Modified RCX to: 0x%llx\n", ExceptionInfo->ContextRecord->Rcx);

	// Continue execution after modifying the context
	return EXCEPTION_CONTINUE_EXECUTION; 
}

int main()
{
	// Register the vectored exception handler
	PVOID handlerHandle = AddVectoredExceptionHandler(1, handler);

	// Generate an access violation exception to test the handler
	RaiseException(STATUS_ACCESS_VIOLATION, 0, 0, nullptr);

	// Unregister the vectored exception handler
	RemoveVectoredExceptionHandler(handlerHandle);
	return 0;
}
