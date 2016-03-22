package netplayServer;

import java.io.IOException;

import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.CommandLineParser;
import org.apache.commons.cli.DefaultParser;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import netplayprotos.NetPlayServerServiceGrpc;
import io.grpc.internal.ServerImpl;
import io.grpc.netty.NettyServerBuilder;

public class ServerStart {
	
	private static Log log = LogFactory.getLog(Server.class);

	public static void main(String [] args) throws IOException, InterruptedException {
		// Parse commandline arguments
		Options options = new Options();
		options.addOption("p", "port", true, "Port number on which to run. Defaults to 10001.");
		options.addOption("testMode", "testMode", false, "If set, run in test mode.");
		CommandLineParser parser = new DefaultParser();
		CommandLine cmd = null;
		try {
			cmd = parser.parse(options, args);
		} catch (ParseException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
		int port = 0;
		if (cmd.hasOption("p")) {
			port = Integer.parseInt(cmd.getOptionValue("p"));
		} else {
			port = 10001;
		}
		boolean testMode = cmd.hasOption("testMode");
		
		Server server = new Server(testMode);
		
		ServerImpl serverImpl = NettyServerBuilder.forPort(port)
		.addService(NetPlayServerServiceGrpc.bindService(server))
        .build();
		
		server.setServerImpl(serverImpl);
		
		serverImpl.start();
		log.info("Server started on port " + port);
		if (testMode) {
			log.warn("Server started in test mode");
		}
		serverImpl.awaitTermination();
	}
}
