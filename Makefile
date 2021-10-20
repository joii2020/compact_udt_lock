TARGET := riscv64-unknown-linux-gnu
CC := $(TARGET)-gcc
LD := $(TARGET)-gcc
OBJCOPY := $(TARGET)-objcopy
ifdef $(CUDT_DEBUG)
CKB_OPTIMIZATION := -O0 -DCKB_C_STDLIB_PRINTF
else
CKB_OPTIMIZATION := -O3
endif

CFLAGS_INCLUDE := \
	-I deps/ckb-c-std-lib \
	-I deps/ckb-c-std-lib/libc \
	-I deps/ckb-c-std-lib/molecule \
	-I deps/sparse-merkle-tree/c \
	-I c \
	-I build

CFLAGS := -fPIC -fno-builtin-printf -fno-builtin-memcmp -nostdinc -nostdlib -nostartfiles -fvisibility=hidden -fdata-sections -ffunction-sections ${CFLAGS_INCLUDE} -Wall -Werror -Wno-nonnull -Wno-nonnull-compare -Wno-unused-function -g
LDFLAGS := -Wl,-static -fdata-sections -ffunction-sections -Wl,--gc-sections

# docker pull nervos/ckb-riscv-gnu-toolchain:gnu-bionic-20191012
BUILDER_DOCKER := nervos/ckb-riscv-gnu-toolchain@sha256:aae8a3f79705f67d505d1f1d5ddc694a4fd537ed1c7e9622420a470d59ba2ec3

BUILD_ALL_SRC := \
	build \
	build/compact_udt_lock

BUILD_CUDT_SRC := \
	c/compact_udt_lock.c \
	c/compact_udt_lock.h

all: ${BUILD_ALL_SRC}

all-via-docker:
	docker run --rm -v `pwd`:/code ${BUILDER_DOCKER} bash -c "cd /code && make all"

build:
	mkdir -p build

mol:
	cd c/mol && make all

build/compact_udt_lock: ${BUILD_CUDT_SRC}
	$(CC) $(CFLAGS) $(CKB_OPTIMIZATION) $(LDFLAGS) -o $@ $^
	$(OBJCOPY) --only-keep-debug $@ $@.debug
	$(OBJCOPY) --strip-debug --strip-all $@

clean:
	rm -rf build/compact_udt_lock build/compact_udt_lock.debug 
