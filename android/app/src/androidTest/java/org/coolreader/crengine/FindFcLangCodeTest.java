package org.coolreader.crengine;

import android.support.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

import static org.junit.Assert.*;

/**
 * Instrumented test, which will execute on an Android device.
 *
 * @see <a href="http://d.android.com/tools/testing">Testing documentation</a>
 */
@RunWith(AndroidJUnit4.class)
public class FindFcLangCodeTest {
	@Test
	public void testFindCompatibleFcLangCode()
	{
		// English codes variants
		String langCode = Engine.findCompatibleFcLangCode("English");
		assertEquals("en", langCode);

		langCode = Engine.findCompatibleFcLangCode("english");
		assertEquals("en", langCode);

		langCode = Engine.findCompatibleFcLangCode("eng");
		assertEquals("en", langCode);

		langCode = Engine.findCompatibleFcLangCode("Eng");
		assertEquals("en", langCode);

		langCode = Engine.findCompatibleFcLangCode("en-US");
		assertEquals("en", langCode);

		langCode = Engine.findCompatibleFcLangCode("eng-US");
		assertEquals("en", langCode);

		langCode = Engine.findCompatibleFcLangCode("en-USA");
		assertEquals("en", langCode);

		langCode = Engine.findCompatibleFcLangCode("eng-USA");
		assertEquals("en", langCode);

		langCode = Engine.findCompatibleFcLangCode("en-GB");
		assertEquals("en", langCode);

		langCode = Engine.findCompatibleFcLangCode("eng-GB");
		assertEquals("en", langCode);

		langCode = Engine.findCompatibleFcLangCode("en-GBR");
		assertEquals("en", langCode);

		langCode = Engine.findCompatibleFcLangCode("eng-GBR");
		assertEquals("en", langCode);

		langCode = Engine.findCompatibleFcLangCode("en_gb");
		assertEquals("en", langCode);

		langCode = Engine.findCompatibleFcLangCode("en");
		assertEquals("en", langCode);

		// Russian codes variants
		langCode = Engine.findCompatibleFcLangCode("Russian");
		assertEquals("ru", langCode);

		langCode = Engine.findCompatibleFcLangCode("russian");
		assertEquals("ru", langCode);

		langCode = Engine.findCompatibleFcLangCode("Rus");
		assertEquals("ru", langCode);

		langCode = Engine.findCompatibleFcLangCode("rus");
		assertEquals("ru", langCode);

		langCode = Engine.findCompatibleFcLangCode("ru-RU");
		assertEquals("ru", langCode);

		langCode = Engine.findCompatibleFcLangCode("rus-RU");
		assertEquals("ru", langCode);

		langCode = Engine.findCompatibleFcLangCode("ru-RUS");
		assertEquals("ru", langCode);

		langCode = Engine.findCompatibleFcLangCode("rus-RUS");
		assertEquals("ru", langCode);

		langCode = Engine.findCompatibleFcLangCode("ru_ru");
		assertEquals("ru", langCode);

		langCode = Engine.findCompatibleFcLangCode("ru");
		assertEquals("ru", langCode);

		// Chinese variants
		langCode = Engine.findCompatibleFcLangCode("zh-CN");
		assertEquals("zh_cn", langCode);

		langCode = Engine.findCompatibleFcLangCode("zho-CN");
		assertEquals("zh_cn", langCode);

		langCode = Engine.findCompatibleFcLangCode("zh-CHN");
		assertEquals("zh_cn", langCode);

		langCode = Engine.findCompatibleFcLangCode("zho-CHN");
		assertEquals("zh_cn", langCode);

		langCode = Engine.findCompatibleFcLangCode("zh-HK");
		assertEquals("zh_hk", langCode);

		langCode = Engine.findCompatibleFcLangCode("zho-HK");
		assertEquals("zh_hk", langCode);

		langCode = Engine.findCompatibleFcLangCode("zh-HKG");
		assertEquals("zh_hk", langCode);

		langCode = Engine.findCompatibleFcLangCode("zho-HKG");
		assertEquals("zh_hk", langCode);

		langCode = Engine.findCompatibleFcLangCode("zh_hk");
		assertEquals("zh_hk", langCode);

		// invalid variant
		langCode = Engine.findCompatibleFcLangCode("abcd");
		assertNull(langCode);
	}
}
