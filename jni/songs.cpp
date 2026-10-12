#include "16gdps.h"

static char* myMenuSong = nullptr;
static char* myPractSong = nullptr;
static char* customSongsPath = nullptr;
static bool practice = false;
static std::string getReplacedSong(const char* song) {
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
                return path;
            }
        }
    }
    return std::string(song);
}

#if defined(__APPLE__)
static void (*stopBackMusic)();
static int (*origPlayBackMusic)(void*, const char*, bool);
static int hookPlayBackMusic(void* self, const char* song, bool flag) { return origPlayBackMusic(self, getReplacedSong(song).c_str(), flag); }
static int (*origPreloadMusic)(void*, const char*);
static int hookPreloadMusic(void* self, const char* song) { return origPreloadMusic(self, getReplacedSong(song).c_str()); }
#else
static void (*stopBackMusic)();
static int (*origPlayBackMusic)(const char*, bool);
static int hookPlayBackMusic(const char* song, bool flag) { return origPlayBackMusic(getReplacedSong(song).c_str(), flag); }
static int (*origPreloadMusic)(const char*);
static int hookPreloadMusic(const char* song) {
    int res = origPreloadMusic(getReplacedSong(song).c_str());
    hookPlayBackMusic(song, false);
    stopBackMusic();
}
#endif

static void* levelCell;
static const char* hookGetAudioTitle(int num) {
    int id = 0;
    uintptr_t caller = (uintptr_t)__builtin_return_address(0) - base;
    if (caller == SONGNAME_EDIT_LVL_CELL_CALLER) id = getLevelLocalID(levelCell);
    else if (caller == SONGNAME_EDIT_LVL_CALLER) id = getLevelLocalID(currentLevel);

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

#if defined(__APPLE__)
static int (*origSongSetOffset)(void*, float);
static int hookSongSetOffset(void* self, float time) {
#else
static int (*origSongSetOffset)(float);
static int hookSongSetOffset(float time) {
#endif
    if (currentLevel && !practice) {
        int localID = getLevelLocalID(currentLevel);
        for (int i = 0; i < songOffsets.size(); i++) if (songOffsets[i].first == localID) {
            time += songOffsets[i].second;
            break;
        }
    }
#if defined(__APPLE__)
    return origSongSetOffset(self, time);
#else
    return origSongSetOffset(time);
#endif
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
            buildLoadingDialog(customSongs[i].second.c_str());
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

void setCustomSong(const char* id) {
    int localID = getLevelLocalID(currentLevel);
    std::string idStr(id);
    for (int i = 0; i < customSongs.size(); i++) {
        if (customSongs[i].first == localID) {
            customSongs[i].second = idStr;
            return;
        }
    }
    customSongs.push_back(std::make_pair(localID, idStr));
}

void setOffset(float offset) {
    int localID = getLevelLocalID(currentLevel);
    for (int i = 0; i < songOffsets.size(); i++) {
        if (songOffsets[i].first == localID) {
            songOffsets[i].second = offset;
            return;
        }
    }
    songOffsets.push_back(std::make_pair(localID, offset));
}

void updateMenuPractSongNative(bool isMenuSong, const char* songPath, bool updateMenuNow) {
    if (isMenuSong) {
        if (myMenuSong) free(myMenuSong);
        if (songPath[0]) myMenuSong = strdup(songPath);
        else myMenuSong = nullptr;
        if (updateMenuNow) queueOnCocosThread([]() {
        #if defined(__APPLE__)
            stopBackMusic();
            hookPlayBackMusic(nullptr, "menuLoop.mp3", true);
        #else
            hookPlayBackMusic("menuLoop.mp3", true);
        #endif
        });
    } else {
        if (myPractSong) free(myPractSong);
        if (songPath[0]) {
            myPractSong = strdup(songPath);
        } else myPractSong = nullptr;
    }
}

void updateSongsPathNative(const char* songPath) {
    if (customSongsPath) free(customSongsPath);
    customSongsPath = strdup(songPath);
}

void playLevel() {
    queueOnCocosThread([]() {
        if (currentLevelLayer) origCustomLevelOnPlay(currentLevelLayer);
        else origEditLevelOnPlay(currentEditLevelLayer);
    });
}

#ifndef __APPLE__
extern "C" JNIEXPORT void JNICALL
Java_com_necytdamu_onesixgdps_SongsDialog_setCustomSong(JNIEnv* env, jclass, jstring idJava) {
    const char* id = env->GetStringUTFChars(idJava, nullptr);
    setCustomSong(id);
    env->ReleaseStringUTFChars(idJava, id);
}

extern "C" JNIEXPORT void JNICALL
Java_com_necytdamu_onesixgdps_SongsDialog_setOffset(JNIEnv* env, jclass, jfloat offsetJava) {
    setOffset(offsetJava);
}

extern "C" JNIEXPORT void JNICALL
Java_com_necytdamu_onesixgdps_SongsDialog_updateMenuPractSongNative(JNIEnv* env, jclass, jboolean isMenuSong, jstring songPathJ, jboolean updateMenuNow) {
    const char* songPath = env->GetStringUTFChars(songPathJ, nullptr);
    updateMenuPractSongNative(isMenuSong != JNI_FALSE, songPath, updateMenuNow != JNI_FALSE);
    env->ReleaseStringUTFChars(songPathJ, songPath);
}

extern "C" JNIEXPORT void JNICALL
Java_com_necytdamu_onesixgdps_SongsDialog_updateSongsPathNative(JNIEnv* env, jclass, jstring songPathJava) {
    const char* songPath = env->GetStringUTFChars(songPathJava, nullptr);
    updateSongsPathNative(songPath);
    env->ReleaseStringUTFChars(songPathJava, songPath);
}

extern "C" JNIEXPORT void JNICALL
Java_com_necytdamu_onesixgdps_SongsDialog_playLevel(JNIEnv* env, jclass) {
    playLevel();
}
#endif

void songscpp_init() {
#if defined(__APPLE__)
    stopBackMusic = (void(*)())((base + STOP_SONG_ADDR) | 1);
    MSHookFunction((void*)((base + PLAY_SONG_ADDR) | 1), (void*)hookPlayBackMusic, (void**)&origPlayBackMusic);
    MSHookFunction((void*)((base + PRELOAD_SONG_ADDR) | 1), (void*)hookPreloadMusic, (void**)&origPreloadMusic);
    MSHookFunction((void*)((base + SONG_TITLE_ADDR) | 1), (void*)hookGetAudioTitle, nullptr);
    MSHookFunction((void*)((base + CELL_FROM_LEVEL_ADDR) | 1), (void*)hookLoadCellFromLevel, (void**)&origLoadCellFromLevel);
    MSHookFunction((void*)((base + SONG_OFFSET_ADDR) | 1), (void*)hookSongSetOffset, (void**)&origSongSetOffset);
    MSHookFunction((void*)((base + EDIT_LVL_PLAY_ADDR) | 1), (void*)hookEditLevelOnPlay, (void**)&origEditLevelOnPlay);
    MSHookFunction((void*)((base + CUSTOM_LVL_PLAY_ADDR) | 1), (void*)hookCustomLevelOnPlay, (void**)&origCustomLevelOnPlay);

    uint16_t patch = 0xBF00;
    mprotectPatch(SKIP_SONG_OFFSET_ADDR, &patch, 2, true);
#else
    stopBackMusic = (void(*)())(dlsym(handle, "pauseBackgroundMusicJNI"));

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

    uint64_t patch = 0x909090909090;
    mprotectPatch(0x178F14, &patch, 6, true);

    uint16_t patchEasyPause = 0xE990;
    mprotectPatch(0x1777B4, &patchEasyPause, 2, true);
#else
    origCustomLevelOnPlay = (void(*)(void*))(dlsym(handle, "_ZN14LevelInfoLayer6onPlayEv"));
    void* hookAddr = (void*)hookCustomLevelOnPlay;
    mprotectPatch(0x46CC4C, &hookAddr, 4, true);

    uint16_t patch = 0xBF00;
    mprotectPatch(0x13991C, &patch, 2, true);

    uint16_t patchEasyPause = 0xE032;
    mprotectPatch(0x138B4E, &patchEasyPause, 2, true);
#endif

#endif
}
