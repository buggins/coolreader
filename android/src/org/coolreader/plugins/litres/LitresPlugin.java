package org.coolreader.plugins.litres;

import org.coolreader.crengine.FileInfo;
import org.coolreader.crengine.Scanner;
import org.coolreader.plugins.AsyncOperationControl;
import org.coolreader.plugins.AuthenticationCallback;
import org.coolreader.plugins.FileInfoCallback;
import org.coolreader.plugins.OnlineStorePlugin;
import org.coolreader.plugins.litres.LitresConnection.ResultHandler;

public class LitresPlugin implements OnlineStorePlugin {

	public static final String PACKAGE_NAME = "org.coolreader.plugins.litres";
	
	private final LitresConnection connection = LitresConnection.create();
	
	@Override
	public String getPackageName() {
		return PACKAGE_NAME;
	}

	@Override
	public String getDescription() {
		return "LitRes";
	}

	
	@Override
	public void authenticate(final AsyncOperationControl control, String login, String password, final AuthenticationCallback callback) {
		connection.authorize(login, password, new ResultHandler() {
			@Override
			public void onResponse(LitresResponse response) {
				control.finished();
				if (response instanceof ErrorResponse) {
					ErrorResponse error = (ErrorResponse)response;
					callback.onError(error.errorCode, error.errorMessage);
				} else if (response instanceof LitresConnection.LitresAuthInfo) {
					LitresConnection.LitresAuthInfo result = (LitresConnection.LitresAuthInfo)response;
					callback.onSuccess();
				}
			}
		});
	}

	@Override
	public String getLogin() {
		return connection.getLogin();
	}

	@Override
	public String getPassword() {
		return connection.getPassword();
	}

	private void addGenres(FileInfo dir, LitresConnection.LitresGenre genre) {
		for (int i=0; i<genre.getChildCount(); i++) {
			LitresConnection.LitresGenre item = genre.get(i);
			FileInfo subdir = Scanner.createOnlineLibraryPluginItem(PACKAGE_NAME + ":genre=" + item.id, item.title);
			dir.addDir(subdir);
		}
	}
	
	@Override
	public void fillGenres(final AsyncOperationControl control, final FileInfo dir,
			final FileInfoCallback callback) {
		dir.clear();
		connection.loadGenres(new ResultHandler() {
			@Override
			public void onResponse(LitresResponse response) {
				control.finished();
				if (response instanceof ErrorResponse) {
					ErrorResponse error = (ErrorResponse)response;
					callback.onError(error.errorCode, error.errorMessage);
				} else if (response instanceof LitresConnection.LitresGenre) {
					LitresConnection.LitresGenre result = (LitresConnection.LitresGenre)response;
					addGenres(dir, result);
					callback.onFileInfoReady(dir);
				}
			}
		});
	}

}
