package org.coolreader.crengine;

import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;

/**
 * Instrumented test, which will execute on an Android device.
 *
 * @see <a href="http://d.android.com/tools/testing">Testing documentation</a>
 */
public class EngineTests {
	@Test
	public void testGetHumanReadableLocaleName()
	{
		// English codes variants
		String localeName = Engine.getHumanReadableLocaleName("English");
		assertEquals("English", localeName);

		localeName = Engine.getHumanReadableLocaleName("english");
		assertEquals("English", localeName);

		localeName = Engine.getHumanReadableLocaleName("eng");
		assertEquals("English", localeName);

		localeName = Engine.getHumanReadableLocaleName("Eng");
		assertEquals("English", localeName);

		localeName = Engine.getHumanReadableLocaleName("en-US");
		assertEquals("English (USA)", localeName);

		localeName = Engine.getHumanReadableLocaleName("eng-US");
		assertEquals("English (USA)", localeName);

		localeName = Engine.getHumanReadableLocaleName("en-USA");
		assertEquals("English (USA)", localeName);

		localeName = Engine.getHumanReadableLocaleName("eng-USA");
		assertEquals("English (USA)", localeName);

		localeName = Engine.getHumanReadableLocaleName("en-GB");
		assertEquals("English (GBR)", localeName);

		localeName = Engine.getHumanReadableLocaleName("eng-GB");
		assertEquals("English (GBR)", localeName);

		localeName = Engine.getHumanReadableLocaleName("en-GBR");
		assertEquals("English (GBR)", localeName);

		localeName = Engine.getHumanReadableLocaleName("eng-GBR");
		assertEquals("English (GBR)", localeName);

		localeName = Engine.getHumanReadableLocaleName("en_gb");
		assertEquals("English (GBR)", localeName);

		localeName = Engine.getHumanReadableLocaleName("en");
		assertEquals("English", localeName);

		// Russian codes variants
		localeName = Engine.getHumanReadableLocaleName("Russian");
		assertEquals("Russian", localeName);

		localeName = Engine.getHumanReadableLocaleName("russian");
		assertEquals("Russian", localeName);

		localeName = Engine.getHumanReadableLocaleName("Rus");
		assertEquals("Russian", localeName);

		localeName = Engine.getHumanReadableLocaleName("rus");
		assertEquals("Russian", localeName);

		localeName = Engine.getHumanReadableLocaleName("ru-RU");
		assertEquals("Russian (RUS)", localeName);

		localeName = Engine.getHumanReadableLocaleName("rus-RU");
		assertEquals("Russian (RUS)", localeName);

		localeName = Engine.getHumanReadableLocaleName("ru-RUS");
		assertEquals("Russian (RUS)", localeName);

		localeName = Engine.getHumanReadableLocaleName("rus-RUS");
		assertEquals("Russian (RUS)", localeName);

		localeName = Engine.getHumanReadableLocaleName("ru_ru");
		assertEquals("Russian (RUS)", localeName);

		localeName = Engine.getHumanReadableLocaleName("ru");
		assertEquals("Russian", localeName);

		// Chinese variants
		localeName = Engine.getHumanReadableLocaleName("zh-CN");
		assertEquals("Chinese (CHN)", localeName);

		localeName = Engine.getHumanReadableLocaleName("zho-CN");
		assertEquals("Chinese (CHN)", localeName);

		localeName = Engine.getHumanReadableLocaleName("zh-CHN");
		assertEquals("Chinese (CHN)", localeName);

		localeName = Engine.getHumanReadableLocaleName("zho-CHN");
		assertEquals("Chinese (CHN)", localeName);

		localeName = Engine.getHumanReadableLocaleName("zh-HK");
		assertEquals("Chinese (HKG)", localeName);

		localeName = Engine.getHumanReadableLocaleName("zho-HK");
		assertEquals("Chinese (HKG)", localeName);

		localeName = Engine.getHumanReadableLocaleName("zh-HKG");
		assertEquals("Chinese (HKG)", localeName);

		localeName = Engine.getHumanReadableLocaleName("zho-HKG");
		assertEquals("Chinese (HKG)", localeName);

		localeName = Engine.getHumanReadableLocaleName("zh_hk");
		assertEquals("Chinese (HKG)", localeName);

		// invalid langTag
		localeName = Engine.getHumanReadableLocaleName("abcd");
		assertNull(localeName);
	}
}
