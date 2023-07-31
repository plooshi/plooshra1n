#include <getopt.h>
#include <stdint.h>
#include <common/log.h>
#include <paleinfo.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <options.h>

static struct option longopts[] = {
	{"setup-bindfs", no_argument, NULL, 'B'},
	{"setup-fakefs", no_argument, NULL, 'c'},
	{"clean-fakefs", no_argument, NULL, 'C'},
	{"dfuhelper", no_argument, NULL, 'd'},
	{"pongo-shell", no_argument, NULL, 'p'},
	{"pongo-full", no_argument, NULL, 'P'},
	{"serial", no_argument, NULL, 'S'},
	{"boot-args", required_argument, NULL, 'b'},
	{"fakefs", no_argument, NULL, 'f'},
	{"rootless", no_argument, NULL, 'l'},
	{"force-revert", no_argument, NULL, 'R'},
	{"safe-mode", no_argument, NULL, 's'},
	/*{"override-pongo", required_argument, NULL, 'k'},
	{"override-overlay", required_argument, NULL, 'o'},
	{"override-ramdisk", required_argument, NULL, 'r'},
	{"override-kpf", required_argument, NULL, 'K'},*/
	{NULL, 0, NULL, 0}
};

char xargs_cmd[0x270] = "xargs ";
char palerain_flags_cmd[0x30] = "plshrain";
uint64_t palerain_flags = palerain_option_verbose_boot | palerain_option_rootless;

int parse_options(int argc, char* argv[]) {
	int opt;
	int index;
	while ((opt = getopt_long(argc, argv, "BcCdpPSbflRs", longopts, NULL)) != -1)
	{
		switch (opt) {
		case 'B':
			palerain_flags |= palerain_option_setup_partial_root;
			palerain_flags |= palerain_option_setup_rootful;
			palerain_flags |= palerain_option_verbose_boot;
			break;
		case 'c':
			palerain_flags |= palerain_option_setup_rootful;
			palerain_flags |= palerain_option_verbose_boot;
			break;
		case 'C':
			palerain_flags |= palerain_option_clean_fakefs;
			break;
        case 'd':
			palerain_flags |= palerain_option_dfuhelper_only;
			break;
		case 'p':
			palerain_flags |= palerain_option_pongo_exit;
			break;
		case 'P':
			palerain_flags |= palerain_option_pongo_full;
			break;
		case 'S':
			palerain_flags &= ~palerain_option_verbose_boot;
			snprintf(xargs_cmd + strlen(xargs_cmd), sizeof(xargs_cmd), " serial=3");
			break;
		case 'b':
			if (strstr(optarg, "rootdev=") != NULL) {
				log_error("The boot arg rootdev= is already used by palera1n and cannot be overriden");
				return -1;
			} else if (strlen(optarg) > (sizeof(xargs_cmd) - 0x20)) {
                log_error("Boot arguments too long");
                return -1;
            }
			snprintf(xargs_cmd, sizeof(xargs_cmd), "xargs %s", optarg);
			break;
		case 'f':
			palerain_flags |= palerain_option_rootful;
			palerain_flags &= ~palerain_option_rootless;
			break;
		case 'l':
			palerain_flags &= ~palerain_option_rootful;
			palerain_flags |= palerain_option_rootless;
			break;
		case 'R':
			palerain_flags |= palerain_option_force_revert;
			break;
        case 's':
			palerain_flags |= palerain_option_safemode;
			break;
		default:
			break;
		}
	}

	if ((strstr(xargs_cmd, "serial=") != NULL) && (palerain_flags & palerain_option_setup_rootful)) {
		palerain_flags &= ~palerain_option_verbose_boot;
	}

	if (!(palerain_flags & palerain_option_rootful)) {
		if ((palerain_flags & palerain_option_setup_rootful)) {
			log_error("Cannot setup rootful when rootless is requested. Use -f to enable rootful mode.");
			return -1;
		}
	}
	if (!(
			(palerain_flags & palerain_option_dfuhelper_only) ||
			(palerain_flags & palerain_option_enter_recovery) ||
			(palerain_flags & palerain_option_exit_recovery) ||
			(palerain_flags & palerain_option_reboot_device)))
	{
		if (!((palerain_flags & palerain_option_pongo_exit) || (palerain_flags & palerain_option_pongo_exit)))
		{
#ifdef NO_KPF
			if (checkra1n_kpf_pongo_len == 0)
			{
				log_error("kernel patchfinder omitted in build but no override specified");
				return -1;
			}
#endif
		}
	}

	for (index = optind; index < argc; index++)
	{
		fprintf(stderr, "%s: unknown argument: %s\n", argv[0], argv[index]);
	}

    snprintf(palerain_flags_cmd, 0x30, "palera1n_flags 0x%" PRIx64, palerain_flags);
    return 0;
}