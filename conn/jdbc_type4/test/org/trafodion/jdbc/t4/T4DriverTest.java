package org.trafodion.jdbc.t4;

import java.sql.SQLException;

import org.junit.Assert;
import org.junit.BeforeClass;
import org.junit.Test;

public class T4DriverTest extends BaseTest {
	private static T4Driver driver;

	@BeforeClass
	public static void setUpBeforeClass() throws Exception {
		driver = (T4Driver) Class.forName("org.trafodion.jdbc.t4.T4Driver").newInstance();
	}


	@Test
	public void acceptsURL() throws SQLException {
		String url = "jdbc:t4jdbc://192.168.1.103:37800/:";
		Assert.assertTrue(driver.acceptsURL(url));
		url = "jdbc:abc://192.168.1.103:37800/:";
		Assert.assertFalse(driver.acceptsURL(url));
	}

}
