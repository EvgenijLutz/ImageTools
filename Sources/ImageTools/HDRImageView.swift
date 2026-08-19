//
//  HDRView.swift
//  ImageTools
//
//  Created by Evgenij Lutz on 19.08.26.
//

#if canImport(SwiftUI) && (os(macOS) || os(iOS))

import SwiftUI


@available(macOS 13.0, iOS 16.0, *)
fileprivate func _sizeThatFits(_ proposal: ProposedViewSize, _ cgImage: CGImage, _ width: CGFloat?, _ height: CGFloat?) -> CGSize? {
    let imageWidth = CGFloat(cgImage.width)
    let imageHeight = CGFloat(cgImage.height)
    let imageAspect = imageWidth / imageHeight
    
    
    let width = proposal.width ?? .infinity
    let height = proposal.height ?? .infinity
    
    let size: CGSize? = {
        if width.isInfinite {
            if height.isInfinite {
                return nil
            }
            
            return .init(width: height * imageAspect, height: height)
        }
        else if height.isInfinite {
            return .init(width: width, height: width / imageAspect)
        }
        
        let aspect = width / height
        if imageAspect > aspect {
            return .init(width: width, height: width / imageAspect)
        }
        else {
            return .init(width: height * imageAspect, height: height)
        }
    }()
    
    //let source = "\(proposal.width, default: "nil") x \(proposal.height, default: "nil")"
    //print("\(source) ~ \(imageAspect) -> \(size, default: "nil")")
    
    return size
}


#if os(macOS)

public struct HDRImageView: NSViewRepresentable {
    private let cgImage: CGImage
    private let nsImage: NSImage
    
    public init(_ cgImage: CGImage) {
        self.cgImage = cgImage
        self.nsImage = .init(cgImage: cgImage, size: .init(width: cgImage.width, height: cgImage.height))
    }
    
    public func makeNSView(context: Context) -> NSImageView {
        let nsView = NSImageView()
        return nsView
    }
    
    public func updateNSView(_ nsView: NSImageView, context: Context) {
        nsView.image = nsImage
        
        if #available(macOS 14.0, *) {
            nsView.preferredImageDynamicRange = .high
        }
        
        if #available(macOS 26.0, *) {
            nsView.layer?.contentsHeadroom = 8//.init(cgImage.contentHeadroom)
            nsView.layer?.preferredDynamicRange = .high
        } else {
            if #available(macOS 14.0, *) {
                nsView.layer?.wantsExtendedDynamicRangeContent = true
            }
        }
        nsView.imageScaling = .scaleProportionallyUpOrDown
    }
    
    @available(macOS 13.0, *)
    public func sizeThatFits(_ proposal: ProposedViewSize, nsView: NSImageView, context: Context) -> CGSize? {
        _sizeThatFits(proposal, cgImage, proposal.width ?? .infinity, proposal.height ?? .infinity)
    }
}

#elseif os(iOS)

public struct HDRImageView: UIViewRepresentable {
    private let cgImage: CGImage
    private let uiImage: UIImage
    
    public init(_ cgImage: CGImage) {
        self.cgImage = cgImage
        self.uiImage = .init(cgImage: cgImage)
    }
    
    
    public func makeUIView(context: Context) -> UIImageView {
        let uiView = UIImageView()
        return uiView
    }
    
    public func updateUIView(_ uiView: UIImageView, context: Context) {
        uiView.image = uiImage
        
        if #available(iOS 17.0, *) {
            uiView.preferredImageDynamicRange = .high
        }
        
        if #available(iOS 26.0, *) {
            uiView.layer.contentsHeadroom = 8//.init(cgImage.contentHeadroom)
            uiView.layer.preferredDynamicRange = .high
        } else {
            if #available(iOS 17.0, *) {
                uiView.layer.wantsExtendedDynamicRangeContent = true
            }
        }
        uiView.contentMode = .scaleAspectFit
    }
    
    @available(iOS 16.0, *)
    public func sizeThatFits(_ proposal: ProposedViewSize, uiView: UIImageView, context: Context) -> CGSize? {
        _sizeThatFits(proposal, cgImage, proposal.width ?? .infinity, proposal.height ?? .infinity)
    }
}

#endif


#endif
