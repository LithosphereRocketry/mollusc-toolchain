.PHONY: clean

COPTS = --std=c99 -g3 -Og -Wall -Wextra

TARGET_DIR = targets
COMMON_DIR = common

OUT_DIR = out

DIRS = $(OUT_DIR)

TARGET_DIRS = $(wildcard $(TARGET_DIR)/*)

SRCS = $(wildcard $(COMMON_DIR)/*.c) $(foreach d,$(TARGET_DIRS),$(wildcard $(d)/*.c))
DEPS = $(SRCS:.c=.d)
OBJS = $(SRCS:.c=.o)

TARGETS = $(TARGET_DIRS:$(TARGET_DIR)/%=$(OUT_DIR)/%)

%.o: %.c
	$(CC) $(COPTS) -c -MMD $< -o $(firstword $@) -I$(COMMON_DIR)

patcard = $(info $1) $(patsubst %.c,%.o,$(wildcard $(1)))

# I hate secondary expansion
.SECONDEXPANSION:
$(TARGETS): $(OUT_DIR)/%:$(call patcard,$(COMMON_DIR)/*.c) $$(call patcard,$(TARGET_DIR)/$$*/*.c) | $(OUT_DIR)
	$(CC) $(COPTS) $^ -o $@

$(DIRS): %:
	mkdir -p $@

clean:
	rm -rf $(DEPS) $(OBJS)

-include $(DEPS)
