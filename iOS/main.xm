#import "iOSBridge.h"
#import <AVFoundation/AVFoundation.h>

%ctor {
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
