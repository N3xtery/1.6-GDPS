package com.necytdamu.onesixgdps;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Environment;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.view.ViewGroup;
import android.content.Context;
import android.widget.LinearLayout;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.List;

public class FileBrowser {
    public interface Callback {
        void onSelected(File file);
    }

    private final boolean dirMode;
    private final Callback callback;
    private final AlertDialog dialog;
    private final ArrayAdapter<String> adapter;
    private final List<File> entries = new ArrayList<File>();
    private final List<String> names = new ArrayList<String>();
    private File current;
    private String allowedExtension;

    public FileBrowser(String allowedExtension, final boolean dirMode, Callback callback) {
        this.allowedExtension = allowedExtension;
        this.dirMode = dirMode;
        this.callback = callback;

        LinearLayout layout = new LinearLayout(OverlayUI.activity);
        layout.setMinimumWidth(OverlayUI.dp(600));

        final ListView listView = new ListView(OverlayUI.activity);
        adapter = new ArrayAdapter<String>(OverlayUI.activity, android.R.layout.simple_list_item_1, names);
        listView.setAdapter(adapter);
        listView.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                File f = entries.get(position);
                if (f.isDirectory()) {
                    navigate(f);
                    listView.setSelection(0);
                } else if (!dirMode) {
                    FileBrowser.this.dialog.dismiss();
                    FileBrowser.this.callback.onSelected(f);
                }
            }
        });

        AlertDialog.Builder builder = new AlertDialog.Builder(OverlayUI.activity);
        layout.addView(listView, new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT));
        builder.setView(layout);
        if (dirMode) {
            builder.setPositiveButton("OK", new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface d, int which) {
                    FileBrowser.this.callback.onSelected(current);
                }
            });
            builder.setNegativeButton("Cancel", null);
        }
        dialog = builder.create();
        dialog.setTitle(dirMode ? "Select a folder" : "Select a file");
        dialog.show();
        navigate(Environment.getExternalStorageDirectory());
    }

    private static final Comparator<File> SORT = new Comparator<File>() {
        public int compare(File a, File b) {
            if (a.isDirectory() != b.isDirectory()) return a.isDirectory() ? -1 : 1;
            return a.getName().compareToIgnoreCase(b.getName());
        }
    };

    private void addEntry(File f, boolean isDir) {
        entries.add(f);
        names.add(isDir ? f.getName() + "/" : f.getName());
    }

    private List<File> getStorageRoots() {
        List<File> roots = new ArrayList<File>();
        roots.add(Environment.getExternalStorageDirectory());
        try {
            File[] dirs = (File[]) Context.class.getMethod("getExternalFilesDirs", String.class).invoke(OverlayUI.activity, new Object[] { null });
            for (File f : dirs) {
                if (f == null) continue;
                String p = f.getAbsolutePath();
                int i = p.indexOf("/Android/data");
                if (i > 0) {
                    File root = new File(p.substring(0, i));
                    if (!roots.contains(root)) roots.add(root);
                }
            }
        } catch (Exception ignored) {}
        return roots;
    }

    private void navigate(File dir) {
        current = dir;
        dialog.setTitle(dir.getAbsolutePath());
        entries.clear();
        names.clear();

        File parent = dir.getParentFile();
        if (parent != null) {
            entries.add(parent);
            names.add("..");
        }

        File[] children = dir.listFiles();
        if (children != null) {
            Arrays.sort(children, SORT);
            for (File f : children) {
                boolean isDir = f.isDirectory();
                if (isDir || f.getName().endsWith("." + allowedExtension)) addEntry(f, isDir);
            }
        } else {
            for (File root : getStorageRoots()) {
                for (File f = root; f != null; f = f.getParentFile()) {
                    if (dir.equals(f.getParentFile()) && !entries.contains(f)) addEntry(f, true);
                }
            }
        }

        if (dirMode) dialog.getButton(AlertDialog.BUTTON_POSITIVE).setEnabled(children != null);

        adapter.notifyDataSetChanged();
    }
}
