package webcrawl;

import java.io.IOException;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.hbase.HColumnDescriptor;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.TableName;
import org.apache.hadoop.hbase.client.Admin;
import org.apache.hadoop.hbase.client.Connection;
import org.apache.hadoop.hbase.client.ConnectionFactory;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Table;
import org.apache.hadoop.hbase.util.Bytes;

/**
 * Hello world!
 *
 */
public class Main 
{
	public void initWebTable(boolean recreate) throws IOException {
		Configuration conf = HBaseConfiguration.create();
		Connection connection = ConnectionFactory.createConnection(conf);
		Admin admin = connection.getAdmin();

		TableName tableName = TableName.valueOf("web");
		if (admin.tableExists(tableName)) {
			if (recreate) {
				if (admin.isTableEnabled(tableName)) {
					admin.disableTable(tableName);
				}
				admin.deleteTable(tableName);
			}
		}
		
		if (!admin.tableExists(tableName)) {
			HTableDescriptor desc = new HTableDescriptor(tableName);
			for (String cf : new String[] { "response", "meta", "link", "rlink" }) {
				HColumnDescriptor coldef = new HColumnDescriptor(cf);
				coldef.setMaxVersions(2);
				desc.addFamily(coldef);
			}
			admin.createTable(desc);
		}

	    Table table = connection.getTable(tableName);
		String[] urls = {
			"http://www.163.com/", "http://www.sina.com.cn/", "http://www.sohu.com/",
			"https://www.hao123.com/", "http://baike.baidu.com/", "https://zhidao.baidu.com/",
			"http://hao.360.cn/", "http://www.wenxuecity.com/", "http://www.6park.com/",
			"http://www.mitbbs.com/", "http://www.ftchinese.com/", "http://www.creaders.net/",
		};
		for (String url : urls) {
			Put put = new Put(Bytes.toBytes(url));
			put.addColumn(Bytes.toBytes("meta"), Bytes.toBytes("found_time"),
					Bytes.toBytes(System.currentTimeMillis()));
			table.put(put);
		}
		table.close();
		connection.close();
	}

	void run() {
		Thread scheduler = new Thread(new Scheduler());
		Thread parser = new Thread(new Parser());
		scheduler.start();
		parser.start();
		try {
			scheduler.join();
			parser.join();
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
	}

    public static void main(String[] args)
    {
    	try {
	    	Main webcrawl = new Main();
	    	webcrawl.initWebTable(true);
	    	webcrawl.run();
    	} catch (IOException e) {
    		e.printStackTrace();
    		System.exit(1);
    	}
    }
}
