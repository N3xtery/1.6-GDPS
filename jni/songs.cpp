#include "16gdps.h"

static char* myMenuSong = nullptr;
static char* myPractSong = nullptr;
static char* customSongsPath = nullptr;
static bool practice = false;
static const char* getReplacedSong(const char* song) {
    if (strcmp(song, "StayInsideMe.mp3") == 0) {
        practice = true;
        if (myPractSong) return myPractSong;
    } else {
        practice = false;
        if (strcmp(song, "menuLoop.mp3") == 0) {
            if (myMenuSong) return myMenuSong;
        } else if (currentLevel) {
            int localID = getLevelLocalID(currentLevel);
            for (int i = 0; i < customSongs.size(); i++) if (customSongs[i].first == localID) {
                if (customSongs[i].second == "") break;
                std::string path(customSongsPath);
                path.append(customSongs[i].second);
                path.append(".mp3");
                return path.c_str();
            }
        }
    }
    return song;
}

static int (*origPlayBackMusic)(const char*, bool);
static int hookPlayBackMusic(const char* song, bool flag) {
    return origPlayBackMusic(getReplacedSong(song), flag);
}

static int (*origPreloadMusic)(const char*);
static int hookPreloadMusic(const char* song) {
    return origPreloadMusic(getReplacedSong(song));
}

extern "C" JNIEXPORT void JNICALL
Java_com_necytdamu_onesixgdps_SongsDialog_setCustomSong(JNIEnv* env, jclass, jstring idJava) {
    const char* id = env->GetStringUTFChars(idJava, nullptr);
    int localID = getLevelLocalID(currentLevel);
    std::string idStr(id);
    env->ReleaseStringUTFChars(idJava, id);
    for (int i = 0; i < customSongs.size(); i++) {
        if (customSongs[i].first == localID) {
            customSongs[i].second = idStr;
            return;
        }
    }
    customSongs.emplace_back(localID, idStr);
}

extern "C" JNIEXPORT void JNICALL
Java_com_necytdamu_onesixgdps_SongsDialog_setOffset(JNIEnv* env, jclass, jfloat offsetJava) {
    int localID = getLevelLocalID(currentLevel);
    for (int i = 0; i < songOffsets.size(); i++) {
        if (songOffsets[i].first == localID) {
            songOffsets[i].second = offsetJava;
            return;
        }
    }
    songOffsets.emplace_back(localID, offsetJava);
}

extern "C" JNIEXPORT void JNICALL
Java_com_necytdamu_onesixgdps_SongsDialog_updateMenuPractSongNative(JNIEnv* env, jclass, jboolean isMenuSong, jstring songPathJava, jboolean updateMenuNow) {
    const char* songPath = env->GetStringUTFChars(songPathJava, nullptr);
    if (isMenuSong != JNI_FALSE) {
        if (myMenuSong) free(myMenuSong);
        if (songPath[0]) myMenuSong = strdup(songPath);
        else myMenuSong = nullptr;
        if (updateMenuNow != JNI_FALSE) queueOnCocosThread([]() {
            hookPlayBackMusic("menuLoop.mp3", true);
        });
    } else {
        if (myPractSong) free(myPractSong);
        if (songPath[0]) {
            myPractSong = strdup(songPath);
        } else myPractSong = nullptr;
    }
    env->ReleaseStringUTFChars(songPathJava, songPath);
}

extern "C" JNIEXPORT void JNICALL
Java_com_necytdamu_onesixgdps_SongsDialog_updateSongsPath(JNIEnv* env, jclass, jstring songPathJava) {
    const char* songPath = env->GetStringUTFChars(songPathJava, nullptr);
    if (customSongsPath) free(customSongsPath);
    customSongsPath = strdup(songPath);
    env->ReleaseStringUTFChars(songPathJava, songPath);
}

static void* levelCell;
static const char* hookGetAudioTitle(int num) {
    uintptr_t caller = (uintptr_t)__builtin_return_address(0) - handle->base;
    int id = 0;
#if defined(__i386__)
    if (caller == 0x189FE6) id = getLevelLocalID(levelCell);
    else if (caller == 0x1BDF6F) id = getLevelLocalID(currentLevel);
#elif defined(__ARM_ARCH_7A__)
    if (caller == 0x140DB3) id = getLevelLocalID(levelCell);
    else if (caller == 0x15A533) id = getLevelLocalID(currentLevel);
#else
    if (caller == 0x142D2B) id = getLevelLocalID(levelCell);
    else if (caller == 0x15CF2D) id = getLevelLocalID(currentLevel);
#endif
    if (id) for (int i = 0; i < customSongs.size(); i++) if (customSongs[i].first == id) {
        if (customSongs[i].second != "") return "Custom";
        break;
    }
    num++;
    switch (num) { // calling the original function leads to a crash so this has to be done
        case 0: return "Practice: Stay Inside Me";
        case 1: return "Stereo Madness";
        case 2: return "Back On Track";
        case 3: return "Polargeist";
        case 4: return "Dry Out";
        case 5: return "Base After Base";
        case 6: return "Cant Let Go";
        case 7: return "Jumper";
        case 8: return "Time Machine";
        case 9: return "Cycles";
        case 10: return "xStep";
        case 11: return "Clutterfunk";
        case 12: return "Theory of Everything";
        case 13: return "Electroman Adventures";
        case 14: return "Clubstep";
        case 15: return "Active";
        default: return "Unknown";
    }
}

static int (*origLoadCellFromLevel)(void*, void*);
static int hookLoadCellFromLevel(void* self, void* level) {
    levelCell = level;
    return origLoadCellFromLevel(self, level);
}

static int (*origSongSetOffset)(float);
static int hookSongSetOffset(float time) {
    if (currentLevel && !practice) {
        int localID = getLevelLocalID(currentLevel);
        for (int i = 0; i < songOffsets.size(); i++) if (songOffsets[i].first == localID) {
            time += songOffsets[i].second;
            break;
        }
    }
    return origSongSetOffset(time);
}

static bool isSongDownloaded() {
    if (!currentLevel) return true;
    int localID = getLevelLocalID(currentLevel);
    for (int i = 0; i < customSongs.size(); i++) if (customSongs[i].first == localID) {
        std::string path(customSongsPath);
        path.append(customSongs[i].second);
        path.append(".mp3");
        FILE* f = fopen(path.c_str(), "rb");
        if (f) {
            fclose(f);
            break;
        } else {
            buildPlayLoadingDialog(customSongs[i].second.c_str());
            return false;
        }
    }
    return true;
}

static void* currentEditLevelLayer;
static void (*origEditLevelOnPlay)(void*);
static void hookEditLevelOnPlay(void* self) {
    currentEditLevelLayer = self;
    if (isSongDownloaded()) origEditLevelOnPlay(self);
}

static void (*origCustomLevelOnPlay)(void*);
static void hookCustomLevelOnPlay(void* self) {
    if (isSongDownloaded()) origCustomLevelOnPlay(self);
}

extern "C" JNIEXPORT void JNICALL
Java_com_necytdamu_onesixgdps_SongsDialog_playLevel(JNIEnv* env, jclass) {
    queueOnCocosThread([]() {
        if (currentLevelLayer) origCustomLevelOnPlay(currentLevelLayer);
        else origEditLevelOnPlay(currentEditLevelLayer);
    });
}

void songscpp_init() {
    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "playBackgroundMusicJNI") | THUMB_BIT),
                  (void*)hookPlayBackMusic, (void**)&origPlayBackMusic);

    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "preloadBackgroundMusicJNI") | THUMB_BIT),
                  (void*)hookPreloadMusic, (void**)&origPreloadMusic);

    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN10LevelTools13getAudioTitleEi") | THUMB_BIT),
                  (void*)hookGetAudioTitle, nullptr);

    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN9LevelCell13loadFromLevelEP11GJGameLevel") | THUMB_BIT),
                  (void*)hookLoadCellFromLevel, (void**)&origLoadCellFromLevel);

    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "setBackgroundMusicTimeJNI") | THUMB_BIT),
                  (void*)hookSongSetOffset, (void**)&origSongSetOffset);

    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN14EditLevelLayer6onPlayEv") | THUMB_BIT),
                  (void*)hookEditLevelOnPlay, (void**)&origEditLevelOnPlay);

#if defined(__i386__)
    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN14LevelInfoLayer6onPlayEv") | THUMB_BIT),
                  (void*)hookCustomLevelOnPlay, (void**)&origCustomLevelOnPlay);
#else
    origCustomLevelOnPlay = (void(*)(void*))(dlsym(handle, "_ZN14LevelInfoLayer6onPlayEv"));
    void* hookAddr = (void*)hookCustomLevelOnPlay;
    mprotectPatch(0x46CC4C, &hookAddr, 4, true);
#endif

#if defined(__i386__)
    uint64_t patch = 0x909090909090;
    mprotectPatch(0x178F14, &patch, 6, true);
#elif defined(__ARM_ARCH_7A__)
    uint16_t patch = 0xBF00;
    mprotectPatch(0x13991C, &patch, 2, true);
#else
    uint16_t patch = 0xBF00;
    mprotectPatch(0x13B530, &patch, 2, true);
#endif
}
