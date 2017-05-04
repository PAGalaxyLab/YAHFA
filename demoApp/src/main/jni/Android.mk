LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= hello.c

LOCAL_LDLIBS    := -llog

LOCAL_MODULE:= hello

include $(BUILD_SHARED_LIBRARY)
