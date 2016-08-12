package webcrawl;

import java.util.List;

public class FetchApi {
	public static class Task {
		public long id = -1;
		public String summary;

		public String method;
		public String uri;
		public List<String> req_headers;

		public int resp_code = -1;
		public String resp_reason;
		public List<String> resp_headers;
		public byte[] resp_body;

		public boolean truncated;
		public String redirect;
		public String error_message;
	}

	public static class Tasks {
		public List<Task> tasks;
	}
}
