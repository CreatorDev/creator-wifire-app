BUILD_DIR ?= $(CURDIR)
OBJ_DIR = $(BUILD_DIR)/obj/extdep/$(notdir $(CURDIR))
LIB_DIR = $(BUILD_DIR)/lib/extdep

.PHONY: all clean
all: $(LIB_DIR)/libnvs_file.a
clean:
	-rm -rf $(OBJ_DIR)
	-rm -rf $(LIB_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(LIB_DIR):
	mkdir -p $(LIB_DIR)

INCLUDE := ../include ../../core/generated/public/include ../../support/resizeable_array/include 
INCLUDE_PARAMS = $(foreach d, $(INCLUDE), -I$d) -DFLOW_DEBUG_ON
override CFLAGS += $(INCLUDE_PARAMS)
vpath %.h ../include
$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@ 
$(LIB_DIR)/libnvs_file.a: $(OBJ_DIR)/flow_nvs_impl.o $(OBJ_DIR)/flow_nvs.o | $(LIB_DIR)
	$(AR) $(ARFLAGS) $@ $^
