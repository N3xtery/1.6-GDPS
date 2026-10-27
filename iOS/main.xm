#import "iOSBridge.h"
#import <AVFoundation/AVFoundation.h>
#import <substrate.h>

static NSUUID* (*origGetVendor)(id, SEL);
static NSUUID* hookGetVendor(id self, SEL _cmd) {
    NSUUID* u = origGetVendor(self, _cmd);
    if (u) return u;
    static NSUUID* fallback;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        NSUserDefaults* d = [NSUserDefaults standardUserDefaults];
        NSString* s = [d stringForKey:@"OnesixgdpsVendorID"];
        NSUUID* n = s ? [[NSUUID alloc] initWithUUIDString:s] : nil;
        if (!n) {
            n = [[NSUUID alloc] init];
            [d setObject:[n UUIDString] forKey:@"OnesixgdpsVendorID"];
            [d synchronize];
        }
        fallback = n;
    });
    return fallback;
}

%ctor {
    MSHookMessageEx([UIDevice class], @selector(identifierForVendor), (IMP)hookGetVendor, (IMP*)&origGetVendor);
    iOSSetupNative();
}

%hook Everyplay
- (void)showExpiredMessage {}
%end

%hook AppController
- (BOOL)application:(UIApplication*)application
    didFinishLaunchingWithOptions:(NSDictionary*)options {
    BOOL result = %orig;
    iOSBuildOverlay();
    return result;
}
%end

static NSString* const kGameDocs = @"/var/mobile/Library/1.6 GDPS";
%hookf(NSArray*, NSSearchPathForDirectoriesInDomains, NSSearchPathDirectory dir, NSSearchPathDomainMask mask, BOOL expand) {
    if (dir != NSDocumentDirectory || !expand) return %orig;
    [[NSFileManager defaultManager] createDirectoryAtPath:kGameDocs withIntermediateDirectories:YES attributes:nil error:NULL];
    return [NSArray arrayWithObject:kGameDocs];
}

%hook CDLongAudioSource
- (void)setCurrentTime:(float)t {
    AVAudioPlayer* p = MSHookIvar<AVAudioPlayer*>(self, "audioSourcePlayer");
    double rate = [[p.settings objectForKey:AVSampleRateKey] doubleValue];
    if (rate != 44100.0) t = t * rate / 44100.0f;
    %orig(t);
}
%end
