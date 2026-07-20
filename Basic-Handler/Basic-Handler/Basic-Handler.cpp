#include <iostream>
#include "windows.h"

// Vectored Exception Handler.
// Windows invokes this callback before SEH whenever an exception occurs
// is dispatched to the process.

LONG WINAPI handler(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
	printf("Exception code: 0x%X\n", ExceptionInfo->ExceptionRecord->ExceptionCode);

    // Handle only access violations generated in this PoC.
    if (ExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_ACCESS_VIOLATION)
    {
        printf("Access Violation!\n");
	}

    // Inform the dispatcher that the exception has been handled
    // and execution may continue.
    return EXCEPTION_CONTINUE_EXECUTION;
}

int main()
{
    // Register the handler with high priority.
    AddVectoredExceptionHandler(1, handler);

	// Generate an access violation exception.
	RaiseException(STATUS_ACCESS_VIOLATION, 0, 0, nullptr);
}
