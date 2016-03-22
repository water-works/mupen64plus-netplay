package netplayServer;

import java.util.List;

import netplayprotos.NetplayServiceProto.PlugControllerResponsePB.PortRejectionPB;

/**
 * Exception thrown when a problem occurred plugging into a console.
 * The exception will contain a list of port rejections, useful for returning
 * to a client.  If rejections are null, then no ports were requested.
 */
@SuppressWarnings("serial")
public class PlugRequestException extends Exception {
	
	private List<PortRejectionPB> rejections;

	public PlugRequestException(List<PortRejectionPB> rejectionList) {
		this.rejections = rejectionList;
	}
	
	public List<PortRejectionPB> getRejections() {
		return rejections;
	}
}
