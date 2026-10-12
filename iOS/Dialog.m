#import "16gdpsIOS.h"

const CGFloat pad = 12.0f;
const CGFloat margin = 12.0f;
const CGFloat buttonHeight = 32.0f;
const CGFloat fieldHeight = 32.0f;
const CGFloat labelHeight = 16.0f;
const CGFloat toggleSize = 28.0f;
const CGFloat titleHeight = 26.0f;
const CGFloat titleMarginTop = 10.0f;
const CGFloat titleMarginBottom = 6.0f;
const CGFloat spacing = 8.0f;
static UIImage* cachedDimImage = nil;

@implementation Dialog

@synthesize autoDismiss, onDismiss, cancelable, moveKeyboard;

- (id)init:(NSString*)titleArg layout:(UIView*)layoutArg {
    self = [super initWithFrame:CGRectZero];
    autoDismiss = YES;
    cancelable = YES;
    offset = 0.0f;
    for (int i = 0; i < 3; i++) buttons[i] = nil;
    self.backgroundColor = [UIColor clearColor];
    self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    self.layer.contents = (id)cachedDimImage.CGImage;
    self.opaque = NO;

    panel = [[UIView alloc] initWithFrame:CGRectZero];
    panel.backgroundColor = [UIColor colorWithRed:0.0f/255.0f green:29.0f/255.0f blue:106.0f/255.0f alpha:0.5f];
    panel.layer.cornerRadius = 10.0f;
    panel.layer.borderWidth = 2.0f;
    panel.layer.borderColor = [UIColor colorWithWhite:1.0f alpha:0.5f].CGColor;
    panel.layer.shadowColor = [UIColor blackColor].CGColor;
    panel.layer.shadowOffset = CGSizeMake(0.0f, 2.0f);
    panel.layer.shadowRadius = 6.0f;
    panel.layer.shadowOpacity = 1.0f;
    contentView = layoutArg;
    [panel addSubview:layoutArg];
    [self addSubview:panel];

    titleLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    titleLabel.backgroundColor = [UIColor clearColor];
    titleLabel.textColor = [UIColor whiteColor];
    titleLabel.textAlignment = UITextAlignmentCenter;
    titleLabel.shadowColor = [UIColor blackColor];
    titleLabel.shadowOffset = CGSizeMake(0.0f, 1.0f);
    titleLabel.font = [UIFont boldSystemFontOfSize:17.0f];
    titleLabel.text = titleArg;
    [panel addSubview:titleLabel];
    moveKeyboard = YES;
    return self;
}

+ (void)dimBackgroundInit {
    CGSize size = CGSizeMake(screenWidth, screenHeight);
    UIGraphicsBeginImageContextWithOptions(size, NO, 0.0f);
    CGContextRef context = UIGraphicsGetCurrentContext();

    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    const int stopCount = 128;
    CGFloat locations[stopCount];
    CGFloat components[stopCount * 4];
    for (int i = 0; i < stopCount; i++) {
        CGFloat t = (CGFloat)i / (CGFloat)(stopCount - 1);
        locations[i] = t;
        components[i * 4 + 0] = 0.0f;
        components[i * 4 + 1] = 0.0f;
        components[i * 4 + 2] = 0.0f;
        components[i * 4 + 3] = (1.0f - expf(-2.0f * i / 127.0f)) / (1.0f - expf(-2.0f)) * 0.5f;
    }
    CGGradientRef gradient = CGGradientCreateWithColorComponents(colorSpace, components, locations, stopCount);

    CGPoint center = CGPointMake(screenWidth * 0.5f, screenHeight * 0.5f);
    CGFloat radius = sqrtf(screenWidth * screenWidth * 0.25f + screenHeight * screenHeight * 0.25f);

    CGContextDrawRadialGradient(context, gradient, center, 0.0f, center, radius, kCGGradientDrawsAfterEndLocation);

    CGGradientRelease(gradient);
    CGColorSpaceRelease(colorSpace);

    cachedDimImage = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
}

- (void)setButton:(int)i title:(NSString*)title handler:(void(^)(void))handler {
    UIButton* button = MakeButton(title, handler);
    [button addTarget:self action:@selector(buttonTapped:) forControlEvents:UIControlEventTouchUpInside];
    [panel addSubview:button];
    buttons[i] = button;
}

- (UIButton*)getButton:(int)i {
    return buttons[i];
}

- (void)buttonTapped:(UIButton*)sender {
    if (autoDismiss) [self dismiss];
}

- (void)show {
    self.frame = hostView.bounds;
    [hostView addSubview:self];

    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardWillShow:) name:UIKeyboardWillShowNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardWillHide:) name:UIKeyboardWillHideNotification object:nil];

    [self setNeedsLayout];
    self.alpha = 0.0f;
    panel.transform = CGAffineTransformMakeScale(0.85f, 0.85f);

    [UIView animateWithDuration:0.20f animations:^{
        self.alpha = 1.0f;
        panel.transform = CGAffineTransformMakeScale(1.00f, 1.00f);
    }];
}

- (void)dismiss {
    if (!self.superview) return;
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [self endEditing:YES];
    [self removeFromSuperview];
    if (onDismiss) onDismiss();
}


- (void)layoutSubviews {
    CGFloat contentWidth = contentView.frame.size.width;
    CGFloat panelWidth = contentWidth + 2.0f * pad;

    CGFloat y = titleMarginTop + titleHeight + titleMarginBottom + contentView.frame.size.height;
    if (buttons[0]) {
        y += spacing;
        int visible = 0;
        CGFloat buttonWidths[3];
        CGFloat buttonWidthSum = 0.0f;
        for (int i = 0; i < 3; i++) if (buttons[i]) {
            visible++;
            buttonWidths[i] = buttons[i].frame.size.width;
            if (buttonWidths[i] < 125.0f) buttonWidths[i] = 125.0f;
            buttonWidthSum += buttonWidths[i];
        }

        CGFloat maxBtnRowWidth = panelWidth - spacing * 2.0f;
        CGFloat buttonRowWidth = buttonWidthSum + spacing * (visible - 1);
        if (maxBtnRowWidth < buttonRowWidth) {
            maxBtnRowWidth = buttonRowWidth;
            panelWidth = maxBtnRowWidth + spacing * 2.0f;
        }
        CGFloat x = spacing;
        CGFloat buttonStarts[3];
        for (int i = 0; i < visible; i++) {
            CGFloat sectionSize = (buttonWidths[i] / buttonWidthSum) * maxBtnRowWidth;
            buttonStarts[i] = x + (sectionSize - buttonWidths[i]) / 2.0f;
            x += sectionSize;
        }
        if (visible > 1) {
            CGFloat diff = buttonStarts[1] - (buttonStarts[0] + buttonWidths[0]);
            if (diff < 8.0f) {
                if (visible == 2) {
                    diff = (8.0f - diff) / 2.0f;
                    buttonStarts[1] += diff;
                } else {
                    diff = 8.0f - diff;
                    buttonStarts[2] += diff;
                }
                buttonStarts[0] -= diff;
            }
        }
        for (int i = 0; i < visible; i++) buttons[i].frame = CGRectMake(buttonStarts[i], y, buttonWidths[i], buttonHeight);

        y += buttonHeight + spacing;
    } else y += pad;

    titleLabel.frame = CGRectMake(pad, titleMarginTop, panelWidth - 2.0f * pad, titleHeight);
    CGRect contentFrame = contentView.frame;
    contentFrame.origin = CGPointMake((panelWidth - contentWidth) / 2.0f, titleMarginTop + titleHeight + titleMarginBottom);
    contentView.frame = contentFrame;

    panel.frame = CGRectMake((screenWidth - panelWidth) / 2.0f, (screenHeight - y) / 2.0f + offset, panelWidth, y);
}

- (void)keyboardWillShow:(NSNotification *)notification {
    NSValue* value = [[notification userInfo] objectForKey:UIKeyboardFrameEndUserInfoKey];
    if (!value || !moveKeyboard) return;
    CGRect keyboard = [self convertRect:[value CGRectValue] fromView:nil];
    offset = -(CGRectGetMaxY(panel.frame) - keyboard.origin.y);
    if (offset > 0.0f) return;
    [UIView animateWithDuration:0.25 animations:^{
        panel.transform = CGAffineTransformMakeTranslation(0.0f, offset);
    }];
}

- (void)keyboardWillHide:(NSNotification *)notification {
    offset = 0.0f;
    [UIView animateWithDuration:0.25 animations:^{
        panel.transform = CGAffineTransformIdentity;
    }];
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {}
- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
    if (!cancelable) return;
    CGPoint point = [[touches anyObject] locationInView:self];
    if (CGRectContainsPoint(panel.frame, point)) return;
    [self dismiss];
}

@end
