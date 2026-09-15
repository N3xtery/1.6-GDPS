#include "16gdps.h"
#include <algorithm>

soinfo2* handle;
static JavaVM* jvm = nullptr;

void mprotectPatch(uintptr_t addr, void* patch, int size, bool flush) {
    uintptr_t targetAddr = handle->base + addr;
    uintptr_t pageStart = targetAddr & ~0xFFF;
    mprotect((void*)pageStart, 0x2000, PROT_READ | PROT_WRITE | PROT_EXEC);
    memcpy((void*)targetAddr, patch, size);
    mprotect((void*)pageStart, 0x2000, PROT_READ | PROT_EXEC);
    if (flush) __builtin___clear_cache((char*)targetAddr, (char*)(targetAddr + size));
}

#if defined(__i386__)
void ZzHookReplace(void* fun, void* my_fun, void** orig) {
    subhook_t fun_hook = subhook_new(fun, my_fun, SUBHOOK_TRAMPOLINE);
    subhook_install(fun_hook);
    if (orig) *orig = subhook_get_trampoline(fun_hook);
}
#endif

template<typename... Args>
static void callJavaFunction(const char* className, const char* func, const char* ret_arg, Args... args) {
    JNIEnv* env = nullptr;
    jvm->GetEnv((void**)&env, JNI_VERSION_1_4);
    char path[50];
    snprintf(path, 50, "com/necytdamu/onesixgdps/%s", className);
    jclass cls = env->FindClass(path);
    jmethodID mid = env->GetStaticMethodID(cls, func, ret_arg);
    env->CallStaticVoidMethod(cls, mid, args...);
    env->DeleteLocalRef(cls);
}

void buildPlayLoadingDialog(const char* id) {
    JNIEnv* env = nullptr;
    jvm->GetEnv((void**)&env, JNI_VERSION_1_4);
    callJavaFunction("SongsDialog", "buildPlayLoadingDialog", "(Ljava/lang/String;)V", env->NewStringUTF(id));
}

static void showButtonsGroup(const char* group, bool show) {
    JNIEnv* env = nullptr;
    jvm->GetEnv((void**)&env, JNI_VERSION_1_4);
    callJavaFunction("OverlayUI", "setButtonVisible", "(Ljava/lang/String;Z)V", env->NewStringUTF(group), (jboolean)show);
}

void showMessageBox(const char* title, const char* msg) {
    JNIEnv* env = nullptr;
    jvm->GetEnv((void**)&env, JNI_VERSION_1_4);
    callJavaFunction("OverlayUI", "showMessageBox", "(Ljava/lang/String;Ljava/lang/String;)V", env->NewStringUTF(title), env->NewStringUTF(msg));
}

static int (*origOptionsShow)(void*);
static int hookOptionsShow(void* self) {
    showButtonsGroup("Options", true);
    return origOptionsShow(self);
}

static int (*origOptionsHide)(void*);
static int hookOptionsHide(void* self) {
    showButtonsGroup("Options", false);
    return origOptionsHide(self);
}

static int (*origLevelSettingsShow)(void*, void*);
static int hookLevelSettingsShow(void* self, void* obj) {
    showButtonsGroup("LevelSettings", true);
    return origLevelSettingsShow(self, obj);
}

static int (*origLevelSettingsHide)(void*);
static int hookLevelSettingsHide(void* self) {
    showButtonsGroup("LevelSettings", false);
    return origLevelSettingsHide(self);
}

static int getLevelStars(void* self) {return *(int*)((char*)self + 0x18C);}
static bool getLevelFeatured(void* self) {return *(bool*)((char*)self + 0x184);}
int getLevelID(void* self) {return *(int*)((char*)self + 0x128);}
int getLevelLocalID(void* self) {return *(int*)((char*)self + 0x1E4);}
void* currentLevelLayer = nullptr;
void* currentLevel = nullptr;
static bool levelMenuShown = false;
static std::vector<std::pair<int, int>> copies;
static int (*origLevelMenuShow)(void*, void*);
static int hookLevelMenuShow(void* self, void* level) {
    currentLevelLayer = self;
    currentLevel = level;
    int orig = 0;
    for (int i = 0; i < copies.size(); i++) if (copies[i].first == getLevelLocalID(level)) {
        orig = copies[i].second;
        break;
    }
    callJavaFunction("OverlayUI", "setOriginalText", "(I)V", (jint)orig);
    showButtonsGroup("LevelMenu", true);
    callJavaFunction("RateMenu", "setStarsFeatured", "(IZ)V", (jint)getLevelStars(level), (jboolean)getLevelFeatured(level));
    levelMenuShown = false;
    return origLevelMenuShow(self, level);
}

std::vector<std::pair<int, std::string>> customSongs;
std::vector<std::pair<int, float>> songOffsets;
static int (*origLevelEditShow)(void*, void*);
static int hookLevelEditShow(void* self, void* level) {
    int localID = getLevelLocalID(level);
#if defined(__i386__)
    if ((uintptr_t)__builtin_return_address(0) - handle->base == 0x1BA606) {
#elif defined(__ARM_ARCH_7A__)
    if ((uintptr_t)__builtin_return_address(0) - handle->base == 0x1589D9) {
#else
    if ((uintptr_t)__builtin_return_address(0) - handle->base == 0x15B373) {
#endif
        copies.emplace_back(localID, getLevelID(currentLevel));
        int origLocalID = getLevelLocalID(currentLevel);
        for (int i = 0; i < customSongs.size(); i++) if (customSongs[i].first == origLocalID) {
            customSongs.emplace_back(localID, customSongs[i].second);
            break;
        }
        for (int i = 0; i < songOffsets.size(); i++) if (songOffsets[i].first == origLocalID) {
            songOffsets.emplace_back(localID, songOffsets[i].second);
            break;
        }
    }
    JNIEnv* env = nullptr;
    jvm->GetEnv((void**)&env, JNI_VERSION_1_4);

    const char* song = "";
    for (int i = 0; i < customSongs.size(); i++) if (customSongs[i].first == localID) {
        song = customSongs[i].second.c_str();
        break;
    }
    callJavaFunction("SongsDialog", "setCurrentSongID", "(Ljava/lang/String;)V", env->NewStringUTF(song));

    float offset = 0;
    for (int i = 0; i < songOffsets.size(); i++) if (songOffsets[i].first == localID) {
        offset = songOffsets[i].second;
        break;
    }
    callJavaFunction("SongsDialog", "setCurrentSongOffset", "(F)V", (jfloat)offset);

    currentLevel = level;
    return origLevelEditShow(self, level);
}

static int (*origMainLevelsShow)(void*, int);
static int hookMainLevelsShow(void* self, int var) {
    currentLevel = nullptr;
    return origMainLevelsShow(self, var);
}

static int (*origTransitionScene)(float, void*, void*);
static int hookTransitionScene(float var, void* scene1, void* scene2) {
    if (currentLevelLayer) {
        if (levelMenuShown) {
            showButtonsGroup("LevelMenu", false);
            currentLevelLayer = nullptr;
        } else levelMenuShown = true;
    }
    return origTransitionScene(var, scene1, scene2);
}

static void (*cloneLevel)(void* self);
extern "C" JNIEXPORT void JNICALL
Java_com_necytdamu_onesixgdps_OverlayUI_copyLevel(JNIEnv* env, jclass) {
    queueOnCocosThread([]() {
        cloneLevel(currentLevelLayer);
    });
}

static void (*dictSetIntForKey)(void* self, const char* key, int value);
static void (*dictSetStrForKey)(void* self, const char* key, std::string* str);
static void (*dictSetFloatForKey)(void* self, const char* key, float value);
static void (*origLevelWriteXml)(void*, void*);
static void hookLevelWriteXml(void* level, void* dict) {
    origLevelWriteXml(level, dict);
    for (int i = 0; i < copies.size(); i++) if (copies[i].first == getLevelLocalID(level)) {
        dictSetIntForKey(dict, "k42", copies[i].second);
        break;
    }
    for (int i = 0; i < customSongs.size(); i++) if (customSongs[i].first == getLevelLocalID(level)) {
        dictSetStrForKey(dict, "k45", &customSongs[i].second);
        break;
    }
    for (int i = 0; i < songOffsets.size(); i++) if (songOffsets[i].first == getLevelLocalID(level)) {
        dictSetFloatForKey(dict, "k1337", songOffsets[i].second);
        break;
    }
}

static int (*dictGetIntForKey)(void* self, const char* key);
static void (*dictGetStrForKey)(std::string** out, void* self, const char* key);
static float (*dictGetFloatForKey)(void* self, const char* key);
static void* (*origLevelReadXml)(int, void*);
static void* hookLevelReadXml(int type, void* dict) {
    void* level = origLevelReadXml(type, dict);
    if (type == 4) {
        int orig_level = dictGetIntForKey(dict, "k42");
        if (orig_level) copies.emplace_back(getLevelLocalID(level), orig_level);
#if defined(__i386__)
        unsigned int songID = (unsigned int)dictGetIntForKey(dict, "k45");
        char songIDstr[10];
        snprintf(songIDstr, 50, "%u", songID);
        if (songID) customSongs.emplace_back(getLevelLocalID(level), std::string(songIDstr));
#else
        std::string* songID;
        dictGetStrForKey(&songID, dict, "k45");
        if (*(uintptr_t**)songID) {
            std::string* str = (std::string*)&songID;
            if (*str != "") customSongs.emplace_back(getLevelLocalID(level), *str);
        }
#endif
        float offset = dictGetFloatForKey(dict, "k1337");
        if (offset) songOffsets.emplace_back(getLevelLocalID(level), offset);
    }
    return level;
}

static const char* changeHttpData(std::string newStr) {
    int localID = getLevelLocalID(currentLevel);
    char buf[50];
    for (int i = 0; i < copies.size(); i++) if (copies[i].first == localID) {
        snprintf(buf, 50, "&original=%d", copies[i].second);
        newStr.append(buf);
        break;
    }
    for (int i = 0; i < customSongs.size(); i++) if (customSongs[i].first == localID) {
        newStr.append("&songID=");
        newStr.append(customSongs[i].second);
        break;
    }
    for (int i = 0; i < songOffsets.size(); i++) if (songOffsets[i].first == localID) {
        snprintf(buf, 50, "&songOffset=%.4f", songOffsets[i].second);
        newStr.append(buf);
        break;
    }
    return newStr.c_str();
}

#if defined(__i386__)
void origHttpSetRequestData(void* self, const char* str, int len) {
    auto data = (std::vector<char>*)((char*)self + 0x20);
    data->assign(str, str + len);
}
static void (*origHttpSend)(void*, void*);
static void hookHttpSend(void* self, void* request) {
    if ((uintptr_t)__builtin_return_address(0) - handle->base == 0x1D5A85) {
        auto data = (std::vector<char>*)((char*)request + 0x20);
        const char* newStr = changeHttpData(std::string(data->begin(), data->end()));
        data->assign(newStr, newStr + strlen(newStr));
    }
    origHttpSend(self, request);
}
#else
void (*origHttpSetRequestData)(void*, const char*, int);
static void hookHttpSetRequestData(void* self, const char* str, int len) {
#if defined(__ARM_ARCH_7A__)
    if ((uintptr_t)__builtin_return_address(0) - handle->base == 0x163945) {
#else
    if ((uintptr_t)__builtin_return_address(0) - handle->base == 0x1665BF) {
#endif
        const char* newStr = changeHttpData(std::string(str));
        origHttpSetRequestData(self, newStr, strlen(newStr));
        return;
    }
    origHttpSetRequestData(self, str, len);
}
#endif

static void* (*CCdictGetValue)(void* self, std::string* key);
static int (*getIntFromCCStr)(void* self);
static const char* (*getStrFromCCStr)(void* self);
static float (*getFloatFromCCStr)(void* self);
static void* (*origCreateLevelFromResponse)(void*);
static void* hookCreateLevelFromResponse(void* dict) {
    void* level = origCreateLevelFromResponse(dict);
#if defined(__i386__)
    if ((uintptr_t)__builtin_return_address(0) - handle->base != 0x1CF97A) {
#else
    if ((uintptr_t)__builtin_return_address(0) - handle->base != 0x1608E7) {
#endif
        if (currentLevel && getLevelID(currentLevel) == getLevelID(level)) *(int*)((char*)level + 0x1E4) = getLevelLocalID(currentLevel);
        int localID = getLevelLocalID(level);

        std::string key = "30";
        void* CCStr = CCdictGetValue(dict, &key);
        int orig = getIntFromCCStr(CCStr);
        if (orig) {
            auto it = std::find_if(copies.begin(), copies.end(), [localID](const std::pair<int, int>& p) { return p.first == localID; });
            if (it != copies.end()) it->second = orig;
            else copies.emplace_back(localID, orig);
        }

        key = "35";
        CCStr = CCdictGetValue(dict, &key);
        const char* song = getStrFromCCStr(CCStr);
        if (song[0] && song[0] != '0') {
            auto it = std::find_if(customSongs.begin(), customSongs.end(), [localID](const std::pair<int, std::string>& p) { return p.first == localID; });
            if (it != customSongs.end()) it->second = std::string(song);
            else customSongs.emplace_back(localID, std::string(song));
        }

        key = "1337";
        CCStr = CCdictGetValue(dict, &key);
        float offset = getFloatFromCCStr(CCStr);
        if (offset) {
            auto it = std::find_if(songOffsets.begin(), songOffsets.end(), [localID](const std::pair<int, float>& p) { return p.first == localID; });
            if (it != songOffsets.end()) it->second = offset;
            else songOffsets.emplace_back(localID, offset);
        }
    }
    return level;
}

extern "C" JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM* vm, void* reserved) {
    handle = (soinfo2*)dlopen("libgame.so", RTLD_LAZY);
    jvm = vm;

    cloneLevel = (void(*)(void*))(dlsym(handle, "_ZN14LevelInfoLayer7onCloneEv"));
    dictSetIntForKey = (void(*)(void*, const char*, int))(dlsym(handle, "_ZN13DS_Dictionary16setIntegerForKeyEPKci"));
    dictGetIntForKey = (int(*)(void*, const char*))(dlsym(handle, "_ZN13DS_Dictionary16getIntegerForKeyEPKc"));
    dictSetStrForKey = (void(*)(void*, const char*, std::string*))(dlsym(handle, "_ZN13DS_Dictionary15setStringForKeyEPKcRKSs"));
    dictGetStrForKey = (void(*)(std::string**, void*, const char*))(dlsym(handle, "_ZN13DS_Dictionary15getStringForKeyEPKc"));

    dictSetFloatForKey = (void(*)(void*, const char*, float))(dlsym(handle, "_ZN13DS_Dictionary14setFloatForKeyEPKcf"));
    dictGetFloatForKey = (float(*)(void*, const char*))(dlsym(handle, "_ZN13DS_Dictionary14getFloatForKeyEPKc"));
    CCdictGetValue = (void*(*)(void*, std::string*))(dlsym(handle, "_ZN7cocos2d12CCDictionary11valueForKeyERKSs"));
    getIntFromCCStr = (int(*)(void*))(dlsym(handle, "_ZNK7cocos2d8CCString8intValueEv"));
    getStrFromCCStr = (const char*(*)(void*))(dlsym(handle, "_ZNK7cocos2d8CCString10getCStringEv"));
    getFloatFromCCStr = (float(*)(void*))(dlsym(handle, "_ZNK7cocos2d8CCString10floatValueEv"));

    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN12OptionsLayer6createEv") | THUMB_BIT),
                  (void*)hookOptionsShow, (void**)&origOptionsShow);

    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN12OptionsLayerD2Ev") | THUMB_BIT),
                  (void*)hookOptionsHide, (void**)&origOptionsHide);

    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN18LevelSettingsLayer4initEP19LevelSettingsObject") | THUMB_BIT),
                  (void*)hookLevelSettingsShow, (void**)&origLevelSettingsShow);

    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN18LevelSettingsLayerD2Ev") | THUMB_BIT),
                  (void*)hookLevelSettingsHide, (void**)&origLevelSettingsHide);

    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN14LevelInfoLayer4initEP11GJGameLevel") | THUMB_BIT),
                  (void*)hookLevelMenuShow, (void**)&origLevelMenuShow);

    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN14EditLevelLayer4initEP11GJGameLevel") | THUMB_BIT),
                  (void*)hookLevelEditShow, (void**)&origLevelEditShow);

    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN16LevelSelectLayer4initEi") | THUMB_BIT),
                  (void*)hookMainLevelsShow, (void**)&origMainLevelsShow);

    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN11GJGameLevel15encodeWithCoderEP13DS_Dictionary") | THUMB_BIT),
                  (void*)hookLevelWriteXml, (void**)&origLevelWriteXml);

    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN13ObjectDecoder16getDecodedObjectEiP13DS_Dictionary") | THUMB_BIT),
                  (void*)hookLevelReadXml, (void**)&origLevelReadXml);
#if defined(__i386__)
    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN7cocos2d9extension12CCHttpClient4sendEPNS0_13CCHttpRequestE") | THUMB_BIT),
                  (void*)hookHttpSend, (void**)&origHttpSend);
#else
    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN7cocos2d9extension13CCHttpRequest14setRequestDataEPKcj") | THUMB_BIT),
                  (void*)hookHttpSetRequestData, (void**)&origHttpSetRequestData);
#endif

    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN11GJGameLevel6createEPN7cocos2d12CCDictionaryE") | THUMB_BIT),
                  (void*)hookCreateLevelFromResponse, (void**)&origCreateLevelFromResponse);

    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN7cocos2d16CCTransitionFade6createEfPNS_7CCSceneERKNS_10_ccColor3BE") | THUMB_BIT),
                  (void*)hookTransitionScene, (void**)&origTransitionScene);

    networkcpp_init();
    percentagecpp_init();
    songscpp_init();
    utilscpp_init();

    const char* path = "/data/data/com.necytdamu.onesixgdps/";
    const char* server = "http://gdpsnazarva.7m.pl";

#if defined(__i386__)
    uint32_t patchNewBlock = 0x3FFF3D;
    mprotectPatch(0x15004A, &patchNewBlock, 3, true);
    mprotectPatch(0x14E97E, &patchNewBlock, 3, true);
    uint64_t patchMaxError = 0x4000042444C7;
    mprotectPatch(0x14E2C2, &patchMaxError, 6, true);
    uint64_t patchPause = 0x4000082444C7;
    mprotectPatch(0x166130, &patchPause, 6, true);
    mprotectPatch(0x450DB8, (void*)path, strlen(path), false);

    const uintptr_t serverAddrs[] = {0x55616C, 0x5561A4, 0x55620C, 0x556260, 0x556294, 0x5562C8, 0x5562FC, 0x556354, 0x556388, 0x5563BC, 0x55649C, 0x556500, 0x556558, 0x55658C, 0x556604, 0x556670, 0x55670C};
#elif defined(__ARM_ARCH_7A__)
    uint32_t patchNewBlock = 0x4F80F5B0;
    mprotectPatch(0x15004A, &patchNewBlock, 4, true);
    mprotectPatch(0x14E97E, &patchNewBlock, 4, true);
    uint32_t patchMaxError = 0x0100F244;
    mprotectPatch(0x14E2C2, &patchMaxError, 4, true);
    uint32_t patchPause = 0x0200F244;
    mprotectPatch(0x166130, &patchPause, 4, true);
    mprotectPatch(0x2F5BBF, (void*)path, strlen(path), false);

    const uintptr_t serverAddrs[] = {0x3FEEB2, 0x3FEF07, 0x3FEF67, 0x3FEFD4, 0x3FF006, 0x3FF078, 0x3FF0CD, 0x3FF11C, 0x3FF163, 0x3FF1CA, 0x3FF1FC, 0x3FF264, 0x3FF2B6, 0x3FF2FC, 0x3FF36F, 0x3FF3D9, 0x3FF499};
#else
    uint16_t patchNewBlock = 0x3FFF;
    mprotectPatch(0x1529E4, &patchNewBlock, 2, true);
    mprotectPatch(0x1513A4, &patchNewBlock, 2, true);
    uint32_t patchMaxError = 0x01C92180;
    mprotectPatch(0x150CA4, &patchMaxError, 4, true);
    uint32_t patchPause = 0x01D22280;
    mprotectPatch(0x168E3A, &patchPause, 4, true);
    mprotectPatch(0x3035BF, (void*)path, strlen(path), false);

    const uintptr_t serverAddrs[] = {0x40C8B2, 0x40C907, 0x40C967, 0x40C9D4, 0x40CA06, 0x40CA78, 0x40CACD, 0x40CB1C, 0x40CB63, 0x40CBCA, 0x40CBFC, 0x40CC64, 0x40CCB6, 0x40CCFC, 0x40CD6F, 0x40CDD9, 0x40CE99};
#endif
    for (int i = 0; i < 17; i++) mprotectPatch(serverAddrs[i], (void*)server, strlen(server), false);

    const char* error = dlerror();
    if (error) __android_log_print(ANDROID_LOG_INFO, "1.6 GDPS", "dlerror: %s", error);
    return JNI_VERSION_1_4;
}
