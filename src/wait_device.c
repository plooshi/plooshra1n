#include <wait_device.h>
#include <common/log.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef USE_LIBUSB
static void init_usb_handle(usb_handle_t *handle, uint16_t vid, uint16_t pid) {
	handle->vid = vid;
	handle->pid = pid;
	handle->device = NULL;
}

int wait_usb_handles(usb_handle_t **found_targets, int targets[][2], unsigned int target_count) {
    libusb_device **list;
    libusb_context *context;
    static usb_handle_t found_targets_big[1024] = { 0 };
    unsigned int found_count = 0;
    static bool quiet = false;

    if (libusb_init(&context) == LIBUSB_SUCCESS) {
        if (!quiet) {
            quiet = true;
            log_debug("Waiting for devices...");
        }

        for (;;) {
            ssize_t device_cnt = libusb_get_device_list(context, &list);

            for (int i = 0; i < device_cnt; i++) {
                libusb_device *device = list[i];
                struct libusb_device_descriptor desc = {0};

                libusb_get_device_descriptor(device, &desc);

                for (int x = 0; x < target_count; x++) {
                    if (targets[x][0] == desc.idVendor && targets[x][1] == desc.idProduct) {
                        init_usb_handle(&found_targets_big[found_count], desc.idVendor, desc.idProduct);
                        found_count++;
                    }
                }
            }

            libusb_free_device_list(list, device_cnt);

            if (found_count > 0) {
                break;
            }
        }

        libusb_exit(context);
    }
    
    *found_targets = found_targets_big;
    return found_count;
}
#else
#include <CoreFoundation/CoreFoundation.h>

struct IOUSBConfigurationDescriptor
{
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t wTotalLength;
    uint8_t  bNumInterfaces;
    uint8_t  bConfigurationValue;
    uint8_t  iConfiguration;
    uint8_t  bmAttributes;
    uint8_t  MaxPower;
} __attribute__((packed));
struct IOUSBFindInterfaceRequest
{
    UInt16 bInterfaceClass;                     // requested class
    UInt16 bInterfaceSubClass;                  // requested subclass
    UInt16 bInterfaceProtocol;                  // requested protocol
    UInt16 bAlternateSetting;                   // requested alt setting
};
enum
{
    kIOUSBFindInterfaceDontCare = 0xFFFF
};


static void
cf_dictionary_set_int16(CFMutableDictionaryRef dict, const void *key, uint16_t val) {
	CFNumberRef cf_val = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt16Type, &val);

	if(cf_val != NULL) {
		CFDictionarySetValue(dict, key, cf_val);
		CFRelease(cf_val);
	}
}

static bool
query_usb_interface(io_service_t serv, CFUUIDRef plugin_type, CFUUIDRef interface_type, LPVOID *interface) {
	IOCFPlugInInterface **plugin_interface;
	bool ret = false;
	SInt32 score;

	if(IOCreatePlugInInterfaceForService(serv, plugin_type, kIOCFPlugInInterfaceID, &plugin_interface, &score) == kIOReturnSuccess) {
		ret = (*plugin_interface)->QueryInterface(plugin_interface, CFUUIDGetUUIDBytes(interface_type), interface) == kIOReturnSuccess;
		IODestroyPlugInInterface(plugin_interface);
	}
	IOObjectRelease(serv);
	return ret;
}

static void
close_usb_device(usb_handle_t *handle) {
	CFRunLoopRemoveSource(CFRunLoopGetCurrent(), handle->async_event_source, kCFRunLoopDefaultMode);
	CFRelease(handle->async_event_source);
	(*handle->device)->USBDeviceClose(handle->device);
	(*handle->device)->Release(handle->device);
}

static void
close_usb_handle(usb_handle_t *handle) {
	close_usb_device(handle);
}

static bool
open_usb_device(io_service_t serv, usb_handle_t *handle) {
	bool ret = false;

	if(query_usb_interface(serv, kIOUSBDeviceUserClientTypeID, kIOUSBDeviceInterfaceID320, (LPVOID *)&handle->device)) {
		if((*handle->device)->USBDeviceOpen(handle->device) == kIOReturnSuccess) {
			if((*handle->device)->SetConfiguration(handle->device, 1) == kIOReturnSuccess && (*handle->device)->CreateDeviceAsyncEventSource(handle->device, &handle->async_event_source) == kIOReturnSuccess) {
				CFRunLoopAddSource(CFRunLoopGetCurrent(), handle->async_event_source, kCFRunLoopDefaultMode);
				ret = true;
			} else {
				(*handle->device)->USBDeviceClose(handle->device);
			}
		}
		if(!ret) {
			(*handle->device)->Release(handle->device);
		}
	}
	return ret;
}

static void
init_usb_handle(usb_handle_t *handle, uint16_t vid, uint16_t pid) {
	handle->vid = vid;
	handle->pid = pid;
	handle->device = NULL;
}

int wait_usb_handles(usb_handle_t **found_targets, int targets[][2], unsigned int target_count) {
    CFMutableDictionaryRef matching_dict;
	const char *darwin_device_class;
	io_iterator_t iter;
	io_service_t serv;
    static usb_handle_t found_targets_big[1024] = { 0 };
    unsigned int found_count = 0;
    static bool quiet = false;

    if (!quiet) {
        quiet = true;
        log_debug("Waiting for devices...");
    }

	usb_handle_t handle;

#if TARGET_OS_IPHONE
	darwin_device_class = "IOUSBHostDevice";
#else
	darwin_device_class = kIOUSBDeviceClassName;
#endif
	while (true) {
		for (unsigned int i = 0; i < 6; i++) {
			if ((matching_dict = IOServiceMatching(darwin_device_class)) != NULL) {
				cf_dictionary_set_int16(matching_dict, CFSTR("idVendor"), targets[i][0]);
				cf_dictionary_set_int16(matching_dict, CFSTR("idProduct"), targets[i][1]);
				if(IOServiceGetMatchingServices(0, matching_dict, &iter) == kIOReturnSuccess) {
					while((serv = IOIteratorNext(iter)) != IO_OBJECT_NULL) {
						if(open_usb_device(serv, &handle)) {
							init_usb_handle(&found_targets_big[found_count], targets[i][0], targets[i][1]);
							found_count++;
							close_usb_device(&handle);
						}
					}
				}
			}
		}

		if (found_count > 0) {
			break;
		}
	}

	*found_targets = found_targets_big;
    return found_count;
}
#endif