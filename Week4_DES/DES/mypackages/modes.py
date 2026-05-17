# -*- coding: utf-8 -*-
import sys
import os

DEFAULT_PATH = os.path.join(
    os.path.dirname(os.path.abspath(__file__))
)

from .DES import DES


# ============================================================
# PKCS#7 Padding for Binary Strings
# ============================================================

def pkcs7_pad(binary_text, block_size=64):
    """
    Apply PKCS#7 padding to binary string.

    Parameters:
        binary_text : str
            Binary string ('0'/'1')
        block_size : int
            Block size in bits (DES = 64)

    Returns:
        str : padded binary string
    """

    # Ensure byte-aligned
    if len(binary_text) % 8 != 0:
        raise ValueError(
            "Binary plaintext must be byte-aligned."
        )

    block_bytes = block_size // 8
    text_bytes = len(binary_text) // 8

    # PKCS#7 padding size
    padding_bytes = block_bytes - (text_bytes % block_bytes)

    # If already aligned -> add full block
    if padding_bytes == 0:
        padding_bytes = block_bytes

    padding_byte = format(padding_bytes, '08b')

    padding = padding_byte * padding_bytes

    padded = binary_text + padding

    print("Text binary:", len(binary_text))
    print("Padded text length:", len(padded))

    return padded


def pkcs7_unpad(binary_text):
    """
    Remove PKCS#7 padding from binary string.
    """

    if len(binary_text) % 8 != 0:
        raise ValueError(
            "Invalid padded binary length."
        )

    padding_byte = binary_text[-8:]
    padding_bytes = int(padding_byte, 2)

    if padding_bytes < 1 or padding_bytes > 8:
        raise ValueError("Invalid PKCS#7 padding.")

    padding_bits = padding_bytes * 8

    # Verify padding
    expected_padding = padding_byte * padding_bytes

    if binary_text[-padding_bits:] != expected_padding:
        raise ValueError("Corrupted PKCS#7 padding.")

    return binary_text[:-padding_bits]


# ============================================================
# DES ECB MODE
# ============================================================

class DES_ECB:

    def __init__(self, key):
        self.des = DES(key)

    @staticmethod
    def ecb_instance(key):
        return DES_ECB(key)

    def encrypt(self, plaintext):

        # Apply PKCS#7 padding
        plaintext = pkcs7_pad(plaintext, 64)

        ciphertext = ""

        # Encrypt block-by-block
        for i in range(0, len(plaintext), 64):

            block = plaintext[i:i+64]

            print("block:", len(block), block)

            ciphertext += self.des.encrypt(block)

        return ciphertext

    def decrypt(self, ciphertext):

        if len(ciphertext) % 64 != 0:
            raise ValueError(
                "Ciphertext length must be multiple of 64 bits."
            )

        plaintext = ""

        # Decrypt block-by-block
        for i in range(0, len(ciphertext), 64):

            block = ciphertext[i:i+64]

            plaintext += self.des.decrypt(block)

        # Remove PKCS#7 padding
        plaintext = pkcs7_unpad(plaintext)

        return plaintext


# ============================================================
# DES CBC MODE
# ============================================================

class DES_CBC:

    def __init__(self, key, iv):

        if len(iv) != 64:
            raise ValueError("IV must be 64 bits.")

        self.des = DES(key)
        self.iv = iv

    @staticmethod
    def cbc_instance(key, iv):
        return DES_CBC(key, iv)

    def encrypt(self, plaintext):

        plaintext = pkcs7_pad(plaintext, 64)

        ciphertext = ""

        previous_block = self.iv

        for i in range(0, len(plaintext), 64):

            block = plaintext[i:i+64]

            # XOR with previous ciphertext (or IV)
            block_to_encrypt = self.xor(
                block,
                previous_block
            )

            encrypted_block = self.des.encrypt(
                block_to_encrypt
            )

            ciphertext += encrypted_block

            previous_block = encrypted_block

        return ciphertext

    def decrypt(self, ciphertext):

        if len(ciphertext) % 64 != 0:
            raise ValueError(
                "Ciphertext length must be multiple of 64 bits."
            )

        plaintext = ""

        previous_block = self.iv

        for i in range(0, len(ciphertext), 64):

            block = ciphertext[i:i+64]

            decrypted_block = self.des.decrypt(block)

            plaintext_block = self.xor(
                decrypted_block,
                previous_block
            )

            plaintext += plaintext_block

            previous_block = block

        plaintext = pkcs7_unpad(plaintext)

        return plaintext

    @staticmethod
    def xor(block1, block2):

        return ''.join(
            '1' if b1 != b2 else '0'
            for b1, b2 in zip(block1, block2)
        )