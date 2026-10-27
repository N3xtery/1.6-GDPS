ARCHS = armv7 armv7s
TARGET := iphone:clang:6.1:4.3
INSTALL_TARGET_PROCESSES = Onesixgdps
THEOS_DEVICE_IP = 192.168.31.164

include $(THEOS)/makefiles/common.mk

TWEAK_NAME = 1.6-GDPS

LIBS_PREFIX = ../../libs/iOS
1.6-GDPS_FILES = main.xm $(wildcard ../jni/*.cpp) $(wildcard ./*.m)
1.6-GDPS_CFLAGS = -I$(THEOS)/sdks/iPhoneOS6.1.sdk/usr/include/libxml2 -I$(LIBS_PREFIX)/include -I../jni -fobjc-arc -Wno-error=unused-function -Wno-error=unused-variable
1.6-GDPS_LDFLAGS += -lobjc -lz -lxml2 -L$(LIBS_PREFIX)/lib/$(THEOS_CURRENT_ARCH) $(if $(filter armv7s,$(THEOS_CURRENT_ARCH)),-target armv7s-apple-ios6.0)
1.6-GDPS_LIBRARIES = substrate wolfssl curl id3tag
1.6-GDPS_FRAMEWORKS = UIKit CoreGraphics QuartzCore Foundation AVFoundation

include $(THEOS_MAKE_PATH)/tweak.mk
