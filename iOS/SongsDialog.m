#import "16gdpsIOS.h"
#import "HTMLNode.h"

static const CGFloat rowHeight = 36.0f;
static const CGFloat headerHeight = 20.0f;
static CGFloat idWidth = 0.0f;
static CGFloat nameWidth = 0.0f;
static CGFloat sizeWidth = 0.0f;
static const CGFloat gap = 4.0f;

static Dialog* loadingDlg;
static NSString* songsDir;
static NSString* menuSong;
static NSString* practiceSong;
static NSString* currentSongID;
static float currentSongOffset;

@interface SongRow : UIView
@property (nonatomic, strong) UILabel* idLabel;
@property (nonatomic, strong) UILabel* nameLabel;
@property (nonatomic, strong) UILabel* sizeLabel;
@property (nonatomic, strong) UIButton* useBtn;
@property (nonatomic, strong) UIButton* menuBtn;
@property (nonatomic, strong) UIButton* practBtn;
@property (nonatomic, strong) UIButton* removeBtn;
@property (nonatomic, assign) BOOL levelSettings;
@end

@implementation SongRow

@synthesize idLabel, nameLabel, sizeLabel;
@synthesize useBtn, menuBtn, practBtn, removeBtn, levelSettings;

- (void)layoutSubviews {
    CGFloat x = 4.0f;
    idLabel.frame = CGRectMake(x, 0.0f, idWidth, rowHeight);
    x += idWidth + gap;
    nameLabel.frame = CGRectMake(x, 0.0f, nameWidth, rowHeight);
    x += nameWidth + gap;
    sizeLabel.frame = CGRectMake(x, 0.0f, sizeWidth, rowHeight);
    x += sizeWidth + gap;

    CGFloat buttonY = (rowHeight - buttonHeight) / 2.0f;
    if (levelSettings) {
        useBtn.frame = CGRectMake(x, buttonY, useBtn.frame.size.width, buttonHeight);
        x += useBtn.frame.size.width + gap;
    } else {
        menuBtn.frame = CGRectMake(x, buttonY, menuBtn.frame.size.width, buttonHeight);
        x += menuBtn.frame.size.width + gap;
        practBtn.frame = CGRectMake(x, buttonY, practBtn.frame.size.width, buttonHeight);
        x += practBtn.frame.size.width + gap;
    }
    removeBtn.frame = CGRectMake(x, buttonY, removeBtn.frame.size.width, buttonHeight);
}

@end

// ---------------------------------------------------------------------------

@interface SongsTable : UIScrollView {
    UILabel *idHeader, *nameHeader, *sizeHeader;
}
@property (nonatomic, assign) BOOL levelSettings;
@property (nonatomic, retain) NSMutableArray* rows;
- (void)addRow:(SongRow *)row atBeginning:(BOOL)atBeginning;
- (void)removeRow:(SongRow *)row;
- (void)removeAllRows;
- (void)relayout;
@end

@implementation SongsTable

@synthesize rows, levelSettings;

- (id)initWithFrame:(CGRect)frame levelSettings:(BOOL)levelSettingsArg {
    self = [super initWithFrame:frame];
    rows = [[NSMutableArray alloc] init];
    self.backgroundColor = [UIColor clearColor];
    self.showsVerticalScrollIndicator = YES;
    self.layer.borderWidth = 2.0f;
    self.layer.borderColor = [UIColor whiteColor].CGColor;
    self.levelSettings = levelSettingsArg;

    NSArray* titles = [NSArray arrayWithObjects:@"ID", @"Name", @"Size", nil];
    UILabel* __strong *headerSlots[3] = { &idHeader, &nameHeader, &sizeHeader };
    for (int i = 0; i < 3; i++) {
        UILabel* headerLabel = MakeLabel([titles objectAtIndex:i]);
        headerLabel.textAlignment = UITextAlignmentCenter;
        headerLabel.font = [UIFont boldSystemFontOfSize:13.0f];
        [self addSubview:headerLabel];
        *headerSlots[i] = headerLabel;
    }
    return self;
}

- (void)addRow:(SongRow *)row atBeginning:(BOOL)atBeginning {
    if (atBeginning) [rows insertObject:row atIndex:0];
    else [rows addObject:row];
    [self addSubview:row];
    [self relayout];
}

- (void)removeRow:(SongRow *)row {
    [row removeFromSuperview];
    [rows removeObject:row];
    [self relayout];
}

- (void)removeAllRows {
    for (SongRow* row in rows) [row removeFromSuperview];
    [rows removeAllObjects];
    [self relayout];
}

- (void)relayout {
    NSArray* headerLabels = [NSArray arrayWithObjects:idHeader, nameHeader, sizeHeader, nil];
    CGFloat* headerWidths[3] = { &idWidth, &nameWidth, &sizeWidth };
    CGFloat x = gap;
    for (int i = 0; i < headerLabels.count; i++) {
        UILabel* headerLabel = [headerLabels objectAtIndex:i];
        if (rows.count) {
            headerLabel.hidden = NO;
            headerLabel.frame = CGRectMake(x, 2.0f, *headerWidths[i], headerHeight + 2.0f);
            x += *headerWidths[i] + gap;
        } else {
            headerLabel.hidden = YES;
            *headerWidths[i] = 0.0f;
        }
    }

    CGFloat y = headerHeight;
    for (SongRow* row in rows) {
        row.frame = CGRectMake(0.0f, y, self.frame.size.width, rowHeight);
        y += rowHeight;
    }
    self.contentSize = CGSizeMake(self.frame.size.width, y);
}

- (void)layoutSubviews {
    [super layoutSubviews];
    [self relayout];
}

@end

void setCurrentSongID(const char* value) {
    currentSongID = [NSString stringWithUTF8String:value];
}

NSString* getCurrentSongID() {
    return currentSongID ? currentSongID : @"";
}

void setCurrentSongOffset(float value) {
    currentSongOffset = value;
}

void setSongVarsDir(NSString* songsDirLocal, NSString* menuSongLocal, NSString* practiceSongLocal) {
    songsDir = [songsDirLocal copy];
    menuSong = [(menuSongLocal ? menuSongLocal : @"") copy];
    practiceSong = [(practiceSongLocal ? practiceSongLocal : @"") copy];
    if (!currentSongID) currentSongID = [@"" copy];
}

void buildOffsetDialog() {
    CGFloat width = 200.0f;

    UIView* layout = [[UIView alloc] initWithFrame:CGRectMake(0, 0, width, fieldHeight)];
    layout.backgroundColor = [UIColor clearColor];

    TextField* inputOffset = [[TextField alloc] initWithFrame:CGRectMake(0, 0, width, fieldHeight)];
    inputOffset.keyboardType = UIKeyboardTypeDecimalPad;
    inputOffset.placeholder = @"Offset";
    inputOffset.text = [NSString stringWithFormat:@"%g", currentSongOffset];
    inputOffset.textAlignment = UITextAlignmentCenter;
    [layout addSubview:inputOffset];

    Dialog* dialog = [[Dialog alloc] init:@"Set the song offset" layout:layout];
    [dialog setButton:0 title:@"Submit" handler:^{
        NSString* offsetStr = [inputOffset.text stringByReplacingOccurrencesOfString:@"," withString:@"."];
        if ([offsetStr length] == 0) offsetStr = @"0";
        currentSongOffset = [offsetStr floatValue];
        setOffset(currentSongOffset);
    }];
    [dialog setButton:1 title:@"Cancel" handler:nil];
    [dialog show];
}

static void findSongID(NSString* songId, SongsTable* table, BOOL levelSettings) {
    if ([songId length] == 0) return;
    NSString* urlString = [NSString stringWithFormat:@"https://en.muzmo.cc/info?id=%@", songId];

    void (^showError)(NSString*) = ^(NSString* error){
        RunOnUiThread(^{
            if (loadingDlg == nil) return;
            if (table != nil) {
                [loadingDlg dismiss];
                loadingDlg = nil;
            }
            showMessageBox("Error", error ? CSTR(error) : "Invalid layout");
        });
    };

    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
      @autoreleasepool {
        NSString* error = nil;
        NSData* html = dataFromURL([NSURL URLWithString:urlString], &error);
        if (error) {
            showError(error);
            return;
        }
        HTMLDocument* doc = html ? [HTMLDocument parseData:html baseURI:urlString] : nil;
        HTMLNode* titleEl = [doc selectFirst:@"b"];

        if (!titleEl || [[titleEl text] isEqualToString:@"Home"]) {
            showError(nil);
            return;
        }

        NSString* title = [titleEl text];
        HTMLNode* currentDiv = [[titleEl parent] nextElementSibling];
        NSMutableArray* songVersions = [NSMutableArray array];
        HTMLNode* firstLink = [currentDiv selectFirst:@"a"];
        if (!firstLink) {
            showError(nil);
            return;
        }
        [songVersions addObject:firstLink];

        while (currentDiv && ![[currentDiv text] hasPrefix:@"Artist"]) currentDiv = [currentDiv nextElementSibling];
        if (!currentDiv) {
            showError(nil);
            return;
        }
        NSString* artistText = [currentDiv text];
        NSRange space = [artistText rangeOfString:@" "];
        NSString* artist = space.location == NSNotFound ? artistText :
            [[artistText substringFromIndex:space.location] stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];

        while (currentDiv && ![currentDiv hasClass:@"sub"]) currentDiv = [currentDiv nextElementSibling];
        if (currentDiv && [[currentDiv text] isEqualToString:@"Generated MP3"]) {
            currentDiv = [currentDiv nextElementSibling];
            while (YES) {
                currentDiv = [currentDiv nextElementSibling];
                if (!currentDiv || [currentDiv hasClass:@"sub"]) break;
                HTMLNode* link = [currentDiv selectFirst:@"a"];
                if (link) [songVersions addObject:link];
            }
        }

        NSMutableArray* versions = [NSMutableArray arrayWithCapacity:[songVersions count]];
        CGFloat maxInfoWidth = 0.0f;
        for (HTMLNode* link in songVersions) {
            HTMLNode* span = [link selectFirst:@"span"];
            NSString* info = span ? [span text] : [link text];
            NSString* href = [link attr:@"href"];
            if (!info) info = @"";
            CGFloat infoWidth = [info sizeWithFont:[UIFont systemFontOfSize:12.0f]].width;
            if (infoWidth > maxInfoWidth) maxInfoWidth = infoWidth;
            [versions addObject:[NSDictionary dictionaryWithObjectsAndKeys:info, @"info",
                [NSString stringWithFormat:@"https://en.muzmo.cc%@", href], @"url", [NSNumber numberWithFloat:infoWidth], @"width", nil]];
        }
        maxInfoWidth += toggleSize;

        RunOnUiThread(^{
            if (loadingDlg == nil) return;
            [loadingDlg dismiss];
            loadingDlg = nil;

            NSString* dlgTitle = [NSString stringWithFormat:@"%@ — %@", artist, title];
            CGFloat width = [dlgTitle sizeWithFont:[UIFont boldSystemFontOfSize:17.0f]].width;
            if (maxInfoWidth > width) width = maxInfoWidth;
            if (width > maxDlgContentWidth) width = maxDlgContentWidth;

            UIView* layout = [[UIView alloc] initWithFrame:CGRectMake(0, 0, width, toggleSize * versions.count)];
            layout.backgroundColor = [UIColor clearColor];

            RadioGroup* radioGroup = [[RadioGroup alloc] init];
            for (NSUInteger i = 0; i < [versions count]; i++) {
                NSDictionary* version = [versions objectAtIndex:i];
                NSString* info = [version objectForKey:@"info"];
                CGFloat offset = (width - (toggleSize + [[version objectForKey:@"width"] floatValue])) / 2.0f;

                ToggleView* button = [[ToggleView alloc] initWithFrame:CGRectMake(offset, i * toggleSize, toggleSize, toggleSize)
                    radio:YES radioGroup:radioGroup];
                button.data = [version objectForKey:@"url"];
                if (i == 0) button.selected = YES;
                [radioGroup addButton:button];
                [layout addSubview:button];

                UILabel* label = MakeLabel(info);
                label.frame = CGRectMake(offset + toggleSize, i * toggleSize, width - toggleSize, toggleSize);
                [layout addSubview:label];

                NSRange kbps = [info rangeOfString:@"Kbps"];
                if (i > 0 && kbps.location != NSNotFound && [[[info substringToIndex:kbps.location] stringByTrimmingCharactersInSet:
                    [NSCharacterSet whitespaceCharacterSet]] intValue] >= 128) {
                        [radioGroup unselectAll];
                        button.selected = YES;
                }
            }

            Dialog* dialog = [[Dialog alloc] init:dlgTitle layout:layout];
            [dialog setButton:0 title:@"Download" handler:^{
                NSString* selected = [radioGroup getSelected].data;
                if (!selected) return;
                NSString* path = [songsDir stringByAppendingPathComponent:[songId stringByAppendingString:@".mp3"]];
                FileDownloadTask* task = [[FileDownloadTask alloc] initWithDestination:path table:(void*)table levelSettings:levelSettings];
                [task execute:selected];
            }];
            if (table == nil) {
                [dialog setButton:1 title:@"Play w/out music" handler:^{
                    playLevel();
                }];
            }
            [dialog setButton:(table == nil ? 2 : 1) title:@"Cancel" handler:nil];
            [dialog show];
        });
      }
    });
}

void buildLoadingDialog(const char* songIdC) {
    RunOnUiThread(^{
        UIView* layout = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 150.0f, 16.0f)];
        layout.backgroundColor = [UIColor clearColor];

        UILabel* label = MakeLabel(@"Loading...");
        label.textAlignment = UITextAlignmentCenter;
        label.frame = CGRectMake(0, 0, 150.0f, labelHeight);
        [layout addSubview:label];

        loadingDlg = [[Dialog alloc] init:@"Custom song" layout:layout];
        if (songIdC) {
            [loadingDlg setButton:0 title:@"Play w/out music" handler:^{
                loadingDlg = nil;
                playLevel();
            }];
        }
        [loadingDlg setButton:(songIdC ? 1 : 0) title:@"Cancel" handler:^{ loadingDlg = nil; }];
        loadingDlg.cancelable = NO;
        [loadingDlg show];
        if (songIdC) {
            NSString* songId = [NSString stringWithUTF8String:songIdC];
            findSongID(songId, nil, NO);
        }
    });
}

void setBtnColor(UIButton* btn, BOOL active) {
    if (active) [btn setTitleColor:[UIColor colorWithRed:0.0f green:1.0f blue:0.0f alpha:1.0f] forState:UIControlStateNormal];
    else [btn setTitleColor:[UIColor colorWithRed:1.0f green:0.0f blue:0.0f alpha:1.0f] forState:UIControlStateNormal];
}

NSString* updateMenuPractSong(SongsTable* table, UIButton* btn, NSString* songGlobal, NSString* songProperties, NSString* songId) {
    if ([songId isEqualToString:songGlobal]) {
        songGlobal = @"";
        if (btn) setBtnColor(btn, NO);
    } else {
        if ([songGlobal length] > 0) {
            for (SongRow* row in [table rows]) {
                if ([row.idLabel.text isEqualToString:songGlobal]) {
                    setBtnColor([songProperties isEqualToString:@"menuSong"] ? row.menuBtn : row.practBtn, NO);
                    break;
                }
            }
        }
        songGlobal = songId;
        setBtnColor(btn, YES);
    }
    NSMutableDictionary* props = loadConfig();
    [props setObject:songGlobal forKey:songProperties];
    saveConfig(props);
    return songGlobal;
}

void songsTableAddRow(void* tableVoid, NSString* file, BOOL toBeginning, BOOL levelSettings) {
    SongsTable* table = (__bridge SongsTable*)tableVoid;
    NSString* artist = @"Unknown";
    NSString* title = @"Unknown";
    MP3File* mp3File = [[MP3File alloc] initWithPath:file];
    if (mp3File) {
        if ([mp3File.artist length] > 0) artist = mp3File.artist;
        if ([mp3File.title length] > 0) title = mp3File.title;
    }

    NSDictionary* attributes = [[NSFileManager defaultManager] attributesOfItemAtPath:file error:NULL];
    unsigned long long fileLength = [attributes fileSize];

    SongRow* row = [[SongRow alloc] initWithFrame:CGRectZero];
    row.levelSettings = levelSettings;
    NSString* songId = [[file lastPathComponent] stringByReplacingOccurrencesOfString:@".mp3" withString:@""];

    row.idLabel = MakeLabel(songId);
    row.idLabel.font = [UIFont systemFontOfSize:10.0f];
    CGSize idSize = [row.idLabel.text sizeWithFont:row.idLabel.font];
    if (idSize.width > idWidth) idWidth = idSize.width;
    row.idLabel.textAlignment = UITextAlignmentCenter;

    row.nameLabel = MakeLabel([NSString stringWithFormat:@"%@ ‒ %@", artist, title]);
    row.nameLabel.textAlignment = UITextAlignmentCenter;
    row.nameLabel.adjustsFontSizeToFitWidth = YES;
    row.nameLabel.baselineAdjustment = UIBaselineAdjustmentAlignCenters;
    row.nameLabel.minimumFontSize = 6.0f;

    row.sizeLabel = MakeLabel([NSString stringWithFormat:@"%.2f MB", fileLength / (1024.0 * 1024.0)]);
    row.sizeLabel.font = [UIFont systemFontOfSize:10.0f];
    CGSize sizeSize = [row.sizeLabel.text sizeWithFont:row.sizeLabel.font];
    if (sizeSize.width > sizeWidth) sizeWidth = sizeSize.width;
    row.sizeLabel.textAlignment = UITextAlignmentCenter;

    [row addSubview:row.idLabel];
    [row addSubview:row.nameLabel];
    [row addSubview:row.sizeLabel];

    if (levelSettings) {
        UIButton* useBtn = MakeButton(@"Use", ^{
            if ([currentSongID length] > 0) {
                for (SongRow* other in [table rows]) {
                    if ([other.idLabel.text isEqualToString:currentSongID]) {
                        other.useBtn.enabled = YES;
                        break;
                    }
                }
            } else setOrigSongBtnVisible();
            setCurrentSongID(CSTR(row.idLabel.text));
            setCustomSong(CSTR(row.idLabel.text));
            row.useBtn.enabled = NO;
        });
        if ([songId isEqualToString:currentSongID]) useBtn.enabled = NO;
        row.useBtn = useBtn;
        useBtn.contentEdgeInsets = UIEdgeInsetsMake(0, 6.0f, 0, 6.0f);
        [useBtn sizeToFit];
        [row addSubview:useBtn];
        nameWidth = table.frame.size.width - (useBtn.frame.size.width + gap);
    } else {
        UIButton* menuBtn = MakeButton(@"Menu", ^{
            menuSong = updateMenuPractSong(table, row.menuBtn, menuSong, @"menuSong", row.idLabel.text);
            NSString* songFile = [songsDir stringByAppendingPathComponent:[menuSong stringByAppendingString:@".mp3"]];
            updateMenuPractSongNative(true, [menuSong length] == 0 ? "" : CSTR(songFile), true);
        });
        if ([songId isEqualToString:menuSong]) setBtnColor(menuBtn, YES);
        else setBtnColor(menuBtn, NO);
        row.menuBtn = menuBtn;
        menuBtn.contentEdgeInsets = UIEdgeInsetsMake(0, 6.0f, 0, 6.0f);
        [menuBtn sizeToFit];
        [row addSubview:menuBtn];

        UIButton* practBtn = MakeButton(@"Pract", ^{
            practiceSong = updateMenuPractSong(table, row.practBtn, practiceSong, @"practiceSong", row.idLabel.text);
            NSString* songFile = [songsDir stringByAppendingPathComponent:[practiceSong stringByAppendingString:@".mp3"]];
            updateMenuPractSongNative(false, CSTR(songFile), false);
        });
        if ([songId isEqualToString:practiceSong]) setBtnColor(practBtn, YES);
        else setBtnColor(practBtn, NO);
        row.practBtn = practBtn;
        practBtn.contentEdgeInsets = UIEdgeInsetsMake(0, 6.0f, 0, 6.0f);
        [practBtn sizeToFit];
        [row addSubview:practBtn];

        nameWidth = table.frame.size.width - (menuBtn.frame.size.width + practBtn.frame.size.width + gap * 2.0f);
    }

    UIButton* removeBtn = MakeButton(@" – ", ^{
        if ([row.idLabel.text isEqualToString:menuSong]) {
            updateMenuPractSongNative(true, "", true);
            menuSong = updateMenuPractSong(nil, nil, menuSong, @"menuSong", row.idLabel.text);
        }
        if ([row.idLabel.text isEqualToString:practiceSong]) {
            updateMenuPractSongNative(false, "", false);
            practiceSong = updateMenuPractSong(nil, nil, practiceSong, @"practiceSong", row.idLabel.text);
        }
        BOOL deleted = [[NSFileManager defaultManager] removeItemAtPath:file error:NULL];
        if (deleted) [table removeRow:row];
    });
    row.removeBtn = removeBtn;
    removeBtn.contentEdgeInsets = UIEdgeInsetsMake(0, 6.0f, 0, 6.0f);
    [removeBtn sizeToFit];
    [row addSubview:removeBtn];
    nameWidth -= (idWidth + sizeWidth + removeBtn.frame.size.width + gap * 2.0f + 12.0f);

    [table addRow:row atBeginning:toBeginning];
}

void songsTablePopulate(SongsTable* table, BOOL levelSettings) {
    table.levelSettings = levelSettings;
    [table removeAllRows];

    NSFileManager* fileManager = [NSFileManager defaultManager];
    NSArray* names = [fileManager contentsOfDirectoryAtPath:songsDir error:NULL];
    NSMutableArray* files = [NSMutableArray array];

    for (NSString* name in names) {
        if (![[name pathExtension] isEqualToString:@"mp3"]) continue;
        NSString* stem = [name stringByDeletingPathExtension];
        if ([stem length] == 0) continue;
        BOOL digitsOnly = YES;
        for (NSUInteger i = 0; i < [stem length]; i++) {
            unichar character = [stem characterAtIndex:i];
            if (character < '0' || character > '9') { digitsOnly = NO; break; }
        }
        if (digitsOnly) [files addObject:[songsDir stringByAppendingPathComponent:name]];
    }

    [files sortUsingComparator:^NSComparisonResult(id path1, id path2) {
        NSDate* date1 = [[fileManager attributesOfItemAtPath:path1 error:NULL] fileModificationDate];
        NSDate* date2 = [[fileManager attributesOfItemAtPath:path2 error:NULL] fileModificationDate];
        if (!date1 || !date2) return NSOrderedSame;
        return [date2 compare:date1];
    }];

    for (NSString* file in files) {
        songsTableAddRow((__bridge void*)table, file, NO, levelSettings);
    }
}

static void setPathText(UIScrollView* scroll, UILabel* label, NSString* text) {
    label.text = text;
    CGFloat width = MAX(scroll.frame.size.width, [text sizeWithFont:label.font].width);
    label.frame = CGRectMake(0, 0, width, scroll.frame.size.height);
    scroll.contentSize = label.frame.size;
    [scroll setContentOffset:CGPointZero animated:NO];
}

void updateSongsPath(NSString* dir, bool defaultPath, bool levelSettings) {
    if (![dir isEqualToString:songsDir]) {
        songsDir = [dir copy];
        NSMutableDictionary* props = loadConfig();
        [props setObject:(defaultPath ? @"" : songsDir) forKey:@"songsDir"];
        saveConfig(props);
        updateSongsPathNative(CSTR([songsDir stringByAppendingString:@"/"]));
        if ([menuSong length] > 0) {
            updateMenuPractSongNative(true, "", levelSettings ? false : true);
            menuSong = updateMenuPractSong(nil, nil, menuSong, @"menuSong", menuSong);
        }
        if ([practiceSong length] > 0) {
            updateMenuPractSongNative(false, "", false);
            practiceSong = updateMenuPractSong(nil, nil, practiceSong, @"practiceSong", practiceSong);
        }
    }
}

void buildSongsDialog(BOOL levelSettings) {
    CGFloat width = maxDlgContentWidth;
    CGFloat height = maxDlgContentHeight;

    UIView* layout = [[UIView alloc] initWithFrame:CGRectMake(0, 0, width, height)];
    layout.backgroundColor = [UIColor clearColor];

    SongsTable* table = [[SongsTable alloc] initWithFrame:CGRectZero levelSettings:levelSettings];

    UILabel* labelPath = MakeLabel(@"Path:");
    [layout addSubview:labelPath];

    TextField* inputPath = [[TextField alloc] initWithFrame:CGRectZero];
    inputPath.text = nil;
    inputPath.userInteractionEnabled = NO;
    [layout addSubview:inputPath];

    UIScrollView* pathScroll = [[UIScrollView alloc] initWithFrame:CGRectZero];
    pathScroll.showsHorizontalScrollIndicator = NO;
    pathScroll.bounces = NO;

    UILabel* inputPathText = [[UILabel alloc] init];
    inputPathText.backgroundColor = [UIColor clearColor];
    inputPathText.font = [UIFont systemFontOfSize:12.0f];
    inputPathText.textColor = inputPath.textColor;
    inputPathText.textAlignment = inputPath.textAlignment;
    [pathScroll addSubview:inputPathText];
    [layout addSubview:pathScroll];

    UIButton* usePathBtn = MakeButton(@"...", ^{
        showFileBrowser(@"mp3", YES, ^(NSString* dir) {
            updateSongsPath(dir, false, levelSettings);
            setPathText(pathScroll, inputPathText, songsDir);
            songsTablePopulate(table, levelSettings);
        });
    });
    [layout addSubview:usePathBtn];

    UIButton* defaultPathBtn = MakeButton(@"Default", ^{
        NSString* mainDir = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0];
        NSString* songsDirNew = [mainDir stringByAppendingPathComponent:@"songs"];
        updateSongsPath(songsDirNew, true, levelSettings);
        setPathText(pathScroll, inputPathText, songsDir);
        songsTablePopulate(table, levelSettings);
    });
    [layout addSubview:defaultPathBtn];

    UILabel* labelID = MakeLabel(@"Add (en.muzmo.cc):");
    [layout addSubview:labelID];

    TextField* idInput = [[TextField alloc] initWithFrame:CGRectZero];
    idInput.keyboardType = UIKeyboardTypeNumberPad;
    idInput.placeholder = @"Song ID";
    [layout addSubview:idInput];

    UIButton* findIDBtn = MakeButton(@"Find", ^{
        [idInput resignFirstResponder];
        buildLoadingDialog(nil);
        findSongID(idInput.text, table, levelSettings);
    });
    [layout addSubview:findIDBtn];

    inputPath.frame = CGRectMake(0, labelHeight,
        width - usePathBtn.frame.size.width - defaultPathBtn.frame.size.width - 120.0f - findIDBtn.frame.size.width - 4.0f * gap, fieldHeight);
    pathScroll.frame = CGRectInset(inputPath.frame, 8.0f, 0);
    setPathText(pathScroll, inputPathText, songsDir);
    labelPath.frame = CGRectMake(0, 0, inputPath.frame.size.width, labelHeight);
    usePathBtn.frame = CGRectMake(CGRectGetMaxX(inputPath.frame) + gap, inputPath.frame.origin.y, usePathBtn.frame.size.width, buttonHeight);
    defaultPathBtn.frame = CGRectMake(CGRectGetMaxX(usePathBtn.frame) + gap, inputPath.frame.origin.y, defaultPathBtn.frame.size.width, buttonHeight);
    idInput.frame = CGRectMake(CGRectGetMaxX(defaultPathBtn.frame) + gap, inputPath.frame.origin.y, 120.0f, fieldHeight);
    labelID.frame = CGRectMake(idInput.frame.origin.x, 0.0f, idInput.frame.size.width, labelHeight);
    findIDBtn.frame = CGRectMake(CGRectGetMaxX(idInput.frame) + gap, inputPath.frame.origin.y, findIDBtn.frame.size.width, buttonHeight);

    table.frame = CGRectMake(0, CGRectGetMaxY(inputPath.frame) + 8.0f, width, height - 56.0f);
    songsTablePopulate(table, levelSettings);
    [layout addSubview:table];

    Dialog* dialog = [[Dialog alloc] init:@"Custom songs list" layout:layout];
    dialog.moveKeyboard = NO;
    [dialog show];
}
