# -*- coding: utf-8 -*-
import sys, os
#sys.path.append(os.getcwd())
from mypackages import DES, modes
DEFAULT_PATH = os.path.join(
    os.path.dirname(os.path.abspath(__file__)))

# Convert text <--> bin
def message_to_bin(message):
    """Convert a string message to its binary representation using UTF-8 encoding."""
    binary_message = ''.join(format(byte, '08b') for byte in message.encode('utf-8'))
    return binary_message
def bin_to_message(binary_message):
    """Convert binary to string using UTF-8 encoding."""
    byte_array = bytearray()
    for i in range(0, len(binary_message), 8):
        byte = binary_message[i:i+8]
        byte_array.append(int(byte, 2))
    return byte_array.decode('utf-8')

def main(key, encrypt, decrypt):
    # Convert the key to binary format (assuming key is given as a string)
    binary_key = ''.join(format(ord(i), '08b') for i in key)
    # Create a DES instance with the given key
    ecb_instance = modes.DES_ECB.ecb_instance(binary_key)
   
    # Check if the user wants to encrypt
    if encrypt=="yes":
        plaintext = input("Enter the plaintext message (UTF-8): ")
        # Convert plaintext to binary format
        binary_plaintext = message_to_bin(plaintext)
        print("tex binary:", len(binary_plaintext))
        # Encrypt using the desired mode (e.g., ECB)
        ciphertext = ecb_instance.encrypt(binary_plaintext)  # Adjust as per your modes.py structure
        print("Encrypted Ciphertext:", ciphertext)

    # Check if the user wants to decrypt
    if decrypt=="yes":
        ciphertext = input("Enter the ciphertext: ")
        # Decrypt using the desired mode (e.g., ECB)
        decrypted_bỉnary = ecb_instance.decrypt(ciphertext)  # Adjust as per your modes.py structure
        decrypted_text = bin_to_message(decrypted_bỉnary)
        print("plaintex: ", decrypted_text)
def user_selection():
    print("Please select an option:")
    print("1. Encrypt Message")
    print("2. Decrypt Message")
    choice = input("Enter your choice (1/2): ")

    if choice == "1":
        key = input("Enter the DES key (8 characters): ")
        main(key, encrypt ="yes", decrypt="no")
    elif choice == "2":
        key = input("Enter the DES key (8 characters): ")
        main(key, encrypt ="no", decrypt="yes")
    else:
        print("Invalid choice. Please select a valid option.")
if __name__ == "__main__":
    user_selection()