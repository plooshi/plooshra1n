#include <openra1n.h>
#include <common/log.h>
#include <device_detection.h>
#include <ensure_dfu.h>
#include <usb.h>
#include <wait_device.h> 
#include <options.h>
#include <paleinfo.h>
#include <pongo.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
    parse_options(argc, argv);
    if (!ensure_dfu()) return 1;
    if (palerain_flags & palerain_option_dfuhelper_only) return 0;
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
    if (palerain_flags & palerain_option_pongo_exit) return 0;

    boot_handler();
    return 0;
}