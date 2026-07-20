#include <iostream>
#include <windows.h>
#include <fstream>
#include <vector>

using namespace std;

vector<char> shellcode;
void* exec_mem;
SIZE_T leng = 0;

LONG WINAPI handler(struct _EXCEPTION_POINTERS *ExceptionInfo) {
    // Checking if the exception code is a Page Guard violation
    if (ExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_GUARD_PAGE_VIOLATION)
    {
        // Checking if the exception occured on calling WinExec
        if (ExceptionInfo->ContextRecord->Rip == DWORD64(WinExec)) {
            MessageBoxA(NULL, "WinExec has been hooked", "hook", MB_OK);
            return EXCEPTION_CONTINUE_EXECUTION;
        }
        
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

int main()
{
    // Load shellcode
    ifstream infile;
    infile.open(R"(../../calc.bin)", std::ios::in | std::ios::binary);
    infile.seekg(0, std::ios::end);
    size_t file_size_in_byte = infile.tellg();
    shellcode.resize(file_size_in_byte);
    infile.seekg(0, std::ios::beg);
    infile.read(reinterpret_cast<char*>(shellcode.data()), shellcode.size());
    infile.close();

    DWORD old = 0;

    leng = shellcode.size();

    // Allocate executable memory
    void* exec_mem = VirtualAlloc(NULL, leng, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

    // Copy shellcode into allocated memory
    RtlMoveMemory(exec_mem, shellcode.data(), leng);

    // Register Vectored Exception Handler
    AddVectoredExceptionHandler(1, handler);

    // Set up PAGE_GUARD on WinExec
    // WinExec is being called in the shellcode to spawn a calc
    VirtualProtect(WinExec, 1, PAGE_EXECUTE_READ | PAGE_GUARD, &old);

    // Creating a thread with the shellcode and running it
    HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)exec_mem, NULL, 0, NULL);
    WaitForSingleObject(hThread, INFINITE);

    return 0;
}