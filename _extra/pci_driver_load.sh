#!/bin/sh

rmmod xilinx_pci_driver

sudo rm -rf /dev/xilinx_pci_driver
sudo mknod /dev/xilinx_pci_driver c 240 1
sudo chown root /dev/xilinx_pci_driver
sudo chmod 0644 /dev/xilinx_pci_driver
sudo ls -al /dev/xilinx_pci_driver

insmod /home/dimitrios/pci-sobel/drivers/xilinx_pci_driver_no_dbg.ko
