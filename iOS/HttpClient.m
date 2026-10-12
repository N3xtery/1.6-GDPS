#import "16gdpsIOS.h"
#include <curl/curl.h>

typedef struct {
    __unsafe_unretained BOOL (^sink)(const void* bytes, NSUInteger length);
    FileDownloadTask* fileDownloader;
    BOOL failed;
} TransferContext;

static size_t WriteCallback(char* data, size_t size, size_t nmemb, void* userdata) {
    TransferContext* ctx = (TransferContext*)userdata;
    size_t total = size * nmemb;
    if (ctx->fileDownloader && ctx->fileDownloader.cancelled) { ctx->failed = YES; return 0; }
    if (!ctx->sink(data, total)) { ctx->failed = YES; return 0; }
    return total;
}

static int ProgressCallback(void* userdata, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    TransferContext* ctx = (TransferContext*)userdata;
    if (dltotal > 0) ctx->fileDownloader.totalSize = dltotal;
    return ctx->fileDownloader.cancelled ? 1 : 0;
}

static bool performGetURL(NSURL* url, FileDownloadTask* fileDownloader, BOOL (^sink)(const void *bytes, NSUInteger length), NSString** error) {
    curl_global_init(CURL_GLOBAL_ALL);

    char errorBuffer[CURL_ERROR_SIZE];
    errorBuffer[0] = '\0';

    CURL* handle = curl_easy_init();
    if (!handle) {
        *error = [NSString stringWithUTF8String:curl_easy_strerror(CURLE_FAILED_INIT)];
        return false;
    }

    curl_easy_setopt(handle, CURLOPT_URL, [[url absoluteString] UTF8String]);
    curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, errorBuffer);
    curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(handle, CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, 15);
    if (!fileDownloader) curl_easy_setopt(handle, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(handle, CURLOPT_TCP_NODELAY, 1L);
    curl_easy_setopt(handle, CURLOPT_FAILONERROR, 1L);

    curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(handle, CURLOPT_SSLVERSION, (long)CURL_SSLVERSION_TLSv1_3);

    NSString* caPath = [[NSBundle mainBundle] pathForResource:@"isrgrootx" ofType:@"pem"];
    curl_easy_setopt(handle, CURLOPT_CAINFO, [caPath fileSystemRepresentation]);

    struct curl_slist* headerList = NULL;
    headerList = curl_slist_append(headerList, "User-Agent: Mozilla/5.0");
    headerList = curl_slist_append(headerList, "Accept: text/html,*/*");
    headerList = curl_slist_append(headerList, "Accept-Language: ru");
    curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headerList);

    TransferContext ctx = {.sink = sink, .fileDownloader = fileDownloader};
    curl_easy_setopt(handle, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &ctx);
    if (fileDownloader) {
        curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(handle, CURLOPT_XFERINFOFUNCTION, ProgressCallback);
        curl_easy_setopt(handle, CURLOPT_XFERINFODATA, &ctx);
    }

    CURLcode result = curl_easy_perform(handle);

    long status = 0;
    curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &status);

    curl_slist_free_all(headerList);
    curl_easy_cleanup(handle);

    if (result != CURLE_OK) {
        if (!(ctx.failed && ctx.fileDownloader && ctx.fileDownloader.cancelled))
            *error = [NSString stringWithUTF8String:errorBuffer[0] ? errorBuffer : curl_easy_strerror(result)];
        return NO;
    }
    return YES;
}

NSData* dataFromURL(NSURL* url, NSString** error) {
    NSMutableData* body = [NSMutableData data];
    bool ok = performGetURL(url, nil, ^BOOL(const void* bytes, NSUInteger length) {
        [body appendBytes:bytes length:length];
        return YES;
    }, error);
    return ok ? body : nil;
}

bool downloadURL(NSURL* url, NSString* path, FileDownloadTask* fileDownloader, NSString** error) {
    [[NSFileManager defaultManager] createFileAtPath:path contents:[NSData data] attributes:nil];
    NSFileHandle* handle = [NSFileHandle fileHandleForWritingAtPath:path];
    if (!handle) {
        *error = @"Unable to write the file";
        return NO;
    }

    __block long long received = 0;
    bool ok = performGetURL(url, fileDownloader, ^BOOL(const void* bytes, NSUInteger length) {
        @try {
            [handle writeData:[NSData dataWithBytesNoCopy:(void*)bytes length:length freeWhenDone:NO]];
        } @catch (NSException* exception) {
            return NO;
        }
        received += length;
        [fileDownloader progress:received];
        return YES;
    }, error);

    [handle closeFile];
    if (!ok) [[NSFileManager defaultManager] removeItemAtPath:path error:NULL];
    return ok;
}
