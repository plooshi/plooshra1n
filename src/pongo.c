#include <usb.h>
#include <stdint.h>
#include <stdio.h>
#include <common/log.h>
#include <kpf.h>
#include <ramdisk.h>
#include <binpack.h>
#include <options.h>
#include <paleinfo.h>

#define CMD_LEN_MAX 512

// palera1n code
int issue_pongo_command(usb_device_handle_t handle, char *command);
int upload_pongo_file(usb_device_handle_t handle, unsigned char *buf, unsigned int buf_len);

void *boot_device(stuff_t *arg) {
	/*if (get_found_pongo())
		return NULL;
	set_found_pongo(1);*/
	if ((palerain_flags & palerain_option_setup_rootful)) {
		strncat(xargs_cmd, " wdt=-1", 0x270 - strlen(xargs_cmd) - 1);	
	}
	usb_device_handle_t handle = arg->handle;
	issue_pongo_command(handle, NULL);	
	issue_pongo_command(handle, "fuse lock");
	issue_pongo_command(handle, "sep auto");
	upload_pongo_file(handle, deps_kpf, deps_kpf_len);
	issue_pongo_command(handle, "modload");
	issue_pongo_command(handle, palerain_flags_cmd);
	if ((palerain_flags & palerain_option_rootful))
	{
		issue_pongo_command(handle, "rootfs");
	}
#ifdef NO_RAMDISK
	if (deps_ramdisk_len != 0)
#endif
	{
		strncat(xargs_cmd, " rootdev=md0", 0x270 - strlen(xargs_cmd) - 1);
		upload_pongo_file(handle, deps_ramdisk, deps_ramdisk_len);
		issue_pongo_command(handle, "ramdisk");
	}
#ifdef NO_OVERLAY
	if (binpack_dmg_len != 0)
#endif
	{
		upload_pongo_file(handle, deps_binpack, deps_binpack_len);
		issue_pongo_command(handle, "overlay");
	}
	issue_pongo_command(handle, xargs_cmd);
	if ((palerain_flags & palerain_option_pongo_full)) goto done;
	issue_pongo_command(handle, "bootx");
	log_info("Device should now be booting!");
	if ((palerain_flags & palerain_option_setup_partial_root)) {
		log_info("Please wait up to 5 minutes for the bindfs to be created.");
		log_info("Once the device boots up to iOS, run again without the -B (Create BindFS) option to jailbreak.");
	} else if ((palerain_flags & palerain_option_setup_rootful)) {
		log_info("Please wait up to 10 minutes for the fakefs to be created.");
		log_info("Once the device boots up to iOS, run again without the -c (Create FakeFS) option to jailbreak.");
	}
	/*if (dfuhelper_thr_running) {
		pthread_cancel(dfuhelper_thread);
		dfuhelper_thr_running = false;
	}*/
done:
	//device_has_booted = true;
#ifdef USE_LIBUSB
	libusb_unref_device(arg->dev);
#endif
	//set_spin(0);
	return NULL;
}

int issue_pongo_command(usb_device_handle_t handle, char *command)
{
	uint32_t outpos = 0;
	uint32_t outlen = 0;
	int ret = USB_RET_SUCCESS;
	uint8_t in_progress = 1;
	if (command == NULL) goto fetch_output;
	size_t len = strlen(command);
	char command_buf[512];
	char stdout_buf[0x2000];
	if (len > (CMD_LEN_MAX - 2))
	{
		log_error("Pongo command %s too long (max %d)", command, CMD_LEN_MAX - 2);
		return -1;
	}
    log_debug("Executing PongoOS command: '%s'", command);
	snprintf(command_buf, 512, "%s\n", command);
	len = strlen(command_buf);
	ret = USBControlTransfer(handle, 0x21, 4, 1, 0, 0, NULL, NULL);
	if (ret)
		goto bad;
	ret = USBControlTransfer(handle, 0x21, 3, 0, 0, (uint32_t)len, command_buf, NULL);
fetch_output:
	while (in_progress) {
		ret = USBControlTransfer(handle, 0xa1, 2, 0, 0, (uint32_t)sizeof(in_progress), &in_progress, NULL);
		if (ret == USB_RET_SUCCESS)
		{
			ret = USBControlTransfer(handle, 0xa1, 1, 0, 0, 0x1000, stdout_buf + outpos, &outlen);
			if (ret == USB_RET_SUCCESS)
			{
				//write_stdout(stdout_buf + outpos, outlen);
				outpos += outlen;
				if (outpos > 0x1000)
				{
					memmove(stdout_buf, stdout_buf + outpos - 0x1000, 0x1000);
					outpos = 0x1000;
				}
			}
		}
		if (ret != USB_RET_SUCCESS)
		{
			goto bad;
		}
	}
bad:
	if (ret != USB_RET_SUCCESS)
	{
        if (command != NULL && (!strncmp("boot", command, 4))) {
			if (ret == USB_RET_IO || ret == USB_RET_NO_DEVICE || ret == USB_RET_NOT_RESPONDING)
				return 0;
			#ifdef HAVE_LIBUSB
			if (ret == LIBUSB_ERROR_PIPE) return 0;
			#endif
		}
		log_error("USB error: %s", usb_strerror(ret));
		return ret;
	}
	else
		return ret;
}

int upload_pongo_file(usb_device_handle_t handle, unsigned char *buf, unsigned int buf_len)
{
	int ret = 0;
	ret = USBControlTransfer(handle, 0x21, 1, 0, 0, 4, &buf_len, NULL);
	if (ret == USB_RET_SUCCESS)
	{
		ret = USBBulkUpload(handle, buf, buf_len);
		if (ret == USB_RET_SUCCESS)
		{
			log_debug("Uploaded %llu bytes", (unsigned long long)buf_len);
		}
	}
	return ret;
}

void io_start(stuff_t *stuff)
{
    int r = pthread_create(&stuff->th, NULL, (void *(*)(void *)) boot_device, stuff);
    if(r != 0)
    {
        log_error("pthread_create: %s", strerror(r));
		return;
    }
    pthread_join(stuff->th, NULL);
}

void io_stop(stuff_t *stuff)
{
    int r = pthread_cancel(stuff->th);
    if(r != 0)
    {
        log_error("pthread_cancel: %s", strerror(r));
		return;
    }
    r = pthread_join(stuff->th, NULL);
    if(r != 0)
    {
        log_error("pthread_join: %s", strerror(r));
		return;
    }
#ifdef USE_LIBUSB
	libusb_unref_device(stuff->dev);
#endif
}