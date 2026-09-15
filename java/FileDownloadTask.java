package com.necytdamu.onesixgdps;

import android.app.AlertDialog;
import android.content.Context;
import android.os.AsyncTask;
import android.os.Environment;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.LinearLayout;
import android.widget.TableLayout;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.concurrent.TimeUnit;

import com.squareup.okhttp.OkHttpClient;

// ai-generated code

public class FileDownloadTask extends AsyncTask<String, Integer, Boolean> {
    private Context context;
    private AlertDialog dialog;
    private ProgressBar progressBar;
    private TextView tvPercent;
    private File destination;
    private String errorMessage;
    private TableLayout table;
    private boolean levelSettings;

    public FileDownloadTask(Context context, File destination, TableLayout table, boolean levelSettings) {
        this.context = context;
        this.destination = destination;
        this.table = table;
        this.levelSettings = levelSettings;
    }

    @Override
    protected void onPreExecute() {
        if (destination.exists()) {
            OverlayUI.showMessageBox("Error", "The song is already present");
            cancel(true);
            return;
        }

        LinearLayout layout = new LinearLayout(context);
        layout.setOrientation(LinearLayout.VERTICAL);

        int paddingPx = OverlayUI.dp(16);
        layout.setPadding(paddingPx, paddingPx, paddingPx, paddingPx);

        TextView tvProgressLabel = new TextView(context);
        tvProgressLabel.setText("Downloading...");
        LinearLayout.LayoutParams labelParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
        );
        tvProgressLabel.setLayoutParams(labelParams);
        layout.addView(tvProgressLabel);

        progressBar = new ProgressBar(context, null, android.R.attr.progressBarStyleHorizontal);
        progressBar.setMax(100);
        progressBar.setProgress(0);
        LinearLayout.LayoutParams progressParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
        );
        progressParams.topMargin = OverlayUI.dp(8);
        progressBar.setLayoutParams(progressParams);
        layout.addView(progressBar);

        tvPercent = new TextView(context);
        tvPercent.setText("0%");
        LinearLayout.LayoutParams percentParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
        );
        percentParams.topMargin = OverlayUI.dp(4);
        tvPercent.setLayoutParams(percentParams);
        layout.addView(tvPercent);

        AlertDialog.Builder builder = new AlertDialog.Builder(context);
        builder.setTitle("Downloading file");
        builder.setView(layout);
        builder.setCancelable(false);
        builder.setNegativeButton("Cancel", new android.content.DialogInterface.OnClickListener() {
            @Override
            public void onClick(android.content.DialogInterface dialogInterface, int i) {
                cancel(true);
            }
        });

        dialog = builder.create();
        dialog.show();
    }

    @Override
    protected Boolean doInBackground(String... params) {
        String fileUrl = params[0];
        InputStream input = null;
        OutputStream output = null;
        HttpURLConnection connection = null;

        try {
            URL url = new URL(fileUrl);

            OkHttpClient client = new OkHttpClient();
            client.setSslSocketFactory(OverlayUI.sslContext.getSocketFactory());
            client.setConnectTimeout(15000, TimeUnit.MILLISECONDS);
            client.setReadTimeout(15000, TimeUnit.MILLISECONDS);

            connection = client.open(url);

            if (connection.getResponseCode() != HttpURLConnection.HTTP_OK) {
                errorMessage = "Server returned HTTP " + connection.getResponseCode();
                return false;
            }

            int fileLength = connection.getContentLength();

            input = connection.getInputStream();
            output = new FileOutputStream(destination);

            byte[] buffer = new byte[4096];
            long total = 0;
            int count;

            while ((count = input.read(buffer)) != -1) {
                if (isCancelled()) {
                    return false;
                }
                total += count;

                if (fileLength > 0) {
                    publishProgress((int) (total * 100 / fileLength));
                } else {
                    publishProgress(-1); // unknown size, indeterminate
                }

                output.write(buffer, 0, count);
            }

            return true;

        } catch (Exception e) {
            errorMessage = e.getMessage();
            return false;
        } finally {
            try { if (output != null) output.close(); } catch (Exception ignored) {}
            try { if (input != null) input.close(); } catch (Exception ignored) {}
            if (connection != null) connection.disconnect();
        }
    }

    @Override
    protected void onProgressUpdate(Integer... values) {
        if (dialog == null) return;
        int progress = values[0];
        if (progress < 0) {
            progressBar.setIndeterminate(true);
        } else {
            progressBar.setIndeterminate(false);
            progressBar.setProgress(progress);
            tvPercent.setText(progress + "%");
        }
    }

    @Override
    protected void onPostExecute(Boolean success) {
        if (dialog != null && dialog.isShowing()) {
            dialog.dismiss();
        }

        if (success) {
            destination.setReadable(true, false);
            if (table != null) SongsDialog.songsTableAddRow(table, destination, true, levelSettings);
            else SongsDialog.playLevel();
        } else {
            // error
        }
    }

    @Override
    protected void onCancelled() {
        if (dialog != null && dialog.isShowing()) {
            dialog.dismiss();
        }
    }
}
