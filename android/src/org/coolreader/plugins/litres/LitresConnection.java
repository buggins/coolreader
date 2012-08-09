package org.coolreader.plugins.litres;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.Serializable;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLConnection;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

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
import org.xml.sax.Attributes;
import org.xml.sax.SAXException;

import android.os.Handler;
import android.util.Log;

public class LitresConnection {
	final static String TAG = "litres";
	
	public static final String AUTHORIZE_URL = "http://robot.litres.ru/pages/catalit_authorise/";
	public static final String GENRES_URL = "http://robot.litres.ru/pages/catalit_genres/";
	public static final String AUTHORS_URL = "http://robot.litres.ru/pages/catalit_persons/";
	public static final String CATALOG_URL = "http://robot.litres.ru/pages/catalit_browser/";
	
	ServiceThread workerThread;
	
	private LitresConnection() {
		workerThread = new ServiceThread("litres");
		workerThread.start();
	}
	
	public static LitresConnection create () {
		return new LitresConnection();
	}

	public interface ResultHandler {
		void onResponse(LitresResponse response);
	}
	
    public static final int CONNECT_TIMEOUT = 60000;
    public static final int READ_TIMEOUT = 60000;
    public static final int MAX_CONTENT_LEN_TO_BUFFER = 1000000;
	public void sendRequest(final String url, final Map<String, String> params, final ResponseHandler contentHandler, final ResultHandler resultHandler) {
		Log.i(TAG, "sending request to " + url);
		final Handler callbackHandler = new Handler();
		workerThread.post(new Runnable() {
			@Override
			public void run() {
				HttpURLConnection connection = null;
				try {
					URL u = new URL(url);
					URLConnection conn = null;
					try {
						conn = u.openConnection();
					} catch (IOException e) {
						contentHandler.onError(0, "Cannot open connection");
						return;
					}
					if ( conn instanceof HttpsURLConnection ) {
						contentHandler.onError(0, "HTTPs is not supported yet");
						return;
					}
					if ( !(conn instanceof HttpURLConnection) ) {
						contentHandler.onError(0, "Only HTTP supported");
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
						contentHandler.onError(response, "Error " + response + " " + connection.getResponseMessage());
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
						contentHandler.onError(0, "Wrong content length");
						return;
					}
					
					InputStream is = connection.getInputStream();
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
	
	public static class LitresGenre extends LitresResponse implements Serializable {
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
		sendRequest(GENRES_URL, params, new ResponseHandler() {
			LitresGenre result = new LitresGenre();
			LitresGenre currentNode = result;
			@Override
			public LitresResponse getResponse() {
				LitresResponse res =  super.getResponse();
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

	public static class LitresAuthor {
		public String id;
		public String firstName;
		public String lastName;
		public String middleName;
		public String title;
		public String photo;
		@Override
		public String toString() {
			return "LitresAuthor [id=" + id + ", lastName=" + lastName
					+ ", firstName=" + firstName + ", middleName=" + middleName
					+ ", title=" + title + ", photo=" + photo + "]";
		}
	}
	public static class LitresAuthors extends LitresResponse {
		private ArrayList<LitresAuthor> list = new ArrayList<LitresAuthor>();
		public void add(LitresAuthor author) {
			list.add(author);
		}
		public int size() {
			return list.size();
		}
		public LitresAuthor get(int index) {
			return list.get(index);
		}
	}

	public void loadAuthorsByLastName(String lastNamePattern, final ResultHandler resultHandler) {
		Map<String, String> params = new HashMap<String, String>();
		params.put("search_last_name", lastNamePattern + "%");
		loadAuthors(params, resultHandler);
	}

	public void loadAuthors(final Map<String, String> params, final ResultHandler resultHandler) {
		params.put("search_types", "0");
		sendRequest(AUTHORS_URL, params, new ResponseHandler() {
			LitresAuthors result = new LitresAuthors();
			LitresAuthor currentNode;
			boolean insideCatalitPersons;
			String currentElement;
			@Override
			public LitresResponse getResponse() {
				LitresResponse res =  super.getResponse();
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
					if (currentNode.id != null && currentNode.lastName != null)
						result.add(currentNode);
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
					currentNode = new LitresAuthor();
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

	public static class LitresBook {
		public LitresAuthors authors = new LitresAuthors();
		public String bookTitle;
		public String id;
		public double basePrice;
		public double price;
		public int zipSize;
		public boolean hasTrial;
		public String trialId;
		public String cover;
		public String coverPreview;
		public int rating;
		public String sequenceName;
		public int sequenceNumber;
		public String getAuthors() {
			StringBuilder buf = new StringBuilder();
			for (int i=0; i <authors.size(); i++) {
				LitresAuthor author = authors.get(i);
				if (buf.length() > 0)
					buf.append("|");
				String name = Utils.concatWs(author.firstName, author.lastName, " ");
				if (name.length() == 0)
					name = author.title;
				buf.append(name);
			}
			return buf.toString();
		}
	}

	public static class LitresBooks extends LitresResponse {
		public double account;
		public int pages;
		public int records;
		private ArrayList<LitresBook> list = new ArrayList<LitresBook>();
		public void add(LitresBook item) {
			list.add(item);
		}
		public int size() {
			return list.size();
		}
		public LitresBook get(int index) {
			return list.get(index);
		}
	}

	public void loadBooks(final Map<String, String> params, final ResultHandler resultHandler) {
		params.put("search_types", "0");
		if (lastSid != null)
			params.put("sid", lastSid);
		sendRequest(CATALOG_URL, params, new ResponseHandler() {
			LitresBooks result = new LitresBooks();
			LitresBook currentNode;
			LitresAuthor currentAuthor;
			boolean insideCatalitBooks;
			boolean insideTitleInfo;
			boolean insideAuthor;
			String currentElement;
			@Override
			public LitresResponse getResponse() {
				LitresResponse res =  super.getResponse();
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
					if (currentNode.id != null)
						result.add(currentNode);
					currentNode = null;
				}
			}

			@Override
			public void startElement(String uri, String localName,
					String qName, Attributes attributes) throws SAXException {
				//Log.d(TAG, "startElement " + localName);
				if ("catalit-fb2-books".equals(localName))
					insideCatalitBooks = true;
				else if ("title-info".equals(localName) && insideCatalitBooks)
					insideTitleInfo = true;
				else if ("author".equals(localName) && insideTitleInfo) {
					insideAuthor = true;
					currentAuthor = new LitresAuthor();
				} else if ("fb2-book".equals(localName)) {
					if (!insideCatalitBooks)
						return;
					currentNode = new LitresBook();
					currentNode.id = attributes.getValue("hub_id");
					currentNode.hasTrial = stringToInt(attributes.getValue("hub_id"), 0) != 0;
					currentNode.trialId = attributes.getValue("trial_id");
					currentNode.rating = stringToInt(attributes.getValue("rating"), 0);
					currentNode.zipSize = stringToInt(attributes.getValue("zip_size"), 0);
					currentNode.basePrice = stringToDouble(attributes.getValue("base_price"), 0);
					currentNode.price = stringToDouble(attributes.getValue("price"), 0);
					currentNode.cover = attributes.getValue("cover");
					currentNode.coverPreview = attributes.getValue("cover_preview");
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

	public void loadBooksByGenre(String genreId, int offset, int maxCount, final ResultHandler resultHandler) {
		final Map<String, String> params = new HashMap<String, String>();
		params.put("genre", genreId);
		params.put("limit", "" + offset + "," + maxCount);
		loadBooks(params, resultHandler);
	}

	public static class LitresAuthInfo extends LitresResponse {
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
		sendRequest(AUTHORIZE_URL, params, new ResponseHandler() {
			LitresAuthInfo result;
			@Override
			public LitresResponse getResponse() {
				LitresResponse res =  super.getResponse();
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
				} else if ("catalit-authorization-failed".equals(localName)) {
					onError(1, "Authorization failed");
				}
			}
		}, resultHandler);
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
	
	private static LitresConnection instance;
	public static LitresConnection instance() {
		if (instance == null)
			instance = new LitresConnection();
		return instance;
	}
}
