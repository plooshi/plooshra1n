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
                        found_targets_big[found_count].device = libusb_open_device_with_vid_pid(NULL, desc.idVendor, desc.idProduct);
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
static void cf_dictionary_set_int16(CFMutableDictionaryRef dict, const void *key, uint16_t val) {
	CFNumberRef cf_val = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt16Type, &val);

	if(cf_val != NULL) {
		CFDictionarySetValue(dict, key, cf_val);
		CFRelease(cf_val);
	}
}

static bool query_usb_interface(io_service_t serv, CFUUIDRef plugin_type, CFUUIDRef interface_type, LPVOID *interface) {
	IOCFPlugInInterface **plugin_interface;
	bool ret = false;
	SInt32 score;

	if(IOCreatePlugInInterfaceForService(serv, plugin_type, kIOCFPlugInInterfaceID, &plugin_interface, &score) == kIOReturnSuccess) {
		ret = (*plugin_interface)->QueryInterface(plugin_interface, CFUUIDGetUUIDBytes(interface_type), interface) == kIOReturnSuccess;
		IODestroyPlugInInterface(plugin_interface);
	}
	return ret;
}

static void close_usb_device(usb_handle_t *handle) {
	IOObjectRelease(handle->serv);
	CFRunLoopRemoveSource(CFRunLoopGetCurrent(), handle->async_event_source, kCFRunLoopDefaultMode);
	CFRelease(handle->async_event_source);
	(*handle->device)->USBDeviceClose(handle->device);
	(*handle->device)->Release(handle->device);
}

static void close_usb_interface(usb_handle_t *handle) {
	(*handle->interface)->USBInterfaceClose(handle->interface);
	(*handle->interface)->Release(handle->interface);
}

static void close_usb_handle(usb_handle_t *handle) {
	close_usb_interface(handle);
	close_usb_device(handle);
}

static bool open_usb_device(io_service_t serv, usb_handle_t *handle) {
	IOUSBConfigurationDescriptorPtr config;
	bool ret = false;

	if(query_usb_interface(serv, kIOUSBDeviceUserClientTypeID, kIOUSBDeviceInterfaceID320, (LPVOID *)&handle->device)) {
		if((*handle->device)->USBDeviceOpen(handle->device) == kIOReturnSuccess) {
			if((*handle->device)->GetConfigurationDescriptorPtr(handle->device, 0, &config) == kIOReturnSuccess && (*handle->device)->SetConfiguration(handle->device, config->bConfigurationValue) == kIOReturnSuccess && (*handle->device)->CreateDeviceAsyncEventSource(handle->device, &handle->async_event_source) == kIOReturnSuccess) {
				CFRunLoopAddSource(CFRunLoopGetCurrent(), handle->async_event_source, kCFRunLoopDefaultMode);
				handle->serv = serv;
				ret = true;
			} else {
				(*handle->device)->USBDeviceClose(handle->device);
			}
		}
		if(!ret) {
			(*handle->device)->Release(handle->device);
			IOObjectRelease(serv);
		}
	}
	return ret;
}

static bool open_usb_interface(uint8_t usb_interface, uint8_t usb_alt_interface, usb_handle_t *handle) {
	IOUSBFindInterfaceRequest interface_request;
	io_iterator_t iter;
	io_service_t serv;
	bool ret = false;
	size_t i;

	interface_request.bInterfaceProtocol = interface_request.bInterfaceSubClass = interface_request.bAlternateSetting = interface_request.bInterfaceClass = kIOUSBFindInterfaceDontCare;
	if((*handle->device)->CreateInterfaceIterator(handle->device, &interface_request, &iter) == kIOReturnSuccess) {
		serv = IO_OBJECT_NULL;
		for(i = 0; i <= usb_interface; ++i) {
			if((serv = IOIteratorNext(iter)) != IO_OBJECT_NULL) {
				if(i == usb_interface) {
					break;
				}
				IOObjectRelease(serv);
			}
		}
		IOObjectRelease(iter);
		if(serv != IO_OBJECT_NULL) {
			if(query_usb_interface(serv, kIOUSBInterfaceUserClientTypeID, kIOUSBInterfaceInterfaceID300, (LPVOID *)&handle->interface)) {
				if((*handle->interface)->USBInterfaceOpenSeize(handle->interface) == kIOReturnSuccess) {
					if(usb_alt_interface != 1 || (*handle->interface)->SetAlternateInterface(handle->interface, usb_alt_interface) == kIOReturnSuccess) {
						ret = true;
					} else {
						(*handle->interface)->USBInterfaceClose(handle->interface);
					}
				}
				if(!ret) {
					(*handle->interface)->Release(handle->interface);
				}
			}
			IOObjectRelease(serv);
		}
	}
	return ret;
}

int wait_usb_handles(usb_handle_t **found_targets, int targets[][2], unsigned int target_count) {
    CFMutableDictionaryRef matching_dict;
    io_iterator_t iter;
    io_service_t serv;
    static usb_handle_t found_targets_big[1024] = { 0 };
    unsigned int found_count = 0;
    static bool quiet = false;

    if (!quiet) {
        quiet = true;
        log_debug("Waiting for devices...");
    }
    
    while((matching_dict = IOServiceMatching(kIOUSBDeviceClassName)) != NULL) {
        if(IOServiceGetMatchingServices(0, matching_dict, &iter) == kIOReturnSuccess) {
            while((serv = IOIteratorNext(iter)) != IO_OBJECT_NULL) {
                usb_handle_t *handle;

                if(open_usb_device(serv, handle)) {
                    if(open_usb_interface(0, 0, handle)) {
                        unsigned short vid, pid;
                        (*handle->device)->GetDeviceVendor(handle->device, &vid);
                        (*handle->device)->GetDeviceProduct(handle->device, &pid);

                        for (int x = 0; x < target_count; x++) {
                            if (targets[x][0] == vid && targets[x][1] == pid) {
                                found_targets_big[found_count] = handle;
                                found_count++;
                            }
                        }
                    }
                    close_usb_interface(handle);
                }
                close_usb_device(serv);
            }

            IOObjectRelease(iter);
        }

        if (found_count > 0) {
            break;
        }
    }
    
    *found_targets = found_targets_big;
    return found_count;
}
#endif