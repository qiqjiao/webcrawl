package webcrawl;

import java.util.Arrays;

import junit.framework.TestCase;

public class CharsetDetectorTest extends TestCase {

	protected void setUp() throws Exception {
		super.setUp();
	}

	protected void tearDown() throws Exception {
		super.tearDown();
	}

	public void testSmoke() {
		CharsetDetector d = new CharsetDetector();
		assertEquals("UTF-8", d.detect(Arrays.asList("Content-type: text/html; CHARset=UTF-8;"), null));
	}
}
