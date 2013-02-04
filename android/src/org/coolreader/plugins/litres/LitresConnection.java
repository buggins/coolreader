package org.coolreader.plugins.litres;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLConnection;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.zip.GZIPInputStream;

import javax.net.ssl.HttpsURLConnection;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.parsers.SAXParser;
import javax.xml.parsers.SAXParserFactory;

import org.apache.http.NameValuePair;
import org.apache.http.client.entity.UrlEncodedFormEntity;
import org.apache.http.message.BasicNameValuePair;
import org.coolreader.crengine.L;
import org.coolreader.crengine.Utils;
import org.coolreader.db.ServiceThread;
import org.coolreader.plugins.AsyncResponse;
import org.coolreader.plugins.ErrorResponse;
import org.coolreader.plugins.FileResponse;
import org.coolreader.plugins.OnlineStoreAuthor;
import org.coolreader.plugins.OnlineStoreAuthors;
import org.coolreader.plugins.OnlineStoreBook;
import org.coolreader.plugins.OnlineStoreBooks;
import org.xml.sax.Attributes;
import org.xml.sax.SAXException;

import android.content.SharedPreferences;
import android.os.Handler;
import android.util.Log;

public class LitresConnection {
	final static String TAG = "litres";
	
	public static final String AUTHORIZE_URL = "http://robot.litres.ru/pages/catalit_authorise/";
	public static final String GENRES_URL = "http://robot.litres.ru/pages/catalit_genres/";
	public static final String AUTHORS_URL = "http://robot.litres.ru/pages/catalit_persons/";
	public static final String CATALOG_URL = "http://robot.litres.ru/pages/catalit_browser/";
	public static final String TRIALS_URL = "http://robot.litres.ru/static/trials/";
	public static final String PURCHASE_URL = "http://robot.litres.ru/pages/purchase_book/";
	public static final String DOWNLOAD_BOOK_URL = "http://robot.litres.ru/pages/catalit_download_book/";
	public static final String P_ID = "8786915";
	
	ServiceThread workerThread;
	
	SharedPreferences preferences;
	private LitresConnection(SharedPreferences preferences) {
		workerThread = new ServiceThread("litres");
		workerThread.start();
		this.preferences = preferences;
		restorLoginInfo();
	}
	
	public static LitresConnection create (SharedPreferences preferences) {
		return new LitresConnection(preferences);
	}

	public interface ResultHandler {
		void onResponse(AsyncResponse response);
	}
	
    private static final int CONNECT_TIMEOUT = 60000;
    private static final int READ_TIMEOUT = 60000;
    private static final int MAX_CONTENT_LEN_TO_BUFFER = 1000000;
	public void sendXMLRequest(final String url, final Map<String, String> params, final ResponseHandler contentHandler, final ResultHandler resultHandler) {
		Log.i(TAG, "sending request to " + url);
		final Handler callbackHandler = new Handler();
		workerThread.post(new Runnable() {
			void onError(int errorCode, String errorMessage) {
				contentHandler.onError(errorCode, errorMessage);
				callbackHandler.post(new Runnable() {
					@Override
					public void run() {
						resultHandler.onResponse(contentHandler.getResponse());
					}
				});
			}
			
			@Override
			public void run() {
				HttpURLConnection connection = null;
				try {
					URL u = new URL(url);
					URLConnection conn = null;
					try {
						conn = u.openConnection();
					} catch (IOException e) {
						onError(0, "Cannot open connection");
						return;
					}
					if ( conn instanceof HttpsURLConnection ) {
						onError(0, "HTTPs is not supported yet");
						return;
					}
					if ( !(conn instanceof HttpURLConnection) ) {
						onError(0, "Only HTTP supported");
						return;
					}
					connection = (HttpURLConnection)conn;
					Log.i(TAG, "opened connection");
		            connection.setRequestProperty("User-Agent", "CoolReader/3(Android)");
		            connection.setInstanceFollowRedirects(true);
		            connection.setAllowUserInteraction(false);
		            connection.setConnectTimeout(CONNECT_TIMEOUT);
		            connection.setReadTimeout(READ_TIMEOUT);
		            //connection.setDoInput(true);
            		connection.setDoOutput(true);
		            connection.setRequestMethod("POST");
		            
		            List<NameValuePair> list = new LinkedList<NameValuePair>();
		            for (Map.Entry<String, String> entry : params.entrySet())
		            	list.add(new BasicNameValuePair(entry.getKey(), entry.getValue()));
		            UrlEncodedFormEntity postParams = new UrlEncodedFormEntity(list, "utf-8");
					//Log.d(TAG, "params: " + postParams.toString());
					OutputStream wr = connection.getOutputStream();
					//OutputStreamWriter wr = new OutputStreamWriter(connection.getOutputStream());
					postParams.writeTo(wr);
                    //wr.write(postParams.toString());
					wr.flush();
					wr.close();
					
		            String fileName = null;
		            String disp = connection.getHeaderField("Content-Disposition");
		            if ( disp!=null ) {
		            	int p = disp.indexOf("filename=");
		            	if ( p>0 ) {
		            		fileName = disp.substring(p + 9);
		            	}
		            }
		            //connection.setDoOutput(true);
		            //connection.set
		            
		            int response = -1;
					
					response = connection.getResponseCode();
					L.d("Response: " + response);
					if (response != 200) {
						onError(response, "Error " + response + " " + connection.getResponseMessage());
						return;
					}
					String contentType = connection.getContentType();
					String contentEncoding = connection.getContentEncoding();
					int contentLen = connection.getContentLength();
					//connection.getC
					L.d("Entity content length: " + contentLen);
					L.d("Entity content type: " + contentType);
					L.d("Entity content encoding: " + contentEncoding);

					if (contentLen <= 0 || contentLen > MAX_CONTENT_LEN_TO_BUFFER) {
						onError(0, "Wrong content length");
						return;
					}
					
					InputStream is = connection.getInputStream();

					if ("gzip".equals(contentEncoding)) {
						L.d("Stream is compressed with GZIP");
						is = new GZIPInputStream(new BufferedInputStream(is, 8192));
					}
					
					//					byte[] buf = new byte[contentLen];
//					if (is.read(buf) != contentLen) {
//						contentHandler.onError(0, "Wrong content length");
//						return;
//					}
//					is.close();
//					is = null;
//					is = new ByteArrayInputStream(buf);
					
					SAXParserFactory spf = SAXParserFactory.newInstance();
					spf.setValidating(false);
//					spf.setNamespaceAware(true);
//					spf.setFeature("http://xml.org/sax/features/namespaces", false);
					SAXParser sp = spf.newSAXParser();
					//XMLReader xr = sp.getXMLReader();				
					sp.parse(is, contentHandler);
					is.close();
					
				} catch (ParserConfigurationException e) {
					contentHandler.onError(0, "Error while parsing response");
				} catch (SAXException e) {
					contentHandler.onError(0, "Error while parsing response");
				} catch (IOException e) {
					contentHandler.onError(0, "Error while accessing litres server");
				} finally {
					if ( connection!=null ) {
						try {
							connection.disconnect();
						} catch ( Exception e ) {
							// ignore
						}
					}
				}
				callbackHandler.post(new Runnable() {
					@Override
					public void run() {
						resultHandler.onResponse(contentHandler.getResponse());
					}
				});
			}
		});
	}
	
	public void sendFileRequest(final String url, final Map<String, String> params, final File fileToStore, final FileResponse contentHandler, final ResultHandler resultHandler) {
		Log.i(TAG, "sending request to " + url);
		final Handler callbackHandler = new Handler();
		workerThread.post(new Runnable() {
			void onError(int errorCode, String errorMessage) {
				contentHandler.onError(errorCode, errorMessage);
				callbackHandler.post(new Runnable() {
					@Override
					public void run() {
						resultHandler.onResponse(contentHandler.getResponse());
					}
				});
			}
			@Override
			public void run() {
				HttpURLConnection connection = null;
				try {
					URL u = new URL(url);
					URLConnection conn = null;
					try {
						conn = u.openConnection();
					} catch (IOException e) {
						onError(0, "Cannot open connection");
						return;
					}
					if ( conn instanceof HttpsURLConnection ) {
						onError(0, "HTTPs is not supported yet");
						return;
					}
					if ( !(conn instanceof HttpURLConnection) ) {
						onError(0, "Only HTTP supported");
						return;
					}
					connection = (HttpURLConnection)conn;
					Log.i(TAG, "opened connection");
		            connection.setRequestProperty("User-Agent", "CoolReader/3(Android)");
		            connection.setInstanceFollowRedirects(true);
		            connection.setAllowUserInteraction(false);
		            connection.setConnectTimeout(CONNECT_TIMEOUT);
		            connection.setReadTimeout(READ_TIMEOUT);
		            if (params != null) {
	            		connection.setDoOutput(true);
			            connection.setRequestMethod("POST");
			            List<NameValuePair> list = new LinkedList<NameValuePair>();
			            for (Map.Entry<String, String> entry : params.entrySet())
			            	list.add(new BasicNameValuePair(entry.getKey(), entry.getValue()));
			            UrlEncodedFormEntity postParams = new UrlEncodedFormEntity(list, "utf-8");
						//Log.d(TAG, "params: " + postParams.toString());
						OutputStream wr = connection.getOutputStream();
						//OutputStreamWriter wr = new OutputStreamWriter(connection.getOutputStream());
						postParams.writeTo(wr);
	                    //wr.write(postParams.toString());
						wr.flush();
						wr.close();
		            } else {
		            	//Log.d(TAG, "setting up GET method");
			            connection.setDoInput(true);
			            connection.setRequestMethod("GET");
	            		//connection.setDoOutput(true);
			            //connection.setRequestMethod("POST");
	            		connection.connect();
		            }
		            
		            int response = -1;
					
					response = connection.getResponseCode();
					L.d("Response: " + response);
					if (response != 200) {
						onError(response, "Error " + response + " " + connection.getResponseMessage());
						return;
					}
					String contentType = connection.getContentType();
					String contentEncoding = connection.getContentEncoding();
					int contentLen = connection.getContentLength();
					//connection.getC
					L.d("Entity content length: " + contentLen);
					L.d("Entity content type: " + contentType);
					L.d("Entity content encoding: " + contentEncoding);

					if (contentLen <= 0 || contentLen > MAX_CONTENT_LEN_TO_BUFFER) {
						onError(0, "Wrong content length");
						return;
					}
					
					InputStream is = null;
					OutputStream os = null;
					try {
						is = connection.getInputStream();

						if ("gzip".equals(contentEncoding)) {
							L.d("Stream is compressed with GZIP");
							is = new GZIPInputStream(new BufferedInputStream(is, 8192));
						}
						
						Log.i(TAG, "downloading file to " + contentHandler.fileToSave + "  contentLen=" + contentLen);
						os = new FileOutputStream(contentHandler.fileToSave);
						int bytesRead = Utils.copyStreamContent(os, is);
						Log.i(TAG, "downloaded bytes: " + bytesRead);
					} finally {
						try {
							if (os != null)
								os.close();
						} catch (IOException e) {
							// ignore
						}
						try {
							if (is != null)
								is.close();
						} catch (IOException e) {
							// ignore
						}
					}
				} catch (IOException e) {
					contentHandler.fileToSave.delete();
					contentHandler.onError(0, "Error while accessing litres server");
				} finally {
					if ( connection!=null ) {
						try {
							connection.disconnect();
						} catch ( Exception e ) {
							// ignore
						}
					}
				}
				callbackHandler.post(new Runnable() {
					@Override
					public void run() {
						resultHandler.onResponse(contentHandler.getResponse());
					}
				});
			}
		});
	}
	
	public static class LitresGenre implements AsyncResponse {
		@SuppressWarnings("unused")
		private static final long serialVersionUID = 1;
		public String id;
		public String title;
		public String token;
		private LitresGenre parent;
		private ArrayList<LitresGenre> children;
		public LitresGenre getParent() {
			return parent;
		}
		public void addChild(LitresGenre child) {
			if (children == null)
				children = new ArrayList<LitresGenre>();
			children.add(child);
			child.parent = this;
		}
		public int getChildCount() {
			return (children != null) ? children.size() : 0;
		}
		public LitresGenre get(int index) {
			return children.get(index);
		}
	}

	private LitresGenre genres;
	private long genresLastUpdateTimestamp;
	public void loadGenres(final ResultHandler resultHandler) {
		if (genres != null && System.currentTimeMillis() < genresLastUpdateTimestamp + 24*60*60*1000) {
			resultHandler.onResponse(genres);
			return;
		}
		final Map<String, String> params = new HashMap<String, String>();
		params.put("search_types", "0");
		sendXMLRequest(GENRES_URL, params, new ResponseHandler() {
			LitresGenre result = new LitresGenre();
			LitresGenre currentNode = result;
			@Override
			public AsyncResponse getResponse() {
				AsyncResponse res =  super.getResponse();
				if (res != null)
					return res;
				genres = result;
				genresLastUpdateTimestamp = System.currentTimeMillis();
				return result;
			}

			@Override
			public void endElement(String uri, String localName, String qName)
					throws SAXException {
				if ("catalit-genres".equals(localName))
					currentNode = null;
				else if ("genre".equals(localName)) {
					if (currentNode != null)
						currentNode = currentNode.getParent();
				}
			}

			@Override
			public void startElement(String uri, String localName,
					String qName, Attributes attributes) throws SAXException {
				if ("catalit-genres".equals(localName))
					currentNode = result;
				else if ("genre".equals(localName)) {
					if (currentNode == null)
						return;
					LitresGenre item = new LitresGenre();
					item.id = attributes.getValue("id");
					item.title = attributes.getValue("title");
					item.token = attributes.getValue("token");
					if (item.title != null) {
						currentNode.addChild(item);
						currentNode = item;
					}
				}
					
			}
		}, resultHandler);
	}

	public void loadAuthorsByLastName(String lastNamePattern, final ResultHandler resultHandler) {
		Map<String, String> params = new HashMap<String, String>();
		params.put("search_last_name", lastNamePattern + "%");
		loadAuthors(params, resultHandler);
	}

	public void loadAuthors(final Map<String, String> params, final ResultHandler resultHandler) {
		params.put("search_types", "0");
		sendXMLRequest(AUTHORS_URL, params, new ResponseHandler() {
			OnlineStoreAuthors result = new OnlineStoreAuthors();
			OnlineStoreAuthor currentNode;
			boolean insideCatalitPersons;
			String currentElement;
			@Override
			public AsyncResponse getResponse() {
				AsyncResponse res =  super.getResponse();
				if (res != null)
					return res;
				return result;
			}

			@Override
			public void endElement(String uri, String localName, String qName)
					throws SAXException {
				currentElement = null;
				if ("catalit-persons".equals(localName))
					insideCatalitPersons = false;
				else if ("subject".equals(localName)) {
					if (currentNode.id != null && currentNode.lastName != null) {
						if (currentNode.title == null)
							currentNode.title = Utils.concatWs(currentNode.firstName, currentNode.lastName, " ");
						result.add(currentNode);
					}
					currentNode = null;
				}
			}

			@Override
			public void startElement(String uri, String localName,
					String qName, Attributes attributes) throws SAXException {
				//Log.d(TAG, "startElement " + localName);
				if ("catalit-persons".equals(localName))
					insideCatalitPersons = true;
				else if ("subject".equals(localName)) {
					if (!insideCatalitPersons)
						return;
					currentNode = new OnlineStoreAuthor();
					currentNode.id = attributes.getValue("id");
				} else {
					currentElement = localName;
				}
					
			}

			@Override
			public void characters(char[] ch, int start, int length)
					throws SAXException {
				if (currentNode == null)
					return;
				String text = new String(ch, start, length);
				if ("last-name".equals(currentElement))
					currentNode.lastName = text;
				else if ("middle-name".equals(currentElement))
					currentNode.middleName = text;
				else if ("first-name".equals(currentElement))
					currentNode.firstName = text;
				else if ("photo".equals(currentElement))
					currentNode.photo = text;
				else if ("main".equals(currentElement))
					currentNode.title = text;
			}
			
			
		}, resultHandler);
	}
	
	public static String generateFileName(OnlineStoreBook book) {
		StringBuilder buf = new StringBuilder();
		buf.append(book.id);
		if (!Utils.empty(book.bookTitle)) {
			buf.append("-");
			buf.append(Utils.transcribeWithLimit(book.bookTitle, 20));
		}
		StringBuilder authors = new StringBuilder();
		for (int i = 0; i < book.authors.size(); i++) {
			OnlineStoreAuthor author = book.authors.get(i);
			String a = !Utils.empty(author.lastName) ? author.lastName : author.title;
			if (!Utils.empty(a)) {
				if (authors.length() > 0)
					authors.append("-");
				authors.append(a);
			}
		}
		if (authors.length() > 0) {
			buf.append("-");
			buf.append(Utils.transcribeWithLimit(authors.toString(), 30));
		}
		buf.append(".fb2.zip");
		return buf.toString();
	}

	public void loadBooks(final Map<String, String> params, final ResultHandler resultHandler) {
		params.put("search_types", "0");
		if (lastSid != null)
			params.put("sid", lastSid);
		params.put("checkpoint", "2000-01-01 00:00:00");
		sendXMLRequest(CATALOG_URL, params, new ResponseHandler() {
			OnlineStoreBooks result = new OnlineStoreBooks();
			OnlineStoreBook currentNode;
			OnlineStoreAuthor currentAuthor;
			boolean insideCatalitBooks;
			boolean insideTitleInfo;
			boolean insideAuthor;
			String currentElement;
			@Override
			public AsyncResponse getResponse() {
				AsyncResponse res =  super.getResponse();
				if (res != null)
					return res;
				return result;
			}

			@Override
			public void endElement(String uri, String localName, String qName)
					throws SAXException {
				currentElement = null;
				if ("catalit-fb2-books".equals(localName))
					insideCatalitBooks = false;
				else if ("title-info".equals(localName))
					insideTitleInfo = false;
				else if ("author".equals(localName)) {
					if (insideAuthor && currentAuthor != null && currentAuthor.id != null) {
						currentAuthor.title = Utils.concatWs(currentAuthor.firstName, currentAuthor.lastName, " ");
						currentNode.authors.add(currentAuthor);
					}
					insideAuthor = false;
					currentAuthor = null;
				} else if ("fb2-book".equals(localName)) {
					if (currentNode.id != null) {
						String fileName = generateFileName(currentNode);
						if (currentNode.hasTrial) {
							currentNode.trialFileName = "trial-" + fileName;
						}
						currentNode.downloadFileName = fileName;
						result.add(currentNode);
					}
					currentNode = null;
				}
			}

			@Override
			public void startElement(String uri, String localName,
					String qName, Attributes attributes) throws SAXException {
				//Log.d(TAG, "startElement " + localName);
				if ("catalit-authorization-failed".equals(localName)) {
					onError(25, "Authorization failed");
				} else if ("catalit-fb2-books".equals(localName)) {
					insideCatalitBooks = true;
					result.account = stringToDouble(attributes.getValue("account"), 0);
				} else if ("title-info".equals(localName) && insideCatalitBooks)
					insideTitleInfo = true;
				else if ("author".equals(localName) && insideTitleInfo) {
					insideAuthor = true;
					currentAuthor = new OnlineStoreAuthor();
				} else if ("fb2-book".equals(localName)) {
					if (!insideCatalitBooks)
						return;
					currentNode = new OnlineStoreBook();
					currentNode.id = attributes.getValue("hub_id");
					currentNode.hasTrial = stringToInt(attributes.getValue("has_trial"), 0) != 0;
					currentNode.rating = stringToInt(attributes.getValue("rating"), 0);
					currentNode.zipSize = stringToInt(attributes.getValue("zip_size"), 0);
					currentNode.basePrice = stringToDouble(attributes.getValue("base_price"), 0);
					currentNode.price = stringToDouble(attributes.getValue("price"), 0);
					currentNode.cover = attributes.getValue("cover");
					currentNode.coverPreview = attributes.getValue("cover_preview");
					if (currentNode.hasTrial && currentNode.id != null) {
						String trialId = currentNode.id;
						while (trialId.length() < 8)
							trialId = "0" + trialId;
						String path = trialId.substring(0, 2) + "/" + trialId.substring(2, 4) + "/" + trialId.substring(4, 6) + "/" + trialId + ".fb2.zip";
						currentNode.trialUrl = TRIALS_URL + path;
					}
				} else if ("sequence".equals(localName)) {
					if (currentNode == null)
						return;
					currentNode.sequenceName = attributes.getValue("name");
					currentNode.sequenceNumber = stringToInt(attributes.getValue("number"), 0);
				} else {
					currentElement = localName;
				}
					
			}

			@Override
			public void characters(char[] ch, int start, int length)
					throws SAXException {
				if (currentNode == null)
					return;
				String text = new String(ch, start, length);
				if (insideAuthor) {
					if ("id".equals(currentElement))
						currentAuthor.id = text;
					else if ("first-name".equals(currentElement))
						currentAuthor.firstName = text;
					else if ("last-name".equals(currentElement))
						currentAuthor.lastName = text;
					else if ("middle-name".equals(currentElement))
						currentAuthor.middleName = text;
					return;
				}
				if ("book-title".equals(currentElement))
					currentNode.bookTitle = text;
			}
			
			
		}, resultHandler);
	}

	public void loadPurchasedBooks(int offset, int maxCount, final ResultHandler resultHandler) {
		final Map<String, String> params = new HashMap<String, String>();
		params.put("my", "1");
		params.put("limit", "" + offset + "," + maxCount);
		loadBooks(params, resultHandler);
	}

	public void loadPopularBooks(int offset, int maxCount, final ResultHandler resultHandler) {
		final Map<String, String> params = new HashMap<String, String>();
		params.put("rating", "books");
		params.put("limit", "" + offset + "," + maxCount);
		loadBooks(params, resultHandler);
	}

	public void loadNewBooks(int offset, int maxCount, final ResultHandler resultHandler) {
		final Map<String, String> params = new HashMap<String, String>();
		params.put("rating", "hot");
		params.put("limit", "" + offset + "," + maxCount);
		loadBooks(params, resultHandler);
	}

	public void loadBooksByGenre(String genreId, int offset, int maxCount, final ResultHandler resultHandler) {
		final Map<String, String> params = new HashMap<String, String>();
		params.put("genre", genreId);
		params.put("limit", "" + offset + "," + maxCount);
		loadBooks(params, resultHandler);
	}

	public void loadBooksByBookId(String bookId, boolean myOnly, final ResultHandler resultHandler) {
		final Map<String, String> params = new HashMap<String, String>();
		params.put("art", bookId);
		if (myOnly)
			params.put("my", "1");
		loadBooks(params, resultHandler);
	}

	public void loadBooksByAuthor(String authorId, int offset, int maxCount, final ResultHandler resultHandler) {
		final Map<String, String> params = new HashMap<String, String>();
		params.put("person", authorId);
		params.put("limit", "" + offset + "," + maxCount);
		loadBooks(params, resultHandler);
	}

	public static class LitresAuthInfo implements AsyncResponse {
		public String sid;
		public String id;
		public String firstName;
		public String lastName;
		public String middleName;
		public String mail;
		public int bookCount;
		public int authorCount;
		public int userCount;
		public boolean canRebill;
		public String login;
		@Override
		public String toString() {
			return "LitresAuthInfo [id=" + id + ", sid=" + sid + ", login="
					+ login + ", lastName=" + lastName + ", firstName="
					+ firstName + ", middleName=" + middleName + ", bookCount=" 
					+ bookCount + ", authorCount="
					+ authorCount + ", userCount=" + userCount + ", canRebill="
					+ canRebill + "]";
		}
		
	}

	private long lastAuthorizationTimestamp;
	private String lastSid;
	private String lastLogin;
	private String lastPwd;
	private LitresAuthInfo authInfo;
	
	public final static long AUTHORIZATION_TIMEOUT = 3 * 60 * 1000; // 3 minutes
	public boolean authorizationValid() {
		return lastSid != null && (System.currentTimeMillis() < lastAuthorizationTimestamp + AUTHORIZATION_TIMEOUT);
	}

	public String getLogin() {
		return lastLogin;
	}

	public String getPassword() {
		return lastPwd;
	}

	public String getSID() {
		return lastSid;
	}

	public LitresAuthInfo getAuthInfo() {
		return authInfo;
	}

	private void saveLoginInfo(String login, String password) {
		if (login != null && password != null && preferences != null) {
			SharedPreferences.Editor editor = preferences.edit();
			editor.putString("litres.login", login);
			editor.putString("litres.password", password);
			editor.commit();
		}
	}

	private void restorLoginInfo() {
		if (preferences != null) {
			String l = preferences.getString("litres.login", null);
			String p = preferences.getString("litres.password", null);
			if (l != null && p != null) {
				lastLogin = l;
				lastPwd = p;
			}
		}
	}

	public void authorize(String login, String pwd, final ResultHandler resultHandler) {
		if (login == null)
			login = lastLogin;
		if (pwd == null)
			pwd = lastPwd;
		authorize(lastSid, login, pwd, resultHandler);
	}
	public void authorize(final String sid, final String login, final String pwd, final ResultHandler resultHandler) {
		final Map<String, String> params = new HashMap<String, String>();
		if (sid != null)
			params.put("sid", sid);
		if (login != null)
			params.put("login", login);
		if (pwd != null)
			params.put("pwd", pwd);
		sendXMLRequest(AUTHORIZE_URL, params, new ResponseHandler() {
			LitresAuthInfo result;
			@Override
			public AsyncResponse getResponse() {
				AsyncResponse res =  super.getResponse();
				if (res != null)
					return res;
				return result;
			}

			@Override
			public void startElement(String uri, String localName,
					String qName, Attributes attributes) throws SAXException {
				if ("catalit-authorization-ok".equals(localName)) {
					result = new LitresAuthInfo();
					result.sid = attributes.getValue("sid");
					result.id = attributes.getValue("user-id");
					result.firstName = attributes.getValue("first-name");
					result.lastName = attributes.getValue("last-name");
					result.middleName = attributes.getValue("middle-name");
					result.bookCount = stringToInt(attributes.getValue("books-cnt"), 0);
					result.authorCount = stringToInt(attributes.getValue("authors-cnt"), 0);
					result.userCount = stringToInt(attributes.getValue("users-cnt"), 0);
					result.canRebill = stringToInt(attributes.getValue("can-rebill"), 0) == 1;
					authInfo = result;
					if (pwd != null)
						lastPwd = pwd;
					if (login != null)
						lastLogin = login;
					result.login = lastLogin;
					lastSid = result.sid;
					lastAuthorizationTimestamp = System.currentTimeMillis();
					saveLoginInfo(login, pwd);
				} else if ("catalit-authorization-failed".equals(localName)) {
					onError(1, "Authorization failed");
				}
			}
		}, resultHandler);
	}

	public static class PurchaseStatus implements AsyncResponse {
		public String bookId;
		public double newBalance;
	}

	public void purchaseBook(final String bookId, final ResultHandler resultHandler) {
		final Map<String, String> params = new HashMap<String, String>();
		params.put("art", bookId);
		params.put("sid", lastSid);
		params.put("lfrom", P_ID);
		sendXMLRequest(PURCHASE_URL, params, new ResponseHandler() {
			PurchaseStatus result = new PurchaseStatus();
			@Override
			public AsyncResponse getResponse() {
				AsyncResponse res =  super.getResponse();
				if (res != null)
					return res;
				if (result.bookId != null)
					return result;
				return new ErrorResponse(0, "Unknown purchase error");
			}

			@Override
			public void startElement(String uri, String localName,
					String qName, Attributes attributes) throws SAXException {
				if ("catalit-purchase-ok".equals(localName)) {
					result.bookId = attributes.getValue("art");
					result.newBalance = stringToDouble(attributes.getValue("account"), 0.0);
				} else if ("catalit-purchase-failed".equals(localName)) {
					int errorCode = stringToInt(attributes.getValue("error"), 0);
					String comment = attributes.getValue("comment");
					if (comment == null)
						comment = attributes.getValue("coment"); // spelling error in API description
					onError(errorCode, comment);
				}
					
			}
		}, resultHandler);
	}

	public void downloadBook(final File fileToStore, final OnlineStoreBook book, boolean trial, final ResultHandler resultHandler) {
		Map<String, String> params = null;
		String url = null;
		if (trial) {
			url = book.trialUrl;
			Log.d(TAG, "trialUrl=" + url);
		} else {
			params = new HashMap<String, String>();
			url = DOWNLOAD_BOOK_URL;
			params.put("sid", lastSid);
			params.put("art", book.id);
		}
		FileResponse myResponse = new FileResponse(book, fileToStore, trial);
		sendFileRequest(url, params, fileToStore, myResponse, resultHandler);
	}

	
	public static int stringToInt(String v, int defValue) {
		if (v == null || v.length() == 0)
			return defValue;
		try {
			return Integer.valueOf(v);
		} catch (NumberFormatException e) {
			// ignore
			return defValue;
		}
	}

	public static double stringToDouble(String v, double defValue) {
		if (v == null || v.length() == 0)
			return defValue;
		try {
			return Double.valueOf(v);
		} catch (NumberFormatException e) {
			// ignore
			return defValue;
		}
	}
	
	
	public void close() {
		workerThread.stop(5000);
	}
}
