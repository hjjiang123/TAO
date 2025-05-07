# SPDX-License-Identifier: BSD-3-Clause
# Copyright(c) 2010-2014 Intel Corporation

# binary name
APP = node

# all source are stored in SRCS-y
# SRCS-y := main.cpp
SRCS-y := src/main.c src/scheduler.c src/filter.c src/server.c src/subtask.c src/thread.c ../public/object.c ../public/send_command.c ../public/kafka_p.c ../public/kafka_c.c ../public/publish_middlebox.c


PKGCONF ?= pkg-config

# Build using pkg-config variables if possible
ifneq ($(shell $(PKGCONF) --exists libdpdk && echo 0),0)
$(error "no installation of DPDK found")
endif

all: shared
.PHONY: shared static
shared: build/$(APP)-shared
	ln -sf $(APP)-shared build/$(APP)
static: build/$(APP)-static
	ln -sf $(APP)-static build/$(APP)

PC_FILE := $(shell $(PKGCONF) --path libdpdk 2>/dev/null)
# CFLAGS += -O3 $(shell $(PKGCONF) --cflags libdpdk)
CFLAGS += -O0 -D DEBUG -D DEBUG_REAL_TRAFFIC -g $(shell $(PKGCONF) --cflags libdpdk)
LDFLAGS_SHARED = $(shell $(PKGCONF) --libs libdpdk) -ldl -lrdkafka
LDFLAGS_STATIC = $(shell $(PKGCONF) --static --libs libdpdk)

ifeq ($(MAKECMDGOALS),static)
# check for broken pkg-config
ifeq ($(shell echo $(LDFLAGS_STATIC) | grep 'whole-archive.*l:lib.*no-whole-archive'),)
$(warning "pkg-config output list does not contain drivers between 'whole-archive'/'no-whole-archive' flags.")
$(error "Cannot generate statically-linked binaries with this version of pkg-config")
endif
endif

CFLAGS += -DALLOW_EXPERIMENTAL_API
# CFLAGS += -DSTATISTICS
build/$(APP)-shared: $(SRCS-y) Makefile $(PC_FILE) | build
	g++ $(CFLAGS) $(SRCS-y) -I ../public -o $@ $(LDFLAGS) $(LDFLAGS_SHARED)

build/$(APP)-static: $(SRCS-y) Makefile $(PC_FILE) | build
	g++ $(CFLAGS) $(SRCS-y) -I ../public -o $@ $(LDFLAGS) $(LDFLAGS_STATIC)

build:
	@mkdir -p $@

.PHONY: clean
clean:
	rm -f build/$(APP) build/$(APP)-static build/$(APP)-shared
	test -d build && rmdir -p build || true