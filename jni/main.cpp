#include "16gdps.h"
#include <algorithm>
#include <cmath>

uintptr_t base;
#if defined(__APPLE__)
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
    void* handle;
    static JavaVM* jvm = nullptr;

    void mprotectPatch(uintptr_t addr, void* patch, int size, bool flush) {
        uintptr_t targetAddr = base + addr;
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

    static void showGroup(const char* group, bool show) {
        JNIEnv* env = nullptr;
        jvm->GetEnv((void**)&env, JNI_VERSION_1_4);
        jstring javaGroup = env->NewStringUTF(group);
        callJavaFunction("OverlayUI", "showGroup", "(Ljava/lang/String;Z)V", javaGroup, (jboolean)show);
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

bool optionsShown = false;
static int (*origOptionsShow)(void*);
static int hookOptionsShow(void* self) {
    showGroup("Options", true);
    optionsShown = true;
    return origOptionsShow(self);
}

static int (*origDropdownExit)(void*);
static int hookDropdownExit(void* self) {
    if (optionsShown) {
        showGroup("Options", false);
        optionsShown = false;
    }
    return origDropdownExit(self);
}

static int (*origLevelSettingsShow)(void*, void*);
static int hookLevelSettingsShow(void* self, void* obj) {
    showGroup("LevelSettings", true);
    return origLevelSettingsShow(self, obj);
}

static std::string levelStr;
static int (*origLevelSettingsHide)(void*);
static int hookLevelSettingsHide(void* self) {
    showGroup("LevelSettings", false);
    std::string().swap(levelStr);
    return origLevelSettingsHide(self);
}

static void* colorPopup;
static int (*origColorSettingsShow)(void*);
static int hookColorSettingsShow(void* self) {
    colorPopup = self;
    showGroup("Color", true);
    return origColorSettingsShow(self);
}

static int (*origColorSettingsHide)(void*);
static int hookColorSettingsHide(void* self) {
    showGroup("Color", false);
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
    if ((uintptr_t)__builtin_return_address(0) - base != COLOR_SLIDER_WHEEL_CALLER)
        callJavaFunction("ColorPicker", "setColor", "(III)V", (int)(color.r * 255), (int)(color.g * 255), (int)(color.b * 255));
    return color;
}
#else
static void (*origColorSlider)(DoubleColor*, int, int, int, double, double);
static void hookColorSlider(DoubleColor* out, int unused, int h1, int h2, double s, double v) {
    origColorSlider(out, unused, h1, h2, s, v);
    if ((uintptr_t)__builtin_return_address(0) - base != COLOR_SLIDER_WHEEL_CALLER)
#if defined(__APPLE__)
        setColor((int)(out->r * 255), (int)(out->g * 255), (int)(out->b * 255));
#else
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
std::vector<std::pair<int, int>> copies;
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
    showGroup("LevelMenu", true);
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

static void* (*origTransitionScene)(float, void*, Color*);
static void* hookTransitionScene(float var, void* scene, Color* color) {
    if (currentLevelLayer) {
        if (levelMenuShown) {
            showGroup("LevelMenu", false);
            showGroup("LevelInfo", false);
            currentLevelLayer = nullptr;
        } else levelMenuShown = true;
    }
    return origTransitionScene(var, scene, color);
}

static void (*replaceScene)(void* self, void* transition);
static void (*CCArrayInsert)(void* self, void* obj, int pos);
static void goToLevel(void* level) {
    void* director = getDirectorInstance();
    void* scene = hookLevelEditShow(level);
    Color color = {0, 0, 0};
    void* transition = hookTransitionScene(0.5f, scene, &color);
    replaceScene(director, transition);
}
#if defined(__APPLE__)
static void* (*gameManagerCreateLevel)(void* self);
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
#if defined(__i386__)
static std::string (*dictGetStrForKey)(void* self, const char* key);
static std::string dictGetStrForKeyHelper(void* dict, const char* key) {
    std::string str = dictGetStrForKey(dict, key);
#else
static void (*dictGetStrForKey)(std::string* out, void* self, const char* key);
static std::string dictGetStrForKeyHelper(void* dict, const char* key) {
    std::string str;
    dictGetStrForKey(&str, dict, key);
#endif
    if (str.empty()) new (&str) std::string();
    return str;
}
static float (*dictGetFloatForKey)(void* self, const char* key);
static void* (*origLevelReadXml)(int, void*);
static void* hookLevelReadXml(int type, void* dict) {
    void* level = origLevelReadXml(type, dict);
    if (type == 4) {
        int orig_level = dictGetIntForKey(dict, "k42");
        if (orig_level) copies.push_back(std::make_pair(getLevelLocalID(level), orig_level));

        std::string songID = dictGetStrForKeyHelper(dict, "k45");
        if (!songID.empty()) customSongs.push_back(std::make_pair(getLevelLocalID(level), songID));

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
    if ((uintptr_t)__builtin_return_address(0) - base == HTTP_UPLOAD_LVL_CALLER) {
        auto data = (std::vector<char>*)((char*)request + 0x20);
        std::string newStr = changeHttpData(std::string(data->begin(), data->end()));
        const char* newCStr = newStr.c_str();
        data->assign(newCStr, newCStr + strlen(newCStr));
    }
    origHttpSend(self, request);
}
#else
void (*origHttpSetRequestData)(void*, const char*, int);
static void hookHttpSetRequestData(void* self, const char* str, int len) {
    if ((uintptr_t)__builtin_return_address(0) - base == HTTP_UPLOAD_LVL_CALLER) {
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
    if ((uintptr_t)__builtin_return_address(0) - base != RESPONSE_CREATE_LVL_CELL_CALLER) {
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

void importGmd(const char* pathC) {
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

            if (strncmp(data.c_str(), "H4sI", 4) == 0) {
                std::string dataDecoded = base64Decode(data.c_str());
                data = gzipDecompress(dataDecoded.c_str(), dataDecoded.size());
            }

            for (int i = 0; i < 20; i += 2) replaceSubStr(&data, gmdConverts[i + 1], gmdConverts[i]);
            char* dict = (char*)malloc(0xD4);
            dsDictCons(dict);
            dsStrToDict(dict, &data);

            std::string k3 = dictGetStrForKeyHelper(dict, "k3");
            std::string k4 = dictGetStrForKeyHelper(dict, "k4");
            std::string k34 = dictGetStrForKeyHelper(dict, "k34");
            if (strncmp(k4.c_str(), "H4sI", 4) == 0 || strncmp(k4.c_str(), "eJy", 3) == 0) {
                std::string k4Decoded = base64Decode(k4.c_str());
                std::string k4Decompressed = gzipDecompress(k4Decoded.c_str(), k4Decoded.size());
                dictSetStrForKey(dict, "k4", &k4Decompressed);

                std::string k3Decoded = base64Decode(k3.c_str());
                dictSetStrForKey(dict, "k3", &k3Decoded);

                std::string k34Decoded = base64Decode(k34.c_str());
                std::string k34Decompressed = gzipDecompress(k34Decoded.c_str(), k34Decoded.size());
                dictSetStrForKey(dict, "k34", &k34Decompressed);
            }

            void* level = hookLevelReadXml(4, dict);
            *(bool*)((char*)level + 0x15D) = false; // verified
            *(int*)((char*)level + 0x164) = 7; // game version
            *(int*)((char*)level + 0x1E0) = 2; // type
            void* levelManager = levelManagerGetInstance();
            CCArrayInsert(*(void**)((char*)levelManager + 0x130), level, 0);
            showGroup("Options", false);
            goToLevel(level);
            free(dict);
        }
    });
}

static int levelLength = 0;
#if defined(__i386__)
static std::string (*getLevelStr)(void* self);
static int getCurrentLevelLength() {
    getLevelStr(editorLayer);
#else
static void (*getLevelStr)(std::string* out, void* self);
int getCurrentLevelLength() {
    std::string levelStr;
    getLevelStr(&levelStr, editorLayer);
#endif
    int distance = *(int*)((char*)editorLayer + 0x164);
    levelLength = (int)roundf((distance + 340.0f) / 311.0f);
    return levelLength;
}

void exportGmd(const char* pathC, bool gmd) {
    std::string path = std::string(pathC);
    queueOnCocosThread([path, gmd]() {
        FILE* f = fopen(path.c_str(), "w");
        if (f) {
            void* objectLayer = *(void**)((char*)editorLayer + 0x158);
            memcpy((char*)currentLevel + 0x1b8 + 0x18, (char*)objectLayer + 0x2c + 0x18, 8); // editor location
            *(float*)((char*)currentLevel + 0x1d8) = *(float*)((char*)objectLayer + 0x24); // editor zoom

            char* dict = (char*)malloc(0xD4);
            dsDictCons(dict);
            hookLevelWriteXml(currentLevel, dict);

#if defined(__i386__)
            std::string levelStr = getLevelStr(editorLayer);
#else
            std::string levelStr;
            getLevelStr(&levelStr, editorLayer);
#endif
            dictSetStrForKey(dict, "k4", &levelStr);
            dictSetIntForKey(dict, "k23", levelLength < 10 ? 0 : std::min((levelLength / 30) + 1, 3));
            void* levelSettings = *(void**)((char*)editorLayer + 0x160);
            dictSetIntForKey(dict, "k8", *(int*)((char*)levelSettings + 0x130)); // song
#if defined(__i386__)
            std::string data = dsDictToStr(dict);
#else
            std::string data;
            dsDictToStr(&data, dict);
#endif
            for (int i = 0; i < 20; i += 2) replaceSubStr(&data, gmdConverts[i], gmdConverts[i + 1]);

            std::string plist = "plist version=\"1.0\"";
            size_t pos = data.find(plist);
            if (pos != std::string::npos) data.insert(pos + plist.size(), " gjver=\"2.0\"");
            if (!gmd) {
                std::string dataCompressed = gzipCompress(data);
                data = base64Encode(dataCompressed);
            }

            fwrite(&data[0], 1, data.length(), f);
            fclose(f);
            free(dict);
            showMessageBox("Success", "Level export successful");
        } else showMessageBox("Error", "Level export failed");
    });
}

const char* getCurrentLevelName() {
    return ((std::string*)((char*)currentLevel + 0x12C))->c_str();
}

#ifndef __APPLE__
extern "C" JNIEXPORT void JNICALL
Java_com_necytdamu_onesixgdps_OverlayUI_importGmd(JNIEnv* env, jclass, jstring pathJava) {
    const char* pathC = env->GetStringUTFChars(pathJava, nullptr);
    importGmd(pathC);
    env->ReleaseStringUTFChars(pathJava, pathC);
}

extern "C" JNIEXPORT void JNICALL
Java_com_necytdamu_onesixgdps_OverlayUI_exportGmd(JNIEnv* env, jclass, jstring pathJava, jboolean gmdJava) {
    const char* pathC = env->GetStringUTFChars(pathJava, nullptr);
    exportGmd(pathC, gmdJava != JNI_FALSE);
    env->ReleaseStringUTFChars(pathJava, pathC);
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_necytdamu_onesixgdps_OverlayUI_getCurrentLevelName(JNIEnv* env, jclass) {
    return env->NewStringUTF(getCurrentLevelName());
}

extern "C" JNIEXPORT jint JNICALL
Java_com_necytdamu_onesixgdps_OverlayUI_getCurrentLevelLength(JNIEnv* env, jclass) {
    return getCurrentLevelLength();
}

static void (*origCCDictSetObj)(void*, void*, std::string);
static void hookCCDictSetObj(void* dict, void* obj, std::string key) {
    if (!key[0]) return;
    origCCDictSetObj(dict, obj, key);
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

static void* (*origParseLevelObj)(std::string);
static void* hookParseLevelObj(std::string str) {
    if (strncmp(str.c_str(), "1,", 2)) return nullptr;
    std::string::size_type a = 2;
    std::string::size_type b = str.find(',', a);
    std::string second = str.substr(a, b - a);
    b += 3;
    std::string::size_type c = str.find(',', b);
    std::string third = str.substr(b, c - b);
    int id = atoi(second.c_str());
    double x = strtod(third.c_str(), nullptr);
    if ((currentLevel && id == 142) || id > 199 || x < 0) return nullptr;
    return origParseLevelObj(str);
}

static int (*origLevelInfoShow)(void*, void*);
static int hookLevelInfoShow(void* self, void* level) {
    showGroup("LevelMenu", false);
    showGroup("LevelInfo", true);
    return origLevelInfoShow(self, level);
}

static int (*origLevelInfoHide)(void*);
static int hookLevelInfoHide(void* self) {
    showGroup("LevelInfo", false);
    showGroup("LevelMenu", true);
    return origLevelInfoHide(self);
}

#if defined(__APPLE__)
void iOSSetupNative() {
    base = _dyld_get_image_vmaddr_slide(0);

    dictSetIntForKey = (void(*)(void*, const char*, int))((base + DICT_SET_INT_ADDR) | 1);
    dictGetIntForKey = (int(*)(void*, const char*))((base + DICT_GET_INT_ADDR) | 1);
    dictSetStrForKey = (void(*)(void*, const char*, std::string*))((base + DICT_SET_STR_ADDR) | 1);
    dictGetStrForKey = (void(*)(std::string*, void*, const char*))((base + DICT_GET_STR_ADDR) | 1);
    dictSetFloatForKey = (void(*)(void*, const char*, float))((base + DICT_SET_FLOAT_ADDR) | 1);
    dictGetFloatForKey = (float(*)(void*, const char*))((base + DICT_GET_FLOAT_ADDR) | 1);
    CCdictGetValue = (void*(*)(void*, std::string*))((base + CCDICT_GET_VALUE_ADDR) | 1);
    getIntFromCCStr = (int(*)(void*))((base + CCSTR_GET_INT_ADDR) | 1);
    getStrFromCCStr = (const char*(*)(void*))((base + CCSTR_GET_STR_ADDR) | 1);
    getFloatFromCCStr = (float(*)(void*))((base + CCSTR_GET_FLOAT_ADDR) | 1);
    gameManagerCreateLevel = (void*(*)(void*))((base + CREATE_LVL_ADDR) | 1);
    replaceScene = (void(*)(void*, void*))((base + REPLACE_SCENE_ADDR) | 1);
    managerSave = (void(*)(void*))((base + SAVE_MANAGER_ADDR) | 1);
    CCArrayInsert = (void(*)(void*, void*, int))((base + CCARRAY_INSERT_ADDR) | 1);
    keyCreateSearchObj = (void*(*)(const char*))((base + CREATE_SEARCH_OBJ_ADDR) | 1);
    levelBrowserScene = (void*(*)(void*))((base + LVL_BROWSER_SCENE_ADDR) | 1);
    getLevelStr = (void(*)(std::string*, void*))((base + GET_LVL_STR_ADDR) | 1);

    MSHookFunction((void*)((base + SHOW_OPTIONS_ADDR) | 1), (void*)hookOptionsShow, (void**)&origOptionsShow);
    MSHookFunction((void*)((base + EXIT_DROPDOWN_ADDR) | 1), (void*)hookDropdownExit, (void**)&origDropdownExit);
    MSHookFunction((void*)((base + SHOW_LVL_SETS_ADDR) | 1), (void*)hookLevelSettingsShow, (void**)&origLevelSettingsShow);
    MSHookFunction((void*)((base + HIDE_LVL_SETS_ADDR) | 1), (void*)hookLevelSettingsHide, (void**)&origLevelSettingsHide);
    MSHookFunction((void*)((base + SHOW_COLOR_SETS_ADDR) | 1), (void*)hookColorSettingsShow, (void**)&origColorSettingsShow);
    MSHookFunction((void*)((base + HIDE_COLOR_SETS_ADDR) | 1), (void*)hookColorSettingsHide, (void**)&origColorSettingsHide);
    MSHookFunction((void*)((base + SET_COLOR_ADDR) | 1), (void*)hookColorSet, (void**)&origColorSet);
    MSHookFunction((void*)((base + COLOR_SLIDER_ADDR) | 1), (void*)hookColorSlider, (void**)&origColorSlider);
    MSHookFunction((void*)((base + SHOW_CUSTOM_LVL_ADDR) | 1), (void*)hookLevelMenuShow, (void**)&origLevelMenuShow);
    MSHookFunction((void*)((base + SHOW_EDIT_LVL_ADDR) | 1), (void*)hookLevelEditShow, (void**)&origLevelEditShow);
    MSHookFunction((void*)((base + SHOW_MAIN_LVLS_ADDR) | 1), (void*)hookMainLevelsShow, (void**)&origMainLevelsShow);
    MSHookFunction((void*)((base + WRITE_LVL_XML_ADDR) | 1), (void*)hookLevelWriteXml, (void**)&origLevelWriteXml);
    MSHookFunction((void*)((base + READ_LVL_XML_ADDR) | 1), (void*)hookLevelReadXml, (void**)&origLevelReadXml);
    MSHookFunction((void*)((base + HTTP_SEND_ADDR) | 1), (void*)hookHttpSend, (void**)&origHttpSend);
    MSHookFunction((void*)((base + RESPONSE_CREATE_LVL_ADDR) | 1), (void*)hookCreateLevelFromResponse, (void**)&origCreateLevelFromResponse);
    MSHookFunction((void*)((base + TRANSITION_SCENE_ADDR) | 1), (void*)hookTransitionScene, (void**)&origTransitionScene);
    MSHookFunction((void*)((base + SEARCH_SCENE_ADDR) | 1), (void*)hookSearchScene, (void**)&origSearchScene);
    MSHookFunction((void*)((base + SHOW_EDITOR_ADDR) | 1), (void*)hookEditorShow, (void**)&origEditorShow);
    MSHookFunction((void*)((base + SAVE_GAME_ADDR) | 1), (void*)hookSaveGame, (void**)&origSaveGame);
    MSHookFunction((void*)((base + PARSE_LVL_OBJ_ADDR) | 1), (void*)hookParseLevelObj, (void**)&origParseLevelObj);
    MSHookFunction((void*)((base + SHOW_LVL_INFO_ADDR) | 1), (void*)hookLevelInfoShow, (void**)&origLevelInfoShow);
    MSHookFunction((void*)((base + HIDE_LVL_INFO_ADDR) | 1), (void*)hookLevelInfoHide, (void**)&origLevelInfoHide);

    networkcpp_init();
    percentagecpp_init();
    songscpp_init();
    utilscpp_init();

    uint32_t patchNewBlock = 0x4F80F5B0;
    mprotectPatch(OBJ_LIMIT_ADDR_1, &patchNewBlock, 4, true);
    mprotectPatch(OBJ_LIMIT_ADDR_2, &patchNewBlock, 4, true);
    uint32_t patchMaxError = 0x0100F244;
    mprotectPatch(OBJ_LIMIT_ADDR_3, &patchMaxError, 4, true);
    uint32_t patchPause = 0x0200F244;
    mprotectPatch(OBJ_LIMIT_ADDR_4, &patchPause, 4, true);

    float patchLimit = __FLT_MAX__ / 30.0f;
    mprotectPatch(LVL_LIMIT_ADDR_1, &patchLimit, 4, false);
    mprotectPatch(LVL_LIMIT_ADDR_2, &patchLimit, 4, false);
    mprotectPatch(LVL_LIMIT_ADDR_3, &patchLimit, 4, false);
    mprotectPatch(LVL_LIMIT_ADDR_4, &patchLimit, 4, false);

#if defined(__ARM_ARCH_7S__)
    const uintptr_t serverAddrs[] = {0x262842, 0x262938, 0x2629AD, 0x2629FB, 0x262A4D, 0x262ACB, 0x262B23, 0x262B85, 0x262C43, 0x262C75, 0x262CC3, 0x262D48, 0x262E3E, 0x262E9E};
#else
    const uintptr_t serverAddrs[] = {0x262832, 0x262928, 0x26299D, 0x2629EB, 0x262A3D, 0x262ABB, 0x262B13, 0x262B75, 0x262C33, 0x262C65, 0x262CB3, 0x262D38, 0x262E2E, 0x262E8E};
#endif
    const char* server = "http://gdpsnazarva.7m.pl";
    for (int i = 0; i < 14; i++) mprotectPatch(serverAddrs[i], (void*)server, strlen(server), false);
}
#else
extern "C" JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM* vm, void* reserved) {
    handle = dlopen("libgame.so", RTLD_LAZY);
    base = *(uintptr_t*)((char*)handle + 140);
    jvm = vm;

    cloneLevel = (void(*)(void*))(dlsym(handle, "_ZN14LevelInfoLayer7onCloneEv"));
    dictSetIntForKey = (void(*)(void*, const char*, int))(dlsym(handle, "_ZN13DS_Dictionary16setIntegerForKeyEPKci"));
    dictGetIntForKey = (int(*)(void*, const char*))(dlsym(handle, "_ZN13DS_Dictionary16getIntegerForKeyEPKc"));
    dictSetStrForKey = (void(*)(void*, const char*, std::string*))(dlsym(handle, "_ZN13DS_Dictionary15setStringForKeyEPKcRKSs"));
    dictSetFloatForKey = (void(*)(void*, const char*, float))(dlsym(handle, "_ZN13DS_Dictionary14setFloatForKeyEPKcf"));
    dictGetFloatForKey = (float(*)(void*, const char*))(dlsym(handle, "_ZN13DS_Dictionary14getFloatForKeyEPKc"));
    CCdictGetValue = (void*(*)(void*, std::string*))(dlsym(handle, "_ZN7cocos2d12CCDictionary11valueForKeyERKSs"));
    getIntFromCCStr = (int(*)(void*))(dlsym(handle, "_ZNK7cocos2d8CCString8intValueEv"));
    getStrFromCCStr = (const char*(*)(void*))(dlsym(handle, "_ZNK7cocos2d8CCString10getCStringEv"));
    getFloatFromCCStr = (float(*)(void*))(dlsym(handle, "_ZNK7cocos2d8CCString10floatValueEv"));
    replaceScene = (void(*)(void*, void*))(dlsym(handle, "_ZN7cocos2d10CCDirector12replaceSceneEPNS_7CCSceneE"));
    managerSave = (void(*)(void*))(dlsym(handle, "_ZN8GManager4saveEv"));
    CCArrayInsert = (void(*)(void*, void*, int))(dlsym(handle, "_ZN7cocos2d7CCArray12insertObjectEPNS_8CCObjectEj"));
    keyCreateSearchObj = (void*(*)(const char*))(dlsym(handle, "_ZN14GJSearchObject13createFromKeyEPKc"));
    levelBrowserScene = (void*(*)(void*))(dlsym(handle, "_ZN17LevelBrowserLayer5sceneEP14GJSearchObject"));
#if defined(__i386__)
    dictGetStrForKey = (std::string(*)(void*, const char*))(dlsym(handle, "_ZN13DS_Dictionary15getStringForKeyEPKc"));
    getLevelStr = (std::string(*)(void*))(dlsym(handle, "_ZN16LevelEditorLayer14getLevelStringEv"));
#else
    dictGetStrForKey = (void(*)(std::string*, void*, const char*))(dlsym(handle, "_ZN13DS_Dictionary15getStringForKeyEPKc"));
    getLevelStr = (void(*)(std::string*, void*))(dlsym(handle, "_ZN16LevelEditorLayer14getLevelStringEv"));
#endif

    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN12OptionsLayer6createEv") | THUMB_BIT),
                  (void*)hookOptionsShow, (void**)&origOptionsShow);

    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN15GJDropDownLayer9exitLayerEv") | THUMB_BIT),
                  (void*)hookDropdownExit, (void**)&origDropdownExit);

    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN18LevelSettingsLayer4initEP19LevelSettingsObject") | THUMB_BIT),
                  (void*)hookLevelSettingsShow, (void**)&origLevelSettingsShow);

    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN18LevelSettingsLayerD2Ev") | THUMB_BIT),
                  (void*)hookLevelSettingsHide, (void**)&origLevelSettingsHide);

    ZzHookReplace((void*)((base + SHOW_COLOR_SETS_ADDR) | THUMB_BIT), (void*)hookColorSettingsShow, (void**)&origColorSettingsShow);
    ZzHookReplace((void*)((base + HIDE_COLOR_SETS_ADDR) | THUMB_BIT), (void*)hookColorSettingsHide, (void**)&origColorSettingsHide);

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
    ZzHookReplace((void*)((base + 0x16247C) | THUMB_BIT), (void*)hookHttpSetRequestData, (void**)&origHttpSetRequestData);
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

    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN10GameObject16objectFromStringESs") | THUMB_BIT),
                  (void*)hookParseLevelObj, (void**)&origParseLevelObj);

    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN7cocos2d12CCDictionary9setObjectEPNS_8CCObjectERKSs") | THUMB_BIT),
                  (void*)hookCCDictSetObj, (void**)&origCCDictSetObj);

    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN9InfoLayer4initEP11GJGameLevel") | THUMB_BIT),
                  (void*)hookLevelInfoShow, (void**)&origLevelInfoShow);

    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN9InfoLayer7onCloseEv") | THUMB_BIT),
                  (void*)hookLevelInfoHide, (void**)&origLevelInfoHide);

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
