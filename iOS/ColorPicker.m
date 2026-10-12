#import "16gdpsIOS.h"

static int red;
static int green;
static int blue;

void setColor(int r, int g, int b) {
    red = r;
    green = g;
    blue = b;
}

void buildColorDialog() {
    CGFloat width = 360.0f;

    UIView* layout = [[UIView alloc] initWithFrame:CGRectMake(0, 0, width, labelHeight + fieldHeight)];
    layout.backgroundColor = [UIColor clearColor];

    CGFloat columnWidth = (width - 16.0f) / 3.0f;
    NSArray* names = [NSArray arrayWithObjects:@"Red", @"Green", @"Blue", nil];
    int values[3] = { red, green, blue };
    TextField* fields[3];

    for (int i = 0; i < 3; i++) {
        CGFloat x = i * (columnWidth + spacing);

        UILabel* label = MakeLabel([names objectAtIndex:i]);
        label.textAlignment = UITextAlignmentCenter;
        label.frame = CGRectMake(x, 0, columnWidth, labelHeight);
        [layout addSubview:label];

        fields[i] = [[TextField alloc] initWithFrame:CGRectMake(x, labelHeight, columnWidth, fieldHeight)];
        fields[i].keyboardType = UIKeyboardTypeNumberPad;
        fields[i].maxLength = 3;
        fields[i].text = [NSString stringWithFormat:@"%d", values[i]];
        fields[i].textAlignment = UITextAlignmentCenter;
        [layout addSubview:fields[i]];
    }

    TextField* inputR = fields[0];
    TextField* inputG = fields[1];
    TextField* inputB = fields[2];

    Dialog* dialog = [[Dialog alloc] init:@"Set the color" layout:layout];
    [dialog setButton:0 title:@"OK" handler:^{
        red = 0;
        green = 0;
        blue = 0;
        NSString* rStr = inputR.text;
        if ([rStr length] > 0) red = [rStr intValue];
        NSString* gStr = inputG.text;
        if ([gStr length] > 0) green = [gStr intValue];
        NSString* bStr = inputB.text;
        if ([bStr length] > 0) blue = [bStr intValue];
        if (red > 255) red = 255;
        if (green > 255) green = 255;
        if (blue > 255) blue = 255;

        setColorNative(red, green, blue);
    }];
    [dialog setButton:1 title:@"Cancel" handler:nil];
    [dialog show];
}
