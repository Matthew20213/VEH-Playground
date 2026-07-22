# API Hooking

## Overview

This proof of concept demonstrates how Windows Vectored Exception Handling (VEH) can be used to intercept execution of a Windows API without modifying its instructions.

The implementation applies the `PAGE_GUARD` memory protection to the `WinExec` API. When execution reaches the guarded page, Windows generates a `STATUS_GUARD_PAGE_VIOLATION` exception, allowing the registered vectored exception handler to inspect the current execution context before the API executes.

The purpose of this PoC is to demonstrate exception-driven API interception, a technique that can be extended to monitor, instrument, or alter API execution without directly patching the target function.

---

## Implementation

The application loads shellcode from disk into executable memory before registering a vectored exception handler.

The first byte of the `WinExec` function is then protected using `PAGE_GUARD`.

When the shellcode calls `WinExec`, execution triggers a guard-page violation before the API begins executing. The vectored exception handler verifies that the captured instruction pointer (`RIP`) corresponds to the address of `WinExec` before displaying a notification and allowing execution to continue.

---

## Code Walkthrough

### Loading the Payload

The shellcode is loaded from disk and copied into executable memory allocated with `VirtualAlloc()`.

A new thread is then created to execute the payload.

---

### Protecting the Target API

The target API is protected using:

```cpp
VirtualProtect(
    WinExec,
    1,
    PAGE_EXECUTE_READ | PAGE_GUARD,
    &old
);
```

Applying `PAGE_GUARD` causes Windows to generate a guard-page exception whenever execution first reaches the protected page.

---

### Identifying the Target API

Inside the vectored exception handler, the current instruction pointer is compared against the address of `WinExec`.

```cpp
if (ExceptionInfo->ContextRecord->Rip == DWORD64(WinExec))
```

If the instruction pointer matches, the handler confirms that execution has reached the target API.

---

### Continuing Execution

After processing the exception, the handler returns:

```cpp
EXCEPTION_CONTINUE_EXECUTION
```

Because `PAGE_GUARD` is automatically removed after the initial access, Windows retries the original instruction and allows the API to execute normally.

---

## Execution Flow

```
Load Shellcode
        │
        ▼
Allocate Executable Memory
        │
        ▼
Register VEH
        │
        ▼
Protect WinExec with PAGE_GUARD
        │
        ▼
Execute Shellcode
        │
        ▼
Shellcode Calls WinExec
        │
        ▼
STATUS_GUARD_PAGE_VIOLATION
        │
        ▼
Vectored Exception Handler
        │
        ▼
Verify RIP == WinExec
        │
        ▼
Return EXCEPTION_CONTINUE_EXECUTION
        │
        ▼
Windows Retries Instruction
        │
        ▼
WinExec Executes Normally
```

---

## Expected Output

https://github.com/user-attachments/assets/b1aeaf6a-92fd-4d54-8890-abcb48da600b

---

## Notes

Unlike traditional API hooking techniques, this implementation does not modify the target function or overwrite its instructions.

Instead, execution is intercepted by leveraging Windows memory protections and the exception dispatcher. Because the API remains unmodified, this approach can be useful for research into exception-driven execution flow, runtime instrumentation, and advanced memory monitoring techniques.

Since `PAGE_GUARD` is automatically cleared after the first access, this implementation intercepts only the initial execution of the protected page. Repeated interception would require reapplying the guard-page protection after each exception.

---

## References

- Microsoft – VirtualProtect
- Microsoft – PAGE_GUARD
- Microsoft – AddVectoredExceptionHandler
- Windows Internals