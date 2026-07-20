# VEH-Playground – Vectored Exception Handling Techniques

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Windows](https://img.shields.io/badge/Platform-Windows-blue)](https://www.microsoft.com/windows)

> A comprehensive collection of proof‑of‑concept loaders demonstrating the use of Windows Vectored Exception Handlers (VEH) for stealthy shellcode execution, lazy decryption, API hooking, and execution flow manipulation.

---

## What is VEH?

Vectored Exception Handling (VEH) is a Windows mechanism that allows applications to register **global, priority‑based exception handlers**. Unlike Structured Exception Handling (SEH), VEH handlers:

- Are called **before** any debugger or SEH handler.
- Can be registered with **priority** (first/last).
- Provide access to the **full CPU context** (`CONTEXT` structure).
- Can **modify execution flow** by altering `RIP`, registers, or memory.

**Why does this matter for evasion?**  
VEH allows us to intercept exceptions like `PAGE_GUARD` or `PAGE_NOACCESS` violations and perform actions (decryption, redirection, hooking) **transparently** – the shellcode doesn't know it's being monitored.

---

## PoCs Overview

| # | PoC | Description | Key Technique |
| :---: | :--- | :--- | :--- |
| 01 | **Basic Handler** | Minimal VEH registration and exception handling. | `AddVectoredExceptionHandler` |
| 02 | **Handler Priority** | Demonstrates VEH chain order and priority. | Priority flags (1 = first, 0 = last) |
| 03 | **Exception Information** | Extracts exception record and CPU context. | `ExceptionRecord` + `ContextRecord` |
| 04 | **Context Modification** | Modifies CPU registers (RCX) inside the handler. | `ContextRecord->Rcx` |
| 05 | **PAGE_GUARD Handling** | One‑shot decryption on `PAGE_GUARD` violation. | `PAGE_GUARD` + VEH |
| 06 | **PAGE_NOACCESS Handling** | Persistent decryption on `PAGE_NOACCESS` violation. | `PAGE_NOACCESS` + VEH |
| 07 | **API Hooking** | Hooks `WinExec` with `PAGE_GUARD` and redirects execution. | `PAGE_GUARD` on API + VEH |
| 08 | **Ninja Guard** | Intercepts `WinExec`, hides shellcode during API call. | Guard + redirect + hide |
| 09 | **NOACCESS Blinker** | Loop: encrypt → NOACCESS → decrypt on access → re‑encrypt. | AES + Base64 + `PAGE_NOACCESS` |
| 10 | **PAGE Lazy Decryption** | Per‑page decryption + re‑encryption using Trap Flag. | `PAGE_GUARD` + `TF` + single‑step |

---

## Building & Running

### Prerequisites

- **Windows 10/11** (x64)
- **Visual Studio 2022** with C++ workload
- **Python 3.x** (for encryption scripts)

### Build Instructions

1. Clone the repository:
   ```bash
   git clone https://github.com/Matthew20213/VEH-Playground.git
   cd VEH-Playground