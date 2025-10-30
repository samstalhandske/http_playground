CC := gcc
SRC_DIR := src
BUILD_DIR := build
DEFINES := LINUX
CFLAGS := -std=gnu99 -Isrc -Wall -Werror -Wextra -MMD -MP $(addprefix -D,$(DEFINES))
LDFLAGS := -flto -Wl,--gc-sections

LIBS :=
SRC := $(shell find -L $(SRC_DIR)  -type f -name '*.c')
OBJ := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC))
DEP := $(OBJ:.o=.d)

BIN := $(BUILD_DIR)/main

all: $(BIN)
	@echo "Build complete."

$(BIN): $(OBJ)
	@$(CC) $(LDFLAGS) $(OBJ) -o $@ $(LIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling $<..."
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(BIN)
	./$(BIN)

clean:
	@rm -rf $(BUILD_DIR)

-include $(DEP)

.PHONY: all run clean