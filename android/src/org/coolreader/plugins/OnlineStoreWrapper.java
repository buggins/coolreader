package org.coolreader.plugins;

import java.io.File;

import org.coolreader.crengine.FileInfo;
import org.coolreader.crengine.Scanner;

public class OnlineStoreWrapper {
	private OnlineStorePlugin plugin;
	public OnlineStoreWrapper(OnlineStorePlugin plugin) {
		this.plugin = plugin;
	}
	public FileInfo createRootDirectory() {
		final FileInfo root = Scanner.createOnlineLibraryPluginItem(plugin.getPackageName(), plugin.getDescription());
		root.addDir(Scanner.createOnlineLibraryPluginItem(plugin.getPackageName() + ":genres", "Books by genres"));
		FileInfo authors = Scanner.createOnlineLibraryPluginItem(plugin.getPackageName() + ":authors", "Books by authors");
		root.addDir(authors);
		String firstLetters = plugin.getFirstAuthorNameLetters();
		for (char ch : firstLetters.toCharArray()) {
			authors.addDir(Scanner.createOnlineLibraryPluginItem(plugin.getPackageName() + ":authors=" + ch, ("" + ch).toUpperCase()));
		}
		root.addDir(Scanner.createOnlineLibraryPluginItem(plugin.getPackageName() + ":my", "My books"));
		root.addDir(Scanner.createOnlineLibraryPluginItem(plugin.getPackageName() + ":popular", "Popular"));
		root.addDir(Scanner.createOnlineLibraryPluginItem(plugin.getPackageName() + ":new", "Hot new"));
		return root;
	}
	public AsyncOperationControl openDirectory(final FileInfo dir, final FileInfoCallback callback) {
		AsyncOperationControl control = new AsyncOperationControl();
		if (!plugin.getPackageName().equals(dir.getOnlineCatalogPluginPackage())) {
			control.finished();
			callback.onError(0, "wrong plugin");
			return control;
		}
		String path = dir.getOnlineCatalogPluginPath();
		if (path == null) {
			control.finished();
			callback.onError(0, "wrong path");
			return control;
		}
		if ("genres".equals(path)) {
			plugin.fillGenres(control, dir, callback);
			control.finished();
			return control;
		} else if (path.startsWith("genre=")) {
			String genre = dir.getOnlineCatalogPluginId();
			plugin.getBooksForGenre(control, dir, genre, callback);
			control.finished();
			return control;
		} else if (path.startsWith("authors=")) {
			String prefix = dir.getOnlineCatalogPluginId();
			plugin.getAuthorsByPrefix(control, dir, prefix, callback);
			control.finished();
			return control;
		} else if (path.startsWith("author=")) {
			String authorId = dir.getOnlineCatalogPluginId();
			plugin.getBooksByAuthor(control, dir, authorId, callback);
			control.finished();
			return control;
		} else if ("my".equals(path)) {
			plugin.getPurchasedBooks(control, dir, callback);
			return control;
		} else if ("new".equals(path)) {
			plugin.getNewBooks(control, dir, callback);
			return control;
		} else if ("popular".equals(path)) {
			plugin.getPopularBooks(control, dir, callback);
			return control;
		} else {
			
		}
			
		control.finished();
		callback.onFileInfoReady(dir);
		return control;
	}
	public AsyncOperationControl authenticate(String login, String password, AuthenticationCallback callback) {
		AsyncOperationControl control = new AsyncOperationControl();
		plugin.authenticate(control, login, password, callback);
		return control;
	}
	private void loadBookInfoContinue(final AsyncOperationControl control, final String bookId, final boolean isBought, final BookInfoCallback callback) {
		plugin.getBookInfo(control, bookId, false, new BookInfoCallback() {
			@Override
			public void onError(int errorCode, String errorMessage) {
				callback.onError(errorCode, errorMessage);
			}
			@Override
			public void onBookInfoReady(OnlineStoreBookInfo bookInfo) {
				bookInfo.isPurchased = isBought;
				callback.onBookInfoReady(bookInfo);
			}
		});
	}
	private void loadBookInfoSkipAuth(final AsyncOperationControl control, final String bookId, final BookInfoCallback callback) {
		if (plugin.getLogin() == null)
			loadBookInfoContinue(control, bookId, false, callback);
		else
			plugin.getBookInfo(control, bookId, true, new BookInfoCallback() {
				@Override
				public void onError(int errorCode, String errorMessage) {
					loadBookInfoContinue(control, bookId, false, callback);
				}
				
				@Override
				public void onBookInfoReady(OnlineStoreBookInfo bookInfo) {
					loadBookInfoContinue(control, bookId, true, callback);
				}
			});
	}
	public AsyncOperationControl loadBookInfo(final String bookId, final BookInfoCallback callback) {
		final AsyncOperationControl control = new AsyncOperationControl();
		String login = plugin.getLogin();
		String password = plugin.getPassword();
		if (login != null && password != null) {
			plugin.authenticate(control, login, password, new AuthenticationCallback() {
				@Override
				public void onError(int errorCode, String errorMessage) {
					loadBookInfoSkipAuth(control, bookId, callback);
				}
				
				@Override
				public void onSuccess() {
					loadBookInfoSkipAuth(control, bookId, callback);
				}
			});
		} else
			loadBookInfoSkipAuth(control, bookId, callback);
		return control;
	}

	public AsyncOperationControl purchaseBook(final String bookId, final PurchaseBookCallback callback) {
		final AsyncOperationControl control = new AsyncOperationControl();
		plugin.purchaseBook(control, bookId, callback);
		return control;
	}

	public AsyncOperationControl downloadBook(OnlineStoreBook book, boolean trial, File fileToSave, DownloadBookCallback callback) {
		final AsyncOperationControl control = new AsyncOperationControl();
		plugin.downloadBook(control, book, trial, fileToSave, callback);
		return control;
	}

	public String getLogin() {
		return plugin.getLogin();
	}

	public String getPassword() {
		return plugin.getPassword();
	}

	public String getDescription() {
		return plugin.getDescription();
	}
}
