import gRPC

public class NetplayClient {
    public init() {
        gRPC.initialize()
    }
    
    public func greet() -> String {
        return "Hello, world!"
    }
}
