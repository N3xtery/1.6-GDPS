#import "16gdpsIOS.h"

static NSMutableArray* fbNames;
static NSString* fbCurrent;
static NSString* fbExt;
static BOOL fbDirMode;
static BOOL fbReadable;
static void (^fbCallback)(NSString* path);
static UILabel* fbPathLabel;
static UITableView* fbTable;
static Dialog* fbDialog;
static CGFloat fbRawY, fbLastY;

static void fbNavigate(NSString* dir) {
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        NSFileManager* fm = [NSFileManager defaultManager];
        NSArray* children = [[fm contentsOfDirectoryAtPath:dir error:nil] sortedArrayUsingSelector:@selector(caseInsensitiveCompare:)];
        NSMutableArray* names = [NSMutableArray array];
        NSMutableArray* files = [NSMutableArray array];
        if (![dir isEqualToString:@"/"]) [names addObject:@".."];

        for (NSString* name in children) {
            BOOL isDir = NO;
            [fm fileExistsAtPath:[dir stringByAppendingPathComponent:name] isDirectory:&isDir];
            if (isDir) [names addObject:[name stringByAppendingString:@"/"]];
            else if ([[name pathExtension] isEqualToString:fbExt]) [files addObject:name];
        }
        [names addObjectsFromArray:files];

        dispatch_async(dispatch_get_main_queue(), ^{
            fbCurrent = [dir copy];
            fbReadable = children != nil;
            fbNames = names;
            fbPathLabel.text = dir;
            [fbTable reloadData];
            [fbTable setContentOffset:CGPointZero animated:NO];
        });
    });
}

@interface UITableView (FileBrowser) <UITableViewDataSource, UITableViewDelegate>
@end

@implementation UITableView (FileBrowser)
- (NSInteger)tableView:(UITableView*)tv numberOfRowsInSection:(NSInteger)section {
    return [fbNames count];
}

- (UITableViewCell*)tableView:(UITableView*)tv cellForRowAtIndexPath:(NSIndexPath*)indexPath {
    UITableViewCell* cell = [tv dequeueReusableCellWithIdentifier:@"fb"];
    if (!cell) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"fb"];
        cell.textLabel.font = [UIFont systemFontOfSize:14.0f];
    }
    cell.textLabel.text = [fbNames objectAtIndex:indexPath.row];
    return cell;
}

- (void)tableView:(UITableView*)tv didSelectRowAtIndexPath:(NSIndexPath*)indexPath {
    NSString* name = [fbNames objectAtIndex:indexPath.row];
    if ([name isEqualToString:@".."]) {
        fbNavigate([fbCurrent stringByDeletingLastPathComponent]);
    } else if ([name hasSuffix:@"/"]) {
        fbNavigate([fbCurrent stringByAppendingPathComponent:name]);
    } else {
        if (fbDirMode) [tv deselectRowAtIndexPath:indexPath animated:NO];
        else {
            [fbDialog dismiss];
            fbCallback([fbCurrent stringByAppendingPathComponent:name]);
        }
    }
}

- (void)scrollViewWillBeginDragging:(UIScrollView*)sv {
    fbRawY = fbLastY = sv.contentOffset.y;
}

- (void)scrollViewDidScroll:(UIScrollView*)sv {
    if (!sv.dragging) return;  // ignore programmatic changes like reloadData / setContentOffset
    CGFloat maxY = MAX(0, sv.contentSize.height - sv.bounds.size.height);
    fbRawY = MAX(0, MIN(fbRawY + sv.contentOffset.y - fbLastY, maxY));  // accumulate the real movement
    fbLastY = MIN(roundf(fbRawY / self.rowHeight) * self.rowHeight, maxY);
    if (fbLastY != sv.contentOffset.y) sv.contentOffset = CGPointMake(0, fbLastY);
}

- (void)scrollViewWillBeginDecelerating:(UIScrollView*)sv {
    [sv setContentOffset:CGPointMake(0, fbLastY) animated:YES];
}
@end

void showFileBrowser(NSString* extension, BOOL dirMode, void (^callback)(NSString* path)) {
    fbExt = extension;
    fbDirMode = dirMode;
    fbCallback = [callback copy];

    const CGFloat rowHeight = 28.0f;
    const CGFloat labelHeight = 22.0f;

    CGFloat width = maxDlgContentWidth;
    CGFloat tableHeightBase = (dirMode ? maxDlgContentHeightWBtn : maxDlgContentHeight) - labelHeight - spacing;
    CGFloat tableHeight = (tableHeightBase) - fmodf(tableHeightBase, rowHeight);
    CGFloat height = labelHeight + tableHeight + spacing;

    UIView* layout = [[UIView alloc] initWithFrame:CGRectMake(0, 0, width, height)];
    layout.backgroundColor = [UIColor clearColor];

    fbPathLabel = MakeLabel(@"");
    fbPathLabel.frame = CGRectMake(0, 0, width, labelHeight);
    fbPathLabel.lineBreakMode = UILineBreakModeHeadTruncation;
    fbPathLabel.textAlignment = UITextAlignmentCenter;
    [layout addSubview:fbPathLabel];

    fbTable = [[UITableView alloc] initWithFrame:CGRectMake(0, labelHeight, width, tableHeight)];
    fbTable.dataSource = fbTable;
    fbTable.delegate = fbTable;
    fbTable.decelerationRate = UIScrollViewDecelerationRateFast;
    fbTable.bounces = NO;
    fbTable.rowHeight = rowHeight;
    fbTable.tableFooterView = [[UIView alloc] initWithFrame:CGRectZero];
    [layout addSubview:fbTable];

    fbDialog = [[Dialog alloc] init:(dirMode ? @"Select a folder" : @"Select a file") layout:layout];
    if (dirMode) {
        [fbDialog setButton:0 title:@"OK" handler:^{
            if (fbReadable) fbCallback(fbCurrent);
        }];
        [fbDialog setButton:1 title:@"Cancel" handler:nil];
    }

    fbNavigate(@"/var/mobile");
    [fbDialog show];
}
