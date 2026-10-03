#import "iOSBridge.h"

%ctor {
    iOSSetupNative();
}

%hook Everyplay
- (void)showExpiredMessage {}
%end

%hook AppController

- (BOOL)application:(UIApplication *)application
    didFinishLaunchingWithOptions:(NSDictionary *)options {
    BOOL result = %orig;
    iOSBuildOverlay();
    return result;
}
%end
