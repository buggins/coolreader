package org.coolreader.crengine;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLConnection;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Stack;
import java.util.concurrent.Callable;

import javax.net.ssl.HttpsURLConnection;

import org.coolreader.CoolReader;
import org.xml.sax.Attributes;
import org.xml.sax.SAXException;
import org.xml.sax.helpers.DefaultHandler;

import android.util.Log;
import android.util.Xml;
import android.util.Xml.Encoding;

public class OPDSUtil {

	/*
<?xml version="1.0" encoding="utf-8"?>
<feed xmlns:opensearch="http://a9.com/-/spec/opensearch/1.1/" xmlns:relevance="http://a9.com/-/opensearch/extensions/relevance/1.0/" 
xmlns="http://www.w3.org/2005/Atom" 
xml:base="http://lib.ololo.cc/opds/">
<id>http://lib.ololo.cc/opds/</id>
<updated>2011-05-31T10:28:22+04:00</updated>
<title>OPDS: lib.ololo.cc</title>
<subtitle>Librusec mirror.</subtitle>
<author>
  <name>ololo team</name>
  <uri>http://lib.ololo.cc</uri><email>libololo@gmail.com</email>
</author>
<icon>http://lib.ololo.cc/book.png</icon>
<link rel="self" title="This Page" type="application/atom+xml" href="/opds/"/>
<link rel="alternate" type="text/html" title="HTML Page" href="/"/>
<entry>
   <updated>2011-05-31T10:28:22+04:00</updated>
   <id>http://lib.ololo.cc/opds/asearch/</id>
   <title>Авторы</title>
   <content type="text">Поиск по авторам</content>
   <link type="application/atom+xml" href="http://lib.ololo.cc/opds/asearch/"/>
</entry>
</feed>
	 */
	/**
	 * Callback interface for OPDS.
	 */
	public interface DownloadCallback {
		/**
		 * Some entries are downloaded.
		 * @param doc is document
		 * @param entries is list of entries to add
		 */
		public void onEntries( DocInfo doc, Collection<EntryInfo> entries );
		/**
		 * All entries are downloaded.
		 * @param doc is document
		 * @param entries is list of entries to add
		 */
		public void onFinish( DocInfo doc, Collection<EntryInfo> entries );
		/**
		 * Before download: request filename to save as.
		 */
		public File onDownloadStart( String type, String url );
		/**
		 * Download progress
		 */
		public void onDownloadProgress( String type, String url, int percent );
		/**
		 * Book is downloaded.
		 */
		public void onDownloadEnd( String type, String url, File file );
		/**
		 * Error occured
		 */
		public void onError( String message );
	}
	
	public static class DocInfo {
		public String id;
		public long updated;
		public String title;
		public String subtitle;
		public String icon;
		public LinkInfo selfLink;
		public LinkInfo alternateLink;
	}
	
	public static class LinkInfo {
		public String href;
		public String rel;
		public String title;
		public String type;
		public LinkInfo( Attributes attributes ) {
			rel = attributes.getValue("rel");
			type = attributes.getValue("type");
			title = attributes.getValue("title");
			href = attributes.getValue("href");
		}
		public boolean isValid() {
			return href!=null && href.length()!=0;
		}
		public int getPriority() {
			if ( type==null )
				return 0;
			if ( rel!=null && rel.indexOf("acquisition")<0 )
				return 0;
			if ( type.startsWith("application/fb2") )
				return 10;
			if ( type.startsWith("application/epub") )
				return 9;
			if ( type.startsWith("application/x-mobipocket-ebook") )
				return 8;
			if ( type.startsWith("text/html") )
				return 3;
			if ( type.startsWith("text/plain") )
				return 2;
			return 0;
		}
		@Override
		public String toString() {
			return "[ rel=" + rel + ", type=" + type
					+ ", title=" + title + ", href=" + href + "]";
		}
		
	}
	
	public static class AuthorInfo {
		public String name;
		public String uri;
	}
	
	public static class EntryInfo {
		public String id;
		public long updated;
		public String title="";
		public String content="";
		public String summary="";
		public LinkInfo link;
		public ArrayList<LinkInfo> links = new ArrayList<LinkInfo>();
		public String icon;
		public ArrayList<String> categories = new ArrayList<String>(); 
		public ArrayList<AuthorInfo> authors = new ArrayList<AuthorInfo>(); 
		public LinkInfo getBestAcquisitionLink() {
			LinkInfo best = null;
			for ( LinkInfo link : links ) {
				boolean isAcquisition = link.rel!=null && link.rel.indexOf("acquisition")>=0;
				if (isAcquisition) {
					if ( link.getPriority()>0 && (best==null || best.getPriority()<link.getPriority()) )
						best = link;
				}
			}
			return best;
		}
	}
	
	public static class ODPSHandler extends DefaultHandler {
		private DocInfo docInfo = new DocInfo(); 
		private EntryInfo entryInfo = new EntryInfo(); 
		private ArrayList<EntryInfo> entries = new ArrayList<EntryInfo>(); 
		private Stack<String> elements = new Stack<String>();
		private Attributes currentAttributes;
		private AuthorInfo authorInfo;
		private boolean insideFeed;
		private boolean insideEntry;
		private boolean singleEntry;
		private int level = 0;
		//2011-05-31T10:28:22+04:00
		private static SimpleDateFormat tsFormat = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ssZ"); 
		private static SimpleDateFormat tsFormat2 = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss'Z'");
		private long parseTimestamp( String ts ) {
			if ( ts==null )
				return 0;
			ts = ts.trim();
			try {
				if ( ts.length()=="2010-01-10T10:01:10Z".length() )
					return tsFormat2.parse(ts).getTime();
				if ( ts.length()=="2011-11-11T11:11:11+67:87".length()&& ts.lastIndexOf(":")==ts.length()-3 ) {
					ts = ts.substring(0, ts.length()-3) + ts.substring(0, ts.length()-2);
					return tsFormat.parse(ts).getTime();
				}
				if ( ts.length()=="2011-11-11T11:11:11+6787".length()) {
					return tsFormat.parse(ts).getTime();
				}
			} catch (ParseException e) {
			}
			Log.e("cr3", "cannot parse timestamp " + ts);
			return 0;
		}
		@Override
		public void characters(char[] ch, int start, int length)
				throws SAXException {
			super.characters(ch, start, length);
			
			String s = new String( ch, start, length);
			s = s.trim();
			if (s.length()==0 || (s.length()==1 && s.charAt(0) == '\n') )
				return; // ignore empty line
			Log.d("cr3", tab() + "  {" + s + "}");
			String currentElement = elements.peek();
			if ( currentElement==null )
				return;
			if ( insideFeed ) {
				if ( "id".equals(currentElement) ) {
					if ( insideEntry )
						entryInfo.id = s;
					else
						docInfo.id = s;
				} else if ( "updated".equals(currentElement) ) {
					long ts = parseTimestamp(s);
					if ( insideEntry )
						entryInfo.updated = ts;
					else
						docInfo.updated = ts;
				} else if ( "title".equals(currentElement) ) {
					if ( !insideEntry )
						docInfo.title = s;
					else
						entryInfo.title = entryInfo.title + s;
				} else if ( "summary".equals(currentElement) ) {
					if ( insideEntry )
						entryInfo.summary = entryInfo.summary + s;
				} else if ( "name".equals(currentElement) ) {
					if ( authorInfo!=null )
						authorInfo.name = s;
				} else if ( "uri".equals(currentElement) ) {
					if ( authorInfo!=null )
						authorInfo.uri = s;
				} else if ( "icon".equals(currentElement) ) {
					if ( !insideEntry )
						docInfo.icon = s;
					else
						entryInfo.icon = s;
				} else if ( "link".equals(currentElement) ) {
					// rel, type, title, href
					if ( !insideEntry )
						docInfo.icon = s;
					else
						entryInfo.icon = s;
				} else if ( "content".equals(currentElement) ) {
					if ( insideEntry )
						entryInfo.content = entryInfo.content + s;
				} else if ( "subtitle".equals(currentElement) ) {
					if ( !insideEntry )
						docInfo.subtitle = s;
				}
			}
		}

		@Override
		public void endDocument() throws SAXException {
			super.endDocument();
			Log.d("cr3", "endDocument: " + entries.size() + " entries parsed");
			for ( EntryInfo entry : entries ) {
				Log.d("cr3", "   " + entry.title + " : " + entry.link.toString());
			}
		}

		private String tab() {
			if ( level<=1 )
				return "";
			StringBuffer buf = new StringBuffer(level*2);
			for ( int i=1; i<level; i++ )
				buf.append("  ");
			return buf.toString();
		}
		
		@Override
		public void startElement(String uri, String localName,
				String qName, Attributes attributes)
				throws SAXException {
			super.startElement(uri, localName, qName, attributes);
			level++;
			Log.d("cr3", tab() + "<" + localName + ">");
			currentAttributes = attributes;
			elements.push(localName);
			//String currentElement = elements.peek();
			if ( !insideFeed && "feed".equals(localName) ) {
				insideFeed = true;
			} else if ( "entry".equals(localName) ) {
//				if ( !insideFeed || insideEntry )
//					throw new SAXException("unexpected element " + localName);
				if ( !insideFeed ) {
					insideFeed = true;
					singleEntry = true;
				}
				insideEntry = true;
				entryInfo = new EntryInfo();
			} else if ( "category".equals(localName) ) {
				if ( insideEntry ) {
					String category = attributes.getValue("label");
					if ( category!=null )
						entryInfo.categories.add(category);
				}
			} else if ( "id".equals(localName) ) {
				
			} else if ( "updated".equals(localName) ) {
				
			} else if ( "title".equals(localName) ) {
				
			} else if ( "link".equals(localName) ) {
				LinkInfo link = new LinkInfo(attributes);
				if ( link.isValid() && insideFeed ) {
					Log.d("cr3", tab()+link.toString());
					if ( insideEntry ) {
						if ( link.type!=null ) {
							entryInfo.links.add(link);
							boolean isAcquisition = link.rel!=null && link.rel.indexOf("acquisition")>=0;
							if ( link.type.startsWith("application/atom+xml") ) {
								entryInfo.link = link;
							} else if (isAcquisition) {
								if ( link.getPriority()>0 && (entryInfo.link==null || entryInfo.link.getPriority()<link.getPriority()) )
									entryInfo.link = link;
							}
						}
					} else {
						if ( "self".equals(link.rel) )
							docInfo.selfLink = link;
						else if ( "alternate".equals(link.rel) )
							docInfo.alternateLink = link;
					}
				}
			} else if ( "author".equals(localName) ) {
				authorInfo = new AuthorInfo();
			}
		}
		
		@Override
		public void endElement(String uri, String localName,
				String qName) throws SAXException {
			super.endElement(uri, localName, qName);
			Log.d("cr3", tab() + "</" + localName + ">");
			//String currentElement = elements.peek();
			if ( insideFeed && "feed".equals(localName) ) {
				insideFeed = false;
			} else if ( "entry".equals(localName) ) {
				if ( !insideFeed || !insideEntry )
					throw new SAXException("unexpected element " + localName);
				if ( entryInfo.link!=null ) {
					entries.add(entryInfo);
				}
				insideEntry = false;
				entryInfo = null;
			} else if ( "author".equals(localName) ) {
				if ( authorInfo!=null && authorInfo.name!=null )
					entryInfo.authors.add(authorInfo);
				authorInfo = null;
			} 
			currentAttributes = null;
			if ( level>0 )
				level--;
		}

		@Override
		public void startDocument() throws SAXException {
			// TODO Auto-generated method stub
			super.startDocument();
		}

	}
	
	public static class DownloadTask {
		private CoolReader coolReader; 
		private URL url;
		private String referer;
		private DownloadCallback callback;
		private HttpURLConnection connection;
		//private HttpUriRequest request;
		private boolean cancelled;
		//private byte[] result;
		ODPSHandler handler;
		public DownloadTask( CoolReader coolReader, URL url, String referer, DownloadCallback callback ) {
			this.url = url;
			this.coolReader = coolReader;
			this.callback = callback; 
			this.referer = referer;
			//request = new HttpGet(url);
			//request.addHeader("Referer", "http://www.feedbooks.com/books/recent.atom");
			//Log.d("cr3", "Creating HTTP client");
		}
//		public byte[] getResult() {
//			return result;
//		}
		private void onError(final String msg) {
			BackgroundThread.guiExecutor.execute(new Runnable() {
				@Override
				public void run() {
					// TODO Auto-generated method stub
					callback.onError(msg);
				}
			});
		}
		private void parseFeed( InputStream is ) throws Exception {
			try {
				handler = new ODPSHandler();
				Xml.parse(is, Encoding.UTF_8, handler);
			} catch (SAXException se) {
				Log.e("SAX XML", "sax error", se);
				throw se;
			} catch (IOException ioe) {
				Log.e("SAX XML", "sax parse io error", ioe);
				throw ioe;
			}
			BackgroundThread.guiExecutor.execute(new Runnable() {
				@Override
				public void run() {
					Log.d("cr3", "Parsing is finished successfully. " + handler.entries.size() + " entries found");
					callback.onFinish(handler.docInfo, handler.entries);
				}
			});
		}
		
		private File generateFileName( File outDir, String fileName, String type ) {
			DocumentFormat fmt = type!=null ? DocumentFormat.byMimeType(type) : null;
			//DocumentFormat fmtext = fileName!=null ? DocumentFormat.byExtension(fileName) : null;
			if ( fileName==null )
				fileName = "noname";
			fileName = transcribeFileName( fileName );
			String ext = null;
			if ( fileName.lastIndexOf(".")>0 ) {
				ext = fileName.substring(fileName.lastIndexOf(".")+1);
				fileName = fileName.substring(0, fileName.lastIndexOf("."));
			}
			if ( fmt!=null )
				ext = fmt.getExtensions()[0].substring(1);
			for (int i=0; i<1000; i++ ) {
				String fn = fileName + (i==0 ? "" : "(" + i + ")") + "." + ext; 
				File f = new File(outDir, fn);
				if ( !f.exists() && !f.isDirectory() )
					return f;
			}
			return null;
		}
		private void downloadBook( final String type, final String url, InputStream is, int contentLength, final String fileName ) throws Exception {
			Log.d("cr3", "Download requested: " + type + " " + url + " " + contentLength);
			DocumentFormat fmt = DocumentFormat.byMimeType(type);
			if ( fmt==null ) {
				Log.d("cr3", "Download: unknown type " + type);
				throw new Exception("Unknown file type " + type);
			}
			final File outDir = BackgroundThread.instance().callGUI(new Callable<File>() {
				@Override
				public File call() throws Exception {
					return callback.onDownloadStart(type, url);
				}
			});
			if ( outDir==null ) {
				Log.d("cr3", "Cannot find writable location for downloaded file " + url);
				throw new Exception("Cannot save file " + url);
			}
			final File outFile = generateFileName( outDir, fileName, type );
			if ( outFile==null ) {
				Log.d("cr3", "Cannot generate file name");
				throw new Exception("Cannot generate file name");
			}
			Log.d("cr3", "Creating file: " + outFile.getAbsolutePath());
			if ( outFile.exists() || !outFile.createNewFile() ) {
				Log.d("cr3", "Cannot create file " + outFile.getAbsolutePath());
				throw new Exception("Cannot create file");
			}
			
			Log.d("cr3", "Download started: " + outFile.getAbsolutePath());
			long lastTs = System.currentTimeMillis(); 
			int lastPercent = -1;
			FileOutputStream os = null;
			try {
				os = new FileOutputStream(outFile);
				byte[] buf = new byte[16384];
				int totalWritten = 0;
				while (totalWritten<contentLength || contentLength==-1) {
					int bytesRead = is.read(buf);
					if ( bytesRead<=0 )
						break;
					os.write(buf, 0, bytesRead);
					totalWritten += bytesRead;
					final int percent = totalWritten * 100 / contentLength;
					long ts = System.currentTimeMillis(); 
					if ( percent!=lastPercent && ts - lastTs > 1500 ) {
						Log.d("cr3", "Download progress: " + percent + "%");
						BackgroundThread.instance().postGUI(new Runnable() {
							@Override
							public void run() {
								callback.onDownloadProgress(type, url, percent);
							}
						});
					}
				}
			} finally {
				if ( os!=null )
					os.close();
			}
			Log.d("cr3", "Download finished");
			BackgroundThread.instance().executeGUI(new Runnable() {
				@Override
				public void run() {
					callback.onDownloadEnd(type, url, outFile);
				}
			});
		}
		public void runInternal() {
			connection = null;
		    
			try {
				URLConnection conn = url.openConnection();
				if ( conn instanceof HttpsURLConnection ) {
					onError("HTTPs is not supported yet");
					return;
				}
				if ( !(conn instanceof HttpURLConnection) ) {
					onError("Only HTTP supported");
					return;
				}
				connection = (HttpURLConnection)conn;
	            connection.setRequestProperty("User-Agent", "CoolReader/3(Android)");
	            if ( referer!=null )
	            	connection.setRequestProperty("Referer", referer);
	            connection.setInstanceFollowRedirects(true);
	            connection.setAllowUserInteraction(false);
	            connection.setConnectTimeout(20000);
	            connection.setReadTimeout(40000);
	            connection.setDoInput(true);
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
				Log.d("cr3", "Response: " + response);
				if ( response!=200 ) {
					onError("Error " + response);
					return;
				}
				String contentType = connection.getContentType();
				String contentEncoding = connection.getContentEncoding();
				int contentLen = connection.getContentLength();
				//connection.getC
				Log.d("cr3", "Entity content length: " + contentLen);
				Log.d("cr3", "Entity content type: " + contentType);
				Log.d("cr3", "Entity content encoding: " + contentEncoding);
				InputStream is = connection.getInputStream();
				if ( contentType.startsWith("application/atom+xml") ) {
					Log.d("cr3", "Parsing feed");
					parseFeed( is );
				} else {
					Log.d("cr3", "Downloading book: " + contentEncoding);
					downloadBook( contentType, url.toString(), is, contentLen, fileName );
				}
			} catch (Exception e) {
				Log.e("cr3", "Exception while trying to open URI " + url.toString(), e);
				cancelled = true;
				onError("Error occured while reading OPDS catalog");
			} finally {
				if ( connection!=null )
					try {
						connection.disconnect();
					} catch ( Exception e ) {
						// ignore
					}
			}
		}
		public void run() {
			BackgroundThread.backgroundExecutor.execute(new Runnable() {
				@Override
				public void run() {
					try {
						runInternal();
					} catch ( Exception e ) {
						Log.e("cr3", "exception while opening OPDS", e);
					}
				}
			});
		}
		public void cancel() {
		}
	}
	private static DownloadTask currentTask;
	public static DownloadTask create( CoolReader coolReader, URL uri, String referer, DownloadCallback callback ) {
		final DownloadTask task = new DownloadTask(coolReader, uri, referer, callback);
		currentTask = task;
		return task;
	}

	public static String transcribeFileName( String fileName ) {
		StringBuilder buf = new StringBuilder(fileName.length());
		for ( char ch : fileName.toCharArray() ) {
			if ( ch=='\"' || ch=='\'' || ch=='/' || ch=='\\' || ch=='?' || ch<=' ')
				ch = '_';
			if ( !((ch>='a' && ch<='z') || (ch>='A' && ch<='Z') || (ch>='0' && ch<='9') || ch=='-' || ch=='_'))
				ch = '_';
			buf.append(ch);
		}
		return buf.toString();
	}
	
}
