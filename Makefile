INCLDIRS = -I./include -Iopenra1n/include
LIBDIRS ?= -L/usr/local/lib
SRC = $(wildcard src/*)
OBJDIR = obj
OBJS = $(patsubst src/%,$(OBJDIR)/%,$(SRC:.c=.o))
DEP_OBJS = deps/kpf.o deps/ramdisk.o deps/binpack.o

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

all: dirs submodules libopenra1n.a checkra1n-kpf-pongo ramdisk.dmg binpack.dmg $(OBJS) plooshra1n

dirs:
	@mkdir -p $(OBJDIR)

submodules:
	@git submodule update --init --remote --recursive || true

clean:
	@rm -rf plooshra1n obj
	make clean -C openra1n
	rm -rf deps/*.o deps/*.c deps/kpf deps/ramdisk deps/binpack

plooshra1n: libopenra1n.a $(OBJS) checkra1n-kpf-pongo ramdisk.dmg binpack.dmg
	$(CC) $(CFLAGS) $(USBLIB_FLAGS) $(LDFLAGS) $(INCLDIRS) $(OBJS) $(DEP_OBJS) $(LIBDIRS) $(LIBS) -o $@

$(OBJDIR)/%.o: src/%.c
	$(CC) $(CFLAGS) $(USBLIB_FLAGS) $(INCLDIRS) -c -o $@ $<

checkra1n-kpf-pongo:
	@echo "Downloading kpf"
	@curl -sLo deps/kpf https://cdn.discordapp.com/attachments/1089213912651669544/1134132488881582170/checkra1n-kpf-pongo
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

libopenra1n.a:
	@make $(ORA1N_FLAGS) -C openra1n
