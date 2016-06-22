all: lib headers

DIR__BUILD:=$(shell pwd)/$(dir $(firstword $(MAKEFILE_LIST)))
DIR__SDK:=$(DIR__BUILD)/../../../

include $(DIR__SDK)/rules.mk

lib: $(DIR__STAGING)/lib/libcyassl.a

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

# Add inputs and outputs from these tool invocations to the build variables 

# Tool invocations
$(DIR__STAGING)/lib/libcyassl.a: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC Archiver'
	mkdir -p $(dir $@)
	ar -r  "$@" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

SRC_HEADERS:=$(shell find $(DIR__SRC) -name *.h)
STG_HEADERS:=$(patsubst $(DIR__SRC)/%,$(DIR__STAGING)/%,$(SRC_HEADERS))

headers:  $(STG_HEADERS)

$(STG_HEADERS) : $(DIR__STAGING)/include/% : $(DIR__SRC)/include/% 
	mkdir -p $(dir $@)
	cp -fa $< $@

# Other Targets
clean:
	-$(RM) $(OBJS) $(C_DEP) $(ARCHIVES) $(DIR__STAGING)/lib/libcyassl.a $(STG_HEADERS)
	-@echo ' '

.PHONY: all clean dependents
