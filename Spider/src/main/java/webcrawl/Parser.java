package webcrawl;

import java.io.IOException;

import javax.ws.rs.core.MediaType;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.hbase.TableName;
import org.apache.hadoop.hbase.client.Connection;
import org.apache.hadoop.hbase.client.ConnectionFactory;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Table;
import org.apache.hadoop.hbase.util.Bytes;

import com.sun.jersey.api.client.Client;
import com.sun.jersey.api.client.ClientHandlerException;
import com.sun.jersey.api.client.WebResource;
import com.sun.jersey.api.client.config.ClientConfig;
import com.sun.jersey.api.client.config.DefaultClientConfig;
import com.sun.jersey.api.json.JSONConfiguration;

public class Parser implements Runnable {
	public boolean stop = false;

	@Override
	public void run() {
		try {
			ClientConfig clientConfig = new DefaultClientConfig();
			clientConfig.getFeatures().put(JSONConfiguration.FEATURE_POJO_MAPPING, Boolean.TRUE);
			Client c = Client.create(clientConfig);
			
			Configuration conf = HBaseConfiguration.create();
			Connection connection = ConnectionFactory.createConnection(conf);
			TableName tableName = TableName.valueOf("web");
		    Table table = connection.getTable(tableName);
			while (!stop) {
				try {
					WebResource r = c.resource("http://huzide.com:31200/result");
					FetchApi.Tasks resp = r.accept(MediaType.APPLICATION_JSON_TYPE).get(FetchApi.Tasks.class);
					for (FetchApi.Task t : resp.tasks) {
						System.out.format("Downloaded page: %s\n", t.uri);
						Put put = new Put(Bytes.toBytes(t.uri));
						put.addColumn(Bytes.toBytes("meta"), Bytes.toBytes("download_time"), Bytes.toBytes(System.currentTimeMillis()));
						//put.addColumn(Bytes.toBytes("response"), Bytes.toBytes("headers"), Bytes.toBytes(t.resp_headers));
						put.addColumn(Bytes.toBytes("response"), Bytes.toBytes("body"), t.resp_body);
						table.put(put);
					}
				} catch (ClientHandlerException e) {
					e.printStackTrace();
				}
				Thread.sleep(1000);
			}
			table.close();
			connection.close();
		} catch (InterruptedException | IOException e) {
			e.printStackTrace();
		}
	}
}
