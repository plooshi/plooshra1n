#ifndef _PONGO_IOKIT_H
#define _PONGO_IOKIT_H
#ifndef HAVE_LIBUSB

#include <IOKit/IOKitLib.h>
#include <IOKit/usb/IOUSBLib.h>
#include <IOKit/IOCFPlugIn.h>

#define USB_RET_SUCCESS         KERN_SUCCESS
#define USB_RET_NOT_RESPONDING  kIOReturnNotResponding
#define USB_RET_IO              kIOReturnNotReady
#define USB_RET_NO_DEVICE       kIOReturnNoDevice

typedef IOReturn usb_ret_t;
typedef IOUSBInterfaceInterface300 **usb_device_handle_t;

typedef struct
{
    pthread_t th;
    volatile uint64_t regID;
    IOUSBDeviceInterface320 **dev;
    usb_device_handle_t handle;
} stuff_t;

const char *usb_strerror(usb_ret_t err);
usb_ret_t USBControlTransfer(usb_device_handle_t handle, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint32_t wLength, void *data, uint32_t *wLenDone);
usb_ret_t USBBulkUpload(usb_device_handle_t handle, void *data, uint32_t len);

#endif
#endif