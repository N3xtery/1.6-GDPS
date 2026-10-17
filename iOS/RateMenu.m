#import "16gdpsIOS.h"

static int stars;
static BOOL featured;

void setStarsFeatured(int starsLocal, bool featuredLocal) {
    stars = starsLocal;
    featured = featuredLocal;
}

void buildRateDialog() {
    CGFloat width = 420.0f;

    UIView* layout = [[UIView alloc] initWithFrame:CGRectMake(0, 0, width, toggleSize + labelHeight)];
    layout.backgroundColor = [UIColor clearColor];

    CGFloat columnWidth = width / 12.0f;
    RadioGroup* radioGroup = [[RadioGroup alloc] init];
    for (int i = 0; i < 12; i++) {
        if (i < 11) {
            ToggleView* radioButton = [[ToggleView alloc] initWithFrame:CGRectMake(i * columnWidth + (columnWidth - toggleSize) / 2.0f,
                0, toggleSize, toggleSize) radio:YES radioGroup:radioGroup];
            radioButton.tag = i;
            if (i == stars) radioButton.selected = YES;
            [layout addSubview:radioButton];
        }

        UILabel* label = MakeLabel(i == 11 ? @"Featured" : [NSString stringWithFormat:@"%d", i]);
        label.textAlignment = UITextAlignmentCenter;
        label.adjustsFontSizeToFitWidth = YES;
        label.minimumFontSize = 8.0f;
        label.frame = CGRectMake(i * columnWidth, toggleSize + 2.0f, columnWidth, labelHeight);
        [layout addSubview:label];
    }

    ToggleView* checkBox = [[ToggleView alloc] initWithFrame:CGRectMake(11 * columnWidth + (columnWidth - toggleSize) / 2.0f,
        0, toggleSize, toggleSize) radio:NO radioGroup:nil];
    checkBox.selected = featured;
    [layout addSubview:checkBox];

    Dialog* dialog = [[Dialog alloc] init:@"Rate Level" layout:layout];
    [dialog setButton:0 title:@"Rate" handler:^{
        disableRateBtn();
        sendRateRequest(CSTR(getUser()), CSTR(getPass()), [radioGroup getSelected].tag, checkBox.selected);
    }];
    [dialog setButton:1 title:@"Cancel" handler:nil];
    [dialog show];
}
