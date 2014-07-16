#!/bin/sh

echo 0 > "/sys/class/fpga-bridge/fpga2hps/enable"
echo 0 > "/sys/class/fpga-bridge/hps2fpga/enable"
echo 0 > "/sys/class/fpga-bridge/lwhps2fpga/enable"
dd if=ddr3_dma_demo.rbf of=/dev/fpga0
echo 1 > "/sys/class/fpga-bridge/lwhps2fpga/enable"
mknod /dev/fpga_dma c 240 0
insmod fpga_dma.ko
