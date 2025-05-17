.PHONY: clean

TARGET_DIRS = $(wildcard targets/*)

SRCS = $(wildcard common/*.c) $(foreach d,$(TARGET_DIRS),$(wildcard $(d)/*.c))
DEPS = $(SRCS:.c=.d)
OBJS = $(SRCS:.c=.o)

%.o: %.c
	$(CC) -c -MMD $< -o $(firstword $@)


clean:
	rm -rf $(DEPS) $(OBJS)

-include $(DEPS)
