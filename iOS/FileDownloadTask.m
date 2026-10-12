#import "16gdpsIOS.h"

@interface FileDownloadTask () {
    Dialog* dialog;
    UIProgressView* progressBar;
    UIActivityIndicatorView* spinner;
    UILabel* tvPercent;
    NSString* destination;
    void* table;
    BOOL levelSettings;
    long long lastReported;
}
@end

@implementation FileDownloadTask

@synthesize cancelled, totalSize;

- (id)initWithDestination:(NSString*)aDestination table:(void*)aTable levelSettings:(BOOL)aLevelSettings {
    self = [super init];
    destination = [aDestination copy];
    table = aTable;
    levelSettings = aLevelSettings;
    lastReported = 0;
    return self;
}

- (void)cancel {
    cancelled = YES;
    [dialog dismiss];
    dialog = nil;
}

- (void)execute:(NSString*)url {
    [self onPreExecute];
    if (cancelled) return;
    [NSThread detachNewThreadSelector:@selector(doInBackground:) toTarget:self withObject:url];
}

- (void)onPreExecute {
    if ([[NSFileManager defaultManager] fileExistsAtPath:destination]) {
        showMessageBox("Error", "The song is already present");
        cancelled = YES;
        return;
    }

    const CGFloat width = 240.0f;
    const CGFloat progressHeight = 20.0f;

    UIView* layout = [[UIView alloc] initWithFrame:CGRectMake(0, 0, width, labelHeight + progressHeight)];
    layout.backgroundColor = [UIColor clearColor];

    spinner = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleWhite];
    spinner.frame = CGRectMake(0, 0, 20.0f, 20.0f);
    [layout addSubview:spinner];
    [spinner startAnimating];

    tvPercent = MakeLabel(@"Downloading... 0%");
    tvPercent.frame = CGRectMake(28.0f, 0, width - 28.0f, 20.0f);
    [layout addSubview:tvPercent];

    progressBar = [[UIProgressView alloc] initWithProgressViewStyle:UIProgressViewStyleDefault];
    progressBar.frame = CGRectMake(0, 28.0f, width, progressHeight);
    progressBar.progress = 0.0f;
    [layout addSubview:progressBar];

    dialog = [[Dialog alloc] init:@"Downloading file" layout:layout];
    dialog.cancelable = NO;
    __unsafe_unretained typeof(self) weakSelf = self;
    [dialog setButton:0 title:@"Cancel" handler:^{ [weakSelf cancel]; }];
    [dialog show];
}

- (void)doInBackground:(NSString*)fileUrl {
    @autoreleasepool {
        BOOL success = NO;
        NSString* error = nil;

        NSURL* url = [NSURL URLWithString:fileUrl];
        if (url) success = downloadURL(url, destination, self, &error);

        if (!success && !cancelled) showMessageBox("Error", CSTR(error));
        [self performSelectorOnMainThread:@selector(onPostExecute:) withObject:[NSNumber numberWithBool:success] waitUntilDone:NO];
    }
}

- (void)progress:(long long)received {
    if (totalSize <= 0) return;
    int progress = (int)(received * 100 / totalSize);
    if (progress == lastReported) return;
    lastReported = progress;
    [self performSelectorOnMainThread:@selector(publishProgress:) withObject:[NSNumber numberWithInt:progress] waitUntilDone:NO];
}

- (void)publishProgress:(NSNumber*)value {
    if (dialog == nil) return;
    int progress = [value intValue];
    progressBar.progress = progress / 100.0f;
    tvPercent.text = [NSString stringWithFormat:@"Downloading... %d%%", progress];
}

- (void)onPostExecute:(NSNumber*)value {
    BOOL success = [value boolValue];
    [dialog dismiss];
    dialog = nil;

    if (success) {
        [[NSFileManager defaultManager] setAttributes:[NSDictionary dictionaryWithObject:[NSNumber numberWithShort:0644]
            forKey:NSFilePosixPermissions] ofItemAtPath:destination error:NULL];
        if (table != nil) songsTableAddRow(table, destination, YES, levelSettings);
        else playLevel();
    }
}

@end
