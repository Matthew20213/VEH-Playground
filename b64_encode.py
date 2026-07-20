import base64


shellcode = open("path\\to\\your\\shellcode.bin", "rb")
encoded_shellcode = base64.b64encode(shellcode.read())

with open("b64_shellcode.bin", "wb") as out:
    out.write(encoded_shellcode)
    print("Encoding complete.")