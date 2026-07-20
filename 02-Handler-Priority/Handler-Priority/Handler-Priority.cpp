#include <iostream>
#include <Windows.h>

// Handler inserted at the front of the VEH chain.
LONG WINAPI Handler1(EXCEPTION_POINTERS*)
{
    printf("Handler 1 called\n");

    // Continue traversing the remaining handlers.
    return EXCEPTION_CONTINUE_SEARCH;
}

// Handler inserted at the back of the VEH chain.
LONG WINAPI Handler2(EXCEPTION_POINTERS*)
{
    printf("Handler 2 called\n");

    // Continue traversing the remaining handlers.
    return EXCEPTION_CONTINUE_SEARCH;
}

// Another handler inserted at the front of the chain.
// Since it is registered after Handler1, it will be invoked first.
LONG WINAPI Handler3(EXCEPTION_POINTERS*)
{
    printf("Handler 3 called\n");

    return EXCEPTION_CONTINUE_SEARCH;
}

// Final handler that terminates the exception dispatch.
LONG WINAPI Handler4(EXCEPTION_POINTERS*)
{
    printf("Handler 4 called\n");

    // Stop searching and resume execution.
    return EXCEPTION_CONTINUE_EXECUTION;
}


int main()
{
    // Register handlers at different positions within the VEH chain.
    PVOID h1 = AddVectoredExceptionHandler(1, Handler1);
    PVOID h2 = AddVectoredExceptionHandler(0, Handler2);
    PVOID h3 = AddVectoredExceptionHandler(1, Handler3);
    PVOID h4 = AddVectoredExceptionHandler(0, Handler4);

	// Trigger an access violation exception to test the handlers.
    RaiseException(STATUS_ACCESS_VIOLATION, 0, 0, nullptr);

	// Clean up by removing the handlers from the VEH chain.
    RemoveVectoredExceptionHandler(h1);
    RemoveVectoredExceptionHandler(h2);
    RemoveVectoredExceptionHandler(h3);
    RemoveVectoredExceptionHandler(h4);

    return 0;
}