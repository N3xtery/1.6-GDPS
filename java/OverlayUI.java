package com.necytdamu.onesixgdps;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.Button;
import android.widget.FrameLayout;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.util.Properties;
import java.io.IOException;
import java.io.InputStream;
import java.security.KeyStore;
import java.security.SecureRandom;
import java.security.Security;
import java.security.cert.Certificate;
import java.security.cert.X509Certificate;
import java.security.cert.CertificateException;
import java.util.Enumeration;
import java.util.List;
import java.util.ArrayList;

import javax.net.ssl.SSLContext;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509TrustManager;

public class OverlayUI {
    private static File configFile;
    private static String user;
    private static String pass;
    private static boolean logged_in = false;
    private static Button registerBtn, loginBtn, logoutBtn, saveBtn, loadBtn, reqBtn, percentBtn, songsBtn;
    private static Button copyBtn, rateBtn;
    private static Button offsetBtn, origSongBtn, customSongBtn;
    private static boolean moderator = false;
    private static int percentageType = 0;
    public static Activity activity;
    public static SSLContext sslContext;

    public static native void sendSaveRequest(String user, String pass);
    public static native void sendLoadRequest(String user, String pass);
    public static native void sendReqRequest(String user, String pass);
    public static native void copyLevel();
    public static native void updatePercentageNative(int percentageType);

    public static void setUser(String user) { OverlayUI.user = user; }
    public static String getUser() { return user; }
    public static void setPass(String pass) { OverlayUI.pass = pass; }
    public static String getPass() { return pass; }

    public static int dp(int value) {
        float density = activity.getResources().getDisplayMetrics().density;
        return (int)(value * density + 0.5f);
    }

    public static Properties loadConfig() {
        try {
            Properties props = new Properties();
            FileInputStream in = new FileInputStream(configFile);
            props.load(in);
            in.close();
            return props;
        } catch (IOException e) {
            e.printStackTrace();
            return null;
        }
    }

    public static void saveConfig(Properties props) {
        try {
            FileOutputStream out = new FileOutputStream(configFile);
            props.store(out, null);
            out.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private static void updatePercentage() {
        if (percentageType == 1) percentBtn.setText("X%");
        else if (percentageType == 2) percentBtn.setText("X.X%");
        else if (percentageType == 3) percentBtn.setText("X.XX%");
        else percentBtn.setText("%: off");
        Properties props = loadConfig();
        props.setProperty("percentage", Integer.toString(percentageType));
        saveConfig(props);
        updatePercentageNative(percentageType);
    }

    public static void onStart(final Activity activity) {
        OverlayUI.activity = activity;
        activity.runOnUiThread(new Runnable() {
            public void run() {
                try {
                    Security.addProvider(new org.spongycastle.jce.provider.BouncyCastleProvider());
                    Security.addProvider(new org.spongycastle.jsse.provider.BouncyCastleJsseProvider());

                    KeyStore trustStore = KeyStore.getInstance("BKS", "SC");
                    InputStream trustStoreStream = activity.getAssets().open("isrgrootx.bks");
                    trustStore.load(trustStoreStream, "123456".toCharArray());
                    trustStoreStream.close();

                    final List<Certificate> certs = new ArrayList<Certificate>();
                    Enumeration<String> aliases = trustStore.aliases();
                    while (aliases.hasMoreElements()) certs.add(trustStore.getCertificate(aliases.nextElement()));

                    TrustManager[] trustManager = new TrustManager[]{ new X509TrustManager() {
                            public void checkClientTrusted(X509Certificate[] chain, String authType) {}
                            public void checkServerTrusted(X509Certificate[] chain, String authType) throws CertificateException {
                                if (chain == null || chain.length == 0) throw new CertificateException("Empty server certificate chain");
                                for (X509Certificate serverCert : chain) {
                                    for (Certificate cert : certs) {
                                        try {
                                            serverCert.verify(cert.getPublicKey());
                                            return;
                                        } catch (Exception ignored) {}
                                    }
                                }
                                throw new CertificateException("Server chain does not verify against certs");
                            }
                            public X509Certificate[] getAcceptedIssuers() { return new X509Certificate[0]; }
                        }
                    };

                    sslContext = SSLContext.getInstance("TLS", "SCJSSE");
                    sslContext.init(null, trustManager, new SecureRandom());
                } catch (Exception e) {
                    e.printStackTrace();
                }

                File mainDir = activity.getFilesDir();
                configFile = new File(mainDir, "modData.properties");
                File songsDir = null;
                String menuSong = "";
                String practiceSong = "";
                if (configFile.exists()) {
                    Properties props = loadConfig();
                    user = props.getProperty("username");
                    pass = props.getProperty("password");
                    if (!user.isEmpty() && !pass.isEmpty()) logged_in = true;
                    if (props.getProperty("moderator").equals("1")) moderator = true;
                    percentageType = Integer.parseInt(props.getProperty("percentage"));
                    String songsDirStr = props.getProperty("songsDir");
                    if (!songsDirStr.isEmpty()) {
                        songsDir = new File(songsDirStr);
                        if (!songsDir.isDirectory()) songsDir = null;
                    }
                    menuSong = props.getProperty("menuSong");
                    practiceSong = props.getProperty("practiceSong");
                } else {
                    Properties props = new Properties();
                    props.setProperty("username", "");
                    props.setProperty("password", "");
                    props.setProperty("moderator", "0");
                    props.setProperty("percentage", "0");
                    props.setProperty("songsDir", "");
                    props.setProperty("menuSong", "");
                    props.setProperty("practiceSong", "");
                    saveConfig(props);
                }
                if (songsDir == null) {
                    songsDir = new File(mainDir, "songs");
                    if (!songsDir.exists()) songsDir.mkdir();
                }
                File menuSongFile = new File(songsDir, menuSong + ".mp3");
                if (menuSongFile.exists()) SongsDialog.updateMenuPractSongNative(true, menuSongFile.getAbsolutePath(), false);
                File practSongFile = new File(songsDir, practiceSong + ".mp3");
                if (practSongFile.exists()) SongsDialog.updateMenuPractSongNative(false, practSongFile.getAbsolutePath(), false);
                SongsDialog.updateSongsPath(songsDir.getAbsolutePath() + "/");
                SongsDialog.setSongVars(songsDir, menuSong, practiceSong);

                registerBtn = new Button(activity);
                registerBtn.setText("Register");
                loginBtn = new Button(activity);
                loginBtn.setText("Log in");
                percentBtn = new Button(activity);
                updatePercentage();
                songsBtn = new Button(activity);
                songsBtn.setText("Songs");
                reqBtn = new Button(activity);
                reqBtn.setText("Req");
                saveBtn = new Button(activity);
                saveBtn.setText("Save");
                loadBtn = new Button(activity);
                loadBtn.setText("Load");
                logoutBtn = new Button(activity);
                logoutBtn.setText("Log out");

                offsetBtn = new Button(activity);
                offsetBtn.setText("Set an offset");
                origSongBtn = new Button(activity);
                origSongBtn.setText("Original song");
                customSongBtn = new Button(activity);
                customSongBtn.setText("Custom song");

                LinearLayout optionsRow = new LinearLayout(activity);
                optionsRow.setOrientation(LinearLayout.HORIZONTAL);
                FrameLayout.LayoutParams topRightParams = new FrameLayout.LayoutParams(FrameLayout.LayoutParams.WRAP_CONTENT,
                    FrameLayout.LayoutParams.WRAP_CONTENT);
                topRightParams.gravity = Gravity.TOP | Gravity.RIGHT;
                optionsRow.setLayoutParams(topRightParams);

                LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT,
                    LinearLayout.LayoutParams.WRAP_CONTENT);
                lp.rightMargin = dp(-3);
                lp.leftMargin = dp(-3);
                loginBtn.setLayoutParams(lp);
                registerBtn.setLayoutParams(lp);
                percentBtn.setLayoutParams(lp);
                songsBtn.setLayoutParams(lp);
                reqBtn.setLayoutParams(lp);
                saveBtn.setLayoutParams(lp);
                loadBtn.setLayoutParams(lp);
                logoutBtn.setLayoutParams(lp);
                offsetBtn.setLayoutParams(lp);
                origSongBtn.setLayoutParams(lp);
                customSongBtn.setLayoutParams(lp);

                registerBtn.setOnClickListener(new View.OnClickListener() {
                    public void onClick(View view) {
                        AuthDialog.buildAuthDialog(true);
                    }
                });
                loginBtn.setOnClickListener(new View.OnClickListener() {
                    public void onClick(View view) {
                        AuthDialog.buildAuthDialog(false);
                    }
                });
                percentBtn.setOnClickListener(new View.OnClickListener() {
                    public void onClick(View view) {
                        percentageType++;
                        if (percentageType > 3) percentageType = 0;
                        updatePercentage();
                    }
                });
                songsBtn.setOnClickListener(new View.OnClickListener() {
                    public void onClick(View view) {
                        SongsDialog.buildSongsDialog(false);
                    }
                });
                reqBtn.setOnClickListener(new View.OnClickListener() {
                    public void onClick(View view) {
                        reqBtn.setEnabled(false);
                        sendReqRequest(user, pass);
                    }
                });
                saveBtn.setOnClickListener(new View.OnClickListener() {
                    public void onClick(View view) {
                        showConfirmation("Are you sure you want to save your data?");
                    }
                });
                loadBtn.setOnClickListener(new View.OnClickListener() {
                    public void onClick(View view) {
                        showConfirmation("Are you sure you want to load your data?");
                    }
                });
                logoutBtn.setOnClickListener(new View.OnClickListener() {
                    public void onClick(View view) {
                        showConfirmation("Are you sure you want to log out?");
                    }
                });
                offsetBtn.setOnClickListener(new View.OnClickListener() {
                    public void onClick(View view) {
                        SongsDialog.buildOffsetDialog();
                    }
                });
                origSongBtn.setOnClickListener(new View.OnClickListener() {
                    public void onClick(View view) {
                        SongsDialog.setCustomSong("");
                        SongsDialog.setCurrentSongID("");
                        origSongBtn.setVisibility(View.GONE);
                    }
                });
                customSongBtn.setOnClickListener(new View.OnClickListener() {
                    public void onClick(View view) {
                        SongsDialog.buildSongsDialog(true);
                    }
                });
                registerBtn.setVisibility(View.GONE);
                loginBtn.setVisibility(View.GONE);
                percentBtn.setVisibility(View.GONE);
                songsBtn.setVisibility(View.GONE);
                reqBtn.setVisibility(View.GONE);
                saveBtn.setVisibility(View.GONE);
                loadBtn.setVisibility(View.GONE);
                logoutBtn.setVisibility(View.GONE);
                offsetBtn.setVisibility(View.GONE);
                origSongBtn.setVisibility(View.GONE);
                customSongBtn.setVisibility(View.GONE);

                ViewGroup content = (ViewGroup)activity.findViewById(android.R.id.content);
                optionsRow.addView(percentBtn);
                optionsRow.addView(songsBtn);
                optionsRow.addView(reqBtn);
                optionsRow.addView(saveBtn);
                optionsRow.addView(loadBtn);
                optionsRow.addView(logoutBtn);
                optionsRow.addView(registerBtn);
                optionsRow.addView(loginBtn);
                optionsRow.addView(offsetBtn);
                optionsRow.addView(origSongBtn);
                optionsRow.addView(customSongBtn);
                content.addView(optionsRow);

                copyBtn = new Button(activity);
                copyBtn.setText("Copy");
                rateBtn = new Button(activity);
                rateBtn.setText("Rate");

                LinearLayout levelMenuRow = new LinearLayout(activity);
                levelMenuRow.setOrientation(LinearLayout.VERTICAL);
                FrameLayout.LayoutParams levelMenuParams = new FrameLayout.LayoutParams(FrameLayout.LayoutParams.WRAP_CONTENT,
                    FrameLayout.LayoutParams.WRAP_CONTENT);
                levelMenuParams.gravity = Gravity.CENTER | Gravity.LEFT;
                levelMenuRow.setLayoutParams(levelMenuParams);

                copyBtn.setOnClickListener(new View.OnClickListener() {
                    public void onClick(View view) {
                        copyLevel();
                    }
                });
                rateBtn.setOnClickListener(new View.OnClickListener() {
                    public void onClick(View view) {
                        RateMenu.buildRateDialog();
                    }
                });
                copyBtn.setVisibility(View.GONE);
                rateBtn.setVisibility(View.GONE);

                levelMenuRow.addView(copyBtn);
                levelMenuRow.addView(rateBtn);
                content.addView(levelMenuRow);
            }
        });
    }

    public static void setButtonVisible(final String button, final boolean visible) {
        if (button.equals("Options")) {
            activity.runOnUiThread(new Runnable() {
                public void run() {
                    percentBtn.setVisibility(visible ? View.VISIBLE : View.GONE);
                    songsBtn.setVisibility(visible ? View.VISIBLE : View.GONE);
                    if (logged_in) {
                        if (!moderator) reqBtn.setVisibility(visible ? View.VISIBLE : View.GONE);
                        saveBtn.setVisibility(visible ? View.VISIBLE : View.GONE);
                        loadBtn.setVisibility(visible ? View.VISIBLE : View.GONE);
                        logoutBtn.setVisibility(visible ? View.VISIBLE : View.GONE);
                    } else {
                        registerBtn.setVisibility(visible ? View.VISIBLE : View.GONE);
                        loginBtn.setVisibility(visible ? View.VISIBLE : View.GONE);
                    }
                }
            });
        } else if (button.equals("LevelMenu")) {
            activity.runOnUiThread(new Runnable() {
                public void run() {
                    copyBtn.setVisibility(visible ? View.VISIBLE : View.GONE);
                    if (moderator) rateBtn.setVisibility(visible ? View.VISIBLE : View.GONE);
                }
            });
        } else if (button.equals("LevelSettings")) {
            activity.runOnUiThread(new Runnable() {
                public void run() {
                    offsetBtn.setVisibility(visible ? View.VISIBLE : View.GONE);
                    if (!SongsDialog.getCurrentSongID().isEmpty()) origSongBtn.setVisibility(visible ? View.VISIBLE : View.GONE);
                    customSongBtn.setVisibility(visible ? View.VISIBLE : View.GONE);
                }
            });
        }
    }

    public static void setOriginalText(final int orig) {
        activity.runOnUiThread(new Runnable() {
            public void run() {
                if (orig == 0) copyBtn.setText("Copy");
                else copyBtn.setText(String.format("Copy (orig: %d)", orig));
            }
        });
    }

    public static void showConfirmation(final String message) {
        activity.runOnUiThread(new Runnable() {
            public void run() {
                AlertDialog.Builder builder = new AlertDialog.Builder(activity);
                builder.setTitle("Confirm");
                builder.setMessage(message);

                builder.setPositiveButton("Yes", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        if (message.contains("log out")) {
                            logged_in = false;
                            moderator = false;
                            reqBtn.setVisibility(View.GONE);
                            saveBtn.setVisibility(View.GONE);
                            loadBtn.setVisibility(View.GONE);
                            logoutBtn.setVisibility(View.GONE);
                            registerBtn.setVisibility(View.VISIBLE);
                            loginBtn.setVisibility(View.VISIBLE);
                            Properties props = loadConfig();
                            props.setProperty("username", "");
                            props.setProperty("password", "");
                            props.setProperty("moderator", "0");
                            saveConfig(props);
                        } else if (message.contains("save")) {
                            saveBtn.setEnabled(false);
                            sendSaveRequest(user, pass);
                        } else if (message.contains("load")) {
                            loadBtn.setEnabled(false);
                            sendLoadRequest(user, pass);
                        }
                        dialog.dismiss();
                    }
                });

                builder.setNegativeButton("No", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        dialog.dismiss();
                    }
                });

                builder.show();
            }
        });
    }

    public static void showMessageBox(final String title, final String message) {
        activity.runOnUiThread(new Runnable() {
            public void run() {
                if (title.contains("Account registration")) {
                    if (title.contains("successful")) AuthDialog.dismiss();
                    AuthDialog.activateSubmitBtn();
                } else if (title.contains("Logging in")) {
                    if (title.contains("successful")) {
                        Properties props = loadConfig();
                        props.setProperty("username", user);
                        props.setProperty("password", pass);
                        saveConfig(props);

                        logged_in = true;
                        AuthDialog.dismiss();
                        reqBtn.setVisibility(View.VISIBLE);
                        saveBtn.setVisibility(View.VISIBLE);
                        loadBtn.setVisibility(View.VISIBLE);
                        logoutBtn.setVisibility(View.VISIBLE);
                        registerBtn.setVisibility(View.GONE);
                        loginBtn.setVisibility(View.GONE);
                    }
                    AuthDialog.activateSubmitBtn();
                } else if (title.contains("Req")) {
                    if (title.contains("successful")) {
                        moderator = true;
                        reqBtn.setVisibility(View.GONE);
                        Properties props = loadConfig();
                        props.setProperty("moderator", "1");
                        saveConfig(props);
                    }
                    reqBtn.setEnabled(true);
                } else if (title.contains("Save")) {
                    saveBtn.setEnabled(true);
                } else if (title.contains("Load")) {
                    loadBtn.setEnabled(true);
                } else if (title.contains("Rate") || title.contains("Unrate")) {
                    rateBtn.setEnabled(true);
                }
                new AlertDialog.Builder(activity).setTitle(title).setMessage(message).setPositiveButton("OK", null).show();
            }
        });
    }

    public static void setOrigSongBtnVisible() { origSongBtn.setVisibility(View.VISIBLE); }
    public static void disableRateBtn() { rateBtn.setEnabled(false); }

    public static void onDestroy() {
        AuthDialog.dismiss();
        registerBtn = null;
        loginBtn = null;
        logoutBtn = null;
        saveBtn = null;
        loadBtn = null;
        reqBtn = null;
        percentBtn = null;
        songsBtn = null;
        copyBtn = null;
        rateBtn = null;
        offsetBtn = null;
        origSongBtn = null;
        customSongBtn = null;
        activity = null;
    }
}
