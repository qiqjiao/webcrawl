package webcrawl;

import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.math.BigInteger;
import java.util.ArrayList;
import java.util.Arrays;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

import javax.ws.rs.core.MediaType;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.hbase.TableName;
import org.apache.hadoop.hbase.client.Connection;
import org.apache.hadoop.hbase.client.ConnectionFactory;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.client.Table;
import org.apache.hadoop.hbase.util.Bytes;

import com.sun.jersey.api.client.Client;
import com.sun.jersey.api.client.UniformInterfaceException;
import com.sun.jersey.api.client.WebResource;
import com.sun.jersey.api.client.config.ClientConfig;
import com.sun.jersey.api.client.config.DefaultClientConfig;
import com.sun.jersey.api.json.JSONConfiguration;

public class Scheduler implements Runnable {
	public boolean stop = false;

	long md5(byte[] s) {
		MessageDigest md;
		try {
			md = MessageDigest.getInstance("MD5");
		} catch (NoSuchAlgorithmException e) {
			e.printStackTrace();
			return 0;
		}
		byte[] bytes = md.digest(s);
		BigInteger i = new BigInteger(bytes);
		return Math.abs(i.longValue());
	}

	public void fetch(byte[] uri) throws InterruptedException, UnsupportedEncodingException {
		FetchApi.Task task = new FetchApi.Task();
		task.method = "GET";
		task.uri = new String(uri);
		task.req_headers = new ArrayList<>(Arrays.asList("Accept-Encoding: gzip, deflate"));

		FetchApi.Tasks tasks = new FetchApi.Tasks();
		tasks.tasks = new ArrayList<>();
		tasks.tasks.add(task);

		ClientConfig clientConfig = new DefaultClientConfig();
		clientConfig.getFeatures().put(JSONConfiguration.FEATURE_POJO_MAPPING, Boolean.TRUE);
		Client c = Client.create(clientConfig);
		while (true) {
			try {
				WebResource r = c.resource("http://huzide.com:31200/fetch");
				FetchApi.Tasks resp = r.accept(MediaType.APPLICATION_JSON_TYPE)
						.entity(tasks, MediaType.APPLICATION_JSON_TYPE)
						.post(FetchApi.Tasks.class, tasks);
				for (FetchApi.Task t: resp.tasks) {
					System.out.format("Created task: %d\t%s\n", t.id, t.uri);
				}
				break;
			} catch (UniformInterfaceException e) {
				e.printStackTrace();
				Thread.sleep(1000);
				System.out.println("Retry fetching ...");
			}
		}
	}

	@Override
	public void run() {
		Configuration conf = HBaseConfiguration.create();
		Connection connection = null;
		Table table = null;
		ResultScanner scanner = null;
		try {
			connection = ConnectionFactory.createConnection(conf);
		    table = connection.getTable(TableName.valueOf("web"));

		    int group = 0;
		    byte[] family = Bytes.toBytes("meta");
	        byte[] download_time = Bytes.toBytes("download_time");
	        Scan scan = new Scan();
	        scan.addFamily(family);
	        while (!stop) {
	        	System.out.println("Scanning web:meta...");
		        scanner = table.getScanner(scan);
		        for (Result r : scanner) {
		        	byte[] uri = r.getRow();
		        	if (md5(uri) % 2 == group) {
			        	if (r.containsColumn(family, download_time)) {
				        	System.out.println("Existing: " + r);	        		
			        	} else {
				        	System.out.println("New: " + r);
				        	fetch(uri);
			        	}
		        	}
		        }
		        group = (group + 1) % 2;
		        Thread.sleep(10000);
	        }
		} catch (IOException | InterruptedException e) {
			e.printStackTrace();
		} finally {
			try {
				if (connection != null) connection.close();
				if (table != null) table.close();
				if (scanner != null) scanner.close();
			} catch (IOException e) {
					e.printStackTrace();
			}
		}
	}
}
