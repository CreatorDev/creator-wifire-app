LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE    := flow-extdep-debug-log-ws
LOCAL_SRC_FILES := $(shell find $(LOCAL_PATH) -name '*.c' -printf '%P\n')

LOCAL_STATIC_LIBRARIES := flow-extdep-headers flow-core-headers

include $(BUILD_STATIC_LIBRARY)
