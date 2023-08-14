INCLDIRS = -I./include -Iopenra1n/include
LIBDIRS ?= -L/usr/local/lib
SRC = $(wildcard src/*.c)
OBJDIR = obj
OBJS = $(patsubst src/%,$(OBJDIR)/%,$(SRC:.c=.o))
DEP_SRC = $(wildcard deps/*.c)
DEP_OBJS = $(DEP_SRC:.c=.o)

ifeq ($(shell uname),Darwin)
USBLIB_FLAGS=
CFLAGS ?= -O2 -Wno-incompatible-pointer-types -Wno-deprecated-declarations -I/usr/local/Cellar/libirecovery/1.0.0/include -I/usr/local/Cellar/libimobiledevice/1.3.0_1/include -I/usr/local/Cellar/libusbmuxd/2.0.2/include -I/usr/local/Cellar/libplist/2.2.0/include
LIBS = -lopenra1n -llz4 -lm /usr/local/Cellar/libirecovery/1.0.0/lib/libirecovery-1.0.a /usr/local/Cellar/libimobiledevice/1.3.0_1/lib/libimobiledevice-1.0.a /usr/local/Cellar/libusbmuxd/2.0.2/lib/libusbmuxd-2.0.a /usr/local/Cellar/libplist/2.2.0/lib/libplist-2.0.a /usr/local/Cellar/openssl@3/3.1.2/lib/libssl.a /usr/local/Cellar/openssl@3/3.1.2/lib/libcrypto.a -framework IOKit -framework CoreFoundation
LDFLAGS ?= -Lopenra1n -Lopenra1n/lz4 
CC := xcrun -sdk macosx clang
else
USBLIB_FLAGS=-DHAVE_LIBUSB
ORA1N_FLAGS=LIBUSB=1
CFLAGS ?= -O2 -static
LIBS = -lopenra1n -llz4 -lm -lirecovery-1.0 -limobiledevice-glue-1.0 -limobiledevice-1.0 -lusbmuxd-2.0 -lplist-2.0 -lusb-1.0 -lssl -lcrypto
LDFLAGS ?= -Lopenra1n -Lopenra1n/lz4
LIBDIRS += -L/usr/local/lib64
#CC := clang
endif
ifeq ($(OS),Windows_NT)
LIBS += -lws2_32 -lole32 -liphlpapi -lsetupapi -fstack-protector
CFLAGS += -DIRECV_STATIC
endif

all: dirs submodules libopenra1n.a blobs $(OBJS) plooshra1n

dirs:
	@mkdir -p $(OBJDIR)

submodules:
	@git submodule update --init --remote --recursive || true

clean:
	@rm -rf plooshra1n obj
	make clean -C openra1n
	rm -rf deps/*.o deps/*.c deps/kpf deps/ramdisk deps/binpack deps/old_kpf deps/old_ramdisk deps/old_binpack

plooshra1n: libopenra1n.a $(OBJS) blobs
	$(CC) $(CFLAGS) $(USBLIB_FLAGS) $(LDFLAGS) $(INCLDIRS) $(OBJS) $(DEP_OBJS) $(LIBDIRS) $(LIBS) -o $@

$(OBJDIR)/%.o: src/%.c
	$(CC) $(CFLAGS) $(USBLIB_FLAGS) $(INCLDIRS) -c -o $@ $<

blobs: checkra1n-kpf-pongo ramdisk.dmg binpack.dmg old_ramdisk.dmg old_binpack.dmg

checkra1n-kpf-pongo:
	@echo "Downloading kpf"
	@curl -sLo deps/kpf https://cdn.discordapp.com/attachments/1007048108426940578/1140796662135128155/checkra1n-kpf-pongo
	@#cp ~/Downloads/pongoOS/build/checkra1n-kpf-pongo deps/kpf
	@xxd -i deps/kpf > deps/kpf.c
	@$(CC) $(CFLAGS) $(USBLIB_FLAGS) $(INCLDIRS) -c -o deps/kpf.o deps/kpf.c

ramdisk.dmg:
	@echo "Downloading ramdisk"
	@curl -sLo deps/ramdisk https://cdn.nickchan.lol/palera1n/c-rewrite/deps/ramdisk.dmg
	@xxd -i deps/ramdisk > deps/ramdisk.c
	@$(CC) $(CFLAGS) $(USBLIB_FLAGS) $(INCLDIRS) -c -o deps/ramdisk.o deps/ramdisk.c

binpack.dmg:
	@echo "Downloading binpack"
	@curl -sLo deps/binpack https://cdn.nickchan.lol/palera1n/c-rewrite/deps/binpack.dmg
	@xxd -i deps/binpack > deps/binpack.c
	@$(CC) $(CFLAGS) $(USBLIB_FLAGS) $(INCLDIRS) -c -o deps/binpack.o deps/binpack.c

old_ramdisk.dmg:
	@echo "Downloading old ramdisk"
	@curl -sLo deps/old_ramdisk https://cdn.discordapp.com/attachments/1007048108426940578/1140257994744012860/rdsk_dora
	@xxd -i deps/old_ramdisk > deps/old_ramdisk.c
	@$(CC) $(CFLAGS) $(USBLIB_FLAGS) $(INCLDIRS) -c -o deps/old_ramdisk.o deps/old_ramdisk.c

old_binpack.dmg:
	@echo "Downloading old binpack"
	@curl -sLo deps/old_binpack https://cdn.discordapp.com/attachments/1007048108426940578/1140262817392508978/overlay.dmg
	@xxd -i deps/old_binpack > deps/old_binpack.c
	@$(CC) $(CFLAGS) $(USBLIB_FLAGS) $(INCLDIRS) -c -o deps/old_binpack.o deps/old_binpack.c


libopenra1n.a:
	@make $(ORA1N_FLAGS) -C openra1n
