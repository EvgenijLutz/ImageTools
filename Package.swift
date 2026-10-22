// swift-tools-version: 6.4
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription


let dependencies: [Package.Dependency] = {
#if true
    [
        .package(url: "https://github.com/EvgenijLutz/FastTGA.git", from: "1.2.0"),
        .package(url: "https://github.com/EvgenijLutz/JPEGTurbo.git", from: "3.2.0"),
        .package(url: "https://github.com/EvgenijLutz/LibPNG.git", from: "1.6.58"),
        .package(url: "https://github.com/EvgenijLutz/LittleCMS.git", from: "2.19.1"),
        .package(url: "https://github.com/EvgenijLutz/ASTCEncoder.git", from: "5.7.0"),
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
        .macOS(.v12),
        .iOS(.v15),
        .tvOS(.v15),
        .watchOS(.v9),
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
            resources: [
                .process("Resources")
            ],
            cSettings: [
                .enableWarning("all"),
                //.enableWarning("extra"),
                //.enableWarning("implicit-fallthrough"),
                //.enableWarning("int-conversion"),
                //.enableWarning("pointer-to-int-cast")
            ],
            cxxSettings: [
                .enableWarning("all"),
                //.enableWarning("extra"),
                //.enableWarning("implicit-fallthrough"),
                //.enableWarning("int-conversion"),
                //.enableWarning("pointer-to-int-cast")
            ],
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
    // The astcenc library was compiled using c17, so set it also here
    cLanguageStandard: .c17,
    // Also use c++20, we don't live in the stone age, but still not ready to accept c++23
    cxxLanguageStandard: .cxx20
)
