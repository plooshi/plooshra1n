#ifndef _PONGO_LIBUSB_H
#define _PONGO_LIBUSB_H
#ifdef HAVE_LIBUSB

#include <libusb-1.0/libusb.h>
#include <pthread.h>

#define USB_RET_SUCCESS         LIBUSB_SUCCESS
#define USB_RET_NOT_RESPONDING  LIBUSB_ERROR_OTHER
#define USB_RET_IO              LIBUSB_ERROR_IO
#define USB_RET_NO_DEVICE		LIBUSB_ERROR_NO_DEVICE
typedef int usb_ret_t;
typedef libusb_device_handle *usb_device_handle_t;

typedef struct stuff
{
    pthread_t th;
    libusb_device *dev;
    usb_device_handle_t handle;
} stuff_t;

const char *usb_strerror(usb_ret_t err);
usb_ret_t USBControlTransfer(usb_device_handle_t handle, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint32_t wLength, void *data, uint32_t *wLenDone);
usb_ret_t USBBulkUpload(usb_device_handle_t handle, void *data, int len);

#endif
#endif