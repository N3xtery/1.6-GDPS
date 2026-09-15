package com.necytdamu.onesixgdps;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.text.InputType;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Button;
import android.widget.RadioGroup;
import android.widget.RadioButton;
import android.util.TypedValue;
import android.view.inputmethod.InputMethodManager;
import android.content.Context;
import android.widget.TableLayout;
import android.widget.TableRow;
import android.widget.ScrollView;
import android.graphics.drawable.GradientDrawable;
import android.graphics.Paint;
import android.view.ViewTreeObserver;

import java.io.File;
import java.util.Properties;
import java.io.IOException;
import java.io.FilenameFilter;
import java.util.regex.Pattern;
import java.util.Arrays;
import java.util.Comparator;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;

import org.jsoup.Jsoup;
import org.jsoup.nodes.Document;
import org.jsoup.nodes.Element;
import org.jsoup.select.Elements;

import com.mpatric.mp3agic.Mp3File;
import com.mpatric.mp3agic.ID3v1;
import com.mpatric.mp3agic.ID3v2;

import com.squareup.okhttp.OkHttpClient;

public class SongsDialog {
    private static AlertDialog loadingDlg;
    private static File songsDir;
    private static String menuSong;
    private static String practiceSong;
    private static String currentSongID;
    private static float currentSongOffset;

    public static native void updateMenuPractSongNative(boolean menu, String path, boolean updateMenuNow);
    public static native void updateSongsPath(String path);
    public static native void setCustomSong(String id);
    public static native void setOffset(float offset);
    public static native void playLevel();

    public static void setCurrentSongID(String currentSongID) { SongsDialog.currentSongID = currentSongID; }
    public static String getCurrentSongID() { return currentSongID; }
    public static void setCurrentSongOffset(float currentSongOffset) { SongsDialog.currentSongOffset = currentSongOffset; }
    public static void setSongVars(File songsDir, String menuSong, String practiceSong) {
        SongsDialog.songsDir = songsDir;
        SongsDialog.menuSong = menuSong;
        SongsDialog.practiceSong = practiceSong;
    }

    public static void buildOffsetDialog() {
        LinearLayout layout = new LinearLayout(OverlayUI.activity);
        layout.setOrientation(LinearLayout.VERTICAL);
        int pad = OverlayUI.dp(8);
        layout.setPadding(pad, pad, pad, pad);

        TextView labelOffset = new TextView(OverlayUI.activity);
        labelOffset.setText("Song offset:");
        labelOffset.setTextSize(12);
        layout.addView(labelOffset);

        final EditText inputOffset = new EditText(OverlayUI.activity);
        inputOffset.setInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL);
        inputOffset.setHint("Offset");
        inputOffset.setText(Float.toString(currentSongOffset));
        inputOffset.setGravity(Gravity.CENTER);
        layout.addView(inputOffset);

        AlertDialog.Builder builder = new AlertDialog.Builder(OverlayUI.activity);
        builder.setTitle("Set the song offset");
        builder.setView(layout);

        builder.setPositiveButton("OK", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                String offsetStr = inputOffset.getText().toString();
                if (offsetStr.isEmpty()) offsetStr = "0";
                currentSongOffset = Float.parseFloat(offsetStr);
                setOffset(currentSongOffset);
            }
        });
        builder.setCancelable(true);

        AlertDialog offsetDialog = builder.create();
        offsetDialog.show();
    }

    public static void buildSongsDialog(final boolean levelSettings) {
        LinearLayout layout = new LinearLayout(OverlayUI.activity);
        layout.setOrientation(LinearLayout.VERTICAL);
        int pad = OverlayUI.dp(8);
        layout.setPadding(pad, pad, pad, pad);

        LinearLayout pathRow = new LinearLayout(OverlayUI.activity);
        pathRow.setOrientation(LinearLayout.HORIZONTAL);
        pathRow.setGravity(Gravity.BOTTOM);
        LinearLayout.LayoutParams topParams = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        pathRow.setLayoutParams(topParams);

        final TableLayout table = new TableLayout(OverlayUI.activity);
        table.setLayoutParams(topParams);

        LinearLayout pathLabelInput = new LinearLayout(OverlayUI.activity);
        pathLabelInput.setOrientation(LinearLayout.VERTICAL);
        pathLabelInput.setLayoutParams(new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f));

        TextView labelPath = new TextView(OverlayUI.activity);
        labelPath.setText("Path:");
        labelPath.setTextSize(12);
        pathLabelInput.addView(labelPath);

        final EditText inputPath = new EditText(OverlayUI.activity);
        inputPath.setInputType(InputType.TYPE_CLASS_TEXT);
        inputPath.setHint("Path");
        inputPath.setText(songsDir.getAbsolutePath());
        inputPath.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        pathLabelInput.addView(inputPath);

        Button usePathBtn = new Button(OverlayUI.activity);
        usePathBtn.setText("Use");
        usePathBtn.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                String songsDirStr = inputPath.getText().toString();
                File dir = new File(songsDirStr);
                if (dir.isDirectory()) {
                    if (!dir.getAbsolutePath().equals(songsDir.getAbsolutePath())) {
                        songsDir = dir;
                        Properties props = OverlayUI.loadConfig();
                        props.setProperty("songsDir", songsDirStr);
                        OverlayUI.saveConfig(props);
                        songsTablePopulate(table, levelSettings);
                        updateSongsPath(songsDir.getAbsolutePath() + "/");
                    }
                } else OverlayUI.showMessageBox("Error", "Directory doesn't exist");
            }
        });

        Button defaultPathBtn = new Button(OverlayUI.activity);
        defaultPathBtn.setText("Default");
        defaultPathBtn.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                File mainDir = OverlayUI.activity.getFilesDir();
                File songsDirNew = new File(mainDir, "songs");
                if (!songsDirNew.getAbsolutePath().equals(songsDir.getAbsolutePath())) {
                    songsDir = songsDirNew;
                    Properties props = OverlayUI.loadConfig();
                    props.setProperty("songsDir", "");
                    OverlayUI.saveConfig(props);
                    songsTablePopulate(table, levelSettings);
                    updateSongsPath(songsDir.getAbsolutePath() + "/");
                }
                inputPath.setText(songsDir.getAbsolutePath());
            }
        });

        LinearLayout idLabelInput = new LinearLayout(OverlayUI.activity);
        idLabelInput.setOrientation(LinearLayout.VERTICAL);
        LinearLayout.LayoutParams idParams = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        idParams.setMargins(OverlayUI.dp(20), 0, 0, 0);
        idLabelInput.setLayoutParams(idParams);

        TextView labelID = new TextView(OverlayUI.activity);
        labelID.setText("Add a song (muzmo):");
        labelID.setTextSize(12);
        idLabelInput.addView(labelID);

        final EditText idInput = new EditText(OverlayUI.activity);
        idInput.setInputType(InputType.TYPE_CLASS_NUMBER);
        idInput.setHint("Song ID");
        idInput.setEms(6);
        idLabelInput.addView(idInput);

        Button findIDBtn = new Button(OverlayUI.activity);
        findIDBtn.setText("Find");
        findIDBtn.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                InputMethodManager imm = (InputMethodManager)OverlayUI.activity.getSystemService(Context.INPUT_METHOD_SERVICE);
                imm.hideSoftInputFromWindow(idInput.getWindowToken(), 0);
                loadingDlg = new AlertDialog.Builder(OverlayUI.activity).setTitle("Custom song").setMessage("Loading...").setPositiveButton("Cancel",
                    new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int which) {
                            loadingDlg = null;
                        }
                    }).show();
                findSongID(idInput.getText().toString(), table, levelSettings);
            }
        });

        pathRow.addView(pathLabelInput);
        pathRow.addView(usePathBtn);
        pathRow.addView(defaultPathBtn);
        pathRow.addView(idLabelInput);
        pathRow.addView(findIDBtn);

        ScrollView scrollView = new ScrollView(OverlayUI.activity);
        scrollView.setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.MATCH_PARENT));
        scrollView.addView(table);

        GradientDrawable border = new GradientDrawable();
        border.setColor(0x00000000); // transparent background
        border.setStroke(2, 0xFFFFFFFF); // 2px white stroke
        scrollView.setBackgroundDrawable(border);

        songsTablePopulate(table, levelSettings);

        layout.addView(pathRow);
        layout.addView(scrollView);

        AlertDialog.Builder builder = new AlertDialog.Builder(OverlayUI.activity);
        builder.setTitle("Custom songs list");
        builder.setView(layout);

        builder.setCancelable(true);

        AlertDialog songsDialog = builder.create();
        songsDialog.show();
    }

    public static void buildPlayLoadingDialog(final String id) {
        OverlayUI.activity.runOnUiThread(new Runnable() {
            public void run() {
                loadingDlg = new AlertDialog.Builder(OverlayUI.activity).setTitle("Custom song").setMessage("Loading...").setNegativeButton("Cancel",
                    new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int which) {
                            loadingDlg = null;
                        }
                    }).setPositiveButton("Play without music", new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int which) {
                            loadingDlg = null;
                            playLevel();
                        }
                    }).show();
                findSongID(id, null, false);
            }
        });
    }

    private static void findSongID(final String id, final TableLayout table, final boolean levelSettings) {
        if (id.isEmpty()) return;
        final String url = "https://en.muzmo.cc/info?id=" + id;
        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    OkHttpClient client = new OkHttpClient();
                    client.setSslSocketFactory(OverlayUI.sslContext.getSocketFactory());

                    URL targetUrl = new URL(url);
                    HttpURLConnection connection = client.open(targetUrl);
                    connection.setRequestMethod("GET");
                    connection.setRequestProperty("User-Agent", "Mozilla/5.0");
                    connection.setRequestProperty("Accept", "text/html,*/*");
                    connection.setConnectTimeout(15000);
                    connection.setReadTimeout(15000);

                    BufferedReader reader = new BufferedReader(new InputStreamReader(connection.getInputStream(), "UTF-8"));
                    StringBuilder htmlBuilder = new StringBuilder();
                    String line;
                    while ((line = reader.readLine()) != null) htmlBuilder.append(line).append("\n");
                    reader.close();

                    Document doc = Jsoup.parse(htmlBuilder.toString(), url);
                    if (loadingDlg == null) return;
                    loadingDlg.dismiss();
                    loadingDlg = null;

                    Element titleEl = doc.select("b").first();
                    if (titleEl != null && !titleEl.text().equals("Home")) {
                        final String title = titleEl.text();

                        Element currentDiv = titleEl.parent().nextElementSibling();
                        final Elements songVersions = new Elements();
                        songVersions.add(currentDiv.select("a").first());

                        while (!currentDiv.text().startsWith("Artist")) currentDiv = currentDiv.nextElementSibling();
                        final String artist = currentDiv.text().substring(currentDiv.text().indexOf(" ")).trim();

                        while (!currentDiv.hasClass("sub")) currentDiv = currentDiv.nextElementSibling();
                        if (currentDiv.text().equals("Generated MP3")) {
                            currentDiv = currentDiv.nextElementSibling();
                            while (true) {
                                currentDiv = currentDiv.nextElementSibling();
                                if (currentDiv.hasClass("sub")) break;
                                songVersions.add(currentDiv.select("a").first());
                            }
                        }

                        OverlayUI.activity.runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                final RadioGroup radioGroup = new RadioGroup(OverlayUI.activity);
                                radioGroup.setOrientation(RadioGroup.VERTICAL);
                                radioGroup.setLayoutParams(new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                                    ViewGroup.LayoutParams.WRAP_CONTENT));
                                int setDefaultChoice = 0;
                                for (int i = 0; i < songVersions.size(); i++) {
                                    RadioButton option = new RadioButton(OverlayUI.activity);
                                    String info = songVersions.get(i).select("span").first().text();
                                    option.setText(info);
                                    if (i > 0 && Integer.parseInt(info.substring(0, info.indexOf("Kbps")).trim()) >= 128) setDefaultChoice = i;
                                    option.setId(i);
                                    option.setTag("https://en.muzmo.cc" + songVersions.get(i).attr("href"));
                                    radioGroup.addView(option);
                                }
                                radioGroup.check(setDefaultChoice);
                                AlertDialog.Builder builder = new AlertDialog.Builder(OverlayUI.activity);
                                builder.setTitle(artist + " — " + title);
                                builder.setView(radioGroup);

                                builder.setPositiveButton("Download", new DialogInterface.OnClickListener() {
                                    public void onClick(DialogInterface dialog, int which) {
                                        RadioButton selectedRadioButton = (RadioButton)radioGroup.findViewById(radioGroup.getCheckedRadioButtonId());
                                        new FileDownloadTask(OverlayUI.activity, new File(songsDir, id + ".mp3"), table,
                                            levelSettings).execute(selectedRadioButton.getTag().toString());
                                    }
                                });
                                if (table == null) {
                                    builder.setNeutralButton("Play without music", new DialogInterface.OnClickListener() {
                                        public void onClick(DialogInterface dialog, int which) {
                                            playLevel();
                                        }
                                    });
                                }
                                builder.setNegativeButton("Cancel", null);
                                builder.setCancelable(true);

                                AlertDialog songDialog = builder.create();
                                songDialog.show();
                            }
                        });
                    }
                } catch (Exception e) {
                    e.printStackTrace();
                    if (loadingDlg != null) {
                        OverlayUI.activity.runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                if (table != null) {
                                    loadingDlg.dismiss();
                                    loadingDlg = null;
                                }
                                OverlayUI.showMessageBox("Error", "Song not found");
                            }
                        });
                    }
                }
            }
        }).start();
    }

    private static String updateMenuPractSong(TableLayout table, Button btn, String songGlobal, String songProperties, String id, String name, int pos) {
        if (id.equals(songGlobal)) {
            songGlobal = "";
            btn.setText(name + ": off");
        } else {
            if (!songGlobal.isEmpty()) for (int i = 1; i < table.getChildCount(); i++) {
                TableRow row = (TableRow)table.getChildAt(i);
                String rowId = ((TextView)row.getChildAt(0)).getText().toString();
                if (rowId.equals(songGlobal)) {
                    Button oldBtn = (Button)(row.getChildAt(pos));
                    oldBtn.setText(name + ": off");
                }
            }
            songGlobal = id;
            btn.setText(name + ": on");
        }
        Properties props = OverlayUI.loadConfig();
        props.setProperty(songProperties, songGlobal);
        OverlayUI.saveConfig(props);
        return songGlobal;
    }

    public static void songsTableAddRow(final TableLayout table, final File file, boolean toBeginning, boolean levelSettings) {
        String artist = "Unknown";
        String title = "Unknown";
        int bitrate = 0;
        try {
            Mp3File mp3File = new Mp3File(file.getAbsolutePath());
            bitrate = mp3File.getBitrate();
            if (mp3File.hasId3v2Tag()) {
                ID3v2 tag = mp3File.getId3v2Tag();
                artist = tag.getArtist();
                title = tag.getTitle();
            } else if (mp3File.hasId3v1Tag()) {
                ID3v1 tag = mp3File.getId3v1Tag();
                artist = tag.getArtist();
                title = tag.getTitle();
            }
        } catch (Exception ignored) {}

        final TableRow row = new TableRow(OverlayUI.activity);
        row.setBaselineAligned(false);
        row.setGravity(Gravity.CENTER_VERTICAL);
        final String id = file.getName().replace(".mp3", "");
        row.addView(makeCell(id));
        row.addView(makeCell(artist + " — " + title));
        row.addView(makeCell(Integer.toString(bitrate)));
        row.addView(makeCell(String.format("%.2f MB", file.length() / (1024.0 * 1024.0))));

        if (levelSettings) {
            final Button useBtn = new Button(OverlayUI.activity);
            useBtn.setText("Use");
            if (id.equals(currentSongID)) useBtn.setEnabled(false);
            useBtn.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    if (!currentSongID.isEmpty()) for (int i = 1; i < table.getChildCount(); i++) {
                        TableRow row = (TableRow)table.getChildAt(i);
                        String rowId = ((TextView)row.getChildAt(0)).getText().toString();
                        if (rowId.equals(currentSongID)) {
                            Button oldBtn = (Button)(row.getChildAt(4));
                            oldBtn.setEnabled(true);
                        }
                    }
                    currentSongID = id;
                    setCustomSong(id);
                    useBtn.setEnabled(false);
                    OverlayUI.setOrigSongBtnVisible();
                }
            });
            row.addView(useBtn);
        } else {
            final Button menuBtn = new Button(OverlayUI.activity);
            if (id.equals(menuSong)) menuBtn.setText("Menu: on");
            else menuBtn.setText("Menu: off");
            menuBtn.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    menuSong = updateMenuPractSong(table, menuBtn, menuSong, "menuSong", id, "Menu", 4);
                    File songFile = new File(songsDir, menuSong + ".mp3");
                    updateMenuPractSongNative(true, menuSong.isEmpty() ? "" : songFile.getAbsolutePath(), true);
                }
            });
            row.addView(menuBtn);

            final Button practBtn = new Button(OverlayUI.activity);
            if (id.equals(practiceSong)) practBtn.setText("Pract: on");
            else practBtn.setText("Pract: off");
            practBtn.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    practiceSong = updateMenuPractSong(table, practBtn, practiceSong, "practiceSong", id, "Pract", 5);
                    File songFile = new File(songsDir, practiceSong + ".mp3");
                    updateMenuPractSongNative(false, songFile.getAbsolutePath(), false);
                }
            });
            row.addView(practBtn);
        }

        Button removeBtn = new Button(OverlayUI.activity);
        removeBtn.setText(" – ");
        removeBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (id.equals(menuSong)) updateMenuPractSongNative(true, "", true);
                if (id.equals(practiceSong)) updateMenuPractSongNative(false, "", false);
                boolean deleted = file.delete();
                if (deleted) {
                    table.removeView(row);
                }
            }
        });
        row.addView(removeBtn);

        if (toBeginning) table.addView(row, 1);
        else table.addView(row);
    }

    private static void songsTablePopulate(final TableLayout table, boolean levelSettings) {
        table.removeAllViews();
        TableRow header = new TableRow(OverlayUI.activity);
        header.addView(makeHeaderCell("ID"));
        header.addView(makeHeaderCell("Name"));
        header.addView(makeHeaderCell("Bitrate"));
        header.addView(makeHeaderCell("Size"));
        header.addView(makeHeaderCell(""));
        if (!levelSettings) header.addView(makeHeaderCell(""));
        header.addView(makeHeaderCell(""));
        table.addView(header);

        table.setColumnStretchable(1, true);
        table.setColumnShrinkable(1, true);

        table.getViewTreeObserver().addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
            @Override
            public void onGlobalLayout() {
                for (int i = 0; i < table.getChildCount(); i++) {
                    TableRow tableRow = (TableRow)table.getChildAt(i);
                    TextView tv = (TextView)tableRow.getChildAt(1);
                    int availableWidth = tv.getWidth() - tv.getPaddingLeft() - tv.getPaddingRight();
                    if (availableWidth <= 0) return;

                    Paint paint = new Paint(tv.getPaint());
                    float textSizePx = tv.getTextSize();
                    String text = tv.getText().toString();
                    float measured = paint.measureText(text);
                    while (measured > availableWidth && textSizePx > 8 * tv.getResources().getDisplayMetrics().scaledDensity) {
                        textSizePx -= 1f;
                        paint.setTextSize(textSizePx);
                        measured = paint.measureText(text);
                    }
                    tv.setTextSize(TypedValue.COMPLEX_UNIT_PX, textSizePx);
                }
            }
        });

        final Pattern DIGITS_ONLY = Pattern.compile("^\\d+\\.mp3$");
        File[] files = songsDir.listFiles(new FilenameFilter() {
            @Override
            public boolean accept(File dir, String name) {
                return DIGITS_ONLY.matcher(name).matches();
            }
        });
        if (files != null) {
            Arrays.sort(files, new Comparator<File>() { // sort by last modified
                @Override
                public int compare(File f1, File f2) {
                    long diff = f2.lastModified() - f1.lastModified();
                    if (diff > 0) return 1;
                    if (diff < 0) return -1;
                    return 0;
                }
            });
        }

        for (File file : files) {
            songsTableAddRow(table, file, false, levelSettings);
        }
    }

    private static TextView makeHeaderCell(String text) {
        TextView textView = makeCell(text);
        textView.getPaint().setFakeBoldText(true);
        return textView;
    }

    private static TextView makeCell(String text) {
        TextView textView = new TextView(OverlayUI.activity);
        textView.setText(text);
        int padding = OverlayUI.dp(6);
        textView.setPadding(padding, padding, padding, padding);
        textView.setGravity(Gravity.CENTER);
        return textView;
    }
}
