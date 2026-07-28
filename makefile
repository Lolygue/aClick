# - configuration ------------------------------------------------------------ #

CC := gcc
CCFLAGS :=
LD := gcc
LDFLAGS := -lX11 -lXtst -lraylib

SRC_DIR := ./src
BIN_DIR := ./bin
OBJ_DIR := $(BIN_DIR)/obj
TARGET_BIN := $(BIN_DIR)/aclick-linux

# - variables ---------------------------------------------------------------- #

Q := 

SRC_FILES := $(shell find $(SRC_DIR) | grep -i ".*\.c")
OBJ_FILES := $(patsubst %.c, %.o, $(SRC_FILES))
OBJ_FILES := $(subst $(SRC_DIR), $(OBJ_DIR), $(OBJ_FILES))

$(info $(SRC_FILES))
$(info $(OBJ_FILES))

# - pre targets -------------------------------------------------------------- #

ifeq (debug,$(firstword $(MAKECMDGOALS)))
	CCFLAGS += -g -O0 -DDEBUG
else
	CCFLAGS += -Os -s
endif

ifndef VERBOSE
	Q := @
endif

# - targets ------------------------------------------------------------------ #

.PHONY: all clean

all: prebuild $(TARGET_BIN)
	$(Q)echo "[Build] finished building."
	$(Q)stat -c "[Build] \`$(TARGET_BIN)': %s bytes." $(TARGET_BIN)

prebuild:
	$(Q)echo "[Build] building targets."

$(TARGET_BIN): $(BIN_DIR) $(OBJ_DIR) $(OBJ_FILES)
	$(Q)echo "[Build] linking targets."
	$(Q)$(LD) $(LDFLAGS) $(OBJ_FILES) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(Q)echo "[Build] building \`$^'"
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CC) $(CCFLAGS) -c $^ -o $@

$(BIN_DIR):
	$(Q)mkdir -p $(BIN_DIR)

$(OBJ_DIR):
	$(Q)mkdir -p $(OBJ_DIR)

debug: DEBUG=1
debug: predebug all

predebug:
	$(Q)echo "[Debug] building with debug features."

clean:
	$(Q)rm -rf $(BIN_DIR)
	$(Q)rm -rf $(OBJ_DIR)
