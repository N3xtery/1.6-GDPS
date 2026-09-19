ARCHS = armv7
TARGET := iphone:clang:4.3:4.3
INSTALL_TARGET_PROCESSES = geometryjump

include $(THEOS)/makefiles/common.mk

TWEAK_NAME = 1.6-GDPS

1.6-GDPS_FILES = main.x ../jni/main.cpp ../jni/utils.cpp ../jni/percentage.cpp ../jni/network.cpp
1.6-GDPS_CFLAGS = -I$(THEOS)/sdks/iPhoneOS4.3.sdk/usr/include/c++/4.2.1/armv7-apple-darwin10 -fobjc-arc -Wno-error=unused-function -Wno-error=unused-variable
1.6-GDPS_LDFLAGS += -lz
1.6-GDPS_LIBRARIES = substrate

include $(THEOS_MAKE_PATH)/tweak.mk
