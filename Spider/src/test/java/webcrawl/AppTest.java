package webcrawl;

import org.apache.hadoop.hbase.util.Bytes;

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;

/**
 * Unit test for simple App.
 */
public class AppTest 
    extends TestCase
{
    /**
     * Create the test case
     *
     * @param testName name of the test case
     */
    public AppTest( String testName )
    {
        super( testName );
    }

    /**
     * @return the suite of tests being tested
     */
    public static Test suite()
    {
        return new TestSuite( AppTest.class );
    }

    /**
     * Rigourous Test :-)
     */
    public void testApp()
    {
        assertTrue( true );
        String s = "http://www.google.com/";
        byte[] bytes = Bytes.toBytes(s);
        for (byte b : bytes) {
        	System.out.format("char:%d\n", b);
        }
    }
}
