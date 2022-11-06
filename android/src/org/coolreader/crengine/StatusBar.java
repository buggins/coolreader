/*
 * CoolReader for Android
 * Copyright (C) 2012 Vadim Lopatin <coolreader.org@gmail.com>
 * Copyright (C) 2012 Jeff Doozan <jeff@doozan.com>
 * Copyright (C) 2021 Aleksey Chernov <valexlin@gmail.com>
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

import org.coolreader.CoolReader;
import org.coolreader.R;

import android.util.TypedValue;
import android.view.LayoutInflater;
import android.widget.LinearLayout;
import android.widget.TextView;

public class StatusBar extends LinearLayout implements Settings {
		private CoolReader activity;
		private LinearLayout content;
		private TextView lblTitle;
		private TextView lblPosition;
		private PositionIndicator indicator;
		private int textSize = 14;
		private int color = 0;
		private boolean showBookAuthor;
		private boolean showBookTitle;
		private boolean showBattery;
		private boolean showTime;
		private boolean showPageNumber;
		private boolean showPosPercent;
		private boolean fullscreen;
		private boolean nightMode;
		
		FileInfo book;
		Bookmark position;
		PositionProperties props;
		
		public void updateFullscreen(boolean fullscreen) {
			if (this.fullscreen == fullscreen)
				return;
			this.fullscreen = fullscreen;
			requestLayout();
		}
		
		public boolean updateSettings(Properties props) {
			int newTextSize = props.getInt(Settings.PROP_STATUS_FONT_SIZE, 16);
			boolean needRelayout = (textSize != newTextSize);
			this.textSize = newTextSize;
			showBookTitle = props.getBool(PROP_SHOW_TITLE, true);
			showBattery = true; //props.getBool(PROP_SHOW_BATTERY, true);
			showTime = true; //props.getBool(PROP_SHOW_TIME, true);
			showPageNumber = props.getBool(PROP_SHOW_PAGE_NUMBER, true);
			showPosPercent = props.getBool(PROP_SHOW_POS_PERCENT, true);
			nightMode = props.getBool(PROP_NIGHT_MODE, false);
			this.color = props.getColor(Settings.PROP_STATUS_FONT_COLOR, 0);
			lblTitle.setTextColor(0xFF000000 | color);
			lblPosition.setTextColor(0xFF000000 | color);
			indicator.setColor(this.color);
			lblTitle.setTextSize(TypedValue.COMPLEX_UNIT_PX, textSize);
			lblPosition.setTextSize(TypedValue.COMPLEX_UNIT_PX, textSize);
			if (needRelayout) {
				CoolReader.log.d("changing status bar layout");
				lblPosition.measure(MeasureSpec.UNSPECIFIED, MeasureSpec.UNSPECIFIED);
				lblTitle.measure(MeasureSpec.UNSPECIFIED, MeasureSpec.UNSPECIFIED);
				content.measure(MeasureSpec.UNSPECIFIED, MeasureSpec.UNSPECIFIED);
				content.forceLayout();
				forceLayout();
//				content.measure(MeasureSpec.UNSPECIFIED, MeasureSpec.UNSPECIFIED);
//				measure(MeasureSpec.UNSPECIFIED, MeasureSpec.UNSPECIFIED);
//				content.forceLayout();
//				requestLayout();
//				removeAllViews();
//				addView(content);
//				addView(indicator);
			}
			invalidate();
			return needRelayout;
		}
		
		public StatusBar(CoolReader context) {
			super(context);
			this.activity = context;
			setOrientation(VERTICAL);
			
			this.color = context.settings().getColor(Settings.PROP_STATUS_FONT_COLOR, 0);
			
			LayoutInflater inflater = LayoutInflater.from(activity);
			content = (LinearLayout)inflater.inflate(R.layout.reader_status_bar, null);
			lblTitle = (TextView)content.findViewById(R.id.title);
			lblPosition = (TextView)content.findViewById(R.id.position);

			lblTitle.setText("Cool Reader " + activity.getVersion());
			lblTitle.setTextSize(TypedValue.COMPLEX_UNIT_PX, textSize);
			lblTitle.setTextColor(0xFF000000 | color);

			lblPosition.setText("");
			lblPosition.setTextSize(TypedValue.COMPLEX_UNIT_PX, textSize);
			lblPosition.setTextColor(0xFF000000 | color);
			
			addView(content);
			indicator = new PositionIndicator(activity);
			addView(indicator);
			//content.addView(indicator);
			onThemeChanged(context.getCurrentTheme());
			updateSettings(context.settings());
		}

		public void onThemeChanged(InterfaceTheme theme) {
//			//color = nightMode ? 0x606060 : theme.getStatusTextColor();
//			lblTitle.setTextColor(0xFF000000 | color);
//			lblPosition.setTextColor(0xFF000000 | color);
////			if (DeviceInfo.EINK_SCREEN)
////				setBackgroundColor(0xFFFFFFFF);
////			else if (nightMode)
////				setBackgroundColor(0xFF000000);
////			else
////				setBackgroundResource(theme.getReaderStatusBackground());
//			indicator.setColor(color);
//			if (isShown())
//				invalidate();
		}

		@Override
		protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
			content.measure(widthMeasureSpec, heightMeasureSpec);
			super.onMeasure(widthMeasureSpec, heightMeasureSpec);
		}
		
		private static boolean empty(String s) {
			return s == null || s.length() == 0;
		}
		
		private static void append(StringBuilder buf, String text, String delimiter) {
			if (!Utils.empty(text)) {
				if (buf.length() != 0 && !empty(delimiter)) {
					buf.append(delimiter);
				}
				buf.append(text);
			}
		}
		
		public void updateCurrentPositionStatus(FileInfo book, Bookmark position, PositionProperties props) {
			this.book = book != null ? new FileInfo(book) : null;
			this.position = position != null ? new Bookmark(position) : null;
			this.props = props != null ? new PositionProperties(props) : null;
			updateViews();
		}
		private void updateViews() {
			StringBuilder title = new StringBuilder();
			StringBuilder pos = new StringBuilder();
			if (book != null) {
				String authors = Utils.formatAuthorsNormalNames(book.authors);
				append(title, book.title, null);
				append(title, authors, " - ");
				if (title.length() == 0)
					append(title, book.getFileNameToDisplay(), null);
				if (props != null) {
					if (showPageNumber)
						append(pos, (props.pageNumber + 1) + "/" + props.pageCount, " ");
					if (showPosPercent) {
						String percent = props.getPercent() > 0 ? Utils.formatPercent(props.getPercent()) : "0%";
						append(pos, percent, " ");
					}
				}
//				if (position != null) {
//					if (showPosPercent) {
//						String percent = position.getPercent() > 0 ? Utils.formatPercent(position.getPercent()) : "0%";
//						append(pos, percent, " ");
//					}
//				}
			}
			if (showTime && fullscreen) {
				append(pos, Utils.formatTime(activity, System.currentTimeMillis()), " ");
			}
			if (showBattery && fullscreen) {
				int batteryLevel = activity.getReaderView() != null ? activity.getReaderView().getBatteryChargeLevel() : 0;
				append(pos, "[" + (batteryLevel < 10 ? "0" : "") + batteryLevel + "%]", " ");
			}
			boolean updated = false;
			if (!lblPosition.getText().equals(pos)) {
				this.lblPosition.setText(pos);
				updated = true;
			}
			if (!lblTitle.getText().equals(title)) {
				this.lblTitle.setText(title);
				updated = true;
			}
			if (position != null)
				indicator.setPosition(position.getPercent());
			else
				indicator.setPosition(0);
			if (updated && isShown()) {
				CoolReader.log.d("changing status bar layout");
				measure(MeasureSpec.UNSPECIFIED, MeasureSpec.UNSPECIFIED);
				forceLayout();
			}
		}
	
	}