TARGET := riscv64-unknown-linux-gnu
CC := $(TARGET)-gcc
LD := $(TARGET)-gcc
OBJCOPY := $(TARGET)-objcopy
ifdef $(CUDT_DEBUG)
OPTIMIZATION := -O0 -DCKB_C_STDLIB_PRINTF
else
OPTIMIZATION := -O3
endif

CFLAGS := -fPIC $(OPTIMIZATION) -fno-builtin-printf -fno-builtin-memcmp -nostdinc -nostdlib -nostartfiles -fvisibility=hidden -fdata-sections -ffunction-sections -I deps/ckb-c-std-lib -I deps/ckb-c-std-lib/libc -I deps/ckb-c-std-lib/molecule -I c -I build -Wall -Werror -Wno-nonnull -Wno-nonnull-compare -Wno-unused-function -g
LDFLAGS := -Wl,-static -fdata-sections -ffunction-sections -Wl,--gc-sections

# docker pull nervos/ckb-riscv-gnu-toolchain:gnu-bionic-20191012
BUILDER_DOCKER := nervos/ckb-riscv-gnu-toolchain@sha256:aae8a3f79705f67d505d1f1d5ddc694a4fd537ed1c7e9622420a470d59ba2ec3

all: build build/compact_udt_lock
	

all-via-docker: ${PROTOCOL_HEADER}
	docker run --rm -v `pwd`:/code ${BUILDER_DOCKER} bash -c "cd /code && make"

build:
	mkdir -p build

build/compact_udt_lock: c/compact_udt_lock.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^
	$(OBJCOPY) --only-keep-debug $@ $@.debug
	$(OBJCOPY) --strip-debug --strip-all $@

clean:
	rm -rf build/compact_udt_lock
