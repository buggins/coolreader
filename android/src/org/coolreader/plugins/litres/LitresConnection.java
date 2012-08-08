package org.coolreader.plugins.litres;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStreamWriter;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLConnection;
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
import org.coolreader.db.ServiceThread;
import org.xml.sax.SAXException;

import android.os.Handler;

public class LitresConnection {
	public static final String AUTHORIZE_URL = "http://robot.litres.ru/pages/catalit_authorise/";
	
	ServiceThread workerThread;
	
	private LitresConnection() {
		workerThread = new ServiceThread("litres");
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
		            connection.setRequestProperty("User-Agent", "CoolReader/3(Android)");
		            connection.setInstanceFollowRedirects(true);
		            connection.setAllowUserInteraction(false);
		            connection.setConnectTimeout(CONNECT_TIMEOUT);
		            connection.setReadTimeout(READ_TIMEOUT);
		            connection.setDoInput(true);
            		connection.setDoOutput(true);
		            connection.setRequestMethod("POST");
		            
		            List<NameValuePair> list = new LinkedList<NameValuePair>();
		            for (Map.Entry<String, String> entry : params.entrySet())
		            	list.add(new BasicNameValuePair(entry.getKey(), entry.getValue()));
		            UrlEncodedFormEntity postParams = new UrlEncodedFormEntity(list);
					OutputStreamWriter wr = new OutputStreamWriter(connection.getOutputStream());
                    wr.write(postParams.toString());
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
					byte[] buf = new byte[contentLen];
					if (is.read(buf) != contentLen) {
						contentHandler.onError(0, "Wrong content length");
						return;
					}
					is.close();
					is = null;
					is = new ByteArrayInputStream(buf);
					
					SAXParserFactory spf = SAXParserFactory.newInstance();
					spf.setValidating(false);
//					spf.setNamespaceAware(true);
//					spf.setFeature("http://xml.org/sax/features/namespaces", false);
					SAXParser sp = spf.newSAXParser();
					//XMLReader xr = sp.getXMLReader();				
					sp.parse(is, contentHandler);
					
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
	
	public void authorize(final String sid, final String login, final String pwd, final ResultHandler resultHandler) {
		final Map<String, String> params = new HashMap<String, String>();
		if (sid != null)
			params.put("sid", sid);
		if (login != null)
			params.put("login", login);
		if (pwd != null)
			params.put("pwd", pwd);
		sendRequest(AUTHORIZE_URL, params, new ResponseHandler() {
			
		}, resultHandler);
	}

	public void close() {
		workerThread.stop(5000);
	}
}
