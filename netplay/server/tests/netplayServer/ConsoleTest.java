package netplayServer;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.util.Iterator;
import java.util.List;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import com.google.common.collect.ImmutableList;

import netplayprotos.NetplayServiceProto.PlugControllerResponsePB.PortRejectionPB;
import netplayprotos.NetplayServiceProto.PlugControllerResponsePB.PortRejectionPB.Reason;
import netplayprotos.NetplayServiceProto.Port;

@RunWith
(JUnit4.class)
public class ConsoleTest {
	
	private Console console;
	private List<Port> allPorts = 
			ImmutableList.of(Port.PORT_1, Port.PORT_2, Port.PORT_3, Port.PORT_4);
	
	@Before
	public void setUp() {
		console = new Console();
	}
	
	@Test
	public void testBasicClientOnePort() throws PlugRequestException {
		Client client = console.tryAddPlayers(1, Port.PORT_1);
		assertEquals(1, client.getPorts().size());
		assertEquals(Port.PORT_1, client.getPorts().iterator().next());
	}
	
	@Test
	public void testBasicClientFourPorts() throws PlugRequestException {
		Client client = console.tryAddPlayers(1, Port.PORT_1, Port.PORT_2, Port.PORT_3, Port.PORT_4);
		assertEquals(4, client.getPorts().size());
		assertTrue(client.getPorts().containsAll(allPorts));
	}
	
	@Test
	public void testBasicClientPortAny() throws PlugRequestException {
		Client client = console.tryAddPlayers(1, Port.PORT_1, Port.PORT_ANY, Port.PORT_2, Port.PORT_3);
		assertEquals(4, client.getPorts().size());
		assertTrue(client.getPorts().containsAll(ImmutableList.of(Port.PORT_1, Port.PORT_2,
				Port.PORT_3)));
	}
	
	@Test
	public void testRejectPort() throws PlugRequestException {
		Client client = console.tryAddPlayers(1, Port.PORT_1);
		try {
			console.tryAddPlayers(1, Port.PORT_1);
			fail();
		} catch (PlugRequestException expected) {
			assertEquals(1, expected.getRejections().size());
			PortRejectionPB reject = expected.getRejections().iterator().next();
			assertEquals(Port.PORT_1, reject.getPort());
			assertEquals(Reason.PORT_ALREADY_OCCUPIED, reject.getReason());
		}
		assertEquals(1, client.getPorts().size());
	}
	
	@Test
	public void testFullConsoleReject() throws PlugRequestException {
		Client client = console.tryAddPlayers(1, Port.PORT_1, Port.PORT_2);
		try {
			console.tryAddPlayers(1, Port.PORT_ANY, Port.PORT_ANY, Port.PORT_ANY);
			fail();
		} catch (PlugRequestException expected) {
			assertEquals(3, expected.getRejections().size());
			Iterator<PortRejectionPB> it = expected.getRejections().iterator();
			PortRejectionPB reject = it.next();
			assertEquals(Reason.ACCEPTABLE, reject.getReason());
			reject = it.next();
			assertEquals(Reason.ACCEPTABLE, reject.getReason());
			reject = it.next();
			assertEquals(Reason.ALL_PORTS_OCCUPIED, reject.getReason());
		}
		assertEquals(2, client.getPorts().size());
	}
	
	@Test
	public void testNoPortRequested() throws PlugRequestException {
		try {
			console.tryAddPlayers(1);
			fail();
		} catch (PlugRequestException expected) {
			assertEquals(null, expected.getRejections());
		}
	}
	

}
