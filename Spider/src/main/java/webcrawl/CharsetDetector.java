package webcrawl;

import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class CharsetDetector {
	static Pattern contentTypePattern = null;

	static {
		contentTypePattern = Pattern.compile("content-type:.*charset=([^\\s;]+)", Pattern.CASE_INSENSITIVE);
	}

	String detect(List<String> headers, byte[] body) {
		for (String header: headers) {
		      Matcher m = contentTypePattern.matcher(header);
		      if (m.find()) {
		         return m.group(1);
		      }
		}
		
		return "";
	}
}
