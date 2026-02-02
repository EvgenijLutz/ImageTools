//
//  PlaygroundView.swift
//  ImageTools
//
//  Created by Evgenij Lutz on 28.01.26.
//

#if canImport(SwiftUI)

import SwiftUI
import LibPNG
import ASTCEncoder
import LittleCMS


public struct TestImage: Identifiable, Sendable, Hashable {
    public let id = UUID()
    public let name: String
    public let path: String
    
    public init(name: String, path: String) {
        self.name = name
        self.path = path
    }
}


struct Block2DOption: Identifiable, Sendable, Hashable {
    let id: Int
    let blockSize: ASTCBlockSize
    let name: String
    
    init(_ id: Int, _ blockSize: ASTCBlockSize) {
        self.id = id
        self.blockSize = blockSize
        
        let depthString = blockSize.depth == 1 ? "" : "x\(blockSize.depth)"
        self.name = "\(blockSize.width)x\(blockSize.height)\(depthString)"
    }
}


let block2DOptions: [Block2DOption] = [
    .init(0, ._4x4),
    .init(1, ._5x4),
    .init(2, ._5x5),
    .init(3, ._6x5),
    .init(4, ._6x6),
    .init(5, ._8x5),
    .init(6, ._8x6),
    .init(7, ._8x8),
    .init(8, ._10x5),
    .init(9, ._10x6),
    .init(10, ._10x8),
    .init(11, ._10x10),
    .init(12, ._12x10),
    .init(13, ._12x12),
]


struct CompressionQuality: Identifiable, Sendable, Hashable {
    let id: Int
    let quality: ASTCCompressionQuality
    let name: String
    
    init(_ id: Int, _ quality: Float, _ name: String) {
        self.id = id
        self.quality = quality
        //self.name = String(format: "%.1f", quality * 100)
        self.name = name
    }
}

let compressionQualityPresets: [CompressionQuality] = [
    .init(0, ASTCCompressionQuality.fastest, "Fastest"),
    .init(1, ASTCCompressionQuality.fast, "Fast"),
    .init(2, ASTCCompressionQuality.medium, "Medium"),
    .init(3, ASTCCompressionQuality.thorough, "Thorough"),
    .init(4, ASTCCompressionQuality.veryThorough, "Very thorough"),
    .init(5, ASTCCompressionQuality.exhaustive, "Exhaustive"),
]


@available(macOS 13.3, iOS 16.4, tvOS 16.4, watchOS 9.4, visionOS 1.0, *)
func testImageLoading(path: String) async throws -> CGImage {
    let image = try await ImageContainer.load(path: path)
    
    return try image.cgImage
}


//func testImageResampling(path: String) async throws -> CGImage {
//    let image = try await ImageContainer.load(path: path)
//
//    return try image
//        .createResampled(.lanczos, quality: 3, width: image.width / 2, height: image.height / 2, depth: image.depth / 2)
//        .cgImage
//}


@available(macOS 13.3, iOS 16.4, tvOS 16.4, watchOS 9.4, visionOS 1.0, *)
func testColorSpaceConversion(path: String) async throws -> CGImage {
    let image = try await ImageContainer.load(path: path).createPromoted(.float16)
    
    let displayP3: LCMSColorProfile = LCMSColorProfile.createDCIP3D65()
    guard let linear = displayP3.createLinear(force: true) else {
    //guard let linear = image.colorProfile?.createLinear() else {
        throw ASTCError("Could not create a linear version of the display P3 color profile")
    }
    
    let editor = ImageEditor(image)
    let converted = editor.convertColorProfile(linear)
    guard converted else {
        throw ASTCError("Could not convert to a linear version of the display P3 color profile")
    }
    
    let linearImage = editor.imageCopy
    
    return try linearImage.cgImage
}


@available(macOS 13.3, iOS 16.4, tvOS 16.4, watchOS 9.4, visionOS 1.0, *)
func testCompression(path: String,
                      blockSize: ASTCBlockSize,
                      quality: Float,
                      progressCallback: @Sendable (_ progress: Float) -> Void = { _ in }
) async throws -> [CGImage] {
    let image = try await ImageContainer.load(path: path)
    //let resampled = image.createResampled(.lanczos, quality: 3, width: image.width / 2, height: image.height / 2, depth: image.depth / 2)
    //let astc = try resampled.createASTCCompressed(blockSize: blockSize, quality: quality, ldrAlpha: true, progressCallback)
    let astc = try image.createASTCCompressed(blockSize: blockSize, quality: quality, containsAlpha: true, ldrAlpha: true, normalMap: false, progressCallback)
    let decompressedImage = try astc.decompress()
    return try [decompressedImage.createCgImage(colorSpace: image.colorProfile?.colorSpace)]
}


@available(macOS 13.3, iOS 16.4, tvOS 16.4, watchOS 9.4, visionOS 1.0, *)
func testMips(path: String,
              blockSize: ASTCBlockSize,
              quality: Float,
              progressCallback: @Sendable (_ progress: Float) -> Void = { _ in }
) async throws -> [CGImage] {
    var images: [CGImage] = []
    
    var image = try await ImageContainer.load(path: path).createPromoted(.float16)
    
    // Convert to linear colour space
    if let linearColorProfile = (image.colorProfile ?? .createSRGB())?.createLinear() {
        let editor = ImageEditor(image)
        if editor.convertColorProfile(linearColorProfile) {
            image = editor.imageCopy
        }
    }
    
    let maxSteps = image.calculateMipLevelCount()
    var currentStep = 0
    func notifyProgress(currentCompressionProgress: Float) {
        //print("Step: \(currentStep) / \(maxSteps) ~ \(currentCompressionProgress)")
        let stepLength = 1.0 / Float(maxSteps)
        let substep = stepLength * currentCompressionProgress
        let progress = min(stepLength * Float(currentStep) + substep, 1.0)
        progressCallback(progress)
    }
    
    while true {
        let cgImage = try image.cgImage
        images.append(cgImage)
        
        notifyProgress(currentCompressionProgress: 1)
        
        if image.width == 1 && image.height == 1 && image.depth == 1 {
            break
        }
        
        image = try image.createDownsampled(.lanczos, quality: 3, renormalize: false)
        
        currentStep += 1
    }
    
    progressCallback(1)
    return images
}


@available(macOS 13.3, iOS 16.4, tvOS 16.4, watchOS 9.4, visionOS 1.0, *)
func testCompressedMips(path: String,
                        blockSize: ASTCBlockSize,
                        quality: Float,
                        progressCallback: @Sendable (_ progress: Float) -> Void = { _ in }
) async throws -> [CGImage] {
    var images: [CGImage] = []
    
    // Load image
    let lowercasedPath = path.lowercased()
    let assumeSRGB = lowercasedPath.hasSuffix(".hdr") == false && lowercasedPath.hasSuffix(".tga") == false
    let assumeLinear = assumeSRGB == false
    let image = try ImageEditor.load(path: path, assumeSRGB: assumeSRGB, assumeLinear: assumeLinear)
    //return try [image.cgImage]
    
    if let profile = image.colorProfile {
        let srgb = profile.checkIsSRGB()
        print("sRGB: \(srgb)")
    }
    
    // Assume the image to be a normal map if the name contains "normal"
    let normalMap = image.hdr == false && lowercasedPath.contains("normal")
    
    // Prepare progress callback
    let maxSteps = image.calculateMipLevelCount()
    var currentStep = 0
    func notifyProgress(currentCompressionProgress: Float) {
        let stepLength = 1.0 / Float(maxSteps)
        let substep = stepLength * currentCompressionProgress
        let progress = min(stepLength * Float(currentStep) + substep, 1.0)
        //print("Progress: \(progress)")
        progressCallback(progress)
    }
    
    // Promote pixel format if needed for better accuracy
    let originalComponentType = image.pixelFormat.componentType
    let shouldConvertComponentType = originalComponentType == .uint8
    if shouldConvertComponentType {
        // TODO: Progress callback: progress * 0.15
        image.setComponentType(.float16)
    }
    
    // Convert to linear colour space
    if image.srgb || image.colorProfile != nil {
        if let linearColorProfile = (image.colorProfile ?? .createSRGB())?.createLinear() {
            image.convertColorProfile(linearColorProfile)
        }
    }
    
    // Set to grey
    //do {
    //    let numChannels = image.pixelFormat.numComponents
    //    let editor = ImageEditor()
    //    editor.edit(image)
    //    for i in 0..<numChannels {
    //        editor.setChannel(i, editor, 2)
    //    }
    //}
    
    // Process each mip level
    while true {
        // Convert back to originalComponentType
        var source = image.getImageCopy()
        if shouldConvertComponentType {
            // TODO: Progress callback: 0.3 * decompressionProgress * 0.2
            source = source.createPromoted(originalComponentType)
        }
        
        let containsAlpha = source.pixelFormat.numComponents == 4
        let hdr = source.hdr
        let ldrAlpha = hdr && containsAlpha
        
        
        // Compress image
        let astc = try source.createASTCCompressed(blockSize: blockSize,
                                                   quality: quality,
                                                   containsAlpha: containsAlpha,
                                                   ldrAlpha: ldrAlpha,
                                                   normalMap: normalMap
        ) { compressionProgress in
            notifyProgress(currentCompressionProgress: 0.5 + compressionProgress * 0.5)
        }
        
        // Save astc image - convert it to a CGImage for now
        let decompressedImage = try astc.decompress()
        let cgImage = try decompressedImage.createCgImage(colorSpace: source.colorProfile?.colorSpace, assumeSRGB: assumeSRGB, hdr: image.hdr)
        images.append(cgImage)
        
        
        // Next step
        if image.width == 1 && image.height == 1 && image.depth == 1 {
            break
        }
        currentStep += 1
        
        // Downsample image
        try image.downsample(.lanczos, quality: 3, renormalize: normalMap) { progress in
            notifyProgress(currentCompressionProgress: progress * 0.3)
        }
    }
    
    // Compressed mips generation finished
    progressCallback(1)
    return images
}


@available(macOS 13.3, iOS 16.4, tvOS 16.4, watchOS 9.4, visionOS 1.0, *)
func testImageContainer(
    path: String?,
    blockSize: ASTCBlockSize,
    quality: Float,
    progressCallback: @Sendable (_ progress: Float) -> Void = { _ in }
) async throws -> [CGImage] {
    // Search for the image
    guard let path else {
        throw ASTCError("No path specified")
    }
    
    //return try await testImageLoading(path: path)
    //return try await testImageResampling(path: path)
    //return try await testColorSpaceConversion(path: path)
    //return try await testCompression(path: path, blockSize: blockSize, quality: quality, progressCallback: progressCallback)
    //return try await testMips(path: path, blockSize: blockSize, quality: quality, progressCallback: progressCallback)
    return try await testCompressedMips(path: path, blockSize: blockSize, quality: quality, progressCallback: progressCallback)
}


func loadOriginalImage(path: String?) -> CGImage? {
    guard let path else {
        return nil
    }
    
#if os(macOS)
    return NSImage(contentsOfFile: path)?.cgImage(forProposedRect: nil, context: nil, hints: nil)
#elseif os(iOS) || os(tvOS) || os(visionOS)
    return UIImage(contentsOfFile: path)?.cgImage
#endif
}


@available(macOS 13.3, iOS 16.4, tvOS 16.4, watchOS 9.4, visionOS 1.0, *)
extension Image {
    init(cgImage: CGImage) {
        #if os(macOS)
        self.init(nsImage: .init(cgImage: cgImage, size: .zero))
        #elseif os(iOS) || os(tvOS) || os(visionOS)
        self.init(uiImage: .init(cgImage: cgImage))
        #endif
    }
}


@available(macOS 13.3, iOS 16.4, tvOS 16.4, watchOS 9.4, visionOS 1.0, *)
@MainActor
var lastTask: Task<Void, Never>? = nil


struct ConvertedImage: Identifiable {
    let id = UUID()
    let image: CGImage
}


@available(macOS 14.0, iOS 17.0, tvOS 17.0, watchOS 9.4, visionOS 1.0, *)
struct CompressedImageViewer: View {
    var convertedImages: [ConvertedImage] = []
    var currentMipIndex: Int
    
    var body: some View {
        if currentMipIndex < convertedImages.count {
            Image(cgImage: convertedImages[currentMipIndex].image)
                .resizable()
                .scaledToFit()
                .allowedDynamicRange(.high)
        }
        
    }
}


public typealias CGImageGeneratorFunc = (_ imagePath: String) async throws -> [CGImage]

@available(macOS 14.0, iOS 17.0, tvOS 17.0, watchOS 9.4, visionOS 1.0, *)
public struct ImageToolsPlaygroundView: View {
    let imageList: [TestImage]
    let imagesGeneratorFunc: CGImageGeneratorFunc?
    
    @State var sourceImage: CGImage?
    @State var loadingProgress: Float = 0
    @State var selectedTestImage: TestImage?
    @State var selectedBlockOption: Block2DOption = block2DOptions[0]
    @State var selectedCompressionQuality: CompressionQuality = compressionQualityPresets[0]
    
    @State var convertedImages: [ConvertedImage] = []
    
    
    enum ImageComparisonMode: Int {
        case original
        case compressed
        case sideBySide
    }
    @State var comparisonMode: ImageComparisonMode = .sideBySide
    @State var currentMipProgress: Float = 0
    
    private var progressStep: Float {
        if convertedImages.count < 2 {
            return 1
        }
        
        return 1 / Float(convertedImages.count - 1)
    }
    
    private var currentStep: Int {
        if convertedImages.isEmpty {
            return 0
        }
        
        return Int(currentMipProgress * Float(convertedImages.count - 1))
    }
    
    
    public init(_ imageList: [TestImage], imagesGeneratorFunc: CGImageGeneratorFunc? = nil) {
        self.imageList = imageList
        self.selectedTestImage = imageList.last
        self.imagesGeneratorFunc = imagesGeneratorFunc
    }
    
    func loadTestImage() {
        lastTask?.cancel()
        
        //image = nil
        loadingProgress = 0
        
        lastTask = Task {
            do {
                let img = loadOriginalImage(path: selectedTestImage?.path)
                try Task.checkCancellation()
                sourceImage = img
                convertedImages = []
            } catch {
                print(error)
            }
        }
    }
    
    func testImageCompression(_ customFunc: CGImageGeneratorFunc? = nil) {
        lastTask?.cancel()
        
        //image = nil
        loadingProgress = 0
        
        lastTask = Task {
            do {
                // Get image path
                guard let path = selectedTestImage?.path else {
                    return
                }
                
                // Get images using custom implementation
                if let customFunc = customFunc {
                    let images = try await customFunc(path)
                    convertedImages = images.map { .init(image: $0) }
                    loadingProgress = 1
                    return
                }
                
                // Generate compressed mips
                let images = try await testImageContainer(
                    path: path,
                    blockSize: selectedBlockOption.blockSize,
                    quality: selectedCompressionQuality.quality
                ) { progress in
                    Task { @MainActor in
                        loadingProgress = progress
                    }
                }
                
                try Task.checkCancellation()
                
                convertedImages = images.map { .init(image: $0) }
            }
            catch {
                print(error)
            }
        }
    }
    
    public var body: some View {
        NavigationStack {
            VStack(spacing: 0) {
                HStack {
                    Picker("Block size", selection: $selectedBlockOption) {
                        ForEach(block2DOptions) { option in
                            Text(option.name).tag(option)
                        }
                    }
                    .labelsHidden()
                    
                    
                    Picker("Compreesion quality", selection: $selectedCompressionQuality) {
                        ForEach(compressionQualityPresets) { option in
                            Text(option.name).tag(option)
                        }
                    }
                    .labelsHidden()
                    
                    //Toggle("Generate mips", isOn: .constant(true))
                    
                    
                    Button {
                        testImageCompression()
                    } label: {
                        Text("Compress")
                    }
                    .buttonStyle(.bordered)
                }
                
                if let imagesGeneratorFunc {
                    Button {
                        testImageCompression(imagesGeneratorFunc)
                    } label: {
                        Text("Perform test")
                    }
                    .buttonStyle(.bordered)
                }
                
                ProgressView(value: loadingProgress, total: 1)
                    .progressViewStyle(.linear)
                
                
                VStack {
                    if convertedImages.isEmpty == false {
                        Picker(selection: $comparisonMode) {
                            Text("Original")
                                .tag(ImageComparisonMode.original)
                            
                            Text("Compressed")
                                .tag(ImageComparisonMode.compressed)
                            
                            Text("Side by side")
                                .tag(ImageComparisonMode.sideBySide)
                        } label: {
                            Text("Comparison mode")
                        }
                        .pickerStyle(.segmented)
                        .labelsHidden()
                        
                        
                        if convertedImages.count > 1 {
                            Slider(value: $currentMipProgress, in: 0...1, step: progressStep)
                                .frame(maxWidth: 256)
                        }
                    }
                }
                .padding(.bottom, 8)

                
                switch comparisonMode {
                case .original:
                    if let sourceImage {
                        Image(cgImage: sourceImage)
                            .resizable()
                            .scaledToFit()
                            .allowedDynamicRange(.high)
                    }
                    
                case .compressed:
                    CompressedImageViewer(convertedImages: convertedImages, currentMipIndex: currentStep)
                    
                case .sideBySide:
                    HStack(spacing: 0) {
                        if let sourceImage {
                            Image(cgImage: sourceImage)
                                .resizable()
                                .scaledToFit()
                                .allowedDynamicRange(.high)
                        }
                        
                        CompressedImageViewer(convertedImages: convertedImages, currentMipIndex: currentStep)
                        
                        Spacer(minLength: 0)
                    }
                }
                
                Spacer()
            }
            .toolbar {
                Picker("Test image", selection: $selectedTestImage) {
                    ForEach(imageList) { option in
                        Text(option.name).tag(option)
                    }
                }
                .onChange(of: selectedTestImage) {
                    loadTestImage()
                }
                
                Button {
                    loadTestImage()
                } label: {
                    Text("Reset")
                }
                .buttonStyle(.bordered)
            }
        }
        .task {
            //let profile = LCMSColorProfile.createSRGB()
            //let profile = LCMSColorProfile.createDCIP3()
            //let profile = LCMSColorProfile.createDCIP3D65()
            //_ = profile.checkIsSRGB()
            
            loadTestImage()
        }
    }
}

#endif
