#import "16gdpsIOS.h"
#import <objc/runtime.h>

static NSString* configFile;
static NSString* user;
static NSString* pass;
static BOOL logged_in = NO;
static UIButton *registerBtn, *loginBtn, *logoutBtn, *saveBtn, *loadBtn, *reqBtn, *percentBtn, *songsBtn, *gmdImportBtn;
static UIButton *copyBtn, *rateBtn;
static UIButton *offsetBtn, *origSongBtn, *customSongBtn, *colorBtn, *gmdExportBtn;
static UILabel *origLabel, *lengthLabel;
static ButtonsLayoutView *optionsRow, *levelMenuRow, *creatorRow;
static BOOL moderator = NO;
static int percentageType = 0;
UIView* hostView;
CGFloat screenWidth;
CGFloat screenHeight;
CGFloat maxDlgContentWidth;
CGFloat maxDlgContentHeight;
CGFloat maxDlgContentHeightWBtn;

void RunOnUiThread(void (^block)(void)) {
    if ([NSThread isMainThread]) block();
    else dispatch_async(dispatch_get_main_queue(), block);
}

void setUser(NSString* userLocal) { user = [userLocal copy]; }
NSString* getUser() { return user; }
void setPass(NSString* passLocal) { pass = [passLocal copy]; }
NSString* getPass() { return pass; }

NSMutableDictionary* loadConfig() {
    NSMutableDictionary *props = [NSMutableDictionary dictionaryWithContentsOfFile:configFile];
    if (!props) props = [NSMutableDictionary dictionary];
    return props;
}

void saveConfig(NSDictionary* props) {
    if (![props writeToFile:configFile atomically:YES]) {
        NSLog(@"OverlayUI: unable to write %@", configFile);
    }
}

void updatePercentage() {
    if (percentageType == 1) [percentBtn setTitle:@"X%" forState:UIControlStateNormal];
    else if (percentageType == 2) [percentBtn setTitle:@"X.X%" forState:UIControlStateNormal];
    else if (percentageType == 3) [percentBtn setTitle:@"X.XX%" forState:UIControlStateNormal];
    else [percentBtn setTitle:@"%: off" forState:UIControlStateNormal];
    [optionsRow relayout];
    NSMutableDictionary* props = loadConfig();
    [props setObject:[NSString stringWithFormat:@"%d", percentageType] forKey:@"percentage"];
    saveConfig(props);
    updatePercentageNative(percentageType);
}

static void showConfirmation(NSString* message) {
    RunOnUiThread(^{
        UILabel* label = MakeLabel(message);
        label.numberOfLines = 0;
        label.lineBreakMode = UILineBreakModeWordWrap;
        CGSize textSize = [message sizeWithFont:label.font
            constrainedToSize:CGSizeMake(280.0f, maxDlgContentHeightWBtn) lineBreakMode:UILineBreakModeWordWrap];
        label.frame = CGRectMake(0, 0, textSize.width, textSize.height);

        UIView* layout = [[UIView alloc] initWithFrame:CGRectMake(0, 0, textSize.width, textSize.height)];
        layout.backgroundColor = [UIColor clearColor];
        [layout addSubview:label];

        Dialog* dialog = [[Dialog alloc] init:@"Confirm" layout:layout];
        [dialog setButton:0 title:@"Yes" handler:^{
            if ([message rangeOfString:@"log out"].location != NSNotFound) {
                logged_in = NO;
                moderator = NO;
                reqBtn.hidden = YES;
                saveBtn.hidden = YES;
                loadBtn.hidden = YES;
                logoutBtn.hidden = YES;
                registerBtn.hidden = NO;
                loginBtn.hidden = NO;
                [optionsRow relayout];
                NSMutableDictionary *props = loadConfig();
                [props setObject:@"" forKey:@"username"];
                [props setObject:@"" forKey:@"password"];
                [props setObject:@"0" forKey:@"moderator"];
                saveConfig(props);
            } else if ([message rangeOfString:@"save"].location != NSNotFound) {
                saveBtn.enabled = NO;
                sendSaveRequest(CSTR(user), CSTR(pass));
            } else if ([message rangeOfString:@"load"].location != NSNotFound) {
                loadBtn.enabled = NO;
                sendLoadRequest(CSTR(user), CSTR(pass), false);
            }
        }];

        if ([message rangeOfString:@"load"].location != NSNotFound) {
            [dialog setButton:1 title:@"W/out my levels" handler:^{
                loadBtn.enabled = NO;
                sendLoadRequest(CSTR(user), CSTR(pass), true);
            }];
            [dialog setButton:2 title:@"No" handler:nil];
        } else [dialog setButton:1 title:@"No" handler:nil];
        [dialog show];
    });
}

void iOSBuildOverlay() {
    id appController = [[UIApplication sharedApplication] delegate];
    Ivar ivar = class_getInstanceVariable([appController class], "viewController");
    UIViewController* viewController = object_getIvar(appController, ivar);
    hostView = viewController.view;

    screenWidth = hostView.bounds.size.width;
    screenHeight = hostView.bounds.size.height;
    maxDlgContentWidth = screenWidth - 2.0f * margin - 2.0f * pad;
    maxDlgContentHeight = screenHeight - 2.0f * margin - titleMarginTop - titleHeight - titleMarginBottom - pad;
    maxDlgContentHeightWBtn = maxDlgContentHeight + pad - 2.0f * spacing - buttonHeight;
    [Dialog dimBackgroundInit];

    RunOnUiThread(^{
        NSString* mainDir = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0];
        configFile = [mainDir stringByAppendingPathComponent:@"modData.plist"];
        NSString* songsDir = nil;
        NSString* menuSong = @"";
        NSString* practiceSong = @"";
        NSFileManager* fileManager = [NSFileManager defaultManager];

        NSMutableDictionary* props = nil;
        if ([fileManager fileExistsAtPath:configFile]) {
            props = loadConfig();
            setUser([props objectForKey:@"username"]);
            setPass([props objectForKey:@"password"]);
            if ([user length] > 0 && [pass length] > 0) logged_in = YES;
            if ([[props objectForKey:@"moderator"] isEqualToString:@"1"]) moderator = YES;
            percentageType = [[props objectForKey:@"percentage"] intValue];
            NSString* songsDirStr = [props objectForKey:@"songsDir"];
            if ([songsDirStr length] > 0) {
                BOOL isDirectory = NO;
                songsDir = songsDirStr;
                if (!([fileManager fileExistsAtPath:songsDir isDirectory:&isDirectory] && isDirectory)) songsDir = nil;
            }
            menuSong = [props objectForKey:@"menuSong"];
            practiceSong = [props objectForKey:@"practiceSong"];
        } else {
            props = [NSMutableDictionary dictionary];
            [props setObject:@"" forKey:@"username"];
            [props setObject:@"" forKey:@"password"];
            [props setObject:@"0" forKey:@"moderator"];
            [props setObject:@"0" forKey:@"percentage"];
            [props setObject:@"" forKey:@"songsDir"];
            [props setObject:@"" forKey:@"menuSong"];
            [props setObject:@"" forKey:@"practiceSong"];
            saveConfig(props);
        }

        if (songsDir == nil) {
            songsDir = [mainDir stringByAppendingPathComponent:@"songs"];
            if (![fileManager fileExistsAtPath:songsDir]) {
                [fileManager createDirectoryAtPath:songsDir withIntermediateDirectories:YES attributes:nil error:NULL];
            }
        }
        if ([menuSong length] > 0) {
            NSString* menuSongFile = [songsDir stringByAppendingPathComponent:[menuSong stringByAppendingString:@".mp3"]];
            if ([fileManager fileExistsAtPath:menuSongFile]) updateMenuPractSongNative(true, CSTR(menuSongFile), false);
            else {
                menuSong = @"";
                [props setObject:@"" forKey:@"menuSong"];
                saveConfig(props);
            }
        }
        if ([practiceSong length] > 0) {
            NSString* practSongFile = [songsDir stringByAppendingPathComponent:[practiceSong stringByAppendingString:@".mp3"]];
            if ([fileManager fileExistsAtPath:practSongFile]) updateMenuPractSongNative(false, CSTR(practSongFile), false);
            else {
                practiceSong = @"";
                [props setObject:@"" forKey:@"practiceSong"];
                saveConfig(props);
            }
        }

        updateSongsPathNative(CSTR([songsDir stringByAppendingString:@"/"]));
        setSongVarsDir(songsDir, menuSong, practiceSong);

        optionsRow = [[ButtonsLayoutView alloc] initWithGravity:UIViewAutoresizingFlexibleLeftMargin|UIViewAutoresizingFlexibleBottomMargin vertical:NO];

        registerBtn = MakeButton(@"Register", ^{ buildAuthDialog(YES); });
        loginBtn = MakeButton(@"Log in", ^{ buildAuthDialog(NO); });
        percentBtn = MakeButton(@"%: off", ^{
            percentageType++;
            if (percentageType > 3) percentageType = 0;
            updatePercentage();
        });
        songsBtn = MakeButton(@"Songs", ^{ buildSongsDialog(NO); });
        reqBtn = MakeButton(@"Req", ^{
            reqBtn.enabled = NO;
            sendReqRequest(CSTR(user), CSTR(pass));
        });
        saveBtn = MakeButton(@"Save", ^{ showConfirmation(@"Are you sure you want to save your data?"); });
        loadBtn = MakeButton(@"Load", ^{ showConfirmation(@"Are you sure you want to load your data?"); });
        logoutBtn = MakeButton(@"Log out", ^{ showConfirmation(@"Are you sure you want to log out?"); });
        gmdImportBtn = MakeButton(@".gdlvl", ^{
            showFileBrowser(@"Import .gdlvl or .gmd", @"gdlvl", NO, ^(NSString* file) {
                importGmd(CSTR(file));
            });
        });
        origLabel = MakeLabel(@"");
        origLabel.shadowColor = [UIColor blackColor];
        origLabel.shadowOffset = CGSizeMake(1.0f, 1.0f);
        origLabel.frame = CGRectMake(0, 0, screenWidth, labelHeight);
        origLabel.textAlignment = UITextAlignmentRight;

        registerBtn.hidden = YES;
        loginBtn.hidden = YES;
        percentBtn.hidden = YES;
        songsBtn.hidden = YES;
        reqBtn.hidden = YES;
        saveBtn.hidden = YES;
        loadBtn.hidden = YES;
        logoutBtn.hidden = YES;
        gmdImportBtn.hidden = YES;
        origLabel.hidden = YES;

        [optionsRow addSubview:gmdImportBtn];
        [optionsRow addSubview:percentBtn];
        [optionsRow addSubview:songsBtn];
        [optionsRow addSubview:reqBtn];
        [optionsRow addSubview:saveBtn];
        [optionsRow addSubview:loadBtn];
        [optionsRow addSubview:logoutBtn];
        [optionsRow addSubview:registerBtn];
        [optionsRow addSubview:loginBtn];
        [hostView addSubview:optionsRow];
        [hostView addSubview:origLabel];

        updatePercentage();

        levelMenuRow = [[ButtonsLayoutView alloc]initWithGravity:UIViewAutoresizingFlexibleRightMargin|UIViewAutoresizingFlexibleTopMargin|
            UIViewAutoresizingFlexibleBottomMargin vertical:YES];

        copyBtn = MakeButton(@"Copy", ^{ cloneLevel(); });
        rateBtn = MakeButton(@"Rate", ^{ buildRateDialog(); });

        copyBtn.hidden = YES;
        rateBtn.hidden = YES;

        [levelMenuRow addSubview:copyBtn];
        [levelMenuRow addSubview:rateBtn];
        [hostView addSubview:levelMenuRow];

        creatorRow = [[ButtonsLayoutView alloc] initWithGravity:UIViewAutoresizingFlexibleRightMargin|UIViewAutoresizingFlexibleBottomMargin vertical:NO];

        offsetBtn = MakeButton(@"Offset", ^{ buildOffsetDialog(); });
        origSongBtn = MakeButton(@"Original song", ^{
            setCustomSong("");
            setCurrentSongID("");
            origSongBtn.hidden = YES;
            [creatorRow relayout];
        });
        customSongBtn = MakeButton(@"Custom song", ^{ buildSongsDialog(YES); });
        colorBtn = MakeButton(@"RGB", ^{ buildColorDialog(); });
        gmdExportBtn = MakeButton(@".gdlvl", ^{
            CGFloat width = 240.0f;

            UIView* layout = [[UIView alloc] initWithFrame:CGRectMake(0, 0, width, fieldHeight + spacing + toggleSize)];
            layout.backgroundColor = [UIColor clearColor];

            TextField* input = [[TextField alloc] initWithFrame:CGRectMake(0, 0, width, fieldHeight)];
            input.text = [NSString stringWithFormat:@"%s.gdlvl", getCurrentLevelName()];
            input.textAlignment = UITextAlignmentCenter;
            [layout addSubview:input];

            UILabel* gdlvlLabel = MakeLabel(@".gdlvl");
            UILabel* gmdLabel = MakeLabel(@".gmd");
            CGFloat start = width / 2.0f - (gdlvlLabel.frame.size.width + gmdLabel.frame.size.width + toggleSize * 2.0f + spacing) / 2.0f;
            gdlvlLabel.frame = CGRectMake(start + toggleSize, fieldHeight + spacing, gdlvlLabel.frame.size.width, toggleSize);
            gmdLabel.frame = CGRectMake(start + toggleSize * 2.0f + gdlvlLabel.frame.size.width + spacing,
                fieldHeight + spacing, gmdLabel.frame.size.width, toggleSize);

            RadioGroup* radioGroup = [[RadioGroup alloc] init];
            ToggleView* gdlvlBtn = [[ToggleView alloc] initWithFrame:CGRectMake(start, fieldHeight + spacing, toggleSize, toggleSize)
                radio:YES radioGroup:radioGroup];
            gdlvlBtn.action = ^{
                if ([[input.text pathExtension] length] > 0) input.text = [[input.text stringByDeletingPathExtension] stringByAppendingString:@".gdlvl"];
            };
            gdlvlBtn.data = @"gdlvl";
            gdlvlBtn.selected = YES;

            ToggleView* gmdBtn = [[ToggleView alloc] initWithFrame:CGRectMake(start + toggleSize + gdlvlLabel.frame.size.width + spacing,
                fieldHeight + spacing, toggleSize, toggleSize) radio:YES radioGroup:radioGroup];
            gmdBtn.data = @"gmd";
            gmdBtn.action = ^{
                if ([[input.text pathExtension] length] > 0) input.text = [[input.text stringByDeletingPathExtension] stringByAppendingString:@".gmd"];
            };

            [layout addSubview:gdlvlLabel];
            [layout addSubview:gdlvlBtn];
            [layout addSubview:gmdLabel];
            [layout addSubview:gmdBtn];

            Dialog* dialog = [[Dialog alloc] init:@"File name" layout:layout];
            [dialog setButton:0 title:@"Export to..." handler:^{
                showFileBrowser(@"Select a folder", @"gdlvl", YES, ^(NSString* dir) {
                    exportGmd(CSTR([dir stringByAppendingPathComponent:[input.text lastPathComponent]]),
                        [[radioGroup getSelected].data isEqualToString:@"gmd"]);
                });
            }];
            [dialog setButton:1 title:@"Cancel" handler:nil];
            [dialog show];

        });
        lengthLabel = MakeLabel(@"");
        lengthLabel.shadowColor = [UIColor blackColor];
        lengthLabel.shadowOffset = CGSizeMake(1.0f, 1.0f);
        lengthLabel.frame = CGRectMake(spacing, buttonHeight + 4.0f, screenWidth - spacing, labelHeight);

        offsetBtn.hidden = YES;
        origSongBtn.hidden = YES;
        customSongBtn.hidden = YES;
        colorBtn.hidden = YES;
        gmdExportBtn.hidden = YES;

        [creatorRow addSubview:offsetBtn];
        [creatorRow addSubview:origSongBtn];
        [creatorRow addSubview:customSongBtn];
        [creatorRow addSubview:colorBtn];
        [creatorRow addSubview:gmdExportBtn];
        [hostView addSubview:creatorRow];
        [hostView addSubview:lengthLabel];

        [optionsRow relayout];
        [levelMenuRow relayout];
        [creatorRow relayout];
    });
}


void showGroup(const char* group, bool visible) {
    int length = (strcmp(group, "LevelSettings") == 0 && visible == true) ? getCurrentLevelLength() : 0;
    RunOnUiThread(^{
        if (strcmp(group, "Options") == 0) {
            gmdImportBtn.hidden = !visible;
            percentBtn.hidden = !visible;
            songsBtn.hidden = !visible;
            if (logged_in) {
                if (!moderator) reqBtn.hidden = !visible;
                saveBtn.hidden = !visible;
                loadBtn.hidden = !visible;
                logoutBtn.hidden = !visible;
            } else {
                registerBtn.hidden = !visible;
                loginBtn.hidden = !visible;
            }
            [optionsRow relayout];
        } else if (strcmp(group, "LevelMenu") == 0) {
            copyBtn.hidden = !visible;
            if (moderator) rateBtn.hidden = !visible;
            [levelMenuRow relayout];
        } else if (strcmp(group, "LevelSettings") == 0) {
            offsetBtn.hidden = !visible;
            if ([getCurrentSongID() length] > 0) origSongBtn.hidden = !visible;
            customSongBtn.hidden = !visible;
            gmdExportBtn.hidden = !visible;
            [creatorRow relayout];
            lengthLabel.text = [NSString stringWithFormat:@"Length: %ds", length];
            lengthLabel.hidden = !visible;
        } else if (strcmp(group, "Color") == 0) {
            colorBtn.hidden = !visible;
            [creatorRow relayout];
        } else if (strcmp(group, "LevelInfo") == 0) {
            if (![origLabel.text isEqualToString:@"original ID: 0"]) origLabel.hidden = !visible;
        }
    });
}

void setOriginalText(int orig) {
    RunOnUiThread(^{
        origLabel.text = [NSString stringWithFormat:@"original ID: %d", orig];
    });
}

void showMessageBox(const char* cTitle, const char* cMessage) {
    RunOnUiThread(^{
        NSString* title = [NSString stringWithUTF8String:cTitle];
        NSString* message = [NSString stringWithUTF8String:cMessage];

        if ([title rangeOfString:@"Account registration"].location != NSNotFound) {
            if ([title rangeOfString:@"successful"].location != NSNotFound) authDialogDismiss();
            activateSubmitBtn();
        } else if ([title rangeOfString:@"Logging in"].location != NSNotFound) {
            if ([title rangeOfString:@"successful"].location != NSNotFound) {
                NSMutableDictionary *props = loadConfig();
                [props setObject:(user ? user : @"") forKey:@"username"];
                [props setObject:(pass ? pass : @"") forKey:@"password"];
                saveConfig(props);

                logged_in = YES;
                authDialogDismiss();
                reqBtn.hidden = NO;
                saveBtn.hidden = NO;
                loadBtn.hidden = NO;
                logoutBtn.hidden = NO;
                registerBtn.hidden = YES;
                loginBtn.hidden = YES;
                [optionsRow relayout];
            }
            activateSubmitBtn();
        } else if ([title rangeOfString:@"Req"].location != NSNotFound) {
            if ([title rangeOfString:@"successful"].location != NSNotFound) {
                moderator = YES;
                reqBtn.hidden = YES;
                [optionsRow relayout];
                NSMutableDictionary* props = loadConfig();
                [props setObject:@"1" forKey:@"moderator"];
                saveConfig(props);
            }
            reqBtn.enabled = YES;
        } else if ([title rangeOfString:@"Save"].location != NSNotFound) {
            saveBtn.enabled = YES;
        } else if ([title rangeOfString:@"Load"].location != NSNotFound) {
            loadBtn.enabled = YES;
        } else if ([title rangeOfString:@"Rate"].location != NSNotFound || [title rangeOfString:@"Unrate"].location != NSNotFound) {
            rateBtn.enabled = YES;
            if ([message isEqualToString:@"You're not a moderator"]) {
                moderator = NO;
                rateBtn.hidden = YES;
                [levelMenuRow relayout];
                NSMutableDictionary* props = loadConfig();
                [props setObject:@"0" forKey:@"moderator"];
                saveConfig(props);
            }
        }

        UILabel* label = MakeLabel(message);
        label.numberOfLines = 0;
        label.lineBreakMode = UILineBreakModeWordWrap;
        CGSize titleSize = [title sizeWithFont:[UIFont boldSystemFontOfSize:17.0f]];
        CGSize msgSize = [message sizeWithFont:label.font constrainedToSize:CGSizeMake(titleSize.width > 280.0f ? titleSize.width : 280.0f,
            maxDlgContentHeightWBtn) lineBreakMode:UILineBreakModeWordWrap];
        label.frame = CGRectMake(msgSize.width >= titleSize.width ? 0 : (titleSize.width - msgSize.width) / 2.0f, 0, msgSize.width, msgSize.height);

        UIView* layout = [[UIView alloc] initWithFrame:CGRectMake(0, 0, msgSize.width >= titleSize.width ? msgSize.width : titleSize.width, msgSize.height)];
        layout.backgroundColor = [UIColor clearColor];
        [layout addSubview:label];

        Dialog* dialog = [[Dialog alloc] init:title layout:layout];
        [dialog setButton:0 title:@"OK" handler:nil];
        [dialog show];
    });
}

void setOrigSongBtnVisible() {
    RunOnUiThread(^{
        origSongBtn.hidden = NO;
        [creatorRow relayout];
    });
}

void disableRateBtn() {
    RunOnUiThread(^{ rateBtn.enabled = NO; });
}
