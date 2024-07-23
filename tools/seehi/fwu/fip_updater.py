#!/usr/bin/env python3

import os
import platform
import signal
import subprocess
import argparse
import time
import serial
import textwrap

def get_processes_using_port(port):
    """Get the list of processes using the specified serial port."""
    try:
        result = subprocess.check_output(['fuser', port], stderr=subprocess.DEVNULL)
        pids = result.decode('utf-8').strip().split()
        return [int(pid) for pid in pids if pid]
    except subprocess.CalledProcessError:
        return []

def suspend_processes(pids):
    """Suspend the specified processes."""
    for pid in pids:
        os.kill(pid, signal.SIGTSTP)

def resume_processes(pids):
    """Resume the specified processes."""
    for pid in pids:
        os.kill(pid, signal.SIGCONT)

def configure_serial_port(port):
    """Configure the serial port for XMODEM transmission using stty."""
    try:
        if platform.system() == 'Linux':
            stty_command = ['stty', '-F', port, '115200', 'cs8', '-cstopb', '-parenb', 'min', '1', 'time', '5']
        elif platform.system() == 'Darwin':  # macOS
            stty_command = ['stty', '-f', port, '115200', 'cs8', '-cstopb', '-parenb', 'min', '1', 'time', '5']
        else:
            print(f"Unsupported OS: {platform.system()}")
            return False
        subprocess.run(stty_command, check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error configuring serial port {port}: {e}")
        return False
    return True

def is_device_ready(port):
    """Check if the device is ready for the second stage by looking for 'C' characters."""
    try:
        with serial.Serial(port, 115200, timeout=1) as ser:
            response = ser.read(1)  # Read one byte
            if response == b'C':
                return True
    except serial.SerialException as e:
        print(f"Error communicating with serial port {port}: {e}")
    return False

def send_firmware(port, firmware_file):
    """Send the firmware file using sx with XMODEM 1KB CRC."""
    try:
        if platform.system() == 'Linux':
            command = f'sx --xmodem --1k --binary {firmware_file} < {port} > {port}'
        elif platform.system() == 'Darwin':  # macOS
            command = f'lsx --xmodem --1k --binary {firmware_file} < {port} > {port}'
        else:
            print(f"Unsupported OS: {platform.system()}")
            return False
        subprocess.run(command, shell=True, check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error sending firmware {firmware_file}: {e}")
        return False
    return True

def main():
    parser = argparse.ArgumentParser(
        description="This is a fip update tool for Rhea.",
        formatter_class=argparse.RawTextHelpFormatter,
        epilog=textwrap.dedent("""\
            Examples of usage:
              1. Update fip with only fip, in this case the fwu_sram.default will be used as fwu_sram:
                 ./fip_updater.py -p /dev/ttyACM0 --fip fip.bin

              2. Update fip with explicit fwu_sram and fip:
                 ./fip_updater.py -p /dev/ttyACM0 --fwu_sram fwu_sram.bin --fip fip.bin

              3. Run a sram program only:
                 ./fip_updater.py -p /dev/ttyACM0 --fwu_sram sram_program.bin

            Note:
               1. This tool will suspend processes using the same serial port during the update.
               2. This tool requires sx/lsx to be installed for XMODEM transfer.
                  1) For Linux: sudo apt install lrzsz
                  2) For macOS: brew install lrzsz
        """)
    )
    parser.add_argument('-p', '--serial_port', metavar="PORT", required=True, help="The serial port to use (e.g., /dev/ttyACM0)")
    parser.add_argument('--fwu_sram', metavar="FILENAME", help="The sram program file to send (e.g., fwu_sram.bin)")
    parser.add_argument('--fip', metavar="FILENAME", help="The fip file to send (e.g., fip.bin)")
    args = parser.parse_args()

    if not args.fwu_sram and not args.fip:
        return

    serial_port = args.serial_port
    
    # Get the directory where this script is located
    script_dir = os.path.dirname(os.path.realpath(__file__))

    # Use the default fwu_sram file located in the script directory
    fwu_sram = args.fwu_sram if args.fwu_sram else os.path.join(script_dir, "fwu_sram.default")
    fip = args.fip

    # Step 1: Identify processes using the serial port
    processes = get_processes_using_port(serial_port)
    if processes:
        print(f"{serial_port} can't be used by others while updating firmware.")
        print(f"Suspending processes: {processes}")
        # Step 2: Suspend the processes
        suspend_processes(processes)

    # Step 3: Configure the serial port
    if not configure_serial_port(serial_port):
        if processes:
            print(f"Resuming processes: {processes}")
            resume_processes(processes)
        return

    # Step 4: Send the fwu_sram (first stage)
    if not send_firmware(serial_port, fwu_sram):
        if processes:
            print(f"Resuming processes: {processes}")
            resume_processes(processes)
        return

    # Step 5: Send the fip (second stage)
    if fip:
        print("Waiting for the device to be ready for the second stage...")
        while not is_device_ready(serial_port):
            time.sleep(0.1)

        if not send_firmware(serial_port, fip):
            if processes:
                print(f"Resuming processes: {processes}")
                resume_processes(processes)
            return

    # Step 6: Resume the processes
    if processes:
        print(f"Resuming processes: {processes}")
        resume_processes(processes)

if __name__ == '__main__':
    main()