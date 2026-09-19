#include "16gdps.h"

static void getResponseCode(char* buf, void* response) {
    char* respBegin = *(char**)((char*)response + 0x20);
    char* respEnd = *(char**)((char*)response + 0x24);
    int len = (int)(respEnd - respBegin);
    if (len >= 128) len = 127;
    memcpy(buf, respBegin, len);
    buf[len] = '\0';
}

#if defined(__APPLE__)
static void (*httpRequestCons)(void* httpRequest);
static void (*httpRequestSetURL)(const char* dest, const char* url, int len);
#elif defined(__i386__)
static void (*cocosObjCons)(void* obj);
static void (*cocosStrCons)(void* obj, const char* src, int size, int trail);
static void httpRequestCons(void* httpRequest) {
    memset(httpRequest, 0, 0x4C);
    cocosObjCons(httpRequest);
    *(uintptr_t*)httpRequest = handle->base + 0x63E128;

    uintptr_t emptyRepPtr = handle->base + 0x66B408;
    uintptr_t emptyRepData = emptyRepPtr + 0x0C;
    uint32_t  emptyLen = *(uint32_t*)emptyRepPtr;

    *(uint32_t*)((char*)httpRequest + 0x1C) = (uint32_t)emptyRepData;
    *(uint32_t*)((char*)httpRequest + 0x2C) = (uint32_t)emptyRepData;

    cocosStrCons((char*)httpRequest + 0x1C, 0, emptyLen, 0);
    *(uint32_t*)((char*)httpRequest + 0x24) = *(uint32_t*)((char*)httpRequest + 0x20);
    cocosStrCons((char*)httpRequest + 0x2C, 0, emptyLen, 0);
}
static void (*httpRequestSetURL)(const char* dest, const char* url, int len);
#else
static void (*httpRequestCons)(void* httpRequest);
static void (*httpRequestSetURL)(void* httpRequest, const char* url);
#endif
static void (*cocosObjRetain)(void* obj);
static void httpRequestSetCallback(void* httpRequest, void* target, void* callback) {
    *(void**)((char*)httpRequest + 0x30) = target;
    *(void**)((char*)httpRequest + 0x34) = callback;
    *(int*)((char*)httpRequest + 0x38) = 0;
    if (target) cocosObjRetain(target);
}
static void* (*httpGetInstance)();
static void (*httpInstanceSend)(void* httpInstance, void* httpRequest);
static void (*httpRequestRelease)(void* httpRequest);
static void sendPacket(const char* url, const char* data, void* callback) {
    char* httpRequest = (char*)malloc(0x4c);
    httpRequestCons(httpRequest);
    *(int*)((char*)httpRequest + 0x18) = 1;
#if defined(__i386__) || defined(__APPLE__)
    httpRequestSetURL((char*)httpRequest + 0x1c, url, strlen(url));
#else
    httpRequestSetURL(httpRequest, url);
#endif
    origHttpSetRequestData(httpRequest, data, strlen(data));
    void* httpInstance = httpGetInstance();
    httpRequestSetCallback(httpRequest, httpInstance, callback);
    httpInstanceSend(httpInstance, httpRequest);
    httpRequestRelease(httpRequest);
}

static void registerCallback(void* self, void* node, void* response) {
    char buf[128];
    getResponseCode(buf, response);

    const char* title = nullptr;
    const char* msg = nullptr;
    if (strcmp(buf, "1") == 0) {
        title = "Account registration successful";
        msg = "Success";
    } else if (strcmp(buf, "-2") == 0) {
        title = "Account registration failed";
        msg = "The username is already taken";
    } else {
        title = "Account registration Unknown";
        msg = buf;
    }
    if (title) showMessageBox(title, msg);
}

static void loginCallback(void* self, void* node, void* response) {
    char buf[128];
    getResponseCode(buf, response);

    const char* title = nullptr;
    const char* msg = nullptr;
    if (strcmp(buf, "-12") == 0) {
        title = "Logging in failed";
        msg = "Account doesn't exist";
    } else if (strcmp(buf, "-1") == 0) {
        title = "Logging in failed";
        msg = "Wrong password";
    } else if (strchr(buf, ',')) {
        title = "Logging in successful";
        msg = "Success";
    } else {
        title = "Logging in Unknown";
        msg = buf;
    }
    if (title) showMessageBox(title, msg);
}

static void saveCallback(void* self, void* node, void* response) {
    char buf[128];
    getResponseCode(buf, response);

    const char* title = nullptr;
    const char* msg = nullptr;
    if (strcmp(buf, "-1") == 0) {
        title = "Save failed";
        msg = "Account session invalid";
    } else if (strcmp(buf, "1") == 0) {
        title = "Save successful";
        msg = "Success";
    } else {
        title = "Save Unknown";
        msg = buf;
    }
    if (title) showMessageBox(title, msg);
}

static void (*dsDictCons)(void* self);
void* (*gameManagerGetInstance)();
static void (*gameManagerEncodeData)(void* self, void* ds);
static void* (*levelManagerGetInstance)();
static void (*levelManagerEncodeData)(void* self, void* ds);
#if defined(__i386__)
static std::string (*dsDictToStr)(void* self);
#else
static void (*dsDictToStr)(std::string* out, void* self);
#endif

static int (*gzDecompress)(const char* in, int len, char** out);
static bool (*dsStrToDict)(void* self, std::string* str);
static void (*gameManagerLoadData)(void* self, void* ds);
static void (*levelManagerLoadData)(void* self, void* ds);
static void (*managerSave)(void* self);
#if defined(__i386__)
static std::string (*responseToStr)(void* response);
#else
static void (*responseToStr)(std::string* out, void* response);
#endif
static bool noLevels = false;
static void loadCallback(void* self, void* node, void* response) {
#if defined(__i386__)
    std::string data = responseToStr(response);
#else
    std::string data;
    responseToStr(&data, response);
#endif

    const char* title = nullptr;
    const char* msg = nullptr;
    if (data == "-2") {
        title = "Load failed";
        msg = "Account session invalid";
    } else if (data == "-1") {
        title = "Load failed";
        msg = "No save found";
    } else if (data == "-3") {
        title = "Load failed";
        msg = "Error -3";
    } else {
        size_t pos = data.find(';');
        if (pos != std::string::npos) {
            std::string gameData = data.substr(0, pos);
            std::string levelData = data.substr(pos + 1);
            size_t levelPos = levelData.find(';');
            if (levelPos != std::string::npos) levelData.erase(levelPos);
            char* dict = (char*)malloc(0xD4);

            std::string gameDataDecoded = base64Decode(gameData.c_str());
            std::string gameDataDecompressed = gzipDecompress(gameDataDecoded.c_str(), gameDataDecoded.size());
            dsDictCons(dict);
            dsStrToDict(dict, &gameDataDecompressed);
            void* gameManager = gameManagerGetInstance();
            gameManagerLoadData(gameManager, dict);

            if (!noLevels) {
                std::string levelDataDecoded = base64Decode(levelData.c_str());
                std::string levelDataDecompressed = gzipDecompress(levelDataDecoded.c_str(), levelDataDecoded.size());
                dsDictCons(dict);
                dsStrToDict(dict, &levelDataDecompressed);
                void* levelManager = levelManagerGetInstance();
                levelManagerLoadData(levelManager, dict);
                managerSave(levelManager);
            }

            title = "Load successful";
            msg = "Success";
            free(dict);
        } else {
            title = "Load Unknown";
            msg = data.c_str();
        }
    }
    if (title) showMessageBox(title, msg);
}

static void reqCallback(void* self, void* node, void* response) {
    char buf[128];
    getResponseCode(buf, response);

    const char* title = nullptr;
    const char* msg = nullptr;
    if (strcmp(buf, "-1") == 0) {
        title = "Req failed";
        msg = "Account session invalid";
    } else if (strcmp(buf, "1") == 0) {
        title = "Req successful";
        msg = "Congratulations, you're a moderator!";
    } else if (strcmp(buf, "0") == 0) {
        title = "Req failed";
        msg = "You're not a moderator, womp womp";
    } else {
        title = "Req Unknown";
        msg = buf;
    }
    if (title) showMessageBox(title, msg);
}

static void rateCallback(void* self, void* node, void* response) {
    char buf[128];
    getResponseCode(buf, response);

    const char* title = nullptr;
    const char* msg = nullptr;
    if (strcmp(buf, "-1") == 0) {
        title = "Rate failed";
        msg = "Account session invalid";
    } else if (strcmp(buf, "0") == 0) {
        title = "Rate failed";
        msg = "You're not a moderator";
    } else if (strcmp(buf, "1") == 0) {
        title = "Rate successful";
        msg = "Success";
    } else if (strcmp(buf, "2") == 0) {
        title = "Unrate successful";
        msg = "Success";
    } else {
        title = "Rate Unknown";
        msg = buf;
    }
    if (title) showMessageBox(title, msg);
}

#if defined(__APPLE__)
#else
extern "C" JNIEXPORT void JNICALL
Java_com_necytdamu_onesixgdps_AuthDialog_sendAuthRequest(JNIEnv* env, jclass, jstring userJava, jstring passJava, jboolean regJava) {
    const char* username = env->GetStringUTFChars(userJava, nullptr);
    const char* password = env->GetStringUTFChars(passJava, nullptr);
    bool reg = (regJava != JNI_FALSE);

    char data[256];
    snprintf(data, 256, "userName=%s&password=%s", username, password);
    sendPacket(reg ? "http://gdpsnazarva.7m.pl/database/accounts/registerGJAccount.php" : "http://gdpsnazarva.7m.pl/database/accounts/loginGJAccount.php",
               data, reg ? (void*)&registerCallback : (void*)&loginCallback);

    env->ReleaseStringUTFChars(userJava, username);
    env->ReleaseStringUTFChars(passJava, password);
}

extern "C" JNIEXPORT void JNICALL
Java_com_necytdamu_onesixgdps_RateMenu_sendRateRequest(JNIEnv* env, jclass, jstring userJava, jstring passJava, jint stars, jboolean featured) {
    const char* username = env->GetStringUTFChars(userJava, nullptr);
    const char* password = env->GetStringUTFChars(passJava, nullptr);

    char data[256];
    snprintf(data, 256, "userName=%s&password=%s&id=%d&stars=%d&featured=%d", username, password, getLevelID(currentLevel), stars, featured != JNI_FALSE);
    sendPacket("http://gdpsnazarva.7m.pl/database/rateGJLevel.php", data, (void*)&rateCallback);

    env->ReleaseStringUTFChars(userJava, username);
    env->ReleaseStringUTFChars(passJava, password);
}

extern "C" JNIEXPORT void JNICALL
Java_com_necytdamu_onesixgdps_OverlayUI_sendSaveRequest(JNIEnv* env, jclass, jstring userJava, jstring passJava) {
    const char* usernameC = env->GetStringUTFChars(userJava, nullptr);
    const char* passwordC = env->GetStringUTFChars(passJava, nullptr);
    std::string username = usernameC;
    std::string password = passwordC;

    queueOnCocosThread([username, password]() {
        char* dict = (char*)malloc(0xD4);
        dsDictCons(dict);
        void* gameManager = gameManagerGetInstance();
        gameManagerEncodeData(gameManager, dict);
    #if defined(__i386__)
        std::string str = dsDictToStr(dict);
    #else
        std::string str = std::string();
        dsDictToStr(&str, dict);
    #endif
        size_t pos = 0;
        while ((pos = str.find("<key>k4</key>", pos)) != std::string::npos) { // remove locally saved levels
            size_t stringClose = str.find("</string>", pos) + std::string("</string>").size();
            if (stringClose == std::string::npos) break;
            str.erase(pos, stringClose - pos);
        }

        dsDictCons(dict);
        void* levelManager = levelManagerGetInstance();
        levelManagerEncodeData(levelManager, dict);
    #if defined(__i386__)
        std::string strLevel = dsDictToStr(dict);
    #else
        std::string strLevel = std::string();
        dsDictToStr(&strLevel, dict);
    #endif
        std::string strReadyLevel = base64Encode(gzipCompress(strLevel));

        std::string strReady = base64Encode(gzipCompress(str));
        strReady.append(";");
        strReady.append(strReadyLevel);

        char data[256];
        snprintf(data, 256, "userName=%s&password=%s&saveData=", username.c_str(), password.c_str());
        strReady.insert(0, data);
        sendPacket("http://gdpsnazarva.7m.pl/database/accounts/backupGJAccount.php", strReady.c_str(), (void*)&saveCallback);

        free(dict);
    });

    env->ReleaseStringUTFChars(userJava, usernameC);
    env->ReleaseStringUTFChars(passJava, passwordC);
}

extern "C" JNIEXPORT void JNICALL
Java_com_necytdamu_onesixgdps_OverlayUI_sendLoadRequest(JNIEnv* env, jclass, jstring userJava, jstring passJava, jboolean javaNolevels) {
    const char* username = env->GetStringUTFChars(userJava, nullptr);
    const char* password = env->GetStringUTFChars(passJava, nullptr);
    noLevels = javaNolevels != JNI_FALSE;

    char data[256];
    snprintf(data, 256, "userName=%s&password=%s", username, password);
    sendPacket("http://gdpsnazarva.7m.pl/database/accounts/syncGJAccount.php", data, (void*)&loadCallback);

    env->ReleaseStringUTFChars(userJava, username);
    env->ReleaseStringUTFChars(passJava, password);
}

extern "C" JNIEXPORT void JNICALL
Java_com_necytdamu_onesixgdps_OverlayUI_sendReqRequest(JNIEnv* env, jclass, jstring userJava, jstring passJava) {
    const char* username = env->GetStringUTFChars(userJava, nullptr);
    const char* password = env->GetStringUTFChars(passJava, nullptr);

    char data[256];
    snprintf(data, 256, "userName=%s&password=%s", username, password);
    sendPacket("http://gdpsnazarva.7m.pl/database/reqGJModerator.php", data, (void*)&reqCallback);

    env->ReleaseStringUTFChars(userJava, username);
    env->ReleaseStringUTFChars(passJava, password);
}
#endif

void networkcpp_init() {
#if defined(__APPLE__)
    httpRequestCons = (void(*)(void*))((base + 0xA0E28) | 1);
    httpRequestSetURL = (void(*)(const char*, const char*, int))((base + 0x3A7B48) | 1);
    cocosObjRetain = (void(*)(void*))((base + 0x20094) | 1);
    httpGetInstance = (void*(*)())((base + 0x2D5B4) | 1);
    httpInstanceSend = (void(*)(void*, void*))((base + 0x2E25C) | 1);
    httpRequestRelease = (void(*)(void*))((base + 0x20078) | 1);

    dsDictCons = (void(*)(void*))((base + 0x1147AC) | 1);
    gameManagerGetInstance = (void*(*)())((base + 0xA133C) | 1);
    gameManagerEncodeData = (void(*)(void*, void*))((base + 0xA391C) | 1);
    levelManagerGetInstance = (void*(*)())((base + 0x157F84) | 1);
    levelManagerEncodeData = (void(*)(void*, void*))((base + 0x15824C) | 1);
    dsDictToStr = (void(*)(std::string*, void*))((base + 0x1149B0) | 1);

    dsStrToDict = (bool(*)(void*, std::string*))((base + 0x114920) | 1);
    gameManagerLoadData = (void(*)(void*, void*))((base + 0xA3058) | 1);
    levelManagerLoadData = (void(*)(void*, void*))((base + 0x158268) | 1);
    managerSave = (void(*)(void*))((base + 0x1198D4) | 1);
    responseToStr = (void(*)(std::string*, void*))((base + 0x118EF8) | 1);
#else
#if defined(__i386__)
    cocosObjCons = (void(*)(void*))(dlsym(handle, "_ZN7cocos2d8CCObjectC2Ev"));
    cocosStrCons = (void(*)(void*, const char*, int, int))(0x418600 + handle->base);
    httpRequestSetURL = (void(*)(const char*, const char*, int))(0x418840 + handle->base);
#else
    httpRequestCons = (void(*)(void*))(dlsym(handle, "_ZN7cocos2d9extension13CCHttpRequestC2Ev"));
    httpRequestSetURL = (void(*)(void*, const char*))(dlsym(handle, "_ZN7cocos2d9extension13CCHttpRequest6setUrlEPKc"));
#endif
    cocosObjRetain = (void(*)(void*))(dlsym(handle, "_ZN7cocos2d8CCObject6retainEv"));
    httpGetInstance = (void*(*)())(dlsym(handle, "_ZN7cocos2d9extension12CCHttpClient11getInstanceEv"));
    httpInstanceSend = (void(*)(void*, void*))(dlsym(handle, "_ZN7cocos2d9extension12CCHttpClient4sendEPNS0_13CCHttpRequestE"));
    httpRequestRelease = (void(*)(void*))(dlsym(handle, "_ZN7cocos2d8CCObject7releaseEv"));

    dsDictCons = (void(*)(void*))(dlsym(handle, "_ZN13DS_DictionaryC1Ev"));
    gameManagerGetInstance = (void*(*)())(dlsym(handle, "_ZN11GameManager11sharedStateEv"));
    gameManagerEncodeData = (void(*)(void*, void*))(dlsym(handle, "_ZN11GameManager12encodeDataToEP13DS_Dictionary"));
    levelManagerGetInstance = (void*(*)())(dlsym(handle, "_ZN17LocalLevelManager11sharedStateEv"));
    levelManagerEncodeData = (void(*)(void*, void*))(dlsym(handle, "_ZN17LocalLevelManager12encodeDataToEP13DS_Dictionary"));
#if defined(__i386__)
    dsDictToStr = (std::string(*)(void*))(dlsym(handle, "_ZN13DS_Dictionary23saveRootSubDictToStringEv"));
#else
    dsDictToStr = (void(*)(std::string*, void*))(dlsym(handle, "_ZN13DS_Dictionary23saveRootSubDictToStringEv"));
#endif

    dsStrToDict = (bool(*)(void*, std::string*))(dlsym(handle, "_ZN13DS_Dictionary25loadRootSubDictFromStringESs"));
    gameManagerLoadData = (void(*)(void*, void*))(dlsym(handle, "_ZN11GameManager10dataLoadedEP13DS_Dictionary"));
    levelManagerLoadData = (void(*)(void*, void*))(dlsym(handle, "_ZN17LocalLevelManager10dataLoadedEP13DS_Dictionary"));
    managerSave = (void(*)(void*))(dlsym(handle, "_ZN8GManager4saveEv"));
#if defined(__i386__)
    responseToStr = (std::string(*)(void*))(dlsym(handle, "_ZN11GameToolbox11getResponseEPN7cocos2d9extension14CCHttpResponseE"));
#else
    responseToStr = (void(*)(std::string*, void*))(dlsym(handle, "_ZN11GameToolbox11getResponseEPN7cocos2d9extension14CCHttpResponseE"));
#endif
#endif
}
