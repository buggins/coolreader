/*
 * CoolReader for Android
 * Copyright (C) 2011,2012,2014 Vadim Lopatin <coolreader.org@gmail.com>
 * Copyright (C) 2014 klush
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

package org.coolreader.crengine;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ListView;
import android.widget.RadioButton;
import android.widget.TextView;

import org.coolreader.CoolReader;
import org.coolreader.R;

public class SwitchProfileDialog extends BaseDialog {
	CoolReader mCoolReader;
	ReaderView mReaderView;
	ListView mListView;
	int currentProfile;
	public SwitchProfileDialog(CoolReader coolReader, ReaderView readerView)
	{
		super(coolReader, coolReader.getResources().getString(R.string.action_switch_settings_profile), false, false);
        setCancelable(true);
		this.mCoolReader = coolReader;
		this.mReaderView = readerView;
		this.mListView = new BaseListView(getContext(), false);
		currentProfile = this.mCoolReader.getCurrentProfile(); // TODO: get from settings
		mListView.setOnItemClickListener((listview, view, position, id) -> {
			mReaderView.setCurrentProfile(position + 1);
			SwitchProfileDialog.this.dismiss();
		});
		mListView.setOnItemLongClickListener((listview, view, position, id) -> {
			// TODO: rename?
			SwitchProfileDialog.this.dismiss();
			return true;
		});
		mListView.setLongClickable(true);
		mListView.setClickable(true);
		mListView.setFocusable(true);
		mListView.setFocusableInTouchMode(true);
		mListView.setChoiceMode(ListView.CHOICE_MODE_SINGLE);
		setView(mListView);
		//
		// cancel
		setFlingHandlers(mListView, SwitchProfileDialog.this::dismiss, SwitchProfileDialog.this::dismiss);
		mListView.setAdapter(new ProfileListAdapter());
	}
	
	private String[] profileNames = {
		"Profile 1",
		"Profile 2",
		"Profile 3",
		"Profile 4",
		"Profile 5",
		"Profile 6",
		"Profile 7",
	};

	class ProfileListAdapter extends BaseListAdapter {
		public boolean areAllItemsEnabled() {
			return true;
		}

		public boolean isEnabled(int arg0) {
			return true;
		}

		public int getCount() {
			return Settings.MAX_PROFILES;
		}

		public Object getItem(int position) {
			return profileNames[position];
		}

		public long getItemId(int position) {
			return position;
		}

		public int getItemViewType(int position) {
			return 0;
		}

		
		public View getView(final int position, View convertView, ViewGroup parent) {
			View view;
			boolean isCurrentItem = position == currentProfile - 1;
			if ( convertView==null ) {
				//view = new TextView(getContext());
				LayoutInflater inflater = LayoutInflater.from(getContext());
				view = inflater.inflate(R.layout.profile_item, null);
			} else {
				view = convertView;
			}
			RadioButton cb = view.findViewById(R.id.option_value_check);
			TextView title = view.findViewById(R.id.option_value_text);
			cb.setChecked(isCurrentItem);
			cb.setFocusable(false);
			cb.setFocusableInTouchMode(false);
			title.setText(profileNames[position]);
			cb.setOnClickListener(v -> {
				mReaderView.setCurrentProfile(position + 1);
				SwitchProfileDialog.this.dismiss();
			});
			return view;
		}

		public int getViewTypeCount() {
			return 1;
		}

		public boolean hasStableIds() {
			return true;
		}

		public boolean isEmpty() {
			return false;
		}
	}

}

