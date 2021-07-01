# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Pierro Zachareas

TARGET_EXEC ?= helsing

BUILD_DIR ?= build
SRC_DIRS ?= .

# Check threads with thread sanitizer -fsanitize=thread

CC := $(shell if [ -z $(CC) ]; then echo $(CC); else echo "gcc"; fi)
CFLAGS := -O2 -Wall -Wextra # compiler flags
LFLAGS := -Wl,--gc-sections # linker flags
LIBS := -pthread $(shell if $(CC) -dM -E -I./configuration.h ./configuration_adv.h | grep -q "CHECKSUM_RESULTS"; then echo "-lcrypto"; fi)

SRCS := $(shell find $(SRC_DIRS) -name *.c)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

CPPFLAGS ?= $(INC_FLAGS) -MMD -MP

$(TARGET_EXEC): $(OBJS) Makefile
	$(CC) $(CFLAGS) $(LFLAGS) $(OBJS) -o $@ $(LDFLAGS) $(LIBS)

# c source
$(BUILD_DIR)/%.c.o: %.c Makefile
	$(MKDIR_P) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	$(RM) -r $(BUILD_DIR) $(TARGET_EXEC)

-include $(DEPS)

MKDIR_P ?= mkdir -p
