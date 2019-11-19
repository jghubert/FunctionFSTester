#!/bin/bash

modprobe libcomposite

sleep 1

CONFIGFS_PATH=/sys/kernel/config
USB_GAGDET_PATH=$CONFIGFS_PATH/usb_gadget
SHIKAI_GADGET_PATH=$USB_GAGDET_PATH/gadget
mkdir -p $SHIKAI_GADGET_PATH

cd $SHIKAI_GADGET_PATH
echo 0x6666 > idVendor
echo 0x0100 > idProduct
echo 0x0001 > bcdDevice
mkdir -p strings/0x409
echo "Progress Technologies" > strings/0x409/manufacturer
echo "Gadget" > strings/0x409/product
echo "1" > strings/0x409/serialnumber
echo "0" > bDeviceClass
echo "0" > bDeviceSubClass
echo "0" > bDeviceProtocol
echo "0x40" > bMaxPacketSize0
echo "0x0210" > bcdUSB

mkdir -p configs/c.1
echo 0 > configs/c.1/MaxPower
mkdir -p configs/c.1/strings/0x409
echo "Config 1" > configs/c.1/strings/0x409/configuration

mkdir functions/ffs.gadget
ln -s functions/ffs.gadget configs/c.1

sleep 1

## We need to mount the filesystem
mkdir -p /dev/gadget
mount gadget -t functionfs /dev/gadget

# Need to launch the daemon handling the connection

sleep 1

UDC=$(echo -n $(ls /sys/class/udc/|head -c -1 -n 1))
echo $UDC

# sleep 1
#echo $UDC > UDC

