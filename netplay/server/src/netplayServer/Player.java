package netplayServer;

import java.util.concurrent.atomic.AtomicLong;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import netplayprotos.NetplayServiceProto.Port;

public class Player {
	public static AtomicLong atomicId = new AtomicLong();
	private Log log = LogFactory.getLog(this.getClass());


	private long id;
	private Port port;
	private Client client;
	
	public Player(Port port, Client client) {
		this.id = atomicId.getAndIncrement();
		this.port = port;
		this.client = client;
	}
	
	public Player(Port port) {
		this(port, null);
	}
	
	public void setClient(Client newClient) {
		log.debug(String.format("Changing client from %s, to %s", client, newClient));
		this.client = newClient;
	}

	public long getId() {
		return id;
	}

	public Port getPort() {
		return port;
	}

	public Client getClient() {
		return client;
	}
	
	@Override
	public String toString() {
		return String.format("Id: %d, Port: %d, ClientId: %d",
				id, port.getNumber(), 
				client != null ? client.getId() : 0);
	}
}
