#ifndef PLOOSHRA1N_USB_H
#define PLOOSHRA1N_USB_H

/* Changing the header guard might lead to header guard clash with IOKit! */

// deps
#if defined(HAVE_LIBUSB) || defined(_WIN32) || defined(__linux__)

#define USE_LIBUSB
#include <libusb-1.0/libusb.h>
#include <pongo_libusb.h>
#else
#include <pongo_iokit.h>
#define USE_IOKIT
#include <IOKit/IOKitLib.h>
#include <IOKit/usb/IOUSBLib.h>
#include <IOKit/IOCFPlugIn.h>

#endif /* defined(HAVE_LIBUSB) || defined(_WIN32) || defined(__linux__) */

typedef struct {
	uint16_t vid, pid;
#ifdef USE_LIBUSB
	int usb_interface;
	struct libusb_context *context;
	struct libusb_device_handle *device;
#else
	io_service_t serv;
	IOUSBDeviceInterface320 **device;
	CFRunLoopSourceRef async_event_source;
	IOUSBInterfaceInterface300 **interface;
#endif
} usb_handle_t;

#endif /* PLOOSHRA1N_USB_H */
