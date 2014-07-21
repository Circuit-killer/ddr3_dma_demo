Example of DMA between HPS SDRAM and FPGA SDRAM on the Cyclone 5.

Running instructions:

* Generate the QSYS system soc\_system.qsys.
* Run Analysis and Synthesis, Fitting, and Assembly
* Generate raw binary file from SOF file
* Build the debug program in software/userspace

Copy appropriate files to the root filesystem

    mount /dev/sdb3 /mnt
    cp output_files/ddr3_dma_demo.rbf /mnt/root
    cp software/userspace/debug_fpga_dma /mnt/root

On the board, program the FPGA, enable the bridge, and run the program

    dd if=ddr3_dma_demo.rbf of=/dev/fpga0
    echo 1 > /sys/class/fpga-bridge/lwhps2fpga/enable
    ./debug_fpga_dma
