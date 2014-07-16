#!/bin/bash

if ! [ -f /dev/fpga_dma ]; then
    ./setup_fpga_dma.sh
    sleep 1
fi

./test_fpga_dma /dev/fpga_dma
