# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Pierro Zachareas

TARGET_EXEC ?= helsing

BUILD_DIR ?= build
SRC_DIRS ?= .

# Check threads with thread sanitizer -fsanitize=thread

CFLAGS := -O2 -Wall -Wextra # compiler flags
LFLAGS := -Wl,--gc-sections # linker flags
LIBS := -pthread

SRCS := $(shell find $(SRC_DIRS) -name *.c)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

CPPFLAGS ?= $(INC_FLAGS) -MMD -MP

$(TARGET_EXEC): $(OBJS) Makefile
	$(CC) $(CFLAGS) $(LIBS) $(LFLAGS) $(OBJS) -o $@ $(LDFLAGS)

# c source
$(BUILD_DIR)/%.c.o: %.c Makefile
	$(MKDIR_P) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	$(RM) -r $(BUILD_DIR) $(TARGET_EXEC)

-include $(DEPS)

MKDIR_P ?= mkdir -p
