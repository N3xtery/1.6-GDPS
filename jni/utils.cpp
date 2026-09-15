#include "16gdps.h"
#include <mutex>
#include <zlib.h>

// ai-generated code

std::mutex g_queueMutex;
std::vector<std::function<void()>> g_mainThreadQueue;

void queueOnCocosThread(std::function<void()> fn) {
    std::lock_guard<std::mutex> lock(g_queueMutex);
    g_mainThreadQueue.push_back(std::move(fn));
}
static int (*origScheduler)(void*, float);
static int hookScheduler(void* self, float var) {
    std::vector<std::function<void()>> toRun;
    {
        std::lock_guard<std::mutex> lock(g_queueMutex);
        toRun.swap(g_mainThreadQueue);
    }
    for (auto& fn : toRun) fn();
    return origScheduler(self, var);
}

std::string gzipCompress(const std::string& data) {
    if (data.empty()) return std::string();

    z_stream zs{};
    if (deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK)
        return std::string();

    zs.next_in = (Bytef*)data.data();
    zs.avail_in = (uInt)data.size();

    std::string out;
    char buf[16384];
    int ret;
    do {
        zs.next_out = (Bytef*)buf;
        zs.avail_out = sizeof(buf);
        ret = deflate(&zs, Z_FINISH);
        out.append(buf, sizeof(buf) - zs.avail_out);
    } while (ret == Z_OK);
    deflateEnd(&zs);

    if (ret != Z_STREAM_END) return std::string();
    return out;
}

std::string base64Encode(const std::string& data) {
    static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);

    size_t i = 0;
    const unsigned char* bytes = (const unsigned char*)data.data();
    size_t len = data.size();

    while (i + 3 <= len) {
        unsigned int n = (bytes[i] << 16) | (bytes[i+1] << 8) | bytes[i+2];
        out += table[(n >> 18) & 0x3F];
        out += table[(n >> 12) & 0x3F];
        out += table[(n >> 6) & 0x3F];
        out += table[n & 0x3F];
        i += 3;
    }

    size_t rem = len - i;
    if (rem == 1) {
        unsigned int n = bytes[i] << 16;
        out += table[(n >> 18) & 0x3F];
        out += table[(n >> 12) & 0x3F];
        out += "==";
    } else if (rem == 2) {
        unsigned int n = (bytes[i] << 16) | (bytes[i+1] << 8);
        out += table[(n >> 18) & 0x3F];
        out += table[(n >> 12) & 0x3F];
        out += table[(n >> 6) & 0x3F];
        out += "=";
    }
    return out;
}

std::string base64Decode(const char* s) {
    static const char* t = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    std::string r;
    int v = 0, b = -8;
    for (; *s; s++) {
        if (*s == '=') break;
        int x = 0;
        for (; x < 64 && t[x] != *s; x++);
        if (x == 64) continue;
        v = (v << 6) | x;
        b += 6;
        if (b >= 0) {
            r += char((v >> b) & 255);
            b -= 8;
        }
    }
    return r;
}

void utilscpp_init() {
    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN7cocos2d11CCScheduler6updateEf") | THUMB_BIT),
                  (void*)hookScheduler, (void**)&origScheduler);
}
