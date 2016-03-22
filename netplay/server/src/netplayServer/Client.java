package netplayServer;

import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;
import java.util.concurrent.atomic.AtomicLong;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import com.google.common.collect.ImmutableSet;
import com.google.common.collect.ImmutableSet.Builder;
import com.google.common.collect.Sets;

import io.grpc.stub.StreamObserver;
import netplayprotos.NetplayServiceProto.IncomingEventPB;
import netplayprotos.NetplayServiceProto.InvalidDataPB;
import netplayprotos.NetplayServiceProto.KeyStatePB;
import netplayprotos.NetplayServiceProto.OutgoingEventPB;
import netplayprotos.NetplayServiceProto.Port;
import netplayprotos.NetplayServiceProto.StartGamePB;
import netplayprotos.NetplayServiceProto.StartGamePB.ConnectedPortPB;

/**
 * The client class represents a destination for incoming/outgoing events.
 * A client may contain any number of players occupying ports.
 */
public class Client implements StreamObserver<OutgoingEventPB> {
	
	private Log log = LogFactory.getLog(this.getClass());
	
	public enum ClientStatus {
		UNKNOWN (0),
		CREATED (1),
		READY (2),
		PLAYING (3),
		DONE (4)
		;
		
		int statusInt;
		
		ClientStatus(int num) {
			this.statusInt = num;
		}
	}
	
	public static AtomicLong atomicId = new AtomicLong();
	private int delay;
	private Set<Player> players;
	private long clientId;
	private ClientStreamHandler streamHandler;
	private Console console;
	private ClientStatus status;
	
	public Client(Console console, int delay) {
		this.clientId = atomicId.incrementAndGet();
		players = Sets.newConcurrentHashSet();
		this.console = console;
		this.status = ClientStatus.CREATED;
		this.delay = delay;
	}
	
	/**
	 * Sets this client to ready state - must be called
	 * before the start of the game.
	 */
	public void setReady() {
		this.status = ClientStatus.READY;
	}
	
	/**
	 * Adds a player to this client.
	 * Will not add a duplicate player.
	 */
	public void addPlayer(Player player) {
		players.add(player);
		player.setClient(this);
	}
	
	/*
	 * Returns the player at this port or null if port is unoccupied.
	 */
	public Player getPlayer(Port port) {
		for (Player player : players) {
			if (player.getPort().equals(port)) {
				return player;
			}
		}
		return null;
	}
	

	public long getId() {
		return clientId;
	}
	
	public ClientStatus getStatus() {
		return status;
	}
	
	public int getDelay() {
		return delay;
	}
	
	/**
	 * Convenience method to return all the ports in this client
	 * @return A set of ports - will not contain duplicates.
	 */
	public Set<Port> getPorts() {
		Builder<Port> portSet = ImmutableSet.builder();
		for (Player p : players) {
			portSet.add(p.getPort());
		}
		log.debug(String.format("Client %d has players: %s", clientId, players.toString()));
		return portSet.build();
	}
	
	public void setStreamObserver(StreamObserver<IncomingEventPB> incomingStream) {	
		if (streamHandler == null) {
			this.streamHandler = new ClientStreamHandler(incomingStream);
		} else {
			log.warn(String.format("Set stream observer called for client: %d but it has"
					+ "already been set", clientId));
		}
	}
	
	/**
	 * Sends the start game message on the outgoing stream.
	 * Behavior is undefined if client is not ready.
	 */
	public void acceptStartGame() {
		if (streamHandler == null) {
			throw new IllegalStateException(
					String.format("acceptStartGame called on client %d with no handler.",
							clientId));
		}
		Set<ConnectedPortPB> playerList = Sets.newConcurrentHashSet();
		Map<Port, Integer> delayMap = console.getPortDelayMap();
		for (Entry<Port, Integer> entry : delayMap.entrySet()) {
			playerList.add(ConnectedPortPB.newBuilder()
					.setPort(entry.getKey())
					.setDelayFrames(entry.getValue())
					.build());
		}
		streamHandler.returnStartGame(playerList);		
	}
	
	/**
	 * Sends the keypresses on the outgoing stream.
	 * The console for this client should be sending the keypresses
	 * back to the client.
	 */
	public void acceptKeyPresses(List<KeyStatePB> keyPressList) {
		if (streamHandler == null) {
			throw new IllegalStateException(
					String.format("Accept called on client %d with no handler.", clientId));
		}
		streamHandler.returnKeypresses(keyPressList);
	}
	
	@Override
	public void onNext(OutgoingEventPB value) {
		if (streamHandler == null) {
			throw new IllegalStateException(
					String.format("onNext called on client %d with no handler.  Message: %s",
							clientId, value));
		}
		streamHandler.onNext(value);
	}
	
	@Override
	public void onError(Throwable t) {
		if (streamHandler == null) {
			throw new IllegalStateException(
					String.format("Error called on client %d with no handler.", clientId));
		}
		streamHandler.onError(t);
	}
	
	@Override
	public void onCompleted() {
		if (streamHandler == null) {
			throw new IllegalStateException(
					String.format("Accept called on client %d with no handler.", clientId));
		}
		streamHandler.onCompleted();
	}
	
	@Override
	public String toString() {
		StringBuilder playerBuilder = new StringBuilder();
		for (Player p : players) {
			playerBuilder.append(p).append("/n");
		}
		return String.format("Id: %d, Players: %s", clientId, playerBuilder.toString());
	}
	
	/**
	 * Class within the client to handle the streaming back of messages.
	 * Unless gRPC supports a way to identify a machine through service methods, we'll
	 * need to encapsulate this in the client and have it also implement the interface.
	 * This is because we need to retrieve the Client Ready message in order to know who we
	 * are talking to.
	 * Ideally, the server should be able to return this class directly through the client.
	 */
	private class ClientStreamHandler implements StreamObserver<OutgoingEventPB>{
		
		private StreamObserver<IncomingEventPB> incomingStream;
		
		public ClientStreamHandler(StreamObserver<IncomingEventPB> incomingStream) {
			this.incomingStream = incomingStream;
		}
		
		public void returnKeypresses(List<KeyStatePB> presses) {
			IncomingEventPB event = IncomingEventPB.newBuilder()
					.addAllKeyPress(presses)
					.build();
			incomingStream.onNext(event);
		}
		
		public void returnStartGame(Set<ConnectedPortPB> connectedPorts) {
			StartGamePB.Builder startPB = StartGamePB.newBuilder();
			startPB.addAllConnectedPorts(connectedPorts);
			startPB.setConsoleId(console.getId());
			IncomingEventPB event = IncomingEventPB.newBuilder()
					.setStartGame(startPB.build())
					.build();
			incomingStream.onNext(event);
		}

		@Override
		public void onNext(OutgoingEventPB value) {
			if (value.hasClientReady()) {
				if (status == ClientStatus.CREATED) {
					log.info(String.format("Client %d is now ready.", clientId));
					status = ClientStatus.READY;
				}
				return;
			}
			for (KeyStatePB keypress : value.getKeyPressList()) {
				if (keypress.getConsoleId() != console.getId()) {
					log.info(String.format("Console %d: invalid id received: %d", console.getId(), 
							keypress.getConsoleId()));
					sendInvalidDataPb(keypress, InvalidDataPB.Status.INVALID_CONSOLE);
					return;
				}
				if (!getPorts().contains(keypress.getPort())) {
					sendInvalidDataPb(keypress, InvalidDataPB.Status.INVALID_PORT);
					return;
				}
			}
			console.broadcastKeypresses(value.getKeyPressList(), clientId);			
		}

		@Override
		public void onError(Throwable t) {
			log.warn(String.format("Error on stream for client %d: %s", clientId, t));
		}

		@Override
		public void onCompleted() {
			log.info(String.format("Stream closed for client: %d", clientId));
			status = ClientStatus.DONE;
		}
		
		private void sendInvalidDataPb(KeyStatePB key, InvalidDataPB.Status reason) {
			InvalidDataPB data = InvalidDataPB.newBuilder()
					.setConsoleId(key.getConsoleId())
					.setPort(key.getPort())
					.setStatus(reason).build();
			IncomingEventPB event = IncomingEventPB.newBuilder()
					.addInvalidData(data)
					.build();
			incomingStream.onNext(event);
		}
				
	}
}
