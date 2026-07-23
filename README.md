# VEH-Playground – Vectored Exception Handling for Malware Development

<p align="center">

![Windows](https://img.shields.io/badge/Platform-Windows-blue)
![Purpose](https://img.shields.io/badge/Purpose-Research-purple)

</p>

> A research-oriented collection of progressively advanced **Vectored Exception Handling (VEH)** proof-of-concepts demonstrating how Windows exceptions can be leveraged for execution flow manipulation, runtime payload protection, API interception, and lazy page decryption.

---

# Overview

Windows **Vectored Exception Handling (VEH)** provides applications with a mechanism to intercept exceptions before Structured Exception Handling (SEH). While primarily intended for debugging and error recovery, VEH also exposes low-level access to the processor context, allowing software to inspect and modify execution at runtime.

This repository explores those capabilities through a collection of progressively advanced proof-of-concepts. Beginning with basic exception handling, each implementation introduces a new concept before combining previous techniques into more sophisticated execution flows.

The objective is to provide practical demonstrations of VEH for malware research, Windows internals, and offensive security education.

---

# Repository Highlights

- 11 progressively advanced proof-of-concepts
- Comprehensive documentation for every implementation
- Execution flow diagrams
- Console demonstrations
- Video demonstrations
- Windows x64
- Visual Studio 2022

---

# Proof-of-Concepts

| # | PoC | Description | Concept |
| :---: | :--- | :--- | :--- |
| 01 | **Basic Handler** | Registers a simple Vectored Exception Handler. | `AddVectoredExceptionHandler` |
| 02 | **Handler Priority** | Demonstrates handler ordering and priority. | VEH priority |
| 03 | **Exception Information** | Examines exception records and CPU context. | `ExceptionRecord` / `ContextRecord` |
| 04 | **Context Modification** | Modifies CPU registers inside the handler. | CPU context |
| 05 | **RIP Redirection** | Redirects execution by modifying RIP. | Execution flow manipulation |
| 06 | **PAGE_GUARD Handling** | Demonstrates guard page exceptions. | `PAGE_GUARD` |
| 07 | **PAGE_NOACCESS Handling** | Runtime payload restoration using `PAGE_NOACCESS`. | `PAGE_NOACCESS` |
| 08 | **API Interception** | Intercepts API execution using VEH. | API interception |
| 09 | **Ninja Guard** | Temporarily hides payloads during API execution. | Runtime payload protection |
| 10 | **NOACCESS Blinker** | Cyclic encryption and runtime restoration. | Memory protection pipeline |
| 11 | **Lazy Page Decryption** | Decrypts only the currently executing page. | Lazy page restoration |

---

# Documentation

Each proof-of-concept contains dedicated documentation covering:

- Overview
- Implementation
- Code Walkthrough
- Execution Flow
- Expected Output
- Demonstration
- Notes
- References

The documentation is intended to explain the implementation rather than simply describe the underlying Windows APIs, making it suitable for readers who want to understand how each technique is constructed step by step.

---

# Project Goals

This repository aims to demonstrate how VEH can be used to:

- Inspect processor state during exceptions
- Modify execution flow
- Redirect instruction pointers
- Protect runtime payloads
- Intercept API execution
- Implement page-based memory protection
- Explore advanced exception-driven execution techniques

The project is designed as an educational resource for reverse engineers, malware researchers, red team operators, and anyone interested in Windows exception handling.

---

# Disclaimer

This repository is provided for **educational and research purposes only**.

The proof-of-concepts are intended to demonstrate Windows internals, exception handling mechanisms, and runtime memory protection techniques in controlled environments. They should **not** be used for unauthorized activities or malicious purposes.
