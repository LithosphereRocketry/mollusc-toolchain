.PHONY: all clean test test-emu

COPTS = --std=c99 -g3 -Og -Wall -Wextra -Werror=implicit-function-declaration

ASMPP_OPTS = -undef 

TARGET_DIR = targets
COMMON_DIR = common
EXAMPLE_DIR = examples
TEST_DIR = tests
TEST_EMU_DIR = $(TEST_DIR)/emu
# Support files to be ignored when scanning for tests
TEST_EMU_SUPPORT = $(TEST_EMU_DIR)/platform.inc $(TEST_EMU_DIR)/.gitignore

TEST_EMU_MAX_CYCLES = 1000

TEST_OUT_DIR = test_out
TEST_EMU_OUT_DIR = $(TEST_OUT_DIR)/emu

OUT_DIR = out

DIRS = $(OUT_DIR) $(TEST_OUT_DIR) $(TEST_EMU_OUT_DIR)

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

EMU_TEST_DIRS = $(filter-out $(TEST_EMU_SUPPORT),$(wildcard $(TEST_EMU_DIR)/*))
EMU_TESTS = $(EMU_TEST_DIRS:$(TEST_EMU_DIR)/%=test_emu_%)
.PHONY: $(EMU_TESTS)

# Skip default rules to stop make from using the system compiler to produce asm .o's
.SUFFIXES:
.DEFAULT_GOAL: all
all: $(TARGETS)

test: test-emu

test-emu: $(EMU_TESTS)

%.yy.c: %.lex
	lex -o $@ $<
%.tab.c: %.yacc
	yacc -o $@ $<

$(EXAMPLE_DIR)/%.o: $(EXAMPLE_DIR)/%.s out/as
	out/as -o $@ $<

%.o: %.c
	$(CC) $(COPTS) -c -MMD $< -o $@ -I$(COMMON_DIR)

%.s: %.S
	$(CPP) $(ASMPP_OPTS) -o $@ $<

patcard = $(info $1) $(patsubst %.c,%.o,$(wildcard $(1)))

# I hate secondary expansion
.SECONDEXPANSION:
$(TARGETS): $(OUT_DIR)/%: $$(call gettgtobjs,$$*) | $(OUT_DIR)
	$(CC) $(COPTS) $^ -o $@

$(TEST_EMU_OUT_DIR)/%_out.bin: $(TEST_EMU_OUT_DIR)/%_rom.bin out/mollusc-emu | $(TEST_EMU_OUT_DIR)
	out/mollusc-emu $< -c $(TEST_EMU_MAX_CYCLES)

$(EMU_TESTS): test_emu_%: $(TEST_EMU_OUT_DIR)/%_out.bin $(TEST_EMU_OUT_DIR)/%_verify.bin
	cmp $^

# Temporary until linker works
$(TEST_EMU_OUT_DIR)/%_rom.bin: $(TEST_EMU_DIR)/%/main.s out/as | $(TEST_EMU_OUT_DIR)
	out/as $< -o $@

$(DIRS): %:
	mkdir -p $@

clean:
	rm -rf $(DEPS) $(OBJS) $(GENSRCS) $(ASSEMBLY_EXS_PREPROCESS) $(TEST_OUT_DIR)

-include $(DEPS)
