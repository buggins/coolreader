package org.coolreader.crengine;

import org.coolreader.CoolReader;
import org.coolreader.R;

import android.view.LayoutInflater;
import android.view.View;
import android.widget.EditText;

public class OPDSCatalogEditDialog extends BaseDialog {

	private final CoolReader mCoolReader;
	private final LayoutInflater mInflater;
	private final FileInfo mItem;
	private final boolean mIsNew;
	private final EditText nameEdit;
	private final EditText urlEdit;
	private final Runnable mOnUpdate;

	public OPDSCatalogEditDialog(CoolReader activity, FileInfo item, Runnable onUpdate) {
		super(activity, R.string.dlg_button_ok, R.string.dlg_button_cancel,
				false);
		mCoolReader = activity;
		mItem = item;
		mIsNew = mItem.id == null;
		mOnUpdate = onUpdate;
		setTitle(mCoolReader.getString(mIsNew ? R.string.dlg_catalog_add_title
				: R.string.dlg_catalog_edit_title));
		mInflater = LayoutInflater.from(getContext());
		View view = mInflater.inflate(R.layout.catalog_edit_dialog, null);
		nameEdit = (EditText) view.findViewById(R.id.catalog_name);
		urlEdit = (EditText) view.findViewById(R.id.catalog_url);
		nameEdit.setText(mItem.filename);
		urlEdit.setText(mItem.getOPDSUrl());
		setView(view);
	}

	@Override
	protected void onPositiveButtonClick() {
		mCoolReader.getDB().saveOPDSCatalog(mItem.id,
				urlEdit.getText().toString(), nameEdit.getText().toString());
		mOnUpdate.run();
		super.onPositiveButtonClick();
	}

	@Override
	protected void onNegativeButtonClick() {
		super.onNegativeButtonClick();
	}

}
