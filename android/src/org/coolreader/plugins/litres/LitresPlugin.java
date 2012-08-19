package org.coolreader.plugins.litres;

import java.io.File;

import org.coolreader.crengine.DocumentFormat;
import org.coolreader.crengine.FileInfo;
import org.coolreader.crengine.Scanner;
import org.coolreader.plugins.AsyncOperationControl;
import org.coolreader.plugins.AsyncResponse;
import org.coolreader.plugins.AuthenticationCallback;
import org.coolreader.plugins.BookInfoCallback;
import org.coolreader.plugins.DownloadBookCallback;
import org.coolreader.plugins.ErrorResponse;
import org.coolreader.plugins.FileInfoCallback;
import org.coolreader.plugins.FileResponse;
import org.coolreader.plugins.OnlineStoreAuthor;
import org.coolreader.plugins.OnlineStoreAuthors;
import org.coolreader.plugins.OnlineStoreBook;
import org.coolreader.plugins.OnlineStoreBookInfo;
import org.coolreader.plugins.OnlineStoreBooks;
import org.coolreader.plugins.OnlineStorePlugin;
import org.coolreader.plugins.PurchaseBookCallback;
import org.coolreader.plugins.litres.LitresConnection.LitresAuthInfo;
import org.coolreader.plugins.litres.LitresConnection.PurchaseStatus;
import org.coolreader.plugins.litres.LitresConnection.ResultHandler;

import android.content.SharedPreferences;

public class LitresPlugin implements OnlineStorePlugin {

	public static final String PACKAGE_NAME = "org.coolreader.plugins.litres";
	
	private final LitresConnection connection;
	public LitresPlugin(SharedPreferences preferences) {
		connection = LitresConnection.create(preferences);
	}
	
	
	
	@Override
	public String getFirstAuthorNameLetters() {
		return "абвгдежзийклмнопрстуфхцчшщыэюя";
	}

	
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
			public void onResponse(AsyncResponse response) {
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
			String basePath = PACKAGE_NAME + ":genre";
			String path = basePath;
			boolean isLinkWithChildren = (item.id != null) && item.getChildCount() > 0;
			if (item.id != null)
				path = path + "=" + item.id;
			FileInfo subdir = Scanner.createOnlineLibraryPluginItem(isLinkWithChildren ? basePath : path, item.title);
			dir.addDir(subdir);
			if (item.getChildCount() > 0) {
				if (isLinkWithChildren) {
					FileInfo subdir2 = Scanner.createOnlineLibraryPluginItem(path, item.title);
					dir.addDir(subdir2);
				}
				addGenres(subdir, item);
			}
		}
	}
	
	@Override
	public void fillGenres(final AsyncOperationControl control, final FileInfo dir,
			final FileInfoCallback callback) {
		//dir.clear();
		connection.loadGenres(new ResultHandler() {
			@Override
			public void onResponse(AsyncResponse response) {
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
	
	private void addBookFileInfo(final FileInfo dir, OnlineStoreBook book) {
		FileInfo fileInfo = new FileInfo();
		fileInfo.authors = book.getAuthors();
		fileInfo.title = book.bookTitle;
		fileInfo.size = book.zipSize;
		fileInfo.format = DocumentFormat.FB2;
		fileInfo.series = book.sequenceName;
		fileInfo.seriesNumber = book.sequenceNumber;
		fileInfo.tag = book;
		fileInfo.pathname = FileInfo.ONLINE_CATALOG_PLUGIN_PREFIX + PACKAGE_NAME + ":book=" + book.id;
		dir.addFile(fileInfo);
		fileInfo.parent = dir;
	}

	private void addBooks(FileInfo dir, OnlineStoreBooks books) {
		books.sortBySeriesAndTitle();
		if (books.size() < BOOK_LIST_GROUP_BY_SERIES_THRESHOLD) {
			for (int i=0; i < books.size(); i++) {
				addBookFileInfo(dir, books.get(i));
			}
		} else {
			String lastSeries = null;
			FileInfo group = null;
			for (int i=0; i < books.size(); i++) {
				OnlineStoreBook book = books.get(i);
				if (book.sequenceName != null) {
					if (group == null || !lastSeries.equals(book.sequenceName)) {
						group = Scanner.createOnlineLibraryPluginItem(PACKAGE_NAME + ":series=" + book.sequenceName, book.sequenceName);
						lastSeries = book.sequenceName;
						dir.addDir(group);
					}
					addBookFileInfo(group, book);
				} else {
					addBookFileInfo(dir, book);
				}
			}
		}
	}
	
	final int BOOK_LIST_GROUP_BY_SERIES_THRESHOLD = 20;
	final int BOOK_LOAD_PAGE_SIZE_AUTHOR = 2000;
	@Override
	public void getBooksByAuthor(final AsyncOperationControl control, final FileInfo dir, final String authorId, final FileInfoCallback callback) {
		connection.loadBooksByAuthor(authorId, dir.fileCount(), BOOK_LOAD_PAGE_SIZE_AUTHOR, new ResultHandler() {
			@Override
			public void onResponse(AsyncResponse response) {
				control.finished();
				if (response instanceof ErrorResponse) {
					ErrorResponse error = (ErrorResponse)response;
					callback.onError(error.errorCode, error.errorMessage);
				} else if (response instanceof OnlineStoreBooks) {
					OnlineStoreBooks result = (OnlineStoreBooks)response;
					addBooks(dir, result);
					callback.onFileInfoReady(dir);
				}
			}
		});
	}
	
	final int BOOK_LOAD_PAGE_SIZE_MY = 2000;
	@Override
	public void getPurchasedBooks(final AsyncOperationControl control, final FileInfo dir, final FileInfoCallback callback) {
		connection.loadPurchasedBooks(dir.fileCount(), BOOK_LOAD_PAGE_SIZE_MY, new ResultHandler() {
			@Override
			public void onResponse(AsyncResponse response) {
				control.finished();
				if (response instanceof ErrorResponse) {
					ErrorResponse error = (ErrorResponse)response;
					callback.onError(error.errorCode, error.errorMessage);
				} else if (response instanceof OnlineStoreBooks) {
					OnlineStoreBooks result = (OnlineStoreBooks)response;
					for (int i=0; i < result.size(); i++) {
						addBookFileInfo(dir, result.get(i));
					}
					callback.onFileInfoReady(dir);
				}
			}
		});
	}
	
	final int BOOK_LOAD_PAGE_SIZE = 200;
	@Override
	public void getBooksForGenre(final AsyncOperationControl control, final FileInfo dir, final String genreId, final FileInfoCallback callback) {
		connection.loadBooksByGenre(genreId, dir.fileCount(), BOOK_LOAD_PAGE_SIZE, new ResultHandler() {
			@Override
			public void onResponse(AsyncResponse response) {
				control.finished();
				if (response instanceof ErrorResponse) {
					ErrorResponse error = (ErrorResponse)response;
					callback.onError(error.errorCode, error.errorMessage);
				} else if (response instanceof OnlineStoreBooks) {
					OnlineStoreBooks result = (OnlineStoreBooks)response;
					for (int i=0; i < result.size(); i++) {
						addBookFileInfo(dir, result.get(i));
					}
					callback.onFileInfoReady(dir);
				}
			}
		});
	}

	private void addAuthor(final FileInfo dir, OnlineStoreAuthor author) {
		FileInfo fileInfo = Scanner.createOnlineLibraryPluginItem(PACKAGE_NAME + ":author=" + author.id, author.title);
		fileInfo.tag = author;
		dir.addDir(fileInfo);
		fileInfo.parent = dir;
	}

	private void addAuthorsGroupedByPrefix(final FileInfo dir, OnlineStoreAuthors authors, int prefixLen) {
		String lastPrefix = "";
		FileInfo group = null;
		for (int i=0; i<authors.size(); i++) {
			OnlineStoreAuthor author = authors.get(i);
			String prefix = author.getPrefix(prefixLen);
			if (group == null || !lastPrefix.equals(prefix)) {
				lastPrefix = prefix;
				group = Scanner.createOnlineLibraryPluginItem(PACKAGE_NAME + ":authors=" + prefix, prefix);
				dir.addDir(group);
				group.parent = dir;
			}
			addAuthor(group, author);
		}
	}

	final int AUTHOR_GROUPING_THRESHOLD = 100;
	@Override
	public void getAuthorsByPrefix(final AsyncOperationControl control, final FileInfo dir, final String prefix, final FileInfoCallback callback) {
		connection.loadAuthorsByLastName(prefix, new ResultHandler() {
			@Override
			public void onResponse(AsyncResponse response) {
				control.finished();
				if (response instanceof ErrorResponse) {
					ErrorResponse error = (ErrorResponse)response;
					callback.onError(error.errorCode, error.errorMessage);
				} else if (response instanceof OnlineStoreAuthors) {
					OnlineStoreAuthors result = (OnlineStoreAuthors)response;
					result.sortByName();
					if (result.size() > AUTHOR_GROUPING_THRESHOLD) {
						addAuthorsGroupedByPrefix(dir, result, prefix.length() + 1);
					} else {
						for (int i=0; i < result.size(); i++) {
							addAuthor(dir, result.get(i));
						}
					}
					callback.onFileInfoReady(dir);
				}
			}
		});
	}
	
	@Override
	public void getBookInfo(final AsyncOperationControl control, final String bookId, final boolean myOnly, final BookInfoCallback callback) {
		connection.loadBooksByBookId(bookId, myOnly, new ResultHandler() {
			@Override
			public void onResponse(AsyncResponse response) {
				control.finished();
				if (response instanceof ErrorResponse) {
					ErrorResponse error = (ErrorResponse)response;
					callback.onError(error.errorCode, error.errorMessage);
				} else if (response instanceof OnlineStoreBooks) {
					OnlineStoreBooks result = (OnlineStoreBooks)response;
					if (result.size() == 0)
						callback.onError(0, "not found");
					else {
						OnlineStoreBookInfo info = new OnlineStoreBookInfo();
						info.book = result.get(0);
						if (connection.getSID() != null)
							info.isLoggedIn = true;
						info.login = connection.getLogin();
						info.accountBalance = result.account;
						callback.onBookInfoReady(info);
					}
				}
			}
		});
	}

	final int BOOK_LOAD_PAGE_SIZE_POPULAR = 50;
	@Override
	public void getPopularBooks(final AsyncOperationControl control, final FileInfo dir, final FileInfoCallback callback) {
		connection.loadPopularBooks(dir.fileCount(), BOOK_LOAD_PAGE_SIZE_POPULAR, new ResultHandler() {
			@Override
			public void onResponse(AsyncResponse response) {
				control.finished();
				if (response instanceof ErrorResponse) {
					ErrorResponse error = (ErrorResponse)response;
					callback.onError(error.errorCode, error.errorMessage);
				} else if (response instanceof OnlineStoreBooks) {
					OnlineStoreBooks result = (OnlineStoreBooks)response;
					for (int i=0; i < result.size(); i++) {
						addBookFileInfo(dir, result.get(i));
					}
					callback.onFileInfoReady(dir);
				}
			}
		});
	}
	
	final int BOOK_LOAD_PAGE_SIZE_NEW = 50;
	@Override
	public void getNewBooks(final AsyncOperationControl control, final FileInfo dir, final FileInfoCallback callback) {
		connection.loadNewBooks(dir.fileCount(), BOOK_LOAD_PAGE_SIZE_NEW, new ResultHandler() {
			@Override
			public void onResponse(AsyncResponse response) {
				control.finished();
				if (response instanceof ErrorResponse) {
					ErrorResponse error = (ErrorResponse)response;
					callback.onError(error.errorCode, error.errorMessage);
				} else if (response instanceof OnlineStoreBooks) {
					OnlineStoreBooks result = (OnlineStoreBooks)response;
					for (int i=0; i < result.size(); i++) {
						addBookFileInfo(dir, result.get(i));
					}
					callback.onFileInfoReady(dir);
				}
			}
		});
	}

	protected void purchaseBookNoAuth(final AsyncOperationControl control, final String bookId, final PurchaseBookCallback callback) {
		connection.purchaseBook(bookId, new ResultHandler() {
			@Override
			public void onResponse(AsyncResponse response) {
				control.finished();
				if (response instanceof ErrorResponse) {
					ErrorResponse error = (ErrorResponse)response;
					callback.onError(error.errorCode, error.errorMessage);
				} else if (response instanceof PurchaseStatus) {
					PurchaseStatus result = (PurchaseStatus)response;
					callback.onBookPurchased(result.bookId, result.newBalance);
				}
			}
		});
	}

	@Override
	public void purchaseBook(final AsyncOperationControl control, final String bookId, final PurchaseBookCallback callback) {
		if (connection.getLogin() == null) {
			callback.onError(0, "Not logged in");
			return;
		}
		if (!connection.authorizationValid()) {
			connection.authorize(null, null, new ResultHandler() {
				@Override
				public void onResponse(AsyncResponse response) {
					control.finished();
					if (response instanceof ErrorResponse) {
						ErrorResponse error = (ErrorResponse)response;
						callback.onError(error.errorCode, error.errorMessage);
					} else if (response instanceof LitresAuthInfo) {
						purchaseBookNoAuth(control, bookId, callback);
					}
				}
			});
		} else {
			purchaseBookNoAuth(control, bookId, callback);
		}
	}

	private void downloadBookNoAuth(final AsyncOperationControl control, final OnlineStoreBook book, final boolean trial, final File fileToSave, final DownloadBookCallback callback) {
		connection.downloadBook(fileToSave, book, trial, new ResultHandler() {
			public void onResponse(AsyncResponse response) {
				if (response instanceof ErrorResponse) {
					ErrorResponse error = (ErrorResponse)response;
					callback.onError(error.errorCode, error.errorMessage);
				} else if (response instanceof FileResponse) {
					FileResponse result = (FileResponse)response;
					callback.onBookDownloaded(book, trial, fileToSave);
				}
			}
		});
	}

	@Override
	public void downloadBook(final AsyncOperationControl control, final OnlineStoreBook book, final boolean trial, final File fileToSave, final DownloadBookCallback callback) {
		if (trial && !book.hasTrial) {
			callback.onError(0, "No trial version");
			return;
		}
			
		if (!trial && connection.getLogin() == null) {
			callback.onError(0, "Not logged in");
			return;
		}
		if (!trial && !connection.authorizationValid()) {
			connection.authorize(null, null, new ResultHandler() {
				@Override
				public void onResponse(AsyncResponse response) {
					control.finished();
					if (response instanceof ErrorResponse) {
						ErrorResponse error = (ErrorResponse)response;
						callback.onError(error.errorCode, error.errorMessage);
					} else if (response instanceof LitresAuthInfo) {
						downloadBookNoAuth(control, book, trial, fileToSave, callback);
					}
				}
			});
		} else {
			downloadBookNoAuth(control, book, trial, fileToSave, callback);
		}
	}

}
