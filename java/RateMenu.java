package com.necytdamu.onesixgdps;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.view.Gravity;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.RadioButton;
import android.widget.CompoundButton;
import android.widget.CheckBox;

public class RateMenu {
    public static int stars, starsDlg;
    public static boolean featured, featuredDlg;

    public static native void sendRateRequest(String user, String pass, int stars, boolean featured);

    public static void setStarsFeatured(int stars, boolean featured) {
        RateMenu.stars = stars;
        RateMenu.featured = featured;
    }

    public static void buildRateDialog() {
        starsDlg = stars;
        featuredDlg = featured;

        LinearLayout layout = new LinearLayout(OverlayUI.activity);
        if (OverlayUI.activity.getResources().getDisplayMetrics().widthPixels / OverlayUI.activity.getResources().getDisplayMetrics().density >= 640) {
            layout.setMinimumWidth(OverlayUI.dp(600));
            layout.setOrientation(LinearLayout.HORIZONTAL);
        } else layout.setOrientation(LinearLayout.VERTICAL);
        int pad = OverlayUI.dp(8);
        layout.setPadding(pad, pad, pad, pad);
        layout.setGravity(Gravity.CENTER);

        LinearLayout radioGroup = new LinearLayout(OverlayUI.activity);
        radioGroup.setOrientation(LinearLayout.HORIZONTAL);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        params.gravity = Gravity.CENTER;
        radioGroup.setLayoutParams(params);

        LinearLayout radioGroup2 = new LinearLayout(OverlayUI.activity);
        radioGroup2.setOrientation(LinearLayout.HORIZONTAL);
        radioGroup2.setLayoutParams(params);

        final RadioButton[] radioButtons = new RadioButton[11];
        for (int i = 0; i < 12; i++) {
            LinearLayout optionLayout = new LinearLayout(OverlayUI.activity);
            optionLayout.setOrientation(LinearLayout.VERTICAL);
            optionLayout.setGravity(Gravity.CENTER_HORIZONTAL);
            optionLayout.setPadding(OverlayUI.dp(4), 0, OverlayUI.dp(4), 0);

            if (i == 11) {
                CheckBox checkBox = new CheckBox(OverlayUI.activity);
                checkBox.setLayoutParams(params);
                checkBox.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
                    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                        featuredDlg = isChecked;
                    }
                });
                checkBox.setChecked(featured);
                optionLayout.addView(checkBox);
            } else {
                RadioButton radioButton = new RadioButton(OverlayUI.activity);
                radioButton.setId(i);
                radioButtons[i] = radioButton;
                final int index = i;
                radioButtons[i].setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
                    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                        if (isChecked) {
                            for (int j = 0; j < 11; j++) if (j != index) radioButtons[j].setChecked(false);
                            starsDlg = index;
                        }
                    }
                });
                optionLayout.addView(radioButton);
            }

            TextView label = new TextView(OverlayUI.activity);
            label.setText(i == 11 ? "Featured" : Integer.toString(i));
            label.setGravity(Gravity.CENTER_HORIZONTAL);
            if (i == 11) label.setLayoutParams(params);

            optionLayout.addView(label);
            if (i < 6) radioGroup.addView(optionLayout);
            else radioGroup2.addView(optionLayout);
        }

        radioButtons[stars].setChecked(true);
        layout.addView(radioGroup);
        layout.addView(radioGroup2);

        AlertDialog.Builder builder = new AlertDialog.Builder(OverlayUI.activity);
        builder.setTitle("Rate Level");
        builder.setView(layout);

        builder.setPositiveButton("Rate", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                OverlayUI.disableRateBtn();
                sendRateRequest(OverlayUI.getUser(), OverlayUI.getPass(), starsDlg, featuredDlg);
            }
        });
        builder.setNegativeButton("Cancel", null);
        builder.setCancelable(true);

        AlertDialog rateDialog = builder.create();
        rateDialog.show();
    }
}
