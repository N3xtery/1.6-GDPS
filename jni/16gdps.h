#include <dlfcn.h>
#include <jni.h>
#include <android/log.h>
#include <sys/mman.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <functional>

#if defined(__i386__)
#include "../../libs/subhook/subhook.h"
void ZzHookReplace(void* fun, void* my_fun, void** orig);
#define THUMB_BIT 0
#else
#include "../../libs/HookZz/include/hookzz.h"
#define THUMB_BIT 1
#endif

// main.cpp
struct soinfo2 {
    char name[128];
    const void* phdr;
    int phnum;
    unsigned entry;
    unsigned base;
    unsigned size;
};
extern soinfo2* handle;
extern void buildPlayLoadingDialog(const char* id);
extern void mprotectPatch(uintptr_t addr, void* patch, int size, bool flush);
extern void showMessageBox(const char* title, const char* msg);
extern std::vector<std::pair<int, std::string>> customSongs;
extern std::vector<std::pair<int, float>> songOffsets;
extern void* currentLevel;
extern void* currentLevelLayer;
#if defined(__i386__)
extern void origHttpSetRequestData(void*, const char*, int);
#else
extern void (*origHttpSetRequestData)(void*, const char*, int);
#endif
extern int getLevelID(void* self);
extern int getLevelLocalID(void* self);

extern void* (*gameManagerGetInstance)();
extern void networkcpp_init();

extern void* getPlayerPosition(void* self);
extern void percentagecpp_init();

extern void songscpp_init();

// utils.cpp
extern void queueOnCocosThread(std::function<void()> fn);
extern std::string gzipCompress(const std::string& data);
extern std::string base64Encode(const std::string& data);
extern std::string base64Decode(const char* s);
extern void utilscpp_init();
