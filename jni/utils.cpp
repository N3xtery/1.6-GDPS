#include "16gdps.h"
#include <zlib.h>
#include <pthread.h>

// ai-generated code

pthread_mutex_t g_queueMutex = PTHREAD_MUTEX_INITIALIZER;

class MutexLock {
public:
    MutexLock(pthread_mutex_t& m) : mutex(m) { pthread_mutex_lock(&mutex); }
    ~MutexLock() { pthread_mutex_unlock(&mutex); }
private:
    pthread_mutex_t& mutex;
    // non-copyable
    MutexLock(const MutexLock&);
    MutexLock& operator=(const MutexLock&);
};

std::vector<ICallable*> g_mainThreadQueue;
void queueOnCocosThreadImpl(ICallable* fn) {
    MutexLock lock(g_queueMutex);
    g_mainThreadQueue.push_back(fn);
}

static int (*origScheduler)(void*, float);
static int hookScheduler(void* self, float var) {
    std::vector<ICallable*> tasks;
    {
        MutexLock lock(g_queueMutex);
        tasks.swap(g_mainThreadQueue);
    }
    for (size_t i = 0; i < tasks.size(); ++i) {
        (*tasks[i])();
        delete tasks[i];
    }
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

std::string gzipDecompress(const char* data, size_t dataSize) {
    std::string out;
    out.resize(dataSize * 4 > 4096 ? dataSize * 4 : 4096);

    z_stream strm{};
    strm.next_in  = reinterpret_cast<Bytef*>(const_cast<char*>(data));
    strm.avail_in = static_cast<uInt>(dataSize);

    int ret = inflateInit2(&strm, 15 + 16);
    if (ret != Z_OK) return {};

    size_t totalOut = 0;
    while (true) {
        if (totalOut == out.size()) {
            out.resize(out.size() * 2);
        }
        strm.next_out  = reinterpret_cast<Bytef*>(&out[totalOut]);
        strm.avail_out = static_cast<uInt>(out.size() - totalOut);

        ret = inflate(&strm, Z_NO_FLUSH);
        totalOut = out.size() - strm.avail_out;

        if (ret == Z_STREAM_END) break;
        if (ret != Z_OK) {
            inflateEnd(&strm);
            return {};
        }
    }

    inflateEnd(&strm);
    out.resize(totalOut);
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
    std::string r = std::string();
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
#if defined(__APPLE__)
    MSHookFunction((void*)((base + 0x1A8B0) | 1), (void*)hookScheduler, (void**)&origScheduler);
#else
    ZzHookReplace((void*)((uintptr_t)dlsym(handle, "_ZN7cocos2d11CCScheduler6updateEf") | THUMB_BIT),
                  (void*)hookScheduler, (void**)&origScheduler);
#endif
}
