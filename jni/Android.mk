LOCAL_PATH := $(call my-dir)

ifeq ($(TARGET_ARCH_ABI),x86)
include $(CLEAR_VARS)
LOCAL_MODULE := subhook
LOCAL_SRC_FILES := ../../libs/subhook/build/libsubhook.a
include $(PREBUILT_STATIC_LIBRARY)
else
include $(CLEAR_VARS)
LOCAL_MODULE := hookzz
LOCAL_SRC_FILES := ../../libs/HookZz/build/$(TARGET_ARCH_ABI)/libhookzz.a
include $(PREBUILT_STATIC_LIBRARY)
endif

include $(CLEAR_VARS)
LOCAL_MODULE    := gdps16
LOCAL_LDLIBS    := -L$(LOCAL_PATH) -ldl -llog -lstdc++ -lz
LOCAL_SRC_FILES :=                                   \
    main.cpp                                         \
    network.cpp                                      \
    percentage.cpp                                   \
    songs.cpp                                        \
    utils.cpp                                        \

ifeq ($(TARGET_ARCH_ABI),x86)
LOCAL_STATIC_LIBRARIES := subhook
else
LOCAL_STATIC_LIBRARIES := hookzz
endif
TARGET_NO_UNDEFINED_LDFLAGS :=
include $(BUILD_SHARED_LIBRARY)
