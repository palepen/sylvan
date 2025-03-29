TARGET      ?= sylvan
SRC         := src
LIB         := lib
BUILD       := build

ARGS        ?=
C_SRC       := $(shell find $(SRC) $(LIB) -type f -name "*.c")
INCLUDE     := include
C_INCLUDE   := $(patsubst %, -I%, $(INCLUDE))

CC_FLAGS    := -Wall -Wextra -Wmissing-field-initializers -MMD -MP $(C_INCLUDE)
LD_FLAGS    := -lreadline

OBJS := $(patsubst %.c, $(BUILD)/%.o, $(C_SRC))
DEPS := $(OBJS:.o=.d)

.PHONY: all clean run

all: $(BUILD)/$(TARGET)

$(BUILD)/%.o: %.c Makefile
	@mkdir -p $(@D)
	$(CC) $(CC_FLAGS) -c $< -o $@

$(BUILD)/$(TARGET): $(OBJS) Makefile
	$(CC) $(OBJS) -o $@ $(LD_FLAGS)

run: $(BUILD)/$(TARGET)
	$(BUILD)/$(TARGET) $(ARGS)

clean:
	-rm -rf $(BUILD)

-include $(DEPS)
