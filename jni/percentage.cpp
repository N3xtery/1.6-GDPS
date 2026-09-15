#include "16gdps.h"

static int percentageType = 0;
extern "C" JNIEXPORT void JNICALL
Java_com_necytdamu_onesixgdps_OverlayUI_updatePercentageNative(JNIEnv* env, jclass, jint percentageTypeJava) {
    percentageType = percentageTypeJava;
}

static void* percentageLabel = nullptr;
static void* (*createLabel)(const char* text, const char* font, float var, int alignment, void* point);
static void (*layerAddChild)(void* self, void* child, int var);
static void (*nodeSetScale)(void* self, float scale);
static void (*nodeSetVisible)(void* self, bool toggle);
static void (*nodeSetAnchorPoint)(void* self, void* point);
static void (*nodeSetPos)(void* self, void* point);
static void (*CCPointCons)(void* self, float x, float z);
static void (*CCPointSet)(void* self, float x, float z);
static void (*CCPointDestr)(void* self);
static void* (*getDirectorInstance)();
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
    }
    return origPlayLayerInit(self, level);
}

void* getPlayerPosition(void* self) {return (void*)((char*)self + 0x2C);}
static void (*labelSetStr)(void* self, char const* str);
static int (*origUpdateProgressBar)(void*);
static int hookUpdateProgressBar(void* self) {
    int res = origUpdateProgressBar(self);
    if (percentageType) {
        void* player = *(void**)((char*)self + 0x274);
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
static bool isProgressBarActive(void* self) {return *(bool*)((char*)self + 0x1A5);}
static void (*labelSetAlignment)(void* self, int alignment);
static int (*origToggleProgressBar)(void*);
static int hookToggleProgressBar(void* self) {
    int res = origToggleProgressBar(self);
    if (percentageType) {
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
    return res;
}

void percentagecpp_init() {
    createLabel = (void*(*)(const char*, const char*, float, int, void*))(dlsym(handle, "_ZN7cocos2d13CCLabelBMFont6createEPKcS2_fNS_15CCTextAlignmentENS_7CCPointE"));
    layerAddChild = (void(*)(void*, void*, int))(dlsym(handle, "_ZN7cocos2d6CCNode8addChildEPS0_i"));
    nodeSetScale = (void(*)(void*, float))(dlsym(handle, "_ZN7cocos2d13CCLabelBMFont8setScaleEf"));
    nodeSetVisible = (void(*)(void*, bool))(dlsym(handle, "_ZN7cocos2d6CCNode10setVisibleEb"));
    nodeSetAnchorPoint = (void(*)(void*, void*))(dlsym(handle, "_ZN7cocos2d13CCLabelBMFont14setAnchorPointERKNS_7CCPointE"));
    nodeSetPos = (void(*)(void*, void*))(dlsym(handle, "_ZN7cocos2d6CCNode11setPositionERKNS_7CCPointE"));
    CCPointCons = (void(*)(void*, float, float))(dlsym(handle, "_ZN7cocos2d7CCPointC1Eff"));
    CCPointSet = (void(*)(void*, float, float))(dlsym(handle, "_ZN7cocos2d7CCPoint8setPointEff"));
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
}
