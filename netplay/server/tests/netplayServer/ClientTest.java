package netplayServer;

import static org.junit.Assert.assertEquals;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import netplayprotos.NetplayServiceProto.Port;

@RunWith
(JUnit4.class)
public class ClientTest {
	
	private Client client;
	private Console console;
	
	@Before
	public void setUp() {
		console = new Console();
		client = new Client(console, 0);
		
	}
	
	@Test
	public void testAddPlayer() {
		Player player = new Player(Port.PORT_1);
		assertEquals(0, client.getPorts().size());
		client.addPlayer(player);
		assertEquals(1, client.getPorts().size());
		client.addPlayer(player);
		assertEquals(1, client.getPorts().size());
	}

}
