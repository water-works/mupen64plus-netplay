package netplayServer;

import java.util.List;
import java.util.Map;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import com.google.common.collect.Maps;

import io.grpc.stub.StreamObserver;
import netplayprotos.NetPlayServerServiceGrpc.NetPlayServerService;
import netplayprotos.NetplayServiceProto.IncomingEventPB;
import netplayprotos.NetplayServiceProto.MakeConsoleRequestPB;
import netplayprotos.NetplayServiceProto.MakeConsoleResponsePB;
import netplayprotos.NetplayServiceProto.MakeConsoleResponsePB.Status;
import netplayprotos.NetplayServiceProto.OutgoingEventPB;
import netplayprotos.NetplayServiceProto.PingPB;
import netplayprotos.NetplayServiceProto.PlugControllerRequestPB;
import netplayprotos.NetplayServiceProto.PlugControllerResponsePB;
import netplayprotos.NetplayServiceProto.PlugControllerResponsePB.PortRejectionPB;
import netplayprotos.NetplayServiceProto.ShutDownServerRequestPB;
import netplayprotos.NetplayServiceProto.ShutDownServerResponsePB;
import netplayprotos.NetplayServiceProto.StartGameRequestPB;
import netplayprotos.NetplayServiceProto.StartGameResponsePB;

/**
 * The main server, which implements the grpc interface.
 * 
 * This class should contain as little functionality as possible and pass off logic
 * to other classes.
 *
 */
public class Server implements NetPlayServerService {
	
	private Log log = LogFactory.getLog(Server.class);
		
	private Map<Long, Console> consoleMap;
	private final boolean testMode;
	
	public Server(boolean testMode) {
		consoleMap = Maps.newConcurrentMap();	
		this.testMode = testMode;
	}

	@Override
	public void ping(PingPB request, StreamObserver<PingPB> responseObserver) {
		responseObserver.onNext(request);
		responseObserver.onCompleted();
	}

	private int numConsolesCreated = 0;
	
	@Override
	public void makeConsole(MakeConsoleRequestPB request, StreamObserver<MakeConsoleResponsePB> responseObserver) {
		log.debug("Received makeConsole request: " + request.toString());
		
		if (testMode && numConsolesCreated >= 10) {
			throw new IllegalStateException("Server running in test mode created a client with ID greater than 10");			
		}
		
		Console newConsole = new Console();
		numConsolesCreated++;
		
		long id = newConsole.getId();
		consoleMap.put(id, newConsole);
		responseObserver.onNext(createMakeConsoleResponse(id));
		responseObserver.onCompleted();
	}
	
	private MakeConsoleResponsePB createMakeConsoleResponse(long id) {
		if (id > 0) {
		return MakeConsoleResponsePB.newBuilder().setConsoleId(id)
				.setStatus(Status.SUCCESS).build();
		}
		else {
			return MakeConsoleResponsePB.newBuilder().setConsoleId(id)
					.setStatus(Status.UNSPECIFIED_FAILURE).build();
		}
	}

	@Override
	public void plugController(PlugControllerRequestPB request,
			StreamObserver<PlugControllerResponsePB> responseObserver) {
		if (!consoleMap.containsKey(request.getConsoleId())) {
			PlugControllerResponsePB resp = PlugControllerResponsePB.newBuilder()
			.setStatus(PlugControllerResponsePB.Status.UNSPECIFIED_FAILURE)
			.setConsoleId(request.getConsoleId()).build();
			responseObserver.onNext(resp);
			responseObserver.onCompleted();
			return;
		}
		Console console = consoleMap.get(request.getConsoleId());
		Client client = null;
		try {
			client = console.tryAddPlayers(request.getDelayFrames(), 
					request.getRequestedPort1(),
					request.getRequestedPort2(), request.getRequestedPort3(),
					request.getRequestedPort4());
		} catch (PlugRequestException e) {
			responseObserver.onNext(getErrorResp(e.getRejections()));
			responseObserver.onCompleted();
			return;
		}
		long clientId = client.getId();
		PlugControllerResponsePB resp = PlugControllerResponsePB.newBuilder()
				.setStatus(PlugControllerResponsePB.Status.SUCCESS)
				.addAllPort(client.getPorts())
				.setClientId(clientId)
				.setConsoleId(console.getId())
				.build();
		responseObserver.onNext(resp);
		responseObserver.onCompleted();
		
	}

	private PlugControllerResponsePB getErrorResp(List<PortRejectionPB> rejections) {
		if (rejections.isEmpty()) {
			return PlugControllerResponsePB.newBuilder()
					.setStatus(PlugControllerResponsePB.Status.NO_PORTS_REQUESTED)
					.build();
		}
		return PlugControllerResponsePB.newBuilder()
				.setStatus(PlugControllerResponsePB.Status.PORT_REQUEST_REJECTED)
				.addAllPortRejections(rejections).build();
	}

	/**
	 * Request from a client to start the game.
	 * Note that all connected clients must be ready before game begins.
	 */
	@Override
	public void startGame(StartGameRequestPB request, StreamObserver<StartGameResponsePB> responseObserver) {
		if (!consoleMap.containsKey(request.getConsoleId())) {
			StartGameResponsePB resp = StartGameResponsePB.newBuilder()
					.setStatus(StartGameResponsePB.Status.UNSPECIFIED_FAILURE)
					.build();
			responseObserver.onNext(resp);
			responseObserver.onCompleted();
			return;
		}
		Console console = consoleMap.get(request.getConsoleId());
		if (!console.verifyClientsReady()) {
			StartGameResponsePB resp = StartGameResponsePB.newBuilder()
					.setStatus(StartGameResponsePB.Status.UNSPECIFIED_FAILURE)
					.build();
			responseObserver.onNext(resp);
			responseObserver.onCompleted();
			return;
		}
		console.broadcastStartGame();
		StartGameResponsePB resp = StartGameResponsePB.newBuilder()
				.setConsoleId(console.getId()).setStatus(StartGameResponsePB.Status.SUCCESS)
				.build();
		responseObserver.onNext(resp);
		responseObserver.onCompleted();
	}

	@Override
	public StreamObserver<OutgoingEventPB> sendEvent(StreamObserver<IncomingEventPB> responseObserver) {
		return new ClientHandoffStreamObserver<OutgoingEventPB>(responseObserver);
	}

	/**
	 * Implementation of an observer that handles incoming messages to the server.
	 * We unfortunately do not know what client this is for until after we receive a request.
	 * 
	 * @param <T> The proto message to observe
	 */
	private class ClientHandoffStreamObserver<T> implements StreamObserver<OutgoingEventPB> {
		
		private StreamObserver<IncomingEventPB> incomingStream;
		
		public ClientHandoffStreamObserver(StreamObserver<IncomingEventPB> responseObserver) {
			this.incomingStream = responseObserver;
		}
		
		private Client client;
		
		@Override
		public void onNext(OutgoingEventPB value) {
			if (client != null) {
				client.onNext(value);
			}
			else if (value.hasClientReady()) {
				Console console = consoleMap.get(value.getClientReady().getConsoleId());
				this.client = console.getClientById(value.getClientReady().getClientId());
				client.setStreamObserver(incomingStream);
				client.setReady();
			} else {
				log.warn(String.format("Stream message sent without client ready first - ignoring: %s",
						value));
			}
		}

		@Override
		public void onError(Throwable t) {
			if (client == null) {
				log.warn(String.format("Error with no client set"), t);
			} else {
				client.onError(t);
			}
		}

		@Override
		public void onCompleted() {
			if (client == null) {
				log.warn(String.format("Completed with no client set"));
			} else {
				client.onCompleted();
			}
		}
		
	}
	
	private io.grpc.Server serverImpl;

	/**
	 * Gives this server a handle to the server implementation wrapper that wraps it.
	 */
	public void setServerImpl(io.grpc.Server serverImpl) {
		this.serverImpl = serverImpl;
	}
	
	/**
	 * Requests that the server shut itself down. The server will only shut itself down if it is in test mode and if it
	 * has had a server handle passed to it via {@link setServerImpl}.
	 */
	@Override
	public void shutDownServer(ShutDownServerRequestPB request,
	        StreamObserver<ShutDownServerResponsePB> responseObserver) {
		if (testMode && serverImpl != null) {
			ShutDownServerResponsePB resp = ShutDownServerResponsePB.newBuilder().setServerWillDie(true).build();
			responseObserver.onNext(resp);
			responseObserver.onCompleted();
			log.info("Shutting down test server.");
			serverImpl.shutdown();
		} else {
			ShutDownServerResponsePB resp = ShutDownServerResponsePB.newBuilder().setServerWillDie(false).build();
			responseObserver.onNext(resp);
			responseObserver.onCompleted();
		}
	}
}
