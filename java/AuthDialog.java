package com.necytdamu.onesixgdps;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.text.InputType;
import android.view.View;
import android.text.InputFilter;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;

public class AuthDialog {
    private static AlertDialog authDialog;

    private static native void sendAuthRequest(String user, String pass, boolean register);
    public static void dismiss() { if (authDialog != null) authDialog.dismiss(); }
    private static boolean waiting;
    public static void activateSubmitBtn() {
        waiting = false;
        if (authDialog != null) authDialog.getButton(AlertDialog.BUTTON_POSITIVE).setEnabled(true);
    }

    public static void buildAuthDialog(final boolean register) {
        LinearLayout layout = new LinearLayout(OverlayUI.activity);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setMinimumWidth(OverlayUI.dp(600));
        int pad = OverlayUI.dp(8);
        layout.setPadding(pad, pad, pad, pad);

        TextView labelUser = new TextView(OverlayUI.activity);
        labelUser.setText("Username:");
        labelUser.setTextSize(12);
        layout.addView(labelUser);

        final EditText inputUser = new EditText(OverlayUI.activity);
        inputUser.setInputType(InputType.TYPE_CLASS_TEXT);
        inputUser.setHint("Username");
        inputUser.setFilters(new InputFilter[] { new InputFilter.LengthFilter(20) });
        layout.addView(inputUser);

        LinearLayout passRow = new LinearLayout(OverlayUI.activity);
        passRow.setOrientation(LinearLayout.HORIZONTAL);
        passRow.setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT));

        LinearLayout passLabelInput = new LinearLayout(OverlayUI.activity);
        passLabelInput.setOrientation(LinearLayout.VERTICAL);
        passLabelInput.setLayoutParams(new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f));

        TextView labelPass = new TextView(OverlayUI.activity);
        labelPass.setText("Password:");
        labelPass.setTextSize(12);
        labelPass.setPadding(0, OverlayUI.dp(6), 0, 0);
        passLabelInput.addView(labelPass);

        final EditText inputPass = new EditText(OverlayUI.activity);
        inputPass.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_PASSWORD);
        inputPass.setHint("Password");
        inputPass.setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT));
        passLabelInput.addView(inputPass);

        passRow.addView(passLabelInput);

        final EditText inputPassRepeat = new EditText(OverlayUI.activity);
        if (register) {
            LinearLayout repeatLabelInput = new LinearLayout(OverlayUI.activity);
            repeatLabelInput.setOrientation(LinearLayout.VERTICAL);
            repeatLabelInput.setLayoutParams(new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f));

            TextView labelPassRepeat = new TextView(OverlayUI.activity);
            labelPassRepeat.setText("Confirm Password:");
            labelPassRepeat.setTextSize(12);
            labelPassRepeat.setPadding(0, OverlayUI.dp(6), 0, 0);
            repeatLabelInput.addView(labelPassRepeat);

            inputPassRepeat.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_PASSWORD);
            inputPassRepeat.setHint("Confirm Password");
            inputPassRepeat.setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT));
            repeatLabelInput.addView(inputPassRepeat);

            passRow.addView(repeatLabelInput);
        }

        layout.addView(passRow);

        AlertDialog.Builder builder = new AlertDialog.Builder(OverlayUI.activity);
        builder.setTitle(register ? "Register" : "Log in");
        builder.setView(layout);

        builder.setPositiveButton("Submit", null);
        builder.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                dialog.dismiss();
            }
        });
        builder.setCancelable(true);

        authDialog = builder.create();
        authDialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialogInterface) {
                authDialog = null;
            }
        });
        if (waiting) authDialog.getButton(AlertDialog.BUTTON_POSITIVE).setEnabled(false);
        authDialog.show();

        authDialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                String user = inputUser.getText().toString();
                String pass = inputPass.getText().toString();
                String pass_repeat = register ? inputPassRepeat.getText().toString() : null;
                if (user.isEmpty()) OverlayUI.showMessageBox(register ? "Registration" : "Logging in", "Please enter a username");
                else if (pass.isEmpty()) OverlayUI.showMessageBox(register ? "Registration" : "Logging in", "Please enter a password");
                else if (register && !pass.equals(pass_repeat)) OverlayUI.showMessageBox("Registration", "Passwords don't match");
                else {
                    authDialog.getButton(AlertDialog.BUTTON_POSITIVE).setEnabled(false);
                    waiting = true;
                    sendAuthRequest(user, pass, register);
                }

                OverlayUI.setUser(user);
                OverlayUI.setPass(pass);
            }
        });
    }
}
