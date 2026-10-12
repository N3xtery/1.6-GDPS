#import "16gdpsIOS.h"
#import <objc/runtime.h>

@implementation ButtonsLayoutView

- (id)initWithGravity:(UIViewAutoresizing)gravityArg vertical:(BOOL)isVertical {
    self = [super initWithFrame:CGRectZero];
    if (self) {
        vertical = isVertical;
        self.autoresizingMask = gravityArg;
        self.backgroundColor = [UIColor clearColor];
    }
    return self;
}

- (void)relayout {
    CGFloat x = 0.0f, widest = 0.0f;
    for (UIView* child in self.subviews) {
        if (child.hidden) continue;
        [child sizeToFit];
        CGSize size = child.frame.size;
        if (vertical) {
            if (size.width > widest) widest = size.width;
        } else {
            child.frame = CGRectMake(x, 0.0f, size.width, buttonHeight);
            x += size.width;
        }
    }
    if (vertical) for (int i = 0; i < self.subviews.count; i++) {
        UIView* subview = [self.subviews objectAtIndex:i];
        subview.frame = CGRectMake(0, i * buttonHeight, widest, buttonHeight);
    }

    CGFloat originX = 0.0f, originY = 0.0f;
    CGSize total = vertical ? CGSizeMake(widest, buttonHeight * self.subviews.count) : CGSizeMake(x, buttonHeight);
    switch ((NSUInteger)self.autoresizingMask) {
        case UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleBottomMargin:
            originX = screenWidth - total.width;
            originY = 0.0f;
            break;
        case UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleTopMargin | UIViewAutoresizingFlexibleBottomMargin:
            originX = 0.0f;
            originY = (screenHeight - total.height) / 2.0f;
            break;
    }
    self.frame = CGRectMake(originX, originY, total.width, total.height);
}

- (void)didMoveToSuperview {
    [super didMoveToSuperview];
    if (self.superview) [self relayout];
}

@end

// ---------------------------------------------------------------------------

static char kButtonActionKey;

@implementation ButtonAction

- (id)initWithBlock:(void (^)(void))aBlock {
    self = [super init];
    if (self) block = [aBlock copy];
    return self;
}

- (void)fire {
    if (block) block();
}

+ (void)attachTo:(UIControl *)control action:(void (^)(void))action {
    if (!action) return;
    ButtonAction* target = [[ButtonAction alloc] initWithBlock:action];
    [control addTarget:target action:@selector(fire) forControlEvents:UIControlEventTouchUpInside];
    objc_setAssociatedObject(control, &kButtonActionKey, target, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

@end

// ---------------------------------------------------------------------------

@implementation TextField

@synthesize maxLength;

- (id)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        maxLength = 0;
        self.borderStyle = UITextBorderStyleRoundedRect;
        self.contentVerticalAlignment = UIControlContentVerticalAlignmentCenter;
        self.autocorrectionType = UITextAutocorrectionTypeNo;
        self.autocapitalizationType = UITextAutocapitalizationTypeNone;
        self.clearButtonMode = UITextFieldViewModeWhileEditing;
        self.returnKeyType = UIReturnKeyDone;
        self.font = [UIFont systemFontOfSize:15.0f];
        [self addTarget:self action:@selector(donePressed:) forControlEvents:UIControlEventEditingDidEndOnExit];
        [self addTarget:self action:@selector(limitText:) forControlEvents:UIControlEventEditingChanged];
    }
    return self;
}

- (void)donePressed:(id)sender {
    [self resignFirstResponder];
}

- (void)limitText:(id)sender {
    if (maxLength <= 0) return;
    if ([self.text length] > maxLength) self.text = [self.text substringToIndex:maxLength];
}

@end

// ---------------------------------------------------------------------------

@implementation RadioGroup

- (id)init {
    self = [super init];
    buttons = [[NSMutableArray alloc] init];
    return self;
}

- (void)addButton:(ToggleView*)button {
    [buttons addObject:button];
}

- (void)unselectAll {
    for (ToggleView* button in buttons) if (button.selected) button.selected = NO;
}

- (ToggleView*)getSelected {
    for (ToggleView* button in buttons) if (button.selected) return button;
    return nil;
}

@end

// ---------------------------------------------------------------------------

@implementation ToggleView

@synthesize data;

- (id)initWithFrame:(CGRect)frame radio:(BOOL)radioArg radioGroup:(RadioGroup*)radioGroupArg {
    self = [super initWithFrame:frame];
    radio = radioArg;
    radioGroup = radioGroupArg;
    self.backgroundColor = [UIColor clearColor];
    self.opaque = NO;
    [self addTarget:self action:@selector(toggle) forControlEvents:UIControlEventTouchUpInside];
    return self;
}

- (void)toggle {
    if (radio) {
        if (self.selected) return;
        [radioGroup unselectAll];
    }
    self.selected = !self.selected;
    [self sendActionsForControlEvents:UIControlEventValueChanged];
}

- (void)drawRect:(CGRect)rect {
    CGContextRef ctx = UIGraphicsGetCurrentContext();
    CGRect box = CGRectMake(4.0f, 4.0f, self.bounds.size.width - 8.0f, self.bounds.size.height - 8.0f);

    CGContextSetLineWidth(ctx, 1.5f);
    CGContextSetStrokeColorWithColor(ctx, [UIColor whiteColor].CGColor);
    CGContextSetFillColorWithColor(ctx, [UIColor colorWithRed:0.20f green:0.60f blue:1.0f alpha:1.0f].CGColor);

    if (radio) {
        CGContextStrokeEllipseInRect(ctx, box);
        if (self.selected) CGContextFillEllipseInRect(ctx, CGRectInset(box, 4.5f, 4.5f));
    } else {
        CGContextStrokeRect(ctx, box);
        if (self.selected) CGContextFillRect(ctx, CGRectInset(box, 4.0f, 4.0f));
    }
}

@end

// ---------------------------------------------------------------------------

static UIImage* buttonImage(CGFloat height, UIColor* top, UIColor* bottom, UIColor* border, CGFloat borderWidth, CGFloat inset) {
    const CGFloat width = 24.0f, radius = 10.5f;

    UIGraphicsBeginImageContextWithOptions(CGSizeMake(width, height), NO, 0.0f);
    CGContextRef ctx = UIGraphicsGetCurrentContext();
    UIBezierPath* shape = [UIBezierPath bezierPathWithRoundedRect:CGRectInset(CGRectMake(0, 0, width, height), inset, inset) cornerRadius:radius];

    CGContextSaveGState(ctx);
    [shape addClip];

    CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
    NSArray* colors = [NSArray arrayWithObjects:(id)top.CGColor, (id)bottom.CGColor, nil];
    CGGradientRef gradient = CGGradientCreateWithColors(space, (__bridge CFArrayRef)colors, NULL);
    CGContextDrawLinearGradient(ctx, gradient, CGPointMake(0, inset), CGPointMake(0, height - inset), 0);
    CGGradientRelease(gradient);
    CGColorSpaceRelease(space);

    [border setStroke];
    shape.lineWidth = borderWidth * 2.0f;
    [shape stroke];
    CGContextRestoreGState(ctx);

    UIImage* image = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    return [image stretchableImageWithLeftCapWidth:(int)(width / 2) topCapHeight:(int)(height / 2)];
}

UIButton* MakeButton(NSString* title, void (^action)(void)) {
    UIButton* button = [UIButton buttonWithType:UIButtonTypeCustom];
    [button setTitle:title forState:UIControlStateNormal];
    [button setTitleColor:[UIColor colorWithRed:56.0f/255.0f green:84.0f/255.0f blue:135.0f/255.0f alpha:1.0f] forState:UIControlStateNormal];
    [button setTitleColor:[UIColor colorWithRed:155.0f/255.0f green:165.0f/255.0f blue:185.0f/255.0f alpha:1.0f] forState:UIControlStateDisabled];
    [button setTitleColor:[UIColor whiteColor] forState:UIControlStateHighlighted];
    UIImage* defaultBg = buttonImage(buttonHeight, [UIColor whiteColor], [UIColor whiteColor],
        [UIColor colorWithWhite:159.0f/255.0f alpha:1.0f], 1.0f, 0.0f);
    [button setBackgroundImage:defaultBg forState:UIControlStateNormal];
    [button setBackgroundImage:defaultBg forState:UIControlStateDisabled];
    [button setBackgroundImage:buttonImage(buttonHeight,
        [UIColor colorWithRed:5.0f/255.0f green:139.0f/255.0f blue:245.0f/255.0f alpha:1.0f],
        [UIColor colorWithRed:1.0f/255.0f green:94.0f/255.0f blue:230.0f/255.0f alpha:1.0f],
        [UIColor colorWithRed:41.0f/255.0f green:70.0f/255.0f blue:103.0f/255.0f alpha:1.0f],
        1.0f / [UIScreen mainScreen].scale, 1.0f / [UIScreen mainScreen].scale) forState:UIControlStateHighlighted];
    button.contentEdgeInsets = UIEdgeInsetsMake(0, 12.0f, 0, 12.0f);
    button.titleLabel.font = [UIFont systemFontOfSize:14.0f];
    [button sizeToFit];
    [ButtonAction attachTo:button action:action];
    return button;
}

UILabel* MakeLabel(NSString* text) {
    UILabel* label = [[UILabel alloc] initWithFrame:CGRectZero];
    label.text = text;
    label.font = [UIFont systemFontOfSize:12.0f];
    label.textColor = [UIColor whiteColor];
    label.backgroundColor = [UIColor clearColor];
    [label sizeToFit];
    return label;
}
