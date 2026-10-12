#import "16gdpsIOS.h"

static Dialog* authDialog;
static BOOL waiting;

void authDialogDismiss() {
    if (authDialog != nil) [authDialog dismiss];
}

void activateSubmitBtn() {
    waiting = NO;
    if (authDialog != nil) [[authDialog getButton:0] setEnabled:YES];
}

void buildAuthDialog(BOOL reg) {
    CGFloat width = 360.0f;

    UIView *layout = [[UIView alloc] initWithFrame:CGRectMake(0, 0, width, 0)];
    layout.backgroundColor = [UIColor clearColor];
    CGFloat y = 0.0f;

    TextField *inputUser = [[TextField alloc] initWithFrame:CGRectMake(0, y, width, fieldHeight)];
    inputUser.placeholder = @"Username";
    inputUser.maxLength = 20;
    [layout addSubview:inputUser];
    y += fieldHeight + spacing;

    CGFloat columnWidth = reg ? (width - spacing) / 2.0f : width;

    TextField *inputPass = [[TextField alloc] initWithFrame:CGRectMake(0, y, columnWidth, fieldHeight)];
    inputPass.placeholder = @"Password";
    inputPass.secureTextEntry = YES;
    [layout addSubview:inputPass];

    TextField *inputPassRepeat = nil;
    if (reg) {
        inputPassRepeat = [[TextField alloc] initWithFrame:CGRectMake(0 + columnWidth + spacing, y, columnWidth, fieldHeight)];
        inputPassRepeat.placeholder = @"Confirm Password";
        inputPassRepeat.secureTextEntry = YES;
        [layout addSubview:inputPassRepeat];
    }
    y += fieldHeight;

    layout.frame = CGRectMake(0, 0, width, y);

    authDialog = [[Dialog alloc] init:(reg ? @"Register" : @"Log in") layout:layout];
    authDialog.autoDismiss = NO;
    authDialog.onDismiss = ^{ authDialog = nil; };
    [authDialog setButton:0 title:@"Submit" handler:^{
        NSString *user = inputUser.text ? inputUser.text : @"";
        NSString *pass = inputPass.text ? inputPass.text : @"";
        NSString *pass_repeat = reg ? (inputPassRepeat.text ? inputPassRepeat.text : @"") : nil;
        if ([user length] == 0) {
            showMessageBox(reg ? "Registration" : "Logging in", "Please enter a username");
        } else if ([pass length] == 0) {
            showMessageBox(reg ? "Registration" : "Logging in", "Please enter a password");
        } else if (reg && ![pass isEqualToString:pass_repeat]) {
            showMessageBox("Registration", "Passwords don't match");
        } else {
            [[authDialog getButton:0] setEnabled:NO];
            waiting = YES;
            sendAuthRequest(CSTR(user), CSTR(pass), reg);
        }

        setUser(user);
        setPass(pass);
        [authDialog endEditing:YES];
    }];
    [authDialog setButton:1 title:@"Cancel" handler:^{ [authDialog dismiss]; }];
    [authDialog show];
    if (waiting) [[authDialog getButton:0] setEnabled:NO];
}
