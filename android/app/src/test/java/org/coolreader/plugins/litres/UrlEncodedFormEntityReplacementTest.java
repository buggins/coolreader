package org.coolreader.plugins.litres;

//import org.apache.http.NameValuePair;
//import org.apache.http.client.entity.UrlEncodedFormEntity;
//import org.apache.http.message.BasicNameValuePair;

import org.junit.Test;

//import java.io.ByteArrayOutputStream;
//import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;
import java.util.HashMap;
import java.util.Iterator;
//import java.util.LinkedList;
//import java.util.List;
import java.util.Map;

import static org.junit.Assert.*;

public class UrlEncodedFormEntityReplacementTest {
	/*
	@Test
	public void testUrlEncodedFormEntity() {
		int offset = 0;
		int maxCount = 50;

		String result1 = null;
		final String result_ref = "checkpoint=2000-01-01+00%3A00%3A00&rating=hot&limit=0%2C50&search_types=0";

		Map<String, String> params = new HashMap<String, String>();
		params.put("rating", "hot");
		params.put("limit", "" + offset + "," + maxCount);
		params.put("search_types", "0");
		params.put("checkpoint", "2000-01-01 00:00:00");

		List<NameValuePair> list = new LinkedList<NameValuePair>();
		for (Map.Entry<String, String> entry : params.entrySet()) {
			list.add(new BasicNameValuePair(entry.getKey(), entry.getValue()));
		}
		try {
			UrlEncodedFormEntity postParams = new UrlEncodedFormEntity(list, "utf-8");
			ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
			postParams.writeTo(outputStream);
			outputStream.flush();
			outputStream.close();
			result1 = outputStream.toString("UTF-8");
		} catch (UnsupportedEncodingException e) {
			fail(e.toString());
		} catch (IOException e) {
			fail(e.toString());
		}
		assertEquals(result1, result_ref);
	}
	 */

	@Test
	public void testUrlEncodedFormEntityReplacement() {
		int offset = 0;
		int maxCount = 50;

		String result1 = null;
		final String result_ref = "checkpoint=2000-01-01+00%3A00%3A00&rating=hot&limit=0%2C50&search_types=0";

		Map<String, String> params = new HashMap<String, String>();
		params.put("rating", "hot");
		params.put("limit", "" + offset + "," + maxCount);
		params.put("search_types", "0");
		params.put("checkpoint", "2000-01-01 00:00:00");

		Iterator<Map.Entry<String, String>> iterator = params.entrySet().iterator();
		String value;
		String entry_str;
		result1 = "";
		while (true) {
			Map.Entry<String, String> entry = iterator.next();
			if (null != entry.getValue()) {
				try {
					value = URLEncoder.encode(entry.getValue(), "UTF-8");
				} catch (UnsupportedEncodingException e) {
					value = "";
				}
			} else {
				value = "";
			}
			entry_str = entry.getKey() + "=" + value;
			result1 += entry_str;
			if (iterator.hasNext())
				result1 += "&";
			else
				break;
		}
		assertEquals(result1, result_ref);
	}
}