#ifndef _PONGO_H
#define _PONGO_H
#include <usb.h>

int issue_pongo_command(usb_device_handle_t handle, char *command);
int upload_pongo_file(usb_device_handle_t handle, unsigned char *buf, unsigned int buf_len);
void boot_handler();

#endif