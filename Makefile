.PHONY: all clean test test-emu

COPTS = --std=c99 -g3 -Og -Wall -Wextra -Werror=implicit-function-declaration -Werror=return-type

ASMPP_OPTS = -undef 

TARGET_DIR = targets
COMMON_DIR = common
EXAMPLE_DIR = examples
TEST_DIR = tests
TEST_EMU_DIR = $(TEST_DIR)/emu
# Support files to be ignored when scanning for tests
TEST_EMU_SUPPORT = platform.inc platform.S platform.s platform.o

TEST_EMU_MAX_CYCLES = 100000

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

getasms = $(wildcard $(1)/*.S)
getasmobjs = $(patsubst %.S,%.o,$(call getasms,$(1)))
gettestasmobjs = $(call getasmobjs,$(TEST_EMU_DIR)) $(call getasmobjs,$(TEST_EMU_DIR)/$(1))

GENSRCS = $(call getgensrcs,$(COMMON_DIR)) $(foreach t,$(TARGET_DIRS),$(call getgensrcs,$(t)))
OBJS = $(call getobjs,$(COMMON_DIR)) $(foreach t,$(TARGET_DIRS),$(call getobjs,$(t)))
DEPS = $(OBJS:.o=.d)

TARGETS = $(TARGET_DIRS:$(TARGET_DIR)/%=$(OUT_DIR)/%)

ASSEMBLY_EXS = $(wildcard $(EXAMPLE_DIR)/*.S)
ASSEMBLY_EXS_PREPROCESS = $(ASSEMBLY_EXS:.S=.s)
EMU_SUPPORT_PATHS = $(TEST_EMU_SUPPORT:%=$(TEST_EMU_DIR)/%)
EMU_TEST_DIRS = $(filter-out $(EMU_SUPPORT_PATHS),$(wildcard $(TEST_EMU_DIR)/*))
EMU_TEST_OBJS = $(call getasmobjs,$(TEST_EMU_DIR)) $(foreach t,$(EMU_TEST_DIRS),$(call getasmobjs,$(t)))
EMU_TESTS = $(EMU_TEST_DIRS:$(TEST_EMU_DIR)/%=test_emu_%)
.PHONY: $(EMU_TESTS)

.DEFAULT_GOAL=all

# Skip default rules to stop make from using the system compiler to produce asm .o's
.SUFFIXES:
all: $(TARGETS)

test: test-emu

test-emu: $(EMU_TESTS)

%.yy.c: %.lex
	lex -o $@ $<
%.tab.c: %.yacc
	yacc -o $@ $<

%.o: %.s out/as
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

.DELETE_ON_ERROR:
$(TEST_EMU_OUT_DIR)/%_out.bin: $(TEST_EMU_OUT_DIR)/%_rom.bin out/emu | $(TEST_EMU_OUT_DIR)
	out/emu $< -c $(TEST_EMU_MAX_CYCLES) -t $(TEST_EMU_OUT_DIR)/$*_trace.txt > $@ 

$(EMU_TESTS): test_emu_%: $(TEST_EMU_OUT_DIR)/%_out.bin $(TEST_EMU_OUT_DIR)/%_verify.bin
	@cmp $^ || (echo "Emulator test $* failed" && exit 255)
	@echo "Emulator test $* passed"

# Temporary until linker works
.PRECIOUS: $(TEST_EMU_OUT_DIR)/%_rom.bin
.SECONDEXPANSION:
$(TEST_EMU_OUT_DIR)/%_rom.bin: out/ld $$(call gettestasmobjs,$$*) | $(TEST_EMU_OUT_DIR)
	out/ld -b binary -o $@ $(filter %.o,$^)

$(TEST_EMU_OUT_DIR)/%_verify.bin: $(TEST_EMU_DIR)/%/reference.py | $(TEST_EMU_OUT_DIR)
	python3 $< > $@

$(DIRS): %:
	mkdir -p $@

clean:
	rm -rf $(DEPS) $(OBJS) $(EMU_TEST_OBJS) $(GENSRCS) $(ASSEMBLY_EXS_PREPROCESS) $(OUT_DIR) $(TEST_OUT_DIR)

-include $(DEPS)
