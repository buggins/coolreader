package org.coolreader.crengine;

import java.io.IOException;
import java.io.InputStream;
import java.net.URI;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Stack;

import org.apache.http.HttpEntity;
import org.apache.http.HttpResponse;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.client.methods.HttpUriRequest;
import org.xml.sax.Attributes;
import org.xml.sax.SAXException;
import org.xml.sax.helpers.DefaultHandler;

import android.net.http.AndroidHttpClient;
import android.util.Log;
import android.util.Xml;
import android.util.Xml.Encoding;

public class ODPSUtil {

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
		@Override
		public String toString() {
			return "LinkInfo [href=" + href + ", rel=" + rel + ", type=" + type
					+ ", title=" + title + "]";
		}
		
	}
	
	public static class EntryInfo {
		public String id;
		public long updated;
		public String title;
		public String content;
		public LinkInfo link;
		public String icon;
	}
	
	public static class ODPSHandler extends DefaultHandler {
		private DocInfo docInfo = new DocInfo(); 
		private EntryInfo entryInfo = new EntryInfo(); 
		private ArrayList<EntryInfo> entries = new ArrayList<EntryInfo>(); 
		private Stack<String> elements = new Stack<String>();
		private Attributes currentAttributes;
		private boolean insideFeed;
		private boolean insideEntry;
		private int level = 0;
		//2011-05-31T10:28:22+04:00
		private static SimpleDateFormat tsFormat = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ssZ"); 
		@Override
		public void characters(char[] ch, int start, int length)
				throws SAXException {
			super.characters(ch, start, length);
			
			String s = new String( ch, start, length);
			Log.d("cr3", "characters(" + s + ")");
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
					try {
						if ( s.lastIndexOf(":")==s.length()-3 )
							s = s.substring(0, s.length()-3) + s.substring(0, s.length()-2); 
						long ts = tsFormat.parse(s).getTime();
						if ( insideEntry )
							entryInfo.updated = ts;
						else
							docInfo.updated = ts;
					} catch (ParseException e) {
						// invalid timestamp: ignore
						Log.e("cr3", "cannot parse timestamp " + s);
					}
				} else if ( "title".equals(currentElement) ) {
					if ( !insideEntry )
						docInfo.title = s;
					else
						entryInfo.title = s;
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
						entryInfo.content = s;
				} else if ( "subtitle".equals(currentElement) ) {
					if ( !insideEntry )
						docInfo.subtitle = s;
				} else if ( "author".equals(currentElement) ) {
					
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
				if ( !insideFeed || insideEntry )
					throw new SAXException("unexpected element " + localName);
				insideEntry = true;
				entryInfo = new EntryInfo();
			} else if ( "id".equals(localName) ) {
				
			} else if ( "updated".equals(localName) ) {
				
			} else if ( "title".equals(localName) ) {
				
			} else if ( "link".equals(localName) ) {
				LinkInfo link = new LinkInfo(attributes);
				if ( link.isValid() && insideFeed ) {
					if ( insideEntry ) {
						entryInfo.link = link;
					} else {
						if ( "self".equals(link.rel) )
							docInfo.selfLink = link;
						else if ( "alternate".equals(link.rel) )
							docInfo.alternateLink = link;
					}
				}
			} else if ( "author".equals(localName) ) {
				
			}
		}
		
		@Override
		public void endElement(String uri, String localName,
				String qName) throws SAXException {
			super.endElement(uri, localName, qName);
			if ( level>0 )
				level--;
			Log.d("cr3", tab() + "</" + localName + ">");
			String currentElement = elements.peek();
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
			}
			currentAttributes = null;
		}

		@Override
		public void startDocument() throws SAXException {
			// TODO Auto-generated method stub
			super.startDocument();
		}

	}
	
	public static class DownloadTask {
		private HttpUriRequest request;
		private HttpResponse response;
		private AndroidHttpClient client;
		private boolean cancelled;
		private byte[] result;
		public DownloadTask( URI uri ) {
			request = new HttpGet(uri);
			Log.d("cr3", "Creating HTTP client");
		}
		public byte[] getResult() {
			return result;
		}
		private void parseFeed( InputStream is ) {
			try {
				ODPSHandler handler = new ODPSHandler();
				Xml.parse(is, Encoding.UTF_8, handler);
			} catch (SAXException se) {
				Log.e("SAX XML", "sax error", se);
			} catch (IOException ioe) {
				Log.e("SAX XML", "sax parse io error", ioe);
			}
		}
		public void run() {
			client = AndroidHttpClient.newInstance("CoolReader3");
			try {
				Log.d("cr3", "Calling httpClient.execute( GET " + request.getURI().toString() + " )");
				response = client.execute(request);
				Log.d("cr3", "Checking response");
				int status = response.getStatusLine().getStatusCode();
				Log.d("cr3", "Status code: " + status);
				if ( status!=200 )
					return;
				HttpEntity entity = response.getEntity();
				Log.d("cr3", "Entity: " + entity);
				if ( entity==null )
					return;
				Log.d("cr3", "Entity content type: " + entity.getContentType());
				Log.d("cr3", "Entity content encoding: " + entity.getContentEncoding());
				InputStream is = entity.getContent();
				if ( entity.getContentType().getValue().startsWith("application/atom+xml;charset=utf-8") ) {
					parseFeed( is );
				} else {
					Log.d("cr3", "Reading content");
					byte[] buf = new byte[512*1024]; // 512K
					int bytesRead = is.read(buf);
					Log.d("cr3", "Bytes read: " + bytesRead);
					result = buf;
				}
			} catch (IOException e) {
				Log.e("cr3", "Exception while trying to open URI " + request.getURI().toString(), e);
				cancelled = true;
			}
		}
		public void cancel() {
			request.abort();
		}
	}
	private static DownloadTask currentTask;
	public static DownloadTask create( URI uri ) {
		DownloadTask task = new DownloadTask(uri);
		currentTask = task;
		return task;
	}
}
