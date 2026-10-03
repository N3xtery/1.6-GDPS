ARCHS = armv7
TARGET := iphone:clang:5.1:4.3
INSTALL_TARGET_PROCESSES = geometryjump
THEOS_DEVICE_IP = 192.168.31.164

include $(THEOS)/makefiles/common.mk

TWEAK_NAME = 1.6-GDPS

LIBS_PREFIX = ../../libs/iOS
1.6-GDPS_FILES = main.x $(wildcard ../jni/*.cpp) $(wildcard ./*.m)
1.6-GDPS_CFLAGS = -I$(THEOS)/sdks/iPhoneOS5.1.sdk/usr/include/libxml2 -I$(LIBS_PREFIX)/include -I../jni -fobjc-arc -Wno-error=unused-function -Wno-error=unused-variable
1.6-GDPS_LDFLAGS += -lobjc -lz -lxml2 -L$(LIBS_PREFIX)/lib
1.6-GDPS_LIBRARIES = substrate wolfssl curl mad id3tag
1.6-GDPS_FRAMEWORKS = UIKit CoreGraphics QuartzCore Foundation

include $(THEOS_MAKE_PATH)/tweak.mk
