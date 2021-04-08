package org.coolreader.crengine;

import android.text.Editable;
import android.text.InputType;
import android.text.TextWatcher;
import android.text.method.DigitsKeyListener;
import android.view.LayoutInflater;
import android.view.ViewGroup;
import android.widget.EditText;
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

	private final InputHandler handler;
	private final EditText input;
	private final int minValue;
	private final int maxValue;

	public InputDialog(BaseActivity activity, final String title, final String prompt, boolean isNumberEdit, int minValue, int maxValue, int currentValue, final InputHandler handler) {
		this(activity, title, true, prompt, isNumberEdit, minValue, maxValue, currentValue, handler);
	}

	public InputDialog(BaseActivity activity, final String title, boolean showNegativeButton, final String prompt, boolean isNumberEdit, int minValue, int maxValue, int currentValue, final InputHandler handler) {
		super(activity, title, showNegativeButton, false);
		this.handler = handler;
		this.minValue = minValue;
		this.maxValue = maxValue;
		LayoutInflater mInflater = LayoutInflater.from(getContext());
		ViewGroup layout = (ViewGroup) mInflater.inflate(R.layout.line_edit_dlg, null);
		input = (EditText) layout.findViewById(R.id.input_field);
		TextView promptView = (TextView) layout.findViewById(R.id.lbl_prompt);
		if (promptView != null) {
			promptView.setText(prompt);
		}
		SeekBar seekBar = layout.findViewById(R.id.goto_position_seek_bar);
		if (seekBar != null) {
			seekBar.setMax(maxValue - minValue);
			seekBar.setProgress(currentValue - minValue);
			seekBar.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {
				@Override
				public void onStopTrackingTouch(SeekBar seekBar) {
				}

				@Override
				public void onStartTrackingTouch(SeekBar seekBar) {
				}

				@Override
				public void onProgressChanged(SeekBar seekBar, int progress,
											  boolean fromUser) {
					if (fromUser) {
						String value = String.valueOf(progress + InputDialog.this.minValue);
						try {
							if (handler.validate(value))
								input.setText(value);
						} catch (Exception e) {
							// ignore
						}
					}
				}
			});
		}
		if (isNumberEdit) {
			input.setKeyListener(DigitsKeyListener.getInstance("0123456789."));
			input.setInputType(InputType.TYPE_CLASS_NUMBER);
			if (currentValue >= minValue)
				input.setText(String.valueOf(currentValue));
//	        input.getText().setFilters(new InputFilter[] {
//	        	new DigitsKeyListener()        
//	        });
			input.addTextChangedListener(new TextWatcher() {
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
						seekBar.setProgress(value - InputDialog.this.minValue);
					} catch (Exception ignored) {
					}
				}
			});
		}
		setView(layout);
	}

	@Override
	protected void onNegativeButtonClick() {
		cancel();
		handler.onCancel();
	}

	@Override
	protected void onPositiveButtonClick() {
		String value = input.getText().toString().trim();
		try {
			if (handler.validate(value))
				handler.onOk(value);
			else
				handler.onCancel();
		} catch (Exception e) {
			handler.onCancel();
		}
		cancel();
	}
}
