#include "16gdps.h"

static int percentageType = 2;
#ifndef __APPLE__
extern "C" JNIEXPORT void JNICALL
Java_com_necytdamu_onesixgdps_OverlayUI_updatePercentageNative(JNIEnv* env, jclass, jint percentageTypeJava) {
    percentageType = percentageTypeJava;
}
#endif

static void* percentageLabel = nullptr;
static void* (*getDirectorInstance)();
static void (*nodeSetAnchorPoint)(void* self, void* point);
static void (*nodeSetPos)(void* self, void* point);
static void (*CCPointCons)(void* self, float x, float z);
static void CCPointSet(void* self, float x, float z) {
    *(float*)((char*)self + 0x18) = x;
    *(float*)((char*)self + 0x1c) = z;
}
static void (*CCPointDestr)(void* self);
struct ScreenSize {
    char pad[24];
    float x;
    float y;
};
#if defined(__i386__)
static ScreenSize (*getScreenSize)(void* director);
#else
static void (*getScreenSize)(ScreenSize* out, void* director);
#endif
static bool isProgressBarActive(void* self) { return *(bool*)((char*)self + 0x1A5); }
static void (*labelSetAlignment)(void* self, int alignment);
static void setupPercentagePos() {
    void* gameManager = gameManagerGetInstance();
    void* director = getDirectorInstance();
#if defined(__i386__)
    ScreenSize size = getScreenSize(director);
#else
    ScreenSize size;
    getScreenSize(&size, director);
#endif
    char point[32];
    if (isProgressBarActive(gameManager)) {
        labelSetAlignment(percentageLabel, 0);
        CCPointCons(point, size.x * 0.5f + 110.0f, size.y - 8.0f);
        nodeSetPos(percentageLabel, point);
        CCPointSet(point, 0.0f, 0.5f);
    } else {
        labelSetAlignment(percentageLabel, 1);
        CCPointCons(point, size.x * 0.5f, size.y - 8.0f);
        nodeSetPos(percentageLabel, point);
        CCPointSet(point, 0.5f, 0.5f);
    }
    nodeSetAnchorPoint(percentageLabel, point);
    CCPointDestr(point);
}

static void* (*createLabel)(const char* text, const char* font, float var, int alignment, void* point);
static void (*layerAddChild)(void* self, void* child, int var);
static void (*nodeSetScale)(void* self, float scale);
static void (*nodeSetVisible)(void* self, bool toggle);
static int (*origPlayLayerInit)(void*, void*);
static int hookPlayLayerInit(void* self, void* level) {
    if (percentageType) {
        char point[32];
        CCPointCons(point, 0.0f, 0.0f);
        percentageLabel = createLabel("0%", "bigFont.fnt", -1, 0, point);
        nodeSetScale(percentageLabel, 0.5f);
        layerAddChild(self, percentageLabel, 15);
        CCPointDestr(point);
        nodeSetVisible(percentageLabel, true);
#if defined(__APPLE__)
        setupPercentagePos();
#endif
    }
    return origPlayLayerInit(self, level);
}

void* getPlayerPosition(void* self) { return (void*)((char*)self + 0x2C); }
static void (*labelSetStr)(void* self, char const* str);
static int (*origUpdateProgressBar)(void*);
static int hookUpdateProgressBar(void* self) {
    int res = origUpdateProgressBar(self);
    if (percentageType) {
#if defined(__APPLE__)
        void* player = *(void**)((char*)self + 0x278);
#else
        void* player = *(void**)((char*)self + 0x274);
#endif
        void* pos = getPlayerPosition(player);
        float percentage = (*(float*)((char*)pos + 24) / *(float*)((char*)self + 0x1DC)) * 100.0f;
        if (percentage >= 100.0f) percentage = 100.0f;
        char percentageStr[8];
        if (percentageType == 3) snprintf(percentageStr, 8, "%.2f%%", percentage);
        else if (percentageType == 2) snprintf(percentageStr, 8, "%.1f%%", percentage);
        else snprintf(percentageStr, 8, "%d%%", (int)percentage);
        labelSetStr(percentageLabel, percentageStr);
    }
    return res;
}

static int (*origToggleProgressBar)(void*);
static int hookToggleProgressBar(void* self) {
    int res = origToggleProgressBar(self);
    if (percentageType) setupPercentagePos();
    return res;
}

void percentagecpp_init() {
#if defined(__APPLE__)
    createLabel = (void*(*)(const char*, const char*, float, int, void*))((base + 0x32CD4) | 1);
    layerAddChild = (void(*)(void*, void*, int))((base + 0x15248) | 1);
    nodeSetScale = (void(*)(void*, float))((base + 0x34BE8) | 1);
    nodeSetVisible = (void(*)(void*, bool))((base + 0x14C48) | 1);
    nodeSetAnchorPoint = (void(*)(void*, void*))((base + 0x340B8) | 1);
    nodeSetPos = (void(*)(void*, void*))((base + 0x14A78) | 1);
    CCPointCons = (void(*)(void*, float, float))((base + 0x1DF40) | 1);
    getDirectorInstance = (void*(*)())((base + 0x165A4) | 1);
    CCPointDestr = (void(*)(void*))((base + 0x9CB0) | 1);
    getScreenSize = (void(*)(ScreenSize*, void*))((base + 0x17B48) | 1);
    labelSetStr = (void(*)(void*, const char*))((base + 0x33D44) | 1);
    labelSetAlignment = (void(*)(void*, int))((base + 0x34BAC) | 1);

    MSHookFunction((void*)((base + 0xDDE0C) | 1), (void*)hookPlayLayerInit, (void**)&origPlayLayerInit);
    MSHookFunction((void*)((base + 0xE1A64) | 1), (void*)hookUpdateProgressBar, (void**)&origUpdateProgressBar);
    MSHookFunction((void*)((base + 0xE1C60) | 1), (void*)hookToggleProgressBar, (void**)&origToggleProgressBar);
#else
    createLabel = (void*(*)(const char*, const char*, float, int, void*))(dlsym(handle, "_ZN7cocos2d13CCLabelBMFont6createEPKcS2_fNS_15CCTextAlignmentENS_7CCPointE"));
    layerAddChild = (void(*)(void*, void*, int))(dlsym(handle, "_ZN7cocos2d6CCNode8addChildEPS0_i"));
    nodeSetScale = (void(*)(void*, float))(dlsym(handle, "_ZN7cocos2d13CCLabelBMFont8setScaleEf"));
    nodeSetVisible = (void(*)(void*, bool))(dlsym(handle, "_ZN7cocos2d6CCNode10setVisibleEb"));
    nodeSetAnchorPoint = (void(*)(void*, void*))(dlsym(handle, "_ZN7cocos2d13CCLabelBMFont14setAnchorPointERKNS_7CCPointE"));
    nodeSetPos = (void(*)(void*, void*))(dlsym(handle, "_ZN7cocos2d6CCNode11setPositionERKNS_7CCPointE"));
    CCPointCons = (void(*)(void*, float, float))(dlsym(handle, "_ZN7cocos2d7CCPointC1Eff"));
    getDirectorInstance = (void*(*)())(dlsym(handle, "_ZN7cocos2d10CCDirector14sharedDirectorEv"));
#if defined(__i386__)
    CCPointDestr = (void(*)(void*))(handle->base + 0x104760);
    getScreenSize = (ScreenSize(*)(void*))(dlsym(handle, "_ZN7cocos2d10CCDirector10getWinSizeEv"));
#else
    CCPointDestr = (void(*)(void*))(dlsym(handle, "_ZN7cocos2d7CCPointD2Ev"));
    getScreenSize = (void(*)(ScreenSize*, void*))(dlsym(handle, "_ZN7cocos2d10CCDirector10getWinSizeEv"));
#endif
    labelSetStr = (void(*)(void*, const char*))(dlsym(handle, "_ZN7cocos2d13CCLabelBMFont9setStringEPKc"));
    labelSetAlignment = (void(*)(void*, int))(dlsym(handle, "_ZN7cocos2d13CCLabelBMFont12setAlignmentENS_15CCTextAlignmentE"));

    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN9PlayLayer4initEP11GJGameLevel") | THUMB_BIT),
                  (void*)hookPlayLayerInit, (void**)&origPlayLayerInit);

    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN9PlayLayer17updateProgressbarEv") | THUMB_BIT),
                  (void*)hookUpdateProgressBar, (void**)&origUpdateProgressBar);

    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN9PlayLayer17toggleProgressbarEv") | THUMB_BIT),
                  (void*)hookToggleProgressBar, (void**)&origToggleProgressBar);
#endif
}
