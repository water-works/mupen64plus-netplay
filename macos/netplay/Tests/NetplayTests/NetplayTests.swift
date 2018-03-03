import XCTest
@testable import Netplay

class NetplayTests: XCTestCase {
    func testExample() {
        // This is an example of a functional test case.
        // Use XCTAssert and related functions to verify your tests produce the correct
        // results.
        XCTAssertEqual(NetplayClient().greet(), "Hello, world!")
    }


    static var allTests = [
        ("testExample", testExample),
    ]
}
