package netplayServer;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.util.Queue;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import com.google.common.collect.Queues;

import io.grpc.stub.StreamObserver;
import netplayprotos.NetplayServiceProto.ClientReadyPB;
import netplayprotos.NetplayServiceProto.IncomingEventPB;
import netplayprotos.NetplayServiceProto.MakeConsoleRequestPB;
import netplayprotos.NetplayServiceProto.MakeConsoleResponsePB;
import netplayprotos.NetplayServiceProto.OutgoingEventPB;
import netplayprotos.NetplayServiceProto.PlugControllerRequestPB;
import netplayprotos.NetplayServiceProto.PlugControllerResponsePB;
import netplayprotos.NetplayServiceProto.PlugControllerResponsePB.Status;
import netplayprotos.NetplayServiceProto.Port;
import netplayprotos.NetplayServiceProto.StartGameRequestPB;
import netplayprotos.NetplayServiceProto.StartGameResponsePB;

@RunWith
(JUnit4.class)
public class ServerTest {
	
	private Server server;
	ResponseObserver<MakeConsoleResponsePB> makeConsoleObserver;
	ResponseObserver<PlugControllerResponsePB> plugControllerObserver;
	ResponseObserver<StartGameResponsePB> startGameObserver;
	
	MakeConsoleRequestPB makeConsoleReq = MakeConsoleRequestPB.newBuilder().build();	
	@Before
	public void setUp() {
		server = new Server(true);
		makeConsoleObserver = new ResponseObserver<MakeConsoleResponsePB>();
		plugControllerObserver = new ResponseObserver<PlugControllerResponsePB>();
		startGameObserver = new ResponseObserver<StartGameResponsePB>();
	}
	
	@Test
	public void testMakeConsole() {
		server.makeConsole(makeConsoleReq, makeConsoleObserver);
		MakeConsoleResponsePB resp = makeConsoleObserver.getNextValue();
		long id1 = resp.getConsoleId();
		server.makeConsole(makeConsoleReq, makeConsoleObserver);
		resp = makeConsoleObserver.getNextValue();
		assertNotEquals(id1, resp.getConsoleId());
	}
	
	@Test
	public void testPlugWithNoConsole() {
		PlugControllerRequestPB pReq = PlugControllerRequestPB.newBuilder()
				.setConsoleId(1).setDelayFrames(2).setRequestedPort1(Port.PORT_ANY)
				.build();
		server.plugController(pReq, plugControllerObserver);
		PlugControllerResponsePB resp = plugControllerObserver.getNextValue();
		assertEquals(Status.UNSPECIFIED_FAILURE, resp.getStatus());
	}
	
	@Test
	public void testMakeAndJoinConsole() {
		server.makeConsole(makeConsoleReq, makeConsoleObserver);
		long id = makeConsoleObserver.getNextValue().getConsoleId();
		PlugControllerRequestPB pReq = PlugControllerRequestPB.newBuilder()
				.setConsoleId(id).setDelayFrames(2).setRequestedPort1(Port.PORT_ANY)
				.build();
		server.plugController(pReq, plugControllerObserver);
		PlugControllerResponsePB resp = plugControllerObserver.getNextValue();
		assertEquals(id, resp.getConsoleId());
	}
	
	@Test
	public void testMakeAndJoinConsole_TwoPlayers() {
		server.makeConsole(makeConsoleReq, makeConsoleObserver);
		long id = makeConsoleObserver.getNextValue().getConsoleId();
		
		// Plug in two controllers in two requests.  The ports should not be equal.
		
		PlugControllerRequestPB pReq = PlugControllerRequestPB.newBuilder()
				.setConsoleId(id).setDelayFrames(2).setRequestedPort1(Port.PORT_ANY)
				.build();
		server.plugController(pReq, plugControllerObserver);
		PlugControllerResponsePB resp = plugControllerObserver.getNextValue();
		assertEquals(id, resp.getConsoleId());
		assertEquals(1, resp.getPortList().size());
		Port port1 = resp.getPortList().get(0);
		pReq = PlugControllerRequestPB.newBuilder()
				.setConsoleId(id).setDelayFrames(2).setRequestedPort1(Port.PORT_ANY)
				.build();
		server.plugController(pReq, plugControllerObserver);
		resp = plugControllerObserver.getNextValue();
		assertEquals(id, resp.getConsoleId());
		assertEquals(1, resp.getPortList().size());
		Port port2 = resp.getPortList().get(0);
		assertNotEquals(port1, port2);
	}
	
	/**
	 * Tests that multiple players from a single client can plug properly.
	 */
	@Test
	public void testMultiplePlayersOneClient() {
		server.makeConsole(makeConsoleReq, makeConsoleObserver);
		long id = makeConsoleObserver.getNextValue().getConsoleId();
		PlugControllerRequestPB pReq = PlugControllerRequestPB.newBuilder()
				.setConsoleId(id).setDelayFrames(2).setRequestedPort1(Port.PORT_ANY)
				.setRequestedPort2(Port.PORT_2).setRequestedPort3(Port.PORT_ANY)
				.build();
		server.plugController(pReq, plugControllerObserver);
		pReq = PlugControllerRequestPB.newBuilder()
				.setConsoleId(id).setDelayFrames(2).setRequestedPort1(Port.PORT_ANY)
				.build();
		server.plugController(pReq, plugControllerObserver);
		PlugControllerResponsePB resp = plugControllerObserver.getNextValue();
		assertEquals(3, resp.getPortCount());
		assertTrue(resp.getPortList().contains(Port.PORT_2));
		resp = plugControllerObserver.getNextValue();
		assertEquals(1, resp.getPortCount());
	}
	
	/**
	 * Tests the normal flow for two players joining and then starting a game.
	 */
	@Test
	public void testReadyAndStartGame() {
		server.makeConsole(makeConsoleReq, makeConsoleObserver);
		long id = makeConsoleObserver.getNextValue().getConsoleId();
		PlugControllerRequestPB pReq = PlugControllerRequestPB.newBuilder()
				.setConsoleId(id).setDelayFrames(2).setRequestedPort1(Port.PORT_ANY)
				.build();
		server.plugController(pReq, plugControllerObserver);
		server.plugController(pReq, plugControllerObserver);
		long clientId1 = plugControllerObserver.getNextValue().getClientId();
		long clientId2 = plugControllerObserver.getNextValue().getClientId();
		OutgoingEventPB readyPb = OutgoingEventPB.newBuilder().setClientReady(
				ClientReadyPB.newBuilder().setClientId(clientId1).setConsoleId(id)
				.build()).build();
		ResponseObserver<IncomingEventPB> eventObserver1 = new ResponseObserver<IncomingEventPB>();
		
		// Begin the streams
		StreamObserver<OutgoingEventPB> outgoingSender1 = server.sendEvent(eventObserver1);
		outgoingSender1.onNext(readyPb);
		
		readyPb = OutgoingEventPB.newBuilder().setClientReady(
				ClientReadyPB.newBuilder().setClientId(clientId2).setConsoleId(id)
				.build()).build();
		ResponseObserver<IncomingEventPB> eventObserver2 = new ResponseObserver<IncomingEventPB>();
		StreamObserver<OutgoingEventPB> outgoingSender2 = server.sendEvent(eventObserver2);
		outgoingSender2.onNext(readyPb);
		
		StartGameRequestPB startRequest = StartGameRequestPB.newBuilder().setConsoleId(id).build();
		server.startGame(startRequest, startGameObserver);
		
		IncomingEventPB startResp1 = eventObserver1.getNextValue();
		IncomingEventPB startResp2 = eventObserver2.getNextValue();
		assertTrue(startResp1.hasStartGame());
		assertTrue(startResp2.hasStartGame());
		assertEquals(startResp1.getStartGame().getConsoleId(), id);
		assertEquals(2, startResp1.getStartGame().getConnectedPortsCount());
		assertEquals(2, startResp2.getStartGame().getConnectedPortsCount());
	}
	
	/**
	 * Test when start game has been called but not all players are ready.
	 */
	@Test
	public void testNotAllPlayersReady() {
		server.makeConsole(makeConsoleReq, makeConsoleObserver);
		long id = makeConsoleObserver.getNextValue().getConsoleId();
		
		PlugControllerRequestPB pReq = PlugControllerRequestPB.newBuilder()
				.setConsoleId(id).setDelayFrames(2).setRequestedPort1(Port.PORT_1)
				.build();
		server.plugController(pReq, plugControllerObserver);
		pReq = PlugControllerRequestPB.newBuilder()
				.setConsoleId(id).setDelayFrames(2).setRequestedPort1(Port.PORT_2)
				.build();
		
		long clientId1 = plugControllerObserver.getNextValue().getClientId();
		
		OutgoingEventPB readyPb = OutgoingEventPB.newBuilder().setClientReady(
				ClientReadyPB.newBuilder().setClientId(clientId1).setConsoleId(id)
				.build()).build();
		ResponseObserver<IncomingEventPB> eventObserver1 = new ResponseObserver<IncomingEventPB>();
		
		// Send ready only for player 1
		StreamObserver<OutgoingEventPB> outgoingSender1 = server.sendEvent(eventObserver1);
		outgoingSender1.onNext(readyPb);
		ResponseObserver<IncomingEventPB> eventObserver2 = new ResponseObserver<IncomingEventPB>();
		server.sendEvent(eventObserver2);
		
		StartGameRequestPB startRequest = StartGameRequestPB.newBuilder().setConsoleId(id).build();
		server.startGame(startRequest, startGameObserver);
		assertEquals(StartGameResponsePB.Status.UNSPECIFIED_FAILURE,
				startGameObserver.getNextValue().getStatus());
	}
	
	@Test
	public void testSendKeypresses() {
		server.makeConsole(makeConsoleReq, makeConsoleObserver);
		long id = makeConsoleObserver.getNextValue().getConsoleId();
		
		PlugControllerRequestPB pReq = PlugControllerRequestPB.newBuilder()
				.setConsoleId(id).setDelayFrames(2).setRequestedPort1(Port.PORT_1)
				.build();
		server.plugController(pReq, plugControllerObserver);
		pReq = PlugControllerRequestPB.newBuilder()
				.setConsoleId(id).setDelayFrames(2).setRequestedPort1(Port.PORT_2)
				.build();
		
		long clientId1 = plugControllerObserver.getNextValue().getClientId();
		
		OutgoingEventPB readyPb = OutgoingEventPB.newBuilder().setClientReady(
				ClientReadyPB.newBuilder().setClientId(clientId1).setConsoleId(id)
				.build()).build();
		ResponseObserver<IncomingEventPB> eventObserver1 = new ResponseObserver<IncomingEventPB>();
		
		// Send ready only for player 1
		StreamObserver<OutgoingEventPB> outgoingSender1 = server.sendEvent(eventObserver1);
		outgoingSender1.onNext(readyPb);
		ResponseObserver<IncomingEventPB> eventObserver2 = new ResponseObserver<IncomingEventPB>();
		server.sendEvent(eventObserver2);

		
	}

	@Test
	public void testTestServerWithMoreThanTenConsoles() {
		Server testServer = new Server(true);
		Server prodServer = new Server(false);
		
		for (int i = 0; i < 10; ++i) {
			testServer.makeConsole(makeConsoleReq, makeConsoleObserver);
			assertEquals(MakeConsoleResponsePB.Status.SUCCESS, makeConsoleObserver.getNextValue().getStatus());
			prodServer.makeConsole(makeConsoleReq, makeConsoleObserver);
			assertEquals(MakeConsoleResponsePB.Status.SUCCESS, makeConsoleObserver.getNextValue().getStatus());
		}
		
		// The test server should fail to create a new console.
		try {
			testServer.makeConsole(makeConsoleReq, makeConsoleObserver);
			fail();
		} catch (IllegalStateException e) {}
		
		// The prod server should not fail
		prodServer.makeConsole(makeConsoleReq, makeConsoleObserver);
		assertEquals(MakeConsoleResponsePB.Status.SUCCESS, makeConsoleObserver.getNextValue().getStatus());
	}
	
	private class ResponseObserver<T> implements StreamObserver<T> {
		
		private Queue<T> respQueue = Queues.newConcurrentLinkedQueue();
		public Throwable lastThrowable;
		public boolean completed = false;

		@Override
		public void onNext(T value) {
			respQueue.add(value);			
		}

		@Override
		public void onError(Throwable t) {
			lastThrowable = t;
		}

		@Override
		public void onCompleted() {
			completed = true;
		}
		
		public T getNextValue() {
			if (respQueue.isEmpty()) {
				fail("Response queue is empty");
			}
			return respQueue.remove();
		}
		
		/**
		 * Flushes all messages from the queue.
		 */
		public void clear() {
			respQueue.clear();
		}	
	}
	
}
