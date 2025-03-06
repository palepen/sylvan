TARGET      ?= sylvan
SRC         := src

ARGS        ?=
C_SRC       := $(shell find $(SRC) -type f -name "*.c")
INCLUDE     := $(SRC)/include
C_INCLUDE   := $(patsubst %, -I%, $(INCLUDE))

CC_FLAGS    := -Wall -Wextra -MMD -MP $(C_INCLUDE)
LD_FLAGS    :=

OBJS := $(patsubst $(SRC)/%.c, build/%.c.o, $(C_SRC))
DEPS := $(OBJS:.o=.d)

.PHONY: all clean run

all: build/$(TARGET)

build/%.c.o: $(SRC)/%.c Makefile
	@mkdir -p $(@D)
	$(CC) $(CC_FLAGS) $< -c -o $@

build/$(TARGET): $(OBJS) Makefile
	$(CC) $(LD_FLAGS) $(OBJS) -o build/$(TARGET)

run: build/$(TARGET)
	build/$(TARGET) $(ARGS)

clean:
	-rm -rf build/

-include $(DEPS)
