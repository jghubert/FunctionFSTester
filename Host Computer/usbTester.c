#include <stdlib.h>
#include <stdio.h>
#include <libusb-1.0/libusb.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include "eventManager.h"

bool exitnow = false;
EventManager events;
libusb_device* shikAICam;
libusb_device_handle* shikAICam_handle;

typedef enum {
	USB_EVENT_CONNECT = 0x0001,
	USB_EVENT_DISCONNECT = 0x0002
} USB_Events;

int usb_hotplug_callback(libusb_context *ctx, libusb_device *device, libusb_hotplug_event event, void *user_data)
{
	if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED) {
		printf("DEVICE CONNECTED.\n");
		shikAICam = device;
		EventManagerSet(&events, USB_EVENT_CONNECT, EM_AND);
	}
	else {
		printf("DEVICE DISCONNECTED.\n");
		EventManagerSet(&events, USB_EVENT_DISCONNECT, EM_AND);
	}
	return 0;
}

bool playWithDevice()
{
	// The goal is send a set_interface to the system to see its reaction, and to see if all endpoints are closed.

	int ret;

	ret = libusb_open(shikAICam, &shikAICam_handle);
	if (ret) {
		printf("Error opening the usb device: ");
		switch(ret) {
		case LIBUSB_ERROR_NO_MEM:
			printf("Memory allocation failure.\n");
			break;
		case LIBUSB_ERROR_ACCESS:
			printf("Insufficient permissions.\n");
			break;
		case LIBUSB_ERROR_NO_DEVICE:
			printf("Device disconnected.\n");
			break;
		default:
			printf("Unknown error.\n");
		}
		return false;
	}


	int curint = 0;
	int altset = 1;
	printf("Claiming interface %d: ", curint);
	ret = libusb_claim_interface(shikAICam_handle, curint);
	if (ret) {
		printf("Error \n");
		goto cancel;
	}
	else {
		printf("Success!\n");
	}

	printf("Switching to the alternate setting %d: ", curint);
	ret = libusb_set_interface_alt_setting(shikAICam_handle, curint, altset);
	if (ret) {
		switch(ret) {
		case LIBUSB_ERROR_NOT_FOUND:
			printf("Error : Interface not claimed or alternate setting does not exist.\n");
			break;
		case LIBUSB_ERROR_NO_DEVICE:
			printf("Error : Device disconnected.\n");
			break;
		default:
			printf("Error: Unknown error %s:\n", libusb_strerror(ret));
			libusb_release_interface(shikAICam_handle, curint);
			break;
		}
		goto cancel;
	}
	else
		printf("Success!\n");

	sleep(1);

	libusb_release_interface(shikAICam_handle, curint);
	libusb_close(shikAICam_handle);
	return true;

cancel:
	libusb_close(shikAICam_handle);
	return false;

}


int main(int argc, char* argv[])
{
	libusb_context* usbContext = NULL;
	int ret;
	shikAICam = NULL;

	EventManagerCreate(&events);

	ret = libusb_init(&usbContext);
	if (ret) {
		printf("Error initializing the usb system. Error is %d\n", ret);
		exit(1);
	}
	else 
		printf("USB System initalized.\n");

	const struct libusb_version* version = libusb_get_version();
	printf("libusb version = %u.%u.%u\n", version->major, version->minor, version->micro);

	// libusb_set_option(usbContext, LIBUSB_LOG_LEVEL_WARNING);
	libusb_set_debug(usbContext, 4);  // Deprecated but some linux distributions have an older version of libusb-1.0

	ret = libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG);
	if (ret) {
		printf ("USB System supports hotplug notification.\n");
	}
	else {
		printf ("USB System does not support hotplug notification.\n");
		goto cancel;
	}

	ret = libusb_hotplug_register_callback(
		usbContext, 
		LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT,
		LIBUSB_HOTPLUG_ENUMERATE,
		0x6666,
		0x0100,
		0x0000,
		usb_hotplug_callback,
		NULL,
		NULL
		);
	if (ret == LIBUSB_SUCCESS)
		printf("Successfully registered the callback function for hotplug notifications.\n");
	else {
		printf("Failed to register the callback function for hotplug notifications.\n");
		goto cancel;
	}

	uint32_t flags = 0;
	// while (!exitnow) {
		EventManagerGet(&events, USB_EVENT_CONNECT | USB_EVENT_DISCONNECT, EM_OR, &flags, WAIT_FOREVER);
		if (flags & USB_EVENT_CONNECT) {
			// Device is connected
			playWithDevice();
		}
		else if (flags & USB_EVENT_DISCONNECT) {
			// Device disconnected
			// Clear the flag and wait
			shikAICam = NULL;
			EventManagerSet(&events, 0x00000000, EM_AND); // Clear the flags
		}
	// }

cancel:
	libusb_exit(usbContext);
	EventManangerDestroy(&events);

	return 0;
}