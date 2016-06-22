BUILD_DIR ?= $(CURDIR)
OBJ_DIR = $(BUILD_DIR)/obj/extdep/$(notdir $(CURDIR))
LIB_DIR = $(BUILD_DIR)/lib/extdep

.PHONY: all clean
all: $(LIB_DIR)/libhttp_curl.a
clean:
	-rm -rf $(OBJ_DIR)
	-rm -rf $(LIB_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(LIB_DIR):
	mkdir -p $(LIB_DIR)

INCLUDE := ../include 
INCLUDE_PARAMS = $(foreach d, $(INCLUDE), -I$d)
override CFLAGS += $(INCLUDE_PARAMS) -DFLOW_HTTP_BYPASS_SSL_CHECK -DFLOW_DEBUG_ON
vpath %.h ../include
$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@ 
$(LIB_DIR)/libhttp_curl.a: $(OBJ_DIR)/http.o | $(LIB_DIR)
	$(AR) $(ARFLAGS) $@ $^
