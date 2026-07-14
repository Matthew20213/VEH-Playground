#include <iostream>
#include "windows.h"

void CustomHandler() {
    printf("Hello from Custom Handler");

    // Exit the process since execution was redirected here
    ExitProcess(0);
}

LONG WINAPI handler(struct _EXCEPTION_POINTERS* ExceptionInfo) {

    // Ignore exceptions that do not belong to this demonstration.
    if (ExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_ACCESS_VIOLATION)
    {
        printf("Current RIP value: %#llx\n", ExceptionInfo->ContextRecord->Rip);

        // Redirect execution by replacing the saved instruction pointer in the captured processor context.
        ExceptionInfo->ContextRecord->Rip = reinterpret_cast<DWORD64>(&CustomHandler);

        // Resume execution using the modified processor context.
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

int main()
{
    // Register the vectored exception handler.
    PVOID handlerHandle = AddVectoredExceptionHandler(1, handler);

    //Generate an exception to trigger the vectored exception handler
    RaiseException(STATUS_ACCESS_VIOLATION, 0, 0, nullptr);

    // This line should not execute because RIP is redirected.
    printf("Execution resumed normally!");

    // Remove the vectored exception handler
    RemoveVectoredExceptionHandler(handlerHandle);
}

