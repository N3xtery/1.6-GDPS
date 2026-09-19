#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#if defined(__APPLE__)
    #include <substrate.h>
    #include <mach-o/dyld.h>
    #include <mach/mach.h>
    #include <libkern/OSCacheControl.h>
    #include <syslog.h>
    extern uintptr_t base;
    extern void onLoad();
#else
    #include <dlfcn.h>
    #include <jni.h>
    #include <android/log.h>
    #include <sys/mman.h>
    #if defined(__i386__)
        #include "../../libs/subhook/subhook.h"
        void ZzHookReplace(void* fun, void* my_fun, void** orig);
        #define THUMB_BIT 0
    #else
        #include "../../libs/HookZz/include/hookzz.h"
        #define THUMB_BIT 1
    #endif
    struct soinfo2 {
        char name[128];
        const void* phdr;
        int phnum;
        unsigned entry;
        unsigned base;
        unsigned size;
    };
    extern soinfo2* handle;
#endif

// main.cpp
extern void buildPlayLoadingDialog(const char* id);
extern void mprotectPatch(uintptr_t addr, void* patch, int size, bool flush);
extern void showMessageBox(const char* title, const char* msg);
extern std::vector<std::pair<int, std::string>> customSongs;
extern std::vector<std::pair<int, float>> songOffsets;
extern void* currentLevel;
extern void* currentLevelLayer;
#if defined(__i386__) || defined(__APPLE__)
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
class ICallable {
public:
    virtual void operator()() = 0;
    virtual ~ICallable() {}
};
extern void queueOnCocosThreadImpl(ICallable* fn);
template<typename F>
class CallableImpl : public ICallable {
public:
    CallableImpl(const F& f) : fn(f) {}
    void operator()() { fn(); }
private:
    F fn;
};
template<typename F>
void queueOnCocosThread(F fn) {
    queueOnCocosThreadImpl(new CallableImpl<F>(fn));
}
extern std::string gzipCompress(const std::string& data);
std::string gzipDecompress(const char* data, size_t dataSize);
extern std::string base64Encode(const std::string& data);
extern std::string base64Decode(const char* s);
extern void utilscpp_init();
