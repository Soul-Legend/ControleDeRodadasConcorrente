include Config.mk
include Utils.mk

vpath %.c $(SRC_DIR)

ifdef INCL_DIR
	CCFLAGS += -I$(INCL_DIR)
endif

OBJS = $(patsubst %,$(BUILD_DIR)/%.o,$(MAIN) $(MODULES))

.PHONY: clean

$(BIN_DIR)/$(MAIN): $(OBJS) | $(BIN_DIR)
	@$(link_binary)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@$(assemble_object)

# Create missing output directories.
$(BUILD_DIR) $(BIN_DIR):
	@$(create_dir)

clean:
	@$(clean)