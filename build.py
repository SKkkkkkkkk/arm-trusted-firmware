#!/usr/bin/env python3

# Use cases:

## 0. help
## ./build.py -h

## 1. build a release version
## ./build.py --bl33 path/to/your/bl33.bin

## 2. build a debug version
## ./build.py -d --bl33 path/to/your/bl33.bin
    
## 3. clean the debug build
## ./build.py -dc
    
## 4. using ur own cross compile toolchain, default is aarch64-none-elf-
## CROSS_COMPILE=/path/to/your/cross/compile/toolchain/bin/aarch64-none-elf- ./build.py


import argparse
import subprocess
import os
import textwrap

def main():
    # parse command line arguments
    parser = argparse.ArgumentParser(
    description="Build script for Rhea TF-A.",
    formatter_class=argparse.RawTextHelpFormatter,
    epilog=textwrap.dedent("""\
        others:
            if aarch64-none-elf- is not in your PATH, you can specify the cross-compile toolchain by setting CROSS_COMPILE environment variable.
            like this:
                CROSS_COMPILE=/path/to/aarch64-none-elf- ./build.py
    """)
    )
    parser.add_argument('-v', '--verbose', action='store_true')
    parser.add_argument('-c', '--clean', action='store_true')
    parser.add_argument('-d', '--debug', action='store_true', help="build debug version, default is release.")
    parser.add_argument('--log_level', metavar="LOG_LEVEL", type=int, help="0:None, 10:Error, 20:Notice, 30:Warning, 40: Info, 50: Verbose, 40 for debug, 10 for release by default.")
    parser.add_argument('--bl33',   metavar="FILENAME", help="add a bl33 image to fip.")
    parser.add_argument('--ns_bl1u', metavar="FILENAME", help="generate rom.bin consisted of bl1 and ns_bl1u.")
    args = parser.parse_args()

    # cross compile
    if os.environ.get('CROSS_COMPILE') is None:
        os.environ['CROSS_COMPILE'] = "aarch64-none-elf-"

    # build options

    ## common
    make = ["make", "PLAT=rhea", "HW_ASSISTED_COHERENCY=1", "USE_COHERENT_MEM=0", "ENABLE_LTO=0", "-j"]
    
    ## OPENSSL_DIR
    openssl_dir = os.environ.get('OPENSSL_DIR')
    if openssl_dir is not None:
        make += ["OPENSSL_DIR=" + openssl_dir]

    ## verbose
    if args.verbose:
        make.append("V=1")
    
    ## build type, default is release
    if args.debug:
        make.append("DEBUG=1")
    
    ## clean, we can skip the rest
    if args.clean:
        subprocess.run(make + ["clean"], check=False)
        return

    ## log level
    if args.log_level is not None:
        make.append("LOG_LEVEL=" + str(args.log_level))
    else:
        if not args.debug:
            make.append("LOG_LEVEL=10")

    ## bl33
    if args.bl33 is not None:
        make.append("BL33=" + args.bl33)
        make.append("fip")
    
    ## ns_bl1u
    if args.ns_bl1u is not None:
        make.append("NS_BL1U=" + args.ns_bl1u)
        make.append("rom")

    ## build
    # print(make)
    subprocess.run(make + ["all"], check=False)

if __name__ == "__main__":
    main()