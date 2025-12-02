// swift-tools-version: 6.2
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription


let dependencies: [Package.Dependency] = {
#if true
    [
        .package(url: "https://github.com/EvgenijLutz/LibPNG.git", from: .init(1, 6, 50)),
        .package(url: "https://github.com/EvgenijLutz/LittleCMS.git", from: .init(2, 17, 1)),
        .package(url: "https://github.com/EvgenijLutz/ASTCEncoder.git", exact: "5.3.0-rev2"),
        //.package(url: "https://github.com/EvgenijLutz/ASTCEncoder.git", from: .init(5, 3, 0)),
    ]
#else
    [
        .package(name: "LibPNG", path: "../LibPNG"),
        .package(name: "LittleCMS", path: "../LittleCMS"),
        .package(name: "ASTCEncoder", path: "../ASTCEncoder"),
    ]
#endif
}()


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
    dependencies: dependencies,
    targets: [
        .target(
            name: "ImageToolsC",
            dependencies: [
                .product(name: "LibPNGC", package: "LibPNG"),
                .product(name: "LCMS2C", package: "LittleCMS"),
                .product(name: "ASTCEncoderC", package: "ASTCEncoder")
            ],
            cxxSettings: [
                .enableWarning("all")
            ]
        ),
        .target(
            name: "ImageTools",
            dependencies: [
                .product(name: "LibPNG", package: "LibPNG"),
                .product(name: "LittleCMS", package: "LittleCMS"),
                .product(name: "ASTCEncoder", package: "ASTCEncoder"),
                .target(name: "ImageToolsC")
            ],
            swiftSettings: [
                .interoperabilityMode(.Cxx),
                //.strictMemorySafety()
            ]
        ),
    ],
    cLanguageStandard: .c17,
    cxxLanguageStandard: .cxx20
)
