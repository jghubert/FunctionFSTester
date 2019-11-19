# FunctionFSTester
Sample code to test SET_INTERFACE with functionfs between a Coral Dev Board and a host computer.

This test sets up a gadget on the Coral Dev board with one interface with two alternate settings: one without endpoints (alternate setting = 0x00) and one with two endpoints (alternate setting = 0x01). 

It consists in two software. The first runs as a driver on the dev board, and the second runs on the host computer that sends a SET_INTERFACE setup command to the dev board before exiting.


## Compilation
### On the dev board
- Files: functionFsTest.c
- Requirements: linux kernel headers  (needed for functionfs.h)
- To compile: cc -Wall -o functionFsTest functionFsTest.c -pthread

### On the host computer
- Files: usbTester.c eventManager.c
- Requirements: libusb-1.0
- To compile: cc -g -Wall -o usbTester usbTester.c eventManager.c -lusb-1.0
- Comment: Possible warning about deprecated function libusb-1.0 depending on the installed version.


## Running the test
The dev board and the computer must be connected by a USB cable from the start to make sure everything works fine.

### On the dev board
- Run the script to setup the gadget: sudo sh usbGadgetStart.sh
- Launch the driver: sudo ./functionFSTest
- Bind the gadget to the UDC: echo 38100000.dwc3 /sys/kernel/config/usb_gadget/gadget/UDC

### On the host computer
- Launch the test software: sudo ./usbTester

EXPECTED RESULT:
usbTester will try to activate the alternate setting #1 on the dev board. The software will report no error but the enumeration following the activation will show no endpoints. This indicates the command failed. Using a USB sniffer (such as a Total Phase Beagle 480), a STALL will be sent back from the dev board as a response to the request.

The dev board will show no trace of the request, either in the functionFsTest, or in the kernel logs. If the request had succeeded, functionFsTest should print "Data: Enable event detected" after usbTester has been launched.

