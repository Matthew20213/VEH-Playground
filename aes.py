import sys
from Crypto.Cipher import AES
from Crypto.Util.Padding import pad
from Crypto.Random import get_random_bytes
import hashlib

KEY = get_random_bytes(16)
iv = 16 * b'\x00'
cipher = AES.new(hashlib.sha256(KEY).digest(), AES.MODE_CBC, iv)
plaintext = open("path\\to\\your\\shellcode.bin", "rb").read()

ciphertext = cipher.encrypt(pad(plaintext, AES.block_size))

with open("encrypted.bin", "wb") as f:
    f.write(ciphertext)

print('AESkey[] = { 0x' + ', 0x'.join(hex(x)[2:] for x in KEY) + ' };')
print("[+] Encrypted output written to encrypted.bin")