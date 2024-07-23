#!/usr/bin/env bash

# Check if the correct number of arguments is provided
if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <device> <fip_file> <cycle_count>"
    exit 1
fi

# Device and FIP file path from command line arguments
DEVICE=$1
FIP_FILE=$2
CYCLE_COUNT=$3

# Get the directory where the script is located
SCRIPT_DIR=$(dirname "$(readlink -f "$0")")

# Perform the FWU flow in a loop
for ((i=1; i<=CYCLE_COUNT; i++))
do
    echo "Starting FWU cycle $i..."
    "$SCRIPT_DIR/fip_updater.py" -p $DEVICE --fip $FIP_FILE
    if [ $? -ne 0 ]; then
        echo "FWU cycle $i failed."
        exit 1
    fi
    echo "FWU cycle $i completed."
done

echo "FWU stress test completed."