package com.twitter.university.exify;

import java.io.File;
import java.io.IOException;
import java.util.Map;

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
import android.view.Menu;
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

import com.ipaulpro.afilechooser.utils.FileUtils;

public class MainActivity extends Activity implements OnClickListener {
    private static final String TAG = MainActivity.class.getSimpleName();

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
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.main, menu);
        return true;
    }

    @Override
    public void onClick(View v) {
        if (v == this.fileChooser) {
            final Intent getContentIntent = new Intent(
                    Intent.ACTION_GET_CONTENT);
            getContentIntent.setType("image/jpeg");
            getContentIntent.addCategory(Intent.CATEGORY_OPENABLE);
            Intent intent = Intent.createChooser(getContentIntent,
                    super.getText(R.string.chooser_label));
            super.startActivityForResult(intent, FILE_CHOOSE_REQUEST);
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        LinearLayout.LayoutParams rowParams = new LinearLayout.LayoutParams(
                LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT);
        TableRow.LayoutParams cellParams = new TableRow.LayoutParams(
                LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT);

        switch (requestCode) {
        case FILE_CHOOSE_REQUEST:
            if (resultCode == RESULT_OK) {
                final Uri uri = data.getData();
                Log.d(TAG, "Got file URI " + uri);
                File file = asFile(uri);
                if (file == null) {
                    Toast.makeText(this, R.string.unknown_selection_message,
                            Toast.LENGTH_SHORT).show();
                    return;
                }
                String filePath = file.getAbsolutePath();
                Log.d(TAG, "Got file path " + filePath);
                this.fileSelection.setText(file.getName());
                try {
                    byte[] thumbnailData = JHead.getThumbnail(filePath);
                    if (thumbnailData != null && thumbnailData.length > 0) {
                        Log.d(TAG, "Got thumbnail of " + thumbnailData.length
                                + " bytes");
                        Bitmap bMap = BitmapFactory.decodeByteArray(
                                thumbnailData, 0, thumbnailData.length);
                        thumbnail.setImageBitmap(bMap);
                    }

                    Map<String, String> imageInfo = JHead
                            .getImageInfo(filePath);
                    Log.d(TAG, "Loaded: " + imageInfo);
                    this.exifAttributes.removeAllViews();
                    for (Map.Entry<String, String> entry : imageInfo.entrySet()) {
                        TableRow row = new TableRow(this);
                        row.setLayoutParams(rowParams);

                        TextView cell;

                        cell = new TextView(this);
                        cell.setText(entry.getKey());
                        cell.setTextAppearance(this, R.style.tableRowHeader);
                        cell.setPadding(0, 0, 10, 0);
                        row.addView(cell, cellParams);

                        cell = new TextView(this);
                        cell.setText(entry.getValue());
                        cell.setEllipsize(TruncateAt.MARQUEE);
                        cell.setSingleLine();
                        cell.setFocusable(true);
                        cell.setFocusableInTouchMode(true);
                        cell.setLayoutParams(cellParams);
                        row.addView(cell);

                        this.exifAttributes.addView(row);
                    }
                } catch (IOException e) {
                    Toast.makeText(this, e.getMessage(), Toast.LENGTH_LONG)
                            .show();
                }
            }
        }
    }

    private static final String[] MEDIA_DATA_ONLY_PROJECTION = { MediaColumns.DATA };

    private File asFile(Uri uri) {
        String scheme = uri.getScheme();
        if ("file".equals(scheme)) {
            return FileUtils.getFile(uri);
        } else if ("content".equals(scheme)) {
            Cursor cursor = super.getContentResolver().query(uri,
                    MEDIA_DATA_ONLY_PROJECTION, null, null, null);
            try {
                return cursor.moveToFirst() ? new File(cursor.getString(0))
                        : null;
            } finally {
                cursor.close();
            }
        } else {
            return null;
        }
    }
}
