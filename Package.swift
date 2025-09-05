// swift-tools-version: 6.1
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "ImageTools",
    // See the "Minimum Deployment Version for Reference Types Imported from C++":
    // https://www.swift.org/documentation/cxx-interop/status/
    platforms: [
        .macOS(.v14),
        .iOS(.v17),
        .tvOS(.v17),
        .watchOS(.v10),
        .visionOS(.v1)
    ],
    products: [
        .library(
            name: "ImageToolsC",
            targets: ["ImageToolsC"]
        ),
        .library(
            name: "ImageTools",
            targets: ["ImageTools"]
        ),
    ],
    targets: [
        .target(
            name: "ImageToolsC"
        ),
        .target(
            name: "ImageTools",
            dependencies: [
                .target(name: "ImageToolsC")
            ],
            //cSettings: [
            //    .unsafeFlags(["-Wall"])
            //],
            //cxxSettings: [
            //    .unsafeFlags(["-Wall"])
            //],
            swiftSettings: [
                .interoperabilityMode(.Cxx)
            ]
        ),
    ],
    cLanguageStandard: .c17,
    cxxLanguageStandard: .cxx20
)
