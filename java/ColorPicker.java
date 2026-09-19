package com.necytdamu.onesixgdps;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.text.InputType;
import android.view.Gravity;
import android.text.InputFilter;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;

class ColorPicker {
    private static int red;
    private static int green;
    private static int blue;

    public static native void setColorNative(int r, int g, int b);
    public static void setColor(int r, int g, int b) { red = r; green = g; blue = b; }

    public static void buildColorDialog() {
        LinearLayout row = new LinearLayout(OverlayUI.activity);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT));
        int pad = OverlayUI.dp(8);
        row.setPadding(pad, pad, pad, pad);

        LinearLayout rLabelInput = new LinearLayout(OverlayUI.activity);
        rLabelInput.setOrientation(LinearLayout.VERTICAL);
        rLabelInput.setLayoutParams(new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f));

        TextView labelR = new TextView(OverlayUI.activity);
        labelR.setText("Red");
        labelR.setTextSize(12);
        labelR.setPadding(0, OverlayUI.dp(6), 0, 0);
        labelR.setGravity(Gravity.CENTER);
        rLabelInput.addView(labelR);

        final EditText inputR = new EditText(OverlayUI.activity);
        inputR.setInputType(InputType.TYPE_CLASS_NUMBER);
        inputR.setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT));
        inputR.setFilters(new InputFilter[] { new InputFilter.LengthFilter(3) });
        inputR.setText(Integer.toString(red));
        inputR.setGravity(Gravity.CENTER);
        rLabelInput.addView(inputR);

        LinearLayout gLabelInput = new LinearLayout(OverlayUI.activity);
        gLabelInput.setOrientation(LinearLayout.VERTICAL);
        gLabelInput.setLayoutParams(new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f));

        TextView labelG = new TextView(OverlayUI.activity);
        labelG.setText("Green");
        labelG.setTextSize(12);
        labelG.setPadding(0, OverlayUI.dp(6), 0, 0);
        labelG.setGravity(Gravity.CENTER);
        gLabelInput.addView(labelG);

        final EditText inputG = new EditText(OverlayUI.activity);
        inputG.setInputType(InputType.TYPE_CLASS_NUMBER);
        inputG.setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT));
        inputG.setFilters(new InputFilter[] { new InputFilter.LengthFilter(3) });
        inputG.setText(Integer.toString(green));
        inputG.setGravity(Gravity.CENTER);
        gLabelInput.addView(inputG);

        LinearLayout bLabelInput = new LinearLayout(OverlayUI.activity);
        bLabelInput.setOrientation(LinearLayout.VERTICAL);
        bLabelInput.setLayoutParams(new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f));

        TextView labelB = new TextView(OverlayUI.activity);
        labelB.setText("Blue");
        labelB.setTextSize(12);
        labelB.setPadding(0, OverlayUI.dp(6), 0, 0);
        labelB.setGravity(Gravity.CENTER);
        bLabelInput.addView(labelB);

        final EditText inputB = new EditText(OverlayUI.activity);
        inputB.setInputType(InputType.TYPE_CLASS_NUMBER);
        inputB.setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT));
        inputB.setFilters(new InputFilter[] { new InputFilter.LengthFilter(3) });
        inputB.setText(Integer.toString(blue));
        inputB.setGravity(Gravity.CENTER);
        bLabelInput.addView(inputB);

        row.addView(rLabelInput);
        row.addView(gLabelInput);
        row.addView(bLabelInput);

        AlertDialog.Builder builder = new AlertDialog.Builder(OverlayUI.activity);
        builder.setTitle("Set the color");
        builder.setView(row);

        builder.setPositiveButton("OK", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                red = 0;
                green = 0;
                blue = 0;
                String rStr = inputR.getText().toString();
                if (!rStr.isEmpty()) red = Integer.parseInt(rStr);
                String gStr = inputG.getText().toString();
                if (!gStr.isEmpty()) green = Integer.parseInt(gStr);
                String bStr = inputB.getText().toString();
                if (!bStr.isEmpty()) blue = Integer.parseInt(bStr);
                if (red > 255) red = 255;
                if (green > 255) green = 255;
                if (blue > 255) blue = 255;

                setColorNative(red, green, blue);
            }
        });
        builder.setNegativeButton("Cancel", null);
        builder.setCancelable(true);

        AlertDialog offsetDialog = builder.create();
        offsetDialog.show();
    }
}
