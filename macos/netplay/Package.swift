// swift-tools-version:4.0

import PackageDescription

let package = Package(
    name: "netplay",
    products: [
        .library(
            name: "Netplay",
            targets: ["Netplay"]),
    ],
    dependencies: [
        .package(url: "https://github.com/grpc/grpc-swift.git", from: "0.3.3"),
        .package(url: "https://github.com/apple/swift-protobuf.git", from: "1.0.2"),
    ],
    targets: [
        .target(
            name: "Netplay",
            dependencies: [
                "SwiftGRPC",
                "SwiftProtobuf",
            ]),
        .testTarget(
            name: "NetplayTests",
            dependencies: ["Netplay"]),
    ]
)
