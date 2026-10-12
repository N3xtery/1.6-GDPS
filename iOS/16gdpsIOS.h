#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import "iOSBridge.h"
#define CSTR(s) ((s) ? [(s) UTF8String] : "")

// OverlayUI.m
void RunOnUiThread(void (^block)(void));
void setUser(NSString* userLocal);
NSString* getUser();
void setPass(NSString* passLocal);
NSString* getPass();
NSMutableDictionary* loadConfig();
void saveConfig(NSDictionary* props);
extern UIView* hostView;
extern CGFloat screenWidth;
extern CGFloat screenHeight;
extern CGFloat maxDlgContentWidth;
extern CGFloat maxDlgContentHeight;
extern CGFloat maxDlgContentHeightWBtn;

// SongsDialog.m
NSString* getCurrentSongID();
void setSongVarsDir(NSString* songsDirLocal, NSString* menuSongLocal, NSString* practiceSongLocal);
void buildOffsetDialog();
void buildSongsDialog(BOOL levelSettings);
void songsTableAddRow(void* table, NSString* file, BOOL toBeginning, BOOL levelSettings);

// RateDialog.m
void buildRateDialog();

// ColorPicker.m
void buildColorDialog();

// AuthDialog.m
void buildAuthDialog(BOOL reg);
void authDialogDismiss();
void activateSubmitBtn();


// Dialog.m
extern const CGFloat pad;
extern const CGFloat margin;
extern const CGFloat buttonHeight;
extern const CGFloat fieldHeight;
extern const CGFloat labelHeight;
extern const CGFloat toggleSize;
extern const CGFloat titleHeight;
extern const CGFloat titleMarginTop;
extern const CGFloat titleMarginBottom;
extern const CGFloat spacing;
@interface Dialog : UIView {
    UIView* panel;
    UILabel* titleLabel;
    UIView* contentView;
    UIButton* buttons[3];
    BOOL cancelable;
    BOOL autoDismiss;
    CGFloat offset;
    BOOL moveKeyboard;
    void (^onDismiss)(void);
}
@property (nonatomic, assign) BOOL cancelable;
@property (nonatomic, assign) BOOL moveKeyboard;
@property (nonatomic, assign) BOOL autoDismiss;
@property (nonatomic, copy) void (^onDismiss)(void);
- (id)init:(NSString*)titleArg layout:(UIView*)layoutArg;
- (UIButton*)getButton:(int)i;
- (void)setButton:(int)i title:(NSString*)title handler:(void(^)(void))handler;
- (void)show;
- (void)dismiss;
+ (void)dimBackgroundInit;
@end


// Widgets.m
@interface ButtonsLayoutView : UIView {
    BOOL vertical;
}
- (id)initWithGravity:(UIViewAutoresizing)gravityArg vertical:(BOOL)isVertical;
- (void)relayout;
@end
// -------------------
@interface ButtonAction : NSObject {
    void (^block)(void);
}
+ (void)attachTo:(UIControl *)control action:(void (^)(void))action;
@end
// -------------------
@interface TextField : UITextField <UITextFieldDelegate> {
    NSInteger maxLength;
}
@property (nonatomic, assign) NSInteger maxLength;
@end
// -------------------
@class ToggleView;
// -------------------
@interface RadioGroup : NSObject { NSMutableArray* buttons; }
- (id)init;
- (void)addButton:(ToggleView*)button;
- (void)unselectAll;
- (ToggleView*)getSelected;
@end
// -------------------
@interface ToggleView : UIControl {
    BOOL radio;
    RadioGroup* radioGroup;
}
@property (nonatomic, retain) NSString* data;
- (id)initWithFrame:(CGRect)frame radio:(BOOL)radioArg radioGroup:(RadioGroup*)radioGroupArg;
@end
// -------------------
UIButton* MakeButton(NSString* title, void (^action)(void));
UILabel* MakeLabel(NSString* text);

// MP3File.m
@interface MP3File : NSObject
@property (nonatomic, readonly, copy) NSString* artist;
@property (nonatomic, readonly, copy) NSString* title;
- (id)initWithPath:(NSString*)path;
@end

// FileBrowser.m
extern void showFileBrowser(NSString* extension, BOOL dirMode, void (^callback)(NSString* path));

// FileDownloadTask.m
@interface FileDownloadTask : NSObject
@property (nonatomic, assign) BOOL cancelled;
@property (nonatomic, assign) long long totalSize;
- (id)initWithDestination:(NSString*)destination table:(void*)table levelSettings:(BOOL)levelSettings;
- (void)execute:(NSString*)url;
- (void)progress:(long long)received;
- (void)cancel;
@end

// HttpClient.m
extern NSData* dataFromURL(NSURL* url, NSString** error);
extern bool downloadURL(NSURL* url, NSString* path, FileDownloadTask* fileDownloader, NSString** error);
