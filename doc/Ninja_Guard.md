# Ninja Guard

## Overview

This proof of concept demonstrates how Windows Vectored Exception Handling (VEH) can be combined with `PAGE_GUARD` and execution context modification to transparently redirect API execution.

Rather than allowing execution to continue inside the original `WinExec` function, the vectored exception handler redirects execution to a custom implementation. This custom routine temporarily hides the executing payload by changing its memory protection to `PAGE_NOACCESS`, invokes the original API, and finally restores the payload's executable permissions.

The purpose of this PoC is to demonstrate exception-driven execution redirection and runtime payload protection using Windows memory management primitives.

---

## Implementation

The application loads an AES-encrypted, Base64-encoded shellcode payload into executable memory before decrypting and decoding it in memory.

A vectored exception handler is registered, and the `WinExec` API is protected using `PAGE_GUARD`.

When the shellcode invokes `WinExec`, Windows raises a `STATUS_GUARD_PAGE_VIOLATION` exception before the API executes. Instead of resuming execution at the original API, the handler modifies the processor context by replacing the current instruction pointer (`RIP`) with the address of a custom `MyWinExec()` routine.

The custom implementation temporarily changes the shellcode memory region to `PAGE_NOACCESS`, invokes the original `WinExec`, restores the original page permissions, and returns to the caller.

---

## Code Walkthrough

### Loading the Payload

The encrypted shellcode is loaded into executable memory using `VirtualAlloc()` before being decrypted and Base64-decoded directly in memory.

---

### Protecting the Target API

The `WinExec` function is protected using:

```cpp
VirtualProtect(
    WinExec,
    1,
    PAGE_EXECUTE_READ | PAGE_GUARD,
    &old
);
```

The first execution of `WinExec` therefore generates a `STATUS_GUARD_PAGE_VIOLATION`.

---

### Redirecting Execution

Inside the vectored exception handler, the current instruction pointer is verified against the address of `WinExec`.

Instead of returning to the original API, the handler redirects execution:

```cpp
ExceptionInfo->ContextRecord->Rip = DWORD64(&MyWinExec);
```

When Windows restores the modified processor context, execution resumes inside the custom implementation rather than inside `WinExec`.

---

### Temporarily Hiding the Payload

The custom `MyWinExec()` routine performs three operations:

1. Changes the shellcode memory region to `PAGE_NOACCESS`.
2. Invokes the original `WinExec`.
3. Restores the shellcode to `PAGE_EXECUTE_READ`.

During the execution of `WinExec`, the payload remains inaccessible until its original protection is restored.

---

## Execution Flow

```
Load Encrypted Shellcode
        │
        ▼
Decrypt & Decode Payload
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
Redirect RIP → MyWinExec
        │
        ▼
Protect Payload (PAGE_NOACCESS)
        │
        ▼
Call Original WinExec
        │
        ▼
Restore Payload Protection
        │
        ▼
Return to Caller
```

---

## Demonstration

https://youtu.be/e4wRpPtxdN8

---

## Notes

Unlike the previous proof of concept, this implementation does not simply observe execution reaching the target API. Instead, it actively redirects execution by modifying the processor context before the original function begins executing.

The custom implementation temporarily hides the executing payload by changing its memory protection to `PAGE_NOACCESS` while the target API executes. Once the API returns, the payload's executable permissions are restored, allowing execution to continue normally.

This proof of concept illustrates how exception dispatching, context modification, and memory protection can be combined to build more advanced runtime execution flows without modifying the target API itself.

---

## References

- Microsoft – AddVectoredExceptionHandler
- Microsoft – VirtualProtect
- Microsoft – Memory Protection Constants
- Windows Internals