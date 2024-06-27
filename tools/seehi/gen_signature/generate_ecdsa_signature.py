#!/usr/bin/env python3

import argparse
import hashlib
from ecdsa import SigningKey
from ecdsa.util import sigencode_der, sigencode_string
import os

def generate_signature(private_key, file, output_file):
    """
    Generate signature for data using private key
    """
    # open and read private key file
    with open(private_key, "rb") as f:
        key_pem = f.read()
    # create signing key from private key
    key = SigningKey.from_pem(key_pem)
    # sign file using key
    with open(file, "rb") as f:
        data = f.read()
    sig = key.sign_deterministic(data, hashfunc=hashlib.sha256, sigencode=sigencode_string)
    # create directory if it doesn't exist
    output_dir = os.path.dirname(output_file)
    if output_dir:  # Only attempt to create the directory if there is a directory component
        os.makedirs(output_dir, exist_ok=True)
    # write signature to file
    with open(output_file, "wb") as f:
        f.write(sig)

def main():
    """
    Main function
    """
    # parse command line arguments
    parser = argparse.ArgumentParser(description="Generate signature for file using private key")
    parser.add_argument("private_key", help="private key to use for signing")
    parser.add_argument("file", help="file to generate signature for")
    parser.add_argument("signature", help="file to write signature to")
    args = parser.parse_args()
    # generate signature for file using private key
    generate_signature(args.private_key, args.file, args.signature)

if __name__ == "__main__":
    main()
