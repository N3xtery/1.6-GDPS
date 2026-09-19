#import "16gdps.h"

%ctor {
    onLoad();
}

%hook Everyplay
- (void)showExpiredMessage {}
%end
