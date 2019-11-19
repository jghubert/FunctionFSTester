#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <endian.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "/usr/src/linux-headers-4.14.98-imx/include/uapi/linux/usb/functionfs.h"

/******************** Little Endian Handling ********************************/

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define cpu_to_le16(x)  (x)
#define cpu_to_le32(x)  (x)
#else
#define cpu_to_le16(x)  ((((x) >> 8) & 0xffu) | (((x) & 0xffu) << 8))
#define cpu_to_le32(x)  \
	((((x) & 0xff000000u) >> 24) | (((x) & 0x00ff0000u) >>  8) | \
	(((x) & 0x0000ff00u) <<  8) | (((x) & 0x000000ffu) << 24))
#endif

/******************** Interface 1 Descriptors ***********************/

static const struct {
	struct usb_functionfs_descs_head_v2 header;
	__le32 fs_count;
	__le32 hs_count;
	struct {
		struct usb_interface_descriptor Interface_1_Alt0;

		struct usb_interface_descriptor Interface_1_Alt1;
		struct usb_endpoint_descriptor_no_audio Interface_1_Alt1_endpoint_OUT;
		struct usb_endpoint_descriptor_no_audio Interface_1_Alt1_endpoint_IN;
	} __attribute__((packed)) fs_descs, hs_descs;
} __attribute__((packed)) descriptors_int1 = {
	.header = {
		.magic = cpu_to_le32(FUNCTIONFS_DESCRIPTORS_MAGIC_V2),
		.flags = cpu_to_le32(FUNCTIONFS_HAS_FS_DESC |
				     FUNCTIONFS_HAS_HS_DESC | FUNCTIONFS_CONFIG0_SETUP | FUNCTIONFS_ALL_CTRL_RECIP),
		.length = cpu_to_le32(sizeof descriptors_int1),
	},
	.fs_count = cpu_to_le32(4),
	.fs_descs = {
		.Interface_1_Alt0 = {
			.bLength = 0x09,
			.bDescriptorType = USB_DT_INTERFACE,
			.bInterfaceNumber = 0x00,
			.bAlternateSetting = 0x00,
			.bNumEndpoints = 0x00,
			.bInterfaceClass = 0xFF,
			.bInterfaceSubClass = 0xF0,
			.bInterfaceProtocol = 0x01,
			.iInterface = 0x01
		},

		.Interface_1_Alt1 = {
			.bLength = 0x09,
			.bDescriptorType = USB_DT_INTERFACE,
			.bInterfaceNumber = 0x00,
			.bAlternateSetting = 0x01,
			.bNumEndpoints = 0x02,
			.bInterfaceClass = 0xFF,
			.bInterfaceSubClass = 0xF0,
			.bInterfaceProtocol = 0x01,
			.iInterface = 0x01
		},
		.Interface_1_Alt1_endpoint_OUT = {
			.bLength = 0x07,
			.bDescriptorType = USB_DT_ENDPOINT,
			.bEndpointAddress = 0x02,
			.bmAttributes = USB_ENDPOINT_XFER_BULK,
			.wMaxPacketSize = cpu_to_le16(0x0040),
			.bInterval = 0x00
		},
		.Interface_1_Alt1_endpoint_IN = {
			.bLength = 0x07,
			.bDescriptorType = USB_DT_ENDPOINT,
			.bEndpointAddress = 0x82,
			.bmAttributes = USB_ENDPOINT_XFER_BULK,
			.wMaxPacketSize = cpu_to_le16(0x0040),
			.bInterval = 0x00
		}
	},
	.hs_count = cpu_to_le32(4),
	.hs_descs = {
		.Interface_1_Alt0 = {
			.bLength = 0x09,
			.bDescriptorType = USB_DT_INTERFACE,
			.bInterfaceNumber = 0x00,
			.bAlternateSetting = 0x00,
			.bNumEndpoints = 0x00,
			.bInterfaceClass = 0xFF,
			.bInterfaceSubClass = 0xF0,
			.bInterfaceProtocol = 0x01,
			.iInterface = 0x01
		},

		.Interface_1_Alt1 = {
			.bLength = 0x09,
			.bDescriptorType = USB_DT_INTERFACE,
			.bInterfaceNumber = 0x00,
			.bAlternateSetting = 0x01,
			.bNumEndpoints = 0x02,
			.bInterfaceClass = 0xFF,
			.bInterfaceSubClass = 0xF0,
			.bInterfaceProtocol = 0x01,
			.iInterface = 0x01
		},
		.Interface_1_Alt1_endpoint_OUT = {
			.bLength = 0x07,
			.bDescriptorType = USB_DT_ENDPOINT,
			.bEndpointAddress = 0x02,
			.bmAttributes = USB_ENDPOINT_XFER_BULK,
			.wMaxPacketSize = cpu_to_le16(0x0200),
			.bInterval = 0x00
		},
		.Interface_1_Alt1_endpoint_IN = {
			.bLength = 0x07,
			.bDescriptorType = USB_DT_ENDPOINT,
			.bEndpointAddress = 0x82,
			.bmAttributes = USB_ENDPOINT_XFER_BULK,
			.wMaxPacketSize = cpu_to_le16(0x0200),
			.bInterval = 0x00
		}
	}
};

#define INTERFACE1_STR_ "Stream"

static const struct {
		struct usb_functionfs_strings_head header;
		struct {
			__le16 code;
			const char str1[sizeof INTERFACE1_STR_];
		} __attribute__((packed)) lang0;
} strings_int1 = {
	.header = {
		.magic = cpu_to_le32(FUNCTIONFS_STRINGS_MAGIC),
		.length = cpu_to_le32(sizeof strings_int1),
		.str_count = cpu_to_le32(1),
		.lang_count = cpu_to_le32(1)
	},
	.lang0 = {
		cpu_to_le16(0x0409), /* en-us */
		INTERFACE1_STR_
	}
};

bool sendInterfaceDescriptors(int fd)
{
	int ret = write(fd, &descriptors_int1, sizeof descriptors_int1);

	if (ret < 0 && errno == EINVAL) {
		printf("%s - %s : New descriptor format has been rejected.\n", __FILE__, __FUNCTION__);
		return false;
	}
	else if (ret != sizeof descriptors_int1) {
		printf("%s - %s : Failed to copy all the Data descriptors on ep0.\n", __FILE__, __FUNCTION__);
		return false;
	}

	ret = write(fd, &strings_int1, sizeof strings_int1);
	if (ret < 0 || ret != sizeof strings_int1) {
		printf("%s - %s : Failed to write the strings on Data ep0.\n", __FILE__, __FUNCTION__);
		return false;
	}

	return true;
}

/**************************** Threads ***********************************/

struct thread {
	const char* name;
	const char* filename_ep0;
	const char* filename_out;
	const char* filename_in;
	int interface_number;
	pthread_t id_ep0;
	pthread_t id_inout;
	int fd_ep0;
	int fd_out;
	int fd_in;
} threads[] = {
	{
		"Data",
		"/dev/gadget/ep0",
		"/dev/gadget/ep1",   // read
		"/dev/gadget/ep2",   // write
		0,
		0, 0, 0, 0, 0
	},
};

bool initThread(struct thread* t)
{
	int ret;

	printf("%s: initializing...", t->name);
	fflush(stdout);

	ret = open(t->filename_ep0, O_RDWR);
	if (ret < 0) {
		char msg[512];
		snprintf(msg, 512, "Failed to open filename %s: ", t->filename_ep0);
		perror(msg);
		return false;
	}
	t->fd_ep0 = ret;

	// send the descriptors before opening the other endpoints.
	sendInterfaceDescriptors(t->fd_ep0);

	ret = open(t->filename_in, O_RDWR);
	if (ret < 0) {
		char msg[512];
		snprintf(msg, 512, "Failed to open filename %s: ", t->filename_ep0);
		perror(msg);
		return false;
	}
	t->fd_in = ret;

	ret = open(t->filename_out, O_RDWR);
	if (ret < 0) {
		char msg[512];
		snprintf(msg, 512, "Failed to open filename %s: ", t->filename_ep0);
		perror(msg);
		return false;
	}
	t->fd_out = ret;

	printf(" Done\n");

	return true;
}

#define NBEVENTS 5

void* ep0ThreadIn(void* context)
{
	struct thread *ep0thread = (struct thread*)context;
	int nevent;

	struct pollfd ep0_poll;
	ep0_poll.fd = ep0thread->fd_ep0;
	ep0_poll.events = POLLIN;

	struct usb_functionfs_event events[NBEVENTS];

	static const char *const names[] = {
		[FUNCTIONFS_BIND] = "BIND",
		[FUNCTIONFS_UNBIND] = "UNBIND",
		[FUNCTIONFS_ENABLE] = "ENABLE",
		[FUNCTIONFS_DISABLE] = "DISABLE",
		[FUNCTIONFS_SETUP] = "SETUP",
		[FUNCTIONFS_SUSPEND] = "SUSPEND",
		[FUNCTIONFS_RESUME] = "RESUME",
	};

	printf("%s : ep0 thread started.\n", ep0thread->name);

	for(;;) {
		pthread_testcancel();

		int status = poll(&ep0_poll, 1, -1);
		if (status < 0) {
			char msg[512];
			snprintf(msg, 512, "%s - %s : Error in the %s EP0 poll: ",__FILE__, __FUNCTION__, ep0thread->name);
			perror(msg);
			break;
		}

		status = read(ep0thread->fd_ep0, &events, sizeof events);
		if (status < 0) {
			if (errno == EAGAIN) {
				sleep(1);
				continue;
			}
			else {
				char msg[512];
				snprintf(msg, 512, "%s - %s : Error in the %s EP0 read: ",__FILE__, __FUNCTION__, ep0thread->name);
				perror(msg);
				break;
			}
		}

		printf("%s: ep0 received data. Size = %d\n", ep0thread->name, status);

		nevent = status / sizeof events[0];
		for (int i=0; i < nevent; ++i) {
			switch(events[i].type) {
			case FUNCTIONFS_ENABLE:
				printf("%s : Enable event detected.\n", ep0thread->name);
				break;
			case FUNCTIONFS_DISABLE:
				printf("%s : Disable event detected.\n", ep0thread->name);
				break;
			case FUNCTIONFS_BIND:
				// The usb interface is been bound to a UDC
			case FUNCTIONFS_UNBIND:
			case FUNCTIONFS_SETUP:
			case FUNCTIONFS_SUSPEND:
			case FUNCTIONFS_RESUME:
				printf("%s: Event %s\n", ep0thread->name, names[events[i].type]);
				break;
			}
		}
	} 

	return NULL;
}

void* inoutThreadIn(void* context)
{
	struct thread* t = (struct thread*)context;
	ssize_t ret;
	uint8_t buffer[1024];

	printf("%s : inout thread started.\n", t->name);

	for(;;) {
		pthread_testcancel();

		ret = read(t->fd_out, buffer, 1024);
		if (ret) {
			if (ret < 0) {
				char msg[512];
				snprintf(msg, 512, "%s : Error reading the inout device: errno = %d ", t->name, errno);
				perror(msg);
			}
			else {
				printf("%s: Data received. Size = %ld\n", t->name, ret);
				printf ("\t\t Data received = ");
				for (ssize_t i=0; i < ret; ++i)
					printf("0x%x ", buffer[i]);
				printf("\n");
			}
		}
	}
}

int main(int argc, char* argv[])
{
	int ret;

	// Sends the descriptors on ep0 
	if (!initThread(&threads[0])) {
		printf("Failed to initialize the interface.\n");
		return 1;
	}


	ret = pthread_create(&threads[0].id_ep0, NULL, ep0ThreadIn, &threads[0]);
	if (ret != 0) {
		printf("Failed to launch the ep0 thread for the iAP2.\n");
		return 1;
	}

	// Just wait for data on ep1.
	ret = pthread_create(&threads[0].id_inout, NULL, inoutThreadIn, &threads[0]);
	if (ret != 0) {
		printf("Failed to launch the inout thread for the iAP2.\n");
		return 1;
	}


	pthread_join(threads[0].id_ep0, NULL);

	return 0;
}