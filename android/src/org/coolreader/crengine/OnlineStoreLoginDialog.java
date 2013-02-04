package org.coolreader.crengine;

import org.coolreader.R;
import org.coolreader.plugins.AuthenticationCallback;
import org.coolreader.plugins.OnlineStoreWrapper;

import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.TextView;

public class OnlineStoreLoginDialog extends BaseDialog {
	private BaseActivity mActivity;
	private OnlineStoreWrapper mPlugin;
	private LayoutInflater mInflater;
	private Runnable mOnLoginHandler;
	public OnlineStoreLoginDialog(BaseActivity activity, OnlineStoreWrapper plugin, Runnable onLoginHandler)
	{
		super(activity, null, false, false);
		DisplayMetrics outMetrics = new DisplayMetrics();
		activity.getWindowManager().getDefaultDisplay().getMetrics(outMetrics);
		this.mActivity = activity;
		this.mPlugin = plugin;
		this.mOnLoginHandler = onLoginHandler;
	}

	@Override
	protected void onCreate() {
        setCancelable(true);
        setCanceledOnTouchOutside(true);
        super.onCreate();
	}

	
    TextView lblTitle;
    Button btnLogin;
    EditText edLogin;
    EditText edPassword;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

        mInflater = LayoutInflater.from(getContext());
        ViewGroup view = (ViewGroup)mInflater.inflate(R.layout.online_store_login_dialog, null);
        
        ImageButton btnBack = (ImageButton)view.findViewById(R.id.base_dlg_btn_back);
        btnBack.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				onNegativeButtonClick();
			}
		});
        btnLogin = (Button)view.findViewById(R.id.btn_login);
        btnLogin.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				onPositiveButtonClick();
			}
		});
        lblTitle = (TextView)view.findViewById(R.id.dlg_title);
        

		lblTitle.setText(mPlugin.getDescription());
		
        edLogin = (EditText)view.findViewById(R.id.ed_login);
        edPassword = (EditText)view.findViewById(R.id.ed_password);
        edLogin.setText(mPlugin.getLogin());
        edPassword.setText(mPlugin.getPassword());
		
        setView(view);
		progress = new ProgressPopup(mActivity, view);
	}
	
	private ProgressPopup progress;
	
	@Override
	protected void onPositiveButtonClick() {
		super.onPositiveButtonClick();
		String login = edLogin.getText().toString();
		String password = edPassword.getText().toString();
		progress.show();
		mPlugin.authenticate(login, password, new AuthenticationCallback() {
			@Override
			public void onError(int errorCode, String errorMessage) {
				progress.hide();
				mActivity.showToast("Cannot login: " + errorMessage);
			}
			@Override
			public void onSuccess() {
				progress.hide();
				mActivity.showToast("Successful login");
				mOnLoginHandler.run();
			}
		});
	}

	@Override
	protected void onNegativeButtonClick() {
		super.onNegativeButtonClick();
	}
}

