#include <openra1n.h>
#include <common/log.h>
#include <device_detection.h>
#include <ensure_dfu.h>
#include <usb.h>
#include <wait_device.h>
#include <ramdisk.h>
#include <kpf.h>
//#include <binpack.h>
#include <pongo.h>
#include <stdio.h>
#include <string.h>

int main() {
    if (!ensure_dfu()) return 1;
    if (strcmp(get_device_mode(), "dfu") == 0) {
        if (!openra1n()) {
            log_error("Failed to put the device into pongoOS\n");
            return 1;
        }

        log_debug("Waiting for device to reconnect in pongoOS mode...");
    }

    usb_handle_t *found_targets;
    int targets[][2] = {
        {0x05ac, 0x4141}
    };
    wait_usb_handles(&found_targets, targets, sizeof(targets) / sizeof(targets[0]));
    log_debug("Device connected in pongoOS mode!");

    boot_handler();
    return 0;
}