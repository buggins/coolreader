/*
 * CoolReader for Android
 * Copyright (C) 2012 Vadim Lopatin <coolreader.org@gmail.com>
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

import android.annotation.SuppressLint;
import android.text.Editable;
import android.text.InputType;
import android.text.TextWatcher;
import android.text.method.DigitsKeyListener;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;

import org.coolreader.R;

public class InputDialog extends BaseDialog {

	public interface InputHandler {
		boolean validate(String s) throws Exception;

		void onOk(String s) throws Exception;

		void onCancel();
	}

	private final InputHandler mInputHandler;
	private final EditText mEditText;
	private final int mMinValue;
	private final int mMaxValue;
	private SeekBar mSeekBar;

	public InputDialog(BaseActivity activity, final String title, final String prompt, boolean isNumberEdit, int minValue, int maxValue, int currentValue, final InputHandler inputHandler) {
		this(activity, title, true, prompt, isNumberEdit, minValue, maxValue, currentValue, inputHandler);
	}

	@SuppressLint("ClickableViewAccessibility")
	public InputDialog(BaseActivity activity, final String title, boolean showNegativeButton, final String prompt, boolean isNumberEdit, int minValue, int maxValue, int currentValue, final InputHandler inputHandler) {
		super(activity, title, showNegativeButton, false);
		mInputHandler = inputHandler;
		mMinValue = minValue;
		mMaxValue = maxValue;
		LayoutInflater mInflater = LayoutInflater.from(getContext());
		ViewGroup layout = (ViewGroup) mInflater.inflate(R.layout.line_edit_dlg, null);
		mEditText = (EditText) layout.findViewById(R.id.input_field);
		TextView promptView = (TextView) layout.findViewById(R.id.lbl_prompt);
		if (promptView != null) {
			promptView.setText(prompt);
		}
		mSeekBar = layout.findViewById(R.id.goto_position_seek_bar);
		if (mSeekBar != null) {
			mSeekBar.setMax(maxValue - minValue);
			mSeekBar.setProgress(currentValue - minValue);
			mSeekBar.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {
				@Override
				public void onStopTrackingTouch(SeekBar seekBar) {
				}

				@Override
				public void onStartTrackingTouch(SeekBar seekBar) {
				}

				@Override
				public void onProgressChanged(SeekBar seekBar, int progress,
											  boolean fromUser) {
					/*if (fromUser)*/
					{
						String value = String.valueOf(progress + InputDialog.this.mMinValue);
						try {
							if (inputHandler.validate(value))
								mEditText.setTextKeepState(value);
						} catch (Exception e) {
							// ignore
						}
					}
				}
			});
		}
		if (isNumberEdit) {
			mEditText.setKeyListener(DigitsKeyListener.getInstance("0123456789."));
			mEditText.setInputType(InputType.TYPE_CLASS_NUMBER);
			if (currentValue >= minValue)
				mEditText.setText(String.valueOf(currentValue));
//	        input.getText().setFilters(new InputFilter[] {
//	        	new DigitsKeyListener()        
//	        });
			mEditText.addTextChangedListener(new TextWatcher() {
				@Override
				public void beforeTextChanged(CharSequence s, int start, int count, int after) {
				}

				@Override
				public void onTextChanged(CharSequence s, int start, int before, int count) {
				}

				@Override
				public void afterTextChanged(Editable s) {
					try {
						int value = Integer.parseInt(s.toString());
						mSeekBar.setProgress(value - InputDialog.this.mMinValue);
					} catch (Exception ignored) {
					}
				}
			});
			ImageView decButton = layout.findViewById(R.id.btn_dec);
			decButton.setVisibility(View.VISIBLE);
			decButton.setOnTouchListener(new RepeatOnTouchListener(500, 150,
					view -> mSeekBar.setProgress(mSeekBar.getProgress() - 1)));
			ImageView incButton = layout.findViewById(R.id.btn_inc);
			incButton.setVisibility(View.VISIBLE);
			incButton.setOnTouchListener(new RepeatOnTouchListener(500, 150,
					view -> mSeekBar.setProgress(mSeekBar.getProgress() + 1)));
		}
		setView(layout);
	}

	@Override
	protected void onNegativeButtonClick() {
		cancel();
		mInputHandler.onCancel();
	}

	@Override
	protected void onPositiveButtonClick() {
		String value = mEditText.getText().toString().trim();
		try {
			if (mInputHandler.validate(value))
				mInputHandler.onOk(value);
			else
				mInputHandler.onCancel();
		} catch (Exception e) {
			mInputHandler.onCancel();
		}
		cancel();
	}
}
