package com.twitter.university.exify;

import android.app.Activity;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.Bundle;
import android.provider.MediaStore.MediaColumns;
import android.text.TextUtils.TruncateAt;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TableLayout;
import android.widget.TableRow;
import android.widget.TableRow.LayoutParams;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;
import java.io.IOException;
import java.util.Map;

public class MainActivity extends Activity implements OnClickListener {
    private static final String TAG = MainActivity.class.getSimpleName();
    private static final LinearLayout.LayoutParams TABLE_ROW_LAYOUT_PARAMS = new LinearLayout.LayoutParams(
            LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT);
    private static final TableRow.LayoutParams TABLE_CELL_LAYOUT_PARAMS = new TableRow.LayoutParams(
            LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT);
    private static final String[] MEDIA_DATA_ONLY_PROJECTION = {MediaColumns.DATA};
    private static final int FILE_CHOOSE_REQUEST = 1;
    private TextView fileSelection;
    private Button fileChooser;
    private ImageView thumbnail;
    private TableLayout exifAttributes;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        super.setContentView(R.layout.activity_main);
        this.fileSelection = (TextView) super.findViewById(R.id.file_selection);
        this.fileSelection.setText(R.string.no_selection_message);

        this.fileChooser = (Button) super.findViewById(R.id.file_chooser);
        this.fileChooser.setOnClickListener(this);
        this.thumbnail = (ImageView) super.findViewById(R.id.thumbnail);
        this.exifAttributes = (TableLayout) super
                .findViewById(R.id.exif_attributes);
    }

    @Override
    public void onClick(View v) {
        if (v == this.fileChooser) {
            final Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            intent.setType("image/jpeg");
            super.startActivityForResult(intent, FILE_CHOOSE_REQUEST);
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        switch (requestCode) {
            case FILE_CHOOSE_REQUEST:
                if (resultCode == RESULT_OK) {
                    Log.d(TAG, "Got result " + data);
                    File file = getFile(data);
                    if (file == null) {
                        Toast.makeText(this, R.string.unknown_selection_message,
                                Toast.LENGTH_SHORT).show();
                    } else {
                        exify(file);
                    }
                }
        }
    }

    private File getFile(Intent intent) {
        Uri uri = intent.getData();
        if (uri == null) {
            return null;
        }
        final Cursor cursor = super.getContentResolver().query(uri,
                MEDIA_DATA_ONLY_PROJECTION, null, null, null);
        try {
            return cursor.moveToFirst() ? new File(cursor.getString(0)) : null;
        } finally {
            cursor.close();
        }
    }

    private void exify(File file) {
        String filePath = file.getAbsolutePath();
        if (BuildConfig.DEBUG) {
            Log.d(TAG, "Got file path " + filePath);
        }
        this.fileSelection.setText(file.getName());
        try {
            // NOTICE: File I/O *should* not be done on the UI thread
            // but for the sake of brevity we are skipping best practices
            byte[] thumbnailData = JHead.getThumbnail(filePath);
            if (thumbnailData != null && thumbnailData.length > 0) {
                if (BuildConfig.DEBUG) {
                    Log.d(TAG, "Got thumbnail of " + thumbnailData.length
                            + " bytes");
                }
                Bitmap bMap = BitmapFactory.decodeByteArray(thumbnailData, 0,
                        thumbnailData.length);
                this.thumbnail.setImageBitmap(bMap);
            }

            Map<String, String> imageInfo = JHead.getImageInfo(filePath);
            if (BuildConfig.DEBUG) {
                Log.d(TAG, "Loaded: " + imageInfo);
            }
            this.exifAttributes.removeAllViews();
            for (Map.Entry<String, String> entry : imageInfo.entrySet()) {
                TableRow row = new TableRow(this);
                row.setLayoutParams(TABLE_ROW_LAYOUT_PARAMS);

                TextView cell;

                cell = new TextView(this);
                cell.setText(entry.getKey());
                cell.setTextAppearance(this, R.style.tableRowHeader);
                cell.setPadding(0, 0, 10, 0);
                row.addView(cell, TABLE_CELL_LAYOUT_PARAMS);

                cell = new TextView(this);
                cell.setText(entry.getValue());
                cell.setEllipsize(TruncateAt.MARQUEE);
                cell.setSingleLine();
                cell.setFocusable(true);
                cell.setFocusableInTouchMode(true);
                cell.setLayoutParams(TABLE_CELL_LAYOUT_PARAMS);
                row.addView(cell);

                this.exifAttributes.addView(row);
            }
        } catch (IOException e) {
            Toast.makeText(this, e.getMessage(), Toast.LENGTH_LONG).show();
        }
    }
}
