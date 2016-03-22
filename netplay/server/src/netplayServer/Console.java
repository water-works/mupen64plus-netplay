package netplayServer;

import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.atomic.AtomicLong;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import com.google.common.collect.ImmutableList;
import com.google.common.collect.Lists;
import com.google.common.collect.Maps;
import com.google.common.collect.Sets;

import netplayServer.Client.ClientStatus;
import netplayprotos.NetplayServiceProto.KeyStatePB;
import netplayprotos.NetplayServiceProto.PlugControllerResponsePB.PortRejectionPB;
import netplayprotos.NetplayServiceProto.PlugControllerResponsePB.PortRejectionPB.Builder;
import netplayprotos.NetplayServiceProto.PlugControllerResponsePB.PortRejectionPB.Reason;
import netplayprotos.NetplayServiceProto.Port;

/**
* A console represents the physical game console that
* maintains the state of the game, and also provides a mapping
* from controller port to netplay client, which can be used to send
* events.
*/
public class Console {
	
	private Log log = LogFactory.getLog(Server.class);

	public enum ConsoleStatus {
		UNKNOWN  (0),
		CREATED  (1),
		POWERED  (2),
		DONE	 (3)
		;
		
		private final int status_code;
		
		private ConsoleStatus(int code) {
			this.status_code = code;
		}
	}
	private List<Port> allPorts = 
			ImmutableList.of(Port.PORT_1, Port.PORT_2, Port.PORT_3, Port.PORT_4);

	
	private static AtomicLong atomicId = new AtomicLong(1);

	private Map<Port, Client> clientPortMap;
	private long consoleId;
	private ConsoleStatus status;
	
	public Console() {
		this.consoleId = atomicId.getAndIncrement();
		this.clientPortMap = Maps.newConcurrentMap();
		this.status = ConsoleStatus.CREATED;
	}
	
	private Set<Client> allClients() {
		return Sets.newHashSet(clientPortMap.values());
	}
	
	/**
	 * Returns the id of this console.
	 */
	public long getId() {
		return consoleId;
	}
	
	/**
	 * Attempts to add the players to this game.  This method will first attempt to assign named ports, 
	 * and then handle the request for PORT_ANY.
	 * 
	 * If this method throws an exception, the ports are not added.  Otherwise, returns the newly
	 * assigned ports.
	 * @param ports
	 * @return
	 */
	public synchronized Client tryAddPlayers(int delay, Port ... ports) throws PlugRequestException {
		Set<Port> availablePorts = Sets.newHashSet();
		List<PortRejectionPB> rejectionList = Lists.newArrayList();
		for (Port port : allPorts) {
			if (!clientPortMap.containsKey(port))
				availablePorts.add(port);
		}
		Set<Port> assignedPorts = Sets.newHashSet();

		boolean canAssign = true;

		List<Port> requestPortList = Lists.newArrayList(ports);
		Collections.sort(requestPortList);

		for (Port requestPort : requestPortList) {
			if (requestPort.equals(Port.UNKNOWN) || requestPort.equals(Port.UNRECOGNIZED)) {
				continue;
			}
			Builder rejectionsBuilder = PortRejectionPB.newBuilder().setPort(requestPort)
					.setReason(Reason.ACCEPTABLE);
			if (requestPort.equals(Port.PORT_ANY)) {
				if (availablePorts.size() > 0) {
					Port nextPort = availablePorts.iterator().next();
					availablePorts.remove(nextPort);
					rejectionsBuilder.clear().setPort(nextPort).setReason(Reason.ACCEPTABLE);
					assignedPorts.add(nextPort);
				} else {
					canAssign = false;
					rejectionsBuilder.clear().setPort(requestPort)
					.setReason(Reason.ALL_PORTS_OCCUPIED);
				}
			} else {
				if (clientPortMap.containsKey(requestPort)) {
					canAssign = false;
					rejectionsBuilder.clear().setPort(requestPort)
					.setReason(Reason.PORT_ALREADY_OCCUPIED);
				} else {
					assignedPorts.add(requestPort);
					availablePorts.remove(requestPort);
				}
			}
			rejectionList.add(rejectionsBuilder.build());
		}
		if (!canAssign) {
			throw new PlugRequestException(rejectionList);
		}
		Client newClient = makeNewClient(assignedPorts, delay);
		return newClient;
	}
	
	private Client makeNewClient(Set<Port> assignedPorts, int delay) throws PlugRequestException {
		Client newClient = new Client(this, delay);
		if (assignedPorts.isEmpty()) {
			throw new PlugRequestException(null);
		}
		for (Port port : assignedPorts) {
			clientPortMap.put(port, newClient);
			Player player = new Player(port, newClient);
			newClient.addPlayer(player);
		}
		return newClient;
	}
	
	public Client getClientById(long id) {
		for (Client client : clientPortMap.values()) {
			if (client.getId() == id) {
				return client;
			}
		}
		return null;
	}
	
	/**
	 * Returns true if all connected clients are in the ready state,
	 * false otherwise.
	 */
	public boolean verifyClientsReady() {	
		for (Client client : clientPortMap.values()) {
			if (!client.getStatus().equals(ClientStatus.READY)) {
				log.warn(String.format("Start game called with client that is not ready: %s",
						client));
				return false;
			}
		}
		return true;
	}

	/*
	 * Broadcasts keys from one client to the rest of the clients.
	 * The console will not perform any processing on the keypresses,
	 * and will not broadcast back to the senderId.
	 */
	public void broadcastKeypresses(List<KeyStatePB> keyPressList, long senderId) {
		log.debug(String.format("Broadcasting keys from client %d:  %s", senderId, 
				keyPressList));
		for (Client client : allClients()) {
			if (client.getId() == senderId) {
				continue;
			}
			client.acceptKeyPresses(keyPressList);
		}
	}
	
	public Map<Port, Integer> getPortDelayMap() {
		Map<Port, Integer> portDelayMap = Maps.newConcurrentMap();
		for (Client client : allClients()) {
			for (Port port : client.getPorts()) {
				portDelayMap.put(port, client.getDelay());
			}
		}
		return portDelayMap;
	}
	
	/**
	 * Broadcasts the ready game message to all clients.
	 * Behavior is undefined if not all clients are ready.
	 */
	public void broadcastStartGame() {
		log.debug(String.format("Broadcasting start game from client"));
		for (Client client : allClients()) {
			client.acceptStartGame();
		}	
	}

}
