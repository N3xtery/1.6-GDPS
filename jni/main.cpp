#include "16gdps.h"
#include <algorithm>

#if defined(__APPLE__)
    uintptr_t base;
    void mprotectPatch(uintptr_t addr, void* patch, int size, bool flush) {
        uintptr_t targetAddr = base + addr;
        uintptr_t pageStart = targetAddr & ~0xFFFUL;
        vm_size_t regionSize = 0x2000;
        vm_protect(mach_task_self(), (vm_address_t)pageStart, regionSize, FALSE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_COPY);
        memcpy((void*)targetAddr, patch, size);
        vm_protect(mach_task_self(), (vm_address_t)pageStart, regionSize, FALSE, VM_PROT_READ | VM_PROT_EXECUTE);
        if (flush) sys_icache_invalidate((char*)targetAddr, size);
    }
#else
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

    void buildLoadingDialog(const char* id) {
        JNIEnv* env = nullptr;
        jvm->GetEnv((void**)&env, JNI_VERSION_1_4);
        jstring javaID = env->NewStringUTF(id);
        callJavaFunction("SongsDialog", "buildPlayLoadingDialog", "(Ljava/lang/String;)V", javaID);
        env->DeleteLocalRef(javaID);
    }

    static void showButtonsGroup(const char* group, bool show) {
        JNIEnv* env = nullptr;
        jvm->GetEnv((void**)&env, JNI_VERSION_1_4);
        jstring javaGroup = env->NewStringUTF(group);
        callJavaFunction("OverlayUI", "setButtonVisible", "(Ljava/lang/String;Z)V", javaGroup, (jboolean)show);
        env->DeleteLocalRef(javaGroup);
    }

    void showMessageBox(const char* title, const char* msg) {
        JNIEnv* env = nullptr;
        jvm->GetEnv((void**)&env, JNI_VERSION_1_4);
        jstring javaTitle = env->NewStringUTF(title);
        jstring javaMsg = env->NewStringUTF(msg);
        callJavaFunction("OverlayUI", "showMessageBox", "(Ljava/lang/String;Ljava/lang/String;)V", javaTitle, javaMsg);
        env->DeleteLocalRef(javaTitle);
        env->DeleteLocalRef(javaMsg);
    }
#endif

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

static void* colorPopup;
static int (*origColorSettingsShow)(void*);
static int hookColorSettingsShow(void* self) {
    colorPopup = self;
    showButtonsGroup("Color", true);
    return origColorSettingsShow(self);
}

static int (*origColorSettingsHide)(void*);
static int hookColorSettingsHide(void* self) {
    showButtonsGroup("Color", false);
    return origColorSettingsHide(self);
}

struct Color {
    unsigned char r;
    unsigned char g;
    unsigned char b;
};
static void (*origColorSet)(void*, Color*);
static void hookColorSet(void* self, Color* color) {
#if defined(__APPLE__)
    setColor((int)color->r, (int)color->g, (int)color->b);
#else
    callJavaFunction("ColorPicker", "setColor", "(III)V", (int)color->r, (int)color->g, (int)color->b);
#endif
    origColorSet(self, color);
}

struct DoubleColor {
    double r;
    double g;
    double b;
    double a;
};
#if defined(__i386__)
static DoubleColor (*origColorSlider)(double, double, double);
static DoubleColor hookColorSlider(double h, double s, double v) {
    DoubleColor color = origColorSlider(h, s, v);
    if ((uintptr_t)__builtin_return_address(0) - handle->base != 0x2467BE)
        callJavaFunction("ColorPicker", "setColor", "(III)V", (int)(color.r * 255), (int)(color.g * 255), (int)(color.b * 255));
    return color;
}
#else
static void (*origColorSlider)(DoubleColor*, int, int, int, double, double);
static void hookColorSlider(DoubleColor* out, int unused, int h1, int h2, double s, double v) {
    origColorSlider(out, unused, h1, h2, s, v);
#if defined(__APPLE__)
    if ((uintptr_t)__builtin_return_address(0) - base != 0x25F3F)
        setColor((int)(out->r * 255), (int)(out->g * 255), (int)(out->b * 255));
#else
    if ((uintptr_t)__builtin_return_address(0) - handle->base != 0x1975B5)
        callJavaFunction("ColorPicker", "setColor", "(III)V", (int)(out->r * 255), (int)(out->g * 255), (int)(out->b * 255));
#endif
}
#endif

void setColorNative(int r, int g, int b) {
    Color color = {(unsigned char)r, (unsigned char)g, (unsigned char)b};
    origColorSet(colorPopup, &color);
}

#ifndef __APPLE__
extern "C" JNIEXPORT void JNICALL
Java_com_necytdamu_onesixgdps_ColorPicker_setColorNative(JNIEnv* env, jclass, jint r, jint g, jint b) {
    setColorNative(r, g, b);
}
#endif

static int getLevelStars(void* self) { return *(int*)((char*)self + 0x18C); }
static bool getLevelFeatured(void* self) { return *(bool*)((char*)self + 0x184); }
int getLevelID(void* self) { return *(int*)((char*)self + 0x128); }
int getLevelLocalID(void* self) { return *(int*)((char*)self + 0x1E4); }
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
#if defined(__APPLE__)
    setOriginalText(orig);
    setStarsFeatured(getLevelStars(level), getLevelFeatured(level));
#else
    callJavaFunction("OverlayUI", "setOriginalText", "(I)V", (jint)orig);
    callJavaFunction("RateMenu", "setStarsFeatured", "(IZ)V", (jint)getLevelStars(level), (jboolean)getLevelFeatured(level));
#endif
    showButtonsGroup("LevelMenu", true);
    levelMenuShown = false;
    return origLevelMenuShow(self, level);
}

std::vector<std::pair<int, std::string>> customSongs;
std::vector<std::pair<int, float>> songOffsets;
static bool copying = false;
static void* (*origLevelEditShow)(void*);
static void* hookLevelEditShow(void* level) {
    int localID = getLevelLocalID(level);
    if (copying) {
        copies.push_back(std::make_pair(localID, getLevelID(currentLevel)));
        int origLocalID = getLevelLocalID(currentLevel);
        for (int i = 0; i < customSongs.size(); i++) if (customSongs[i].first == origLocalID) {
            customSongs.push_back(std::make_pair(localID, customSongs[i].second));
            break;
        }
        for (int i = 0; i < songOffsets.size(); i++) if (songOffsets[i].first == origLocalID) {
            songOffsets.push_back(std::make_pair(localID, songOffsets[i].second));
            break;
        }
    }

    const char* song = "";
    for (int i = 0; i < customSongs.size(); i++) if (customSongs[i].first == localID) {
        song = customSongs[i].second.c_str();
        break;
    }

    float offset = 0;
    for (int i = 0; i < songOffsets.size(); i++) if (songOffsets[i].first == localID) {
        offset = songOffsets[i].second;
        break;
    }

#if defined(__APPLE__)
    setCurrentSongID(song);
    setCurrentSongOffset(offset);
#else
    JNIEnv* env = nullptr;
    jvm->GetEnv((void**)&env, JNI_VERSION_1_4);
    jstring javaSong = env->NewStringUTF(song);
    callJavaFunction("SongsDialog", "setCurrentSongID", "(Ljava/lang/String;)V", javaSong);
    env->DeleteLocalRef(javaSong);
    callJavaFunction("SongsDialog", "setCurrentSongOffset", "(F)V", (jfloat)offset);
#endif

    currentLevel = level;
    return origLevelEditShow(level);
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

static void* (*gameManagerCreateLevel)(void* self);
static void* (*createTransition)(float val, void* scene, Color* color);
static void (*replaceScene)(void* self, void* transition);
static void* (*dictCreateLevel)(void* self);
static void (*CCArrayInsert)(void* self, void* obj, int pos);
static void goToLevel(void* level) {
    void* director = getDirectorInstance();
    void* scene = hookLevelEditShow(level);
    Color color = {0, 0, 0};
    void* transition = createTransition(0.5f, scene, &color);
    replaceScene(director, transition);
}
#if defined(__APPLE__)
void cloneLevel() {
    void* gameManager = gameManagerGetInstance();
    *(int*)((char*)gameManager + 0x178) = 2;

    void* levelManager = levelManagerGetInstance();
    void* newLevel = gameManagerCreateLevel(levelManager);

    *(std::string*)((char*)newLevel + 0x134) = std::string(*(std::string*)((char*)currentLevel + 0x134));
    *(std::string*)((char*)newLevel + 0x12C) = std::string(*(std::string*)((char*)currentLevel + 0x12C));
    *(int*)((char*)newLevel + 0x148) = *(int*)((char*)currentLevel + 0x148);

    copying = true;
    goToLevel(newLevel);
    copying = false;
}
#else
static void (*cloneLevel)(void* self);
extern "C" JNIEXPORT void JNICALL
Java_com_necytdamu_onesixgdps_OverlayUI_copyLevel(JNIEnv* env, jclass) {
    queueOnCocosThread([]() {
        copying = true;
        cloneLevel(currentLevelLayer);
        copying = false;
    });
}
#endif

static void* (*keyCreateSearchObj)(const char* key);
static void* (*levelBrowserScene)(void* searchObj);
static void* (*origSearchScene)(void*, const char*);
static void* hookSearchScene(void* self, const char* str) {
    if (str && *str) {
        void* searchObj = keyCreateSearchObj(str);
        return levelBrowserScene(searchObj);
    } else return nullptr;
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
        if (orig_level) copies.push_back(std::make_pair(getLevelLocalID(level), orig_level));
#if defined(__i386__)
        unsigned int songID = (unsigned int)dictGetIntForKey(dict, "k45");
        char songIDstr[10];
        snprintf(songIDstr, 50, "%u", songID);
        if (songID) customSongs.push_back(std::make_pair(getLevelLocalID(level), std::string(songIDstr)));
#else
        std::string* songID;
        dictGetStrForKey(&songID, dict, "k45");
        if (*(uintptr_t**)songID) {
            std::string* str = (std::string*)&songID;
            if (*str != "") customSongs.push_back(std::make_pair(getLevelLocalID(level), *str));
        }
#endif
        float offset = dictGetFloatForKey(dict, "k1337");
        if (offset) songOffsets.push_back(std::make_pair(getLevelLocalID(level), offset));
    }
    return level;
}

static std::string changeHttpData(std::string newStr) {
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
    return newStr;
}

#if defined(__i386__) || defined(__APPLE__)
void origHttpSetRequestData(void* self, const char* str, int len) {
    auto data = (std::vector<char>*)((char*)self + 0x20);
    data->assign(str, str + len);
}
static void (*origHttpSend)(void*, void*);
static void hookHttpSend(void* self, void* request) {
#if defined(__APPLE__)
    if ((uintptr_t)__builtin_return_address(0) - base == 0x93EC9) {
        auto data = (std::vector<char>*)((char*)request + 0x2C);
#else
    if ((uintptr_t)__builtin_return_address(0) - handle->base == 0x1D5A85) {
        auto data = (std::vector<char>*)((char*)request + 0x20);
#endif
        std::string newStr = changeHttpData(std::string(data->begin(), data->end()));
        const char* newCStr = newStr.c_str();
        data->assign(newCStr, newCStr + strlen(newCStr));
    }
    origHttpSend(self, request);
}
#else
void (*origHttpSetRequestData)(void*, const char*, int);
static void hookHttpSetRequestData(void* self, const char* str, int len) {
    if ((uintptr_t)__builtin_return_address(0) - handle->base == 0x163945) {
        std::string newStr = changeHttpData(std::string(str));
        const char* newCStr = newStr.c_str();
        origHttpSetRequestData(self, newCStr, strlen(newCStr));
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
#if defined(__APPLE__)
    if ((uintptr_t)__builtin_return_address(0) - base != 0x8E309) {
#elif defined(__i386__)
    if ((uintptr_t)__builtin_return_address(0) - handle->base != 0x1CF97A) {
#else
    if ((uintptr_t)__builtin_return_address(0) - handle->base != 0x1608E7) {
#endif
        if (currentLevel && getLevelID(currentLevel) == getLevelID(level)) *(int*)((char*)level + 0x1E4) = getLevelLocalID(currentLevel);
        currentLevel = level;
        int localID = getLevelLocalID(level);

        std::string key = "30";
        void* CCStr = CCdictGetValue(dict, &key);
        int orig = getIntFromCCStr(CCStr);
        if (orig) {
            auto it = std::find_if(copies.begin(), copies.end(), [localID](const std::pair<int, int>& p) { return p.first == localID; });
            if (it != copies.end()) it->second = orig;
            else copies.push_back(std::make_pair(localID, orig));
        }

        key = "35";
        CCStr = CCdictGetValue(dict, &key);
        const char* song = getStrFromCCStr(CCStr);
        if (song[0] && song[0] != '0') {
            auto it = std::find_if(customSongs.begin(), customSongs.end(), [localID](const std::pair<int, std::string>& p) { return p.first == localID; });
            if (it != customSongs.end()) it->second = std::string(song);
            else customSongs.push_back(std::make_pair(localID, std::string(song)));
        }

        key = "1337";
        CCStr = CCdictGetValue(dict, &key);
        float offset = getFloatFromCCStr(CCStr);
        if (offset) {
            auto it = std::find_if(songOffsets.begin(), songOffsets.end(), [localID](const std::pair<int, float>& p) { return p.first == localID; });
            if (it != songOffsets.end()) it->second = offset;
            else songOffsets.push_back(std::make_pair(localID, offset));
        }
    }
    return level;
}

static void* editorLayer = nullptr;
static void (*origEditorShow)(void*, void*);
static void hookEditorShow(void* self, void* level) {
    editorLayer = self;
    origEditorShow(self, level);
}

static void replaceSubStr(std::string* str, const char* fromC, const char* toC) {
    std::string from(fromC), to(toC);
    for (std::string::size_type pos = 0; (pos = str->find(from, pos)) != std::string::npos; pos += to.size())
        str->replace(pos, from.size(), to);
}

static const char* gmdConverts[] = {"<key>", "<k>", "</key>", "</k>", "<integer>", "<i>", "</integer>", "</i>", "<string>", "<s>", "</string>", "</s>",
    "<real>", "<r>", "</real>", "</r>", "<true />", "<t />", "<false />", "<f />"};

static void importGmd(const char* pathC) {
    std::string path = std::string(pathC);
    queueOnCocosThread([path]() {
        FILE* f = fopen(path.c_str(), "r");
        if (f) {
            fseek(f, 0, SEEK_END);
            int length = ftell(f);
            fseek(f, 0, SEEK_SET);
            std::string data;
            data.resize(length);
            fread(&data[0], 1, length, f);
            fclose(f);
            for (int i = 0; i < 20; i += 2) replaceSubStr(&data, gmdConverts[i + 1], gmdConverts[i]);
            char* dict = (char*)malloc(0xD4);
            dsDictCons(dict);
            dsStrToDict(dict, &data);
            void* level = hookLevelReadXml(4, dict);
            *(int*)((char*)level + 0x1E0) = 2; // type
            void* levelManager = levelManagerGetInstance();
            CCArrayInsert(*(void**)((char*)levelManager + 0x130), level, 0);
            goToLevel(level);
            free(dict);
        }
    });
}

static void exportGmd(const char* pathC) {
    std::string path = std::string(pathC);
    queueOnCocosThread([path]() {
        FILE* f = fopen(path.c_str(), "w");
        if (f) {
            char* dict = (char*)malloc(0xD4);
            dsDictCons(dict);
            hookLevelWriteXml(currentLevel, dict);
            #if defined(__i386__)
            std::string data = dsDictToStr(dict);
            #else
            std::string data;
            dsDictToStr(&data, dict);
            #endif

            for (int i = 0; i < 20; i += 2) replaceSubStr(&data, gmdConverts[i], gmdConverts[i + 1]);
            fwrite(&data[0], 1, data.length(), f);
            fclose(f);
            free(dict);
            showMessageBox("Success", "Level export successful");
        } else showMessageBox("Error", "Level export failed");
    });
}

static std::string* getCurrentLevelName() {
    return (std::string*)((char*)currentLevel + 0x12C);
}

#if defined(__i386__)
static std::string (*getLevelStr)(void* self);
static int getCurrentLevelLength() {
    getLevelStr(editorLayer);
#else
static void (*getLevelStr)(void** out, void* self);
static int getCurrentLevelLength() {
    void* str = nullptr;
    getLevelStr(&str, editorLayer);
#endif
    int distance = *(int*)((char*)editorLayer + 0x164);
    return (int)roundf((distance + 340.0f) / 311.0f);
}

#ifndef __APPLE__
extern "C" JNIEXPORT void JNICALL
Java_com_necytdamu_onesixgdps_OverlayUI_importGmd(JNIEnv* env, jclass, jstring pathJava) {
    const char* pathC = env->GetStringUTFChars(pathJava, nullptr);
    importGmd(pathC);
    env->ReleaseStringUTFChars(pathJava, pathC);
}

extern "C" JNIEXPORT void JNICALL
Java_com_necytdamu_onesixgdps_OverlayUI_exportGmd(JNIEnv* env, jclass, jstring pathJava) {
    const char* pathC = env->GetStringUTFChars(pathJava, nullptr);
    exportGmd(pathC);
    env->ReleaseStringUTFChars(pathJava, pathC);
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_necytdamu_onesixgdps_OverlayUI_getCurrentLevelName(JNIEnv* env, jclass) {
    return env->NewStringUTF(getCurrentLevelName()->c_str());
}

extern "C" JNIEXPORT jint JNICALL
Java_com_necytdamu_onesixgdps_OverlayUI_getCurrentLevelLength(JNIEnv* env, jclass) {
    return getCurrentLevelLength();
}
#endif

static void (*managerSave)(void* self);
static void (*origSaveGame)(void*);
static void hookSaveGame(void* self) {
    origSaveGame(self);
    if (*(bool*)((char*)self + 0x14)) {
        void* levelManager = levelManagerGetInstance();
        managerSave(levelManager);
    }
}

#if defined(__APPLE__)
void iOSSetupNative() {
    base = _dyld_get_image_vmaddr_slide(0);

    dictSetIntForKey = (void(*)(void*, const char*, int))((base + 0x115008) | 1);
    dictGetIntForKey = (int(*)(void*, const char*))((base + 0x114BBC) | 1);
    dictSetStrForKey = (void(*)(void*, const char*, std::string*))((base + 0x1155F0) | 1);
    dictGetStrForKey = (void(*)(std::string**, void*, const char*))((base + 0x114DDC) | 1);
    dictSetFloatForKey = (void(*)(void*, const char*, float))((base + 0x11536C) | 1);
    dictGetFloatForKey = (float(*)(void*, const char*))((base + 0x114D14) | 1);
    CCdictGetValue = (void*(*)(void*, std::string*))((base + 0x1C434) | 1);
    getIntFromCCStr = (int(*)(void*))((base + 0x20EBC) | 1);
    getStrFromCCStr = (const char*(*)(void*))((base + 0x20C60) | 1);
    getFloatFromCCStr = (float(*)(void*))((base + 0x20ED4) | 1);
    gameManagerCreateLevel = (void*(*)(void*))((base + 0x8D644) | 1);
    createTransition = (void*(*)(float, void*, Color*))((base + 0x396E0) | 1);
    replaceScene = (void(*)(void*, void*))((base + 0x17CD0) | 1);
    managerSave = (void(*)(void*))((base + 0x1198D4) | 1);

    MSHookFunction((void*)((base + 0x26AE0) | 1), (void*)hookOptionsShow, (void**)&origOptionsShow);
    MSHookFunction((void*)((base + 0x26BC4) | 1), (void*)hookOptionsHide, (void**)&origOptionsHide);
    MSHookFunction((void*)((base + 0xBBDB8) | 1), (void*)hookLevelSettingsShow, (void**)&origLevelSettingsShow);
    MSHookFunction((void*)((base + 0xBBB68) | 1), (void*)hookLevelSettingsHide, (void**)&origLevelSettingsHide);
    MSHookFunction((void*)((base + 0x23ED8) | 1), (void*)hookColorSettingsShow, (void**)&origColorSettingsShow);
    MSHookFunction((void*)((base + 0x24774) | 1), (void*)hookColorSettingsHide, (void**)&origColorSettingsHide);
    MSHookFunction((void*)((base + 0x24670) | 1), (void*)hookColorSet, (void**)&origColorSet);
    MSHookFunction((void*)((base + 0x28658) | 1), (void*)hookColorSlider, (void**)&origColorSlider);
    MSHookFunction((void*)((base + 0xBD750) | 1), (void*)hookLevelMenuShow, (void**)&origLevelMenuShow);
    MSHookFunction((void*)((base + 0x7C2C0) | 1), (void*)hookLevelEditShow, (void**)&origLevelEditShow);
    MSHookFunction((void*)((base + 0xC7A0C) | 1), (void*)hookMainLevelsShow, (void**)&origMainLevelsShow);
    MSHookFunction((void*)((base + 0x9E6BC) | 1), (void*)hookLevelWriteXml, (void**)&origLevelWriteXml);
    MSHookFunction((void*)((base + 0xD3A4C) | 1), (void*)hookLevelReadXml, (void**)&origLevelReadXml);
    MSHookFunction((void*)((base + 0x2E25C) | 1), (void*)hookHttpSend, (void**)&origHttpSend);
    MSHookFunction((void*)((base + 0x8E76C) | 1), (void*)hookCreateLevelFromResponse, (void**)&origCreateLevelFromResponse);
    MSHookFunction((void*)((base + 0x396E0) | 1), (void*)hookTransitionScene, (void**)&origTransitionScene);

    networkcpp_init();
    percentagecpp_init();
    songscpp_init();
    utilscpp_init();

    //const char* path = "/data/data/com.necytdamu.onesixgdps/";
    const char* server = "http://gdpsnazarva.7m.pl";

    uint32_t patchNewBlock = 0x4F80F5B0;
    mprotectPatch(0x85EDA, &patchNewBlock, 4, true);
    mprotectPatch(0x86812, &patchNewBlock, 4, true);
    uint32_t patchMaxError = 0x0100F244;
    mprotectPatch(0x875BA, &patchMaxError, 4, true);
    uint32_t patchPause = 0x0200F244;
    mprotectPatch(0x8050A, &patchPause, 4, true);
    //mprotectPatch(0x450DB8, (void*)path, strlen(path), false);

    float patchLimit = __FLT_MAX__ / 30.0f;
    mprotectPatch(0xBAF38, &patchLimit, 4, false);
    mprotectPatch(0x8A3A8, &patchLimit, 4, false);
    mprotectPatch(0x87BAC, &patchLimit, 4, false);
    mprotectPatch(0x85E40, &patchLimit, 4, false);

    const uintptr_t serverAddrs[] = {0x262832, 0x262928, 0x26299D, 0x2629EB, 0x262A3D, 0x262ABB, 0x262B13, 0x262B75, 0x262C33, 0x262C65, 0x262CB3, 0x262D38, 0x262E2E, 0x262E8E};
    for (int i = 0; i < 14; i++) mprotectPatch(serverAddrs[i], (void*)server, strlen(server), false);
}
#else
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
    gameManagerCreateLevel = (void*(*)(void*))(dlsym(handle, "_ZN16GameLevelManager14createNewLevelEv"));
    createTransition = (void*(*)(float, void*, Color*))(dlsym(handle, "_ZN7cocos2d16CCTransitionFade6createEfPNS_7CCSceneERKNS_10_ccColor3BE"));
    replaceScene = (void(*)(void*, void*))(dlsym(handle, "_ZN7cocos2d10CCDirector12replaceSceneEPNS_7CCSceneE"));
    dictCreateLevel = (void*(*)(void*))(dlsym(handle, "_ZN11GJGameLevel15createWithCoderEP13DS_Dictionary"));
    CCArrayInsert = (void(*)(void*, void*, int))(dlsym(handle, "_ZN7cocos2d7CCArray12insertObjectEPNS_8CCObjectEj"));
    keyCreateSearchObj = (void*(*)(const char*))(dlsym(handle, "_ZN14GJSearchObject13createFromKeyEPKc"));
    levelBrowserScene = (void*(*)(void*))(dlsym(handle, "_ZN17LevelBrowserLayer5sceneEP14GJSearchObject"));
    managerSave = (void(*)(void*))(dlsym(handle, "_ZN8GManager4saveEv"));
#if defined(__i386__)
    getLevelStr = (std::string(*)(void*))(dlsym(handle, "_ZN16LevelEditorLayer14getLevelStringEv"));
#else
    getLevelStr = (void(*)(void**, void*))(dlsym(handle, "_ZN16LevelEditorLayer14getLevelStringEv"));
#endif

    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN12OptionsLayer6createEv") | THUMB_BIT),
                  (void*)hookOptionsShow, (void**)&origOptionsShow);

    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN12OptionsLayerD2Ev") | THUMB_BIT),
                  (void*)hookOptionsHide, (void**)&origOptionsHide);

    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN18LevelSettingsLayer4initEP19LevelSettingsObject") | THUMB_BIT),
                  (void*)hookLevelSettingsShow, (void**)&origLevelSettingsShow);

    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN18LevelSettingsLayerD2Ev") | THUMB_BIT),
                  (void*)hookLevelSettingsHide, (void**)&origLevelSettingsHide);
#if defined(__i386__)
    ZzHookReplace((void*)((handle->base + 0x2458C0) | THUMB_BIT),
                  (void*)hookColorSettingsShow, (void**)&origColorSettingsShow);
    ZzHookReplace((void*)((handle->base + 0x244F80) | THUMB_BIT),
                  (void*)hookColorSettingsHide, (void**)&origColorSettingsHide);
#else
    ZzHookReplace((void*)((handle->base + 0x196D58) | THUMB_BIT),
                  (void*)hookColorSettingsShow, (void**)&origColorSettingsShow);
    ZzHookReplace((void*)((handle->base + 0x1969EC) | THUMB_BIT),
                  (void*)hookColorSettingsHide, (void**)&origColorSettingsHide);
#endif

    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN7cocos2d9extension21CCControlColourPicker13setColorValueERKNS_10_ccColor3BE") | THUMB_BIT),
                  (void*)hookColorSet, (void**)&origColorSet);

    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN7cocos2d9extension14CCControlUtils10RGBfromHSVENS0_3HSVE") | THUMB_BIT),
                  (void*)hookColorSlider, (void**)&origColorSlider);

    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN14LevelInfoLayer4initEP11GJGameLevel") | THUMB_BIT),
                  (void*)hookLevelMenuShow, (void**)&origLevelMenuShow);

    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN14EditLevelLayer5sceneEP11GJGameLevel") | THUMB_BIT),
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
    ZzHookReplace((void*)((handle->base + 0x16247C) | THUMB_BIT),
                  (void*)hookHttpSetRequestData, (void**)&origHttpSetRequestData);
#endif
    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN11GJGameLevel6createEPN7cocos2d12CCDictionaryE") | THUMB_BIT),
                  (void*)hookCreateLevelFromResponse, (void**)&origCreateLevelFromResponse);

    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN7cocos2d16CCTransitionFade6createEfPNS_7CCSceneERKNS_10_ccColor3BE") | THUMB_BIT),
                  (void*)hookTransitionScene, (void**)&origTransitionScene);

    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN16GameLevelManager14getSearchSceneEPKc") | THUMB_BIT),
                  (void*)hookSearchScene, (void**)&origSearchScene);

    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN16LevelEditorLayer4initEP11GJGameLevel") | THUMB_BIT),
                  (void*)hookEditorShow, (void**)&origEditorShow);

    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN11AppDelegate11trySaveGameEv") | THUMB_BIT),
                  (void*)hookSaveGame, (void**)&origSaveGame);

    networkcpp_init();
    percentagecpp_init();
    songscpp_init();
    utilscpp_init();

    const char* path = "/data/data/com.necytdamu.onesixgdps/";
    const char* server = "http://gdpsnazarva.7m.pl";

#if defined(__i386__)
    uint32_t patchNewBlock = 0x3FFF3D;
    mprotectPatch(0x1A9105, &patchNewBlock, 3, true);
    mprotectPatch(0x1A5FB6, &patchNewBlock, 3, true);
    uint64_t patchMaxError = 0x4000042444C7;
    mprotectPatch(0x1A5166, &patchMaxError, 6, true);
    uint64_t patchPause = 0x4000082444C7;
    mprotectPatch(0x1DA8B3, &patchPause, 6, true);
    mprotectPatch(0x450DB8, (void*)path, strlen(path), false);

    float patchLimit = __FLT_MAX__;
    mprotectPatch(0x56A8A8, &patchLimit, 4, false);
    mprotectPatch(0x56A8BC, &patchLimit, 4, false);
    mprotectPatch(0x1A7090, &patchLimit, 4, true);

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

    float patchLimit = __FLT_MAX__;
    mprotectPatch(0x14C230, &patchLimit, 4, false);
    mprotectPatch(0x14EE14, &patchLimit, 4, false);
    mprotectPatch(0x14FFFC, &patchLimit, 4, false);
    mprotectPatch(0x14F21C, &patchLimit, 4, false);

    uint16_t skip = 0xBF00;
    mprotectPatch(0x15AA58, &skip, 2, true);

    const uintptr_t serverAddrs[] = {0x3FEEB2, 0x3FEF07, 0x3FEF67, 0x3FEFD4, 0x3FF006, 0x3FF078, 0x3FF0CD, 0x3FF11C, 0x3FF163, 0x3FF1CA, 0x3FF1FC, 0x3FF264, 0x3FF2B6, 0x3FF2FC, 0x3FF36F, 0x3FF3D9, 0x3FF499};
#endif
    for (int i = 0; i < 17; i++) mprotectPatch(serverAddrs[i], (void*)server, strlen(server), false);

    const char* error = dlerror();
    if (error) __android_log_print(ANDROID_LOG_INFO, "1.6 GDPS", "dlerror: %s", error);
    return JNI_VERSION_1_4;
}
#endif
