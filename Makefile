CC := gcc
SRC_DIR := src
BUILD_DIR := build
CFLAGS := -std=gnu99 -Isrc -Wall -Werror -Wextra -MMD -MP -DLINUX
LDFLAGS := -flto -Wl,--gc-sections

LIBS := 
SRC := $(shell find -L $(SRC_DIR)  -type f -name '*.c')
OBJ := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC))
DEP := $(OBJ:.o=.d)

BIN := main

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
	@rm -rf $(BUILD_DIR) $(BIN)
	
print:
	@echo "Källfiler: $(SRC)"
	@echo "Objektfiler: $(OBJ)"
	@echo "Dependency-filer: $(DEP)"
	
-include $(DEP)

.PHONY: all run clean