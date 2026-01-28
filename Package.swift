// swift-tools-version: 6.2
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription


let dependencies: [Package.Dependency] = {
#if false
    [
        .package(url: "https://github.com/EvgenijLutz/FastTGA.git", from: "1.0.1"),
        .package(url: "https://github.com/EvgenijLutz/JPEGTurbo.git", from: "3.1.3"),
        .package(url: "https://github.com/EvgenijLutz/LibPNG.git", from: .init(1, 6, 50)),
        .package(url: "https://github.com/EvgenijLutz/LittleCMS.git", from: .init(2, 17, 3)),
        .package(url: "https://github.com/EvgenijLutz/ASTCEncoder.git", exact: "5.3.0-rev2"),
        //.package(url: "https://github.com/EvgenijLutz/ASTCEncoder.git", from: .init(5, 3, 0)),
    ]
#else
    [
        .package(name: "FastTGA", path: "../FastTGA"),
        .package(name: "JPEGTurbo", path: "../JPEGTurbo"),
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
        .macOS(.v10_13),
        .iOS(.v12),
        .tvOS(.v12),
        .watchOS(.v8),
        .visionOS(.v1),
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
                .product(name: "FastTGAC", package: "FastTGA"),
                .product(name: "JPEGTurboC", package: "JPEGTurbo"),
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
                .product(name: "FastTGA", package: "FastTGA"),
                .product(name: "JPEGTurbo", package: "JPEGTurbo"),
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
