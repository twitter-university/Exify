package com.twitter.university.exify;

import java.io.File;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.view.Menu;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;

import com.ipaulpro.afilechooser.utils.FileUtils;

public class MainActivity extends Activity implements OnClickListener {
	private static final int FILE_CHOOSE_REQUEST = 1;
	private TextView fileSelection;
	private Button fileChooser;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		super.setContentView(R.layout.activity_main);
		this.fileSelection = (TextView) super.findViewById(R.id.file_selection);
		this.fileSelection.setText(R.string.no_selection_message);
		

		
		this.fileChooser = (Button) super.findViewById(R.id.file_chooser);
		this.fileChooser.setOnClickListener(this);
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
		switch (requestCode) {
		case FILE_CHOOSE_REQUEST:
			if (resultCode == RESULT_OK) {
				final Uri uri = data.getData();
				File file = FileUtils.getFile(uri);
				this.fileSelection.setText(file.getName());
			}
		}
	}
}
