.PHONY: all clean

COPTS = --std=c99 -g3 -Og -Wall -Wextra

TARGET_DIR = targets
COMMON_DIR = common
EXAMPLE_DIR = examples

OUT_DIR = out

DIRS = $(OUT_DIR)

TARGET_DIRS = $(wildcard $(TARGET_DIR)/*)

getgensrcs = $(patsubst %.lex,%.yy.c,$(wildcard $(1)/*.lex))\
			 $(patsubst %.yacc,%.tab.c,$(wildcard $(1)/*.yacc))
getsrcs = $(wildcard $(1)/*.c) $(call getgensrcs,$(1))
getobjs = $(patsubst %.c,%.o,$(call getsrcs,$(1)))
gettgtobjs = $(call getobjs,$(COMMON_DIR)) $(call getobjs,$(TARGET_DIR)/$(1))

GENSRCS = $(call getgensrcs,$(COMMON_DIR)) $(foreach t,$(TARGET_DIRS),$(call getgensrcs,$(t)))
OBJS = $(call getobjs,$(COMMON_DIR)) $(foreach t,$(TARGET_DIRS),$(call getobjs,$(t)))
DEPS = $(OBJS:.o=.d)

TARGETS = $(TARGET_DIRS:$(TARGET_DIR)/%=$(OUT_DIR)/%)

ASSEMBLY_EXS = $(wildcard $(EXAMPLE_DIR)/*.S)
ASSEMBLY_EXS_PREPROCESS = $(ASSEMBLY_EXS:.S=.s)

# Skip default rules to stop make from using the system compiler to produce asm .o's
.SUFFIXES:
.DEFAULT_GOAL: all
all: $(TARGETS)

%.yy.c: %.lex
	lex -o $@ $<
%.tab.c: %.yacc
	yacc -o $@ $<

$(EXAMPLE_DIR)/%.o: $(EXAMPLE_DIR)/%.s out/as
	out/as -o $@ $<

%.o: %.c
	$(CC) $(COPTS) -c -MMD $< -o $(firstword $@) -I$(COMMON_DIR)

%.s: %.S
	$(CPP) -undef -o $@ $<

patcard = $(info $1) $(patsubst %.c,%.o,$(wildcard $(1)))

# I hate secondary expansion
.SECONDEXPANSION:
$(TARGETS): $(OUT_DIR)/%: $$(call gettgtobjs,$$*) | $(OUT_DIR)
	$(CC) $(COPTS) $^ -o $@

$(DIRS): %:
	mkdir -p $@

clean:
	rm -rf $(DEPS) $(OBJS) $(GENSRCS) $(ASSEMBLY_EXS_PREPROCESS)

-include $(DEPS)
