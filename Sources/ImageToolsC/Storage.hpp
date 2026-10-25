//
//  Storage.hpp
//  ImageTools
//
//  Created by Evgenij Lutz on 22.10.26.
//

#include <ImageToolsC/ImageContainer.hpp>


#define LANCZOS_CONCURRENT 1

#if 1
#  define LANCZOS_LOOP_START(_start, _end, _index_name) CONCURRENT_LOOP_START(_start, _end, _index_name)
#  define LANCZOS_LOOP_END CONCURRENT_LOOP_END
#else
#  define LANCZOS_LOOP(_start, _end, _index_name) for (auto _index_name = _start; _index_name < _end; _index_name++)
#  define LANCZOS_LOOP_END
#endif


ImagePixel _getPixel_general(long x, long y, long z, long width, long height, long depth, std::byte* fn_nonnull contents, long numComponents, PixelComponentType componentType);
void _setPixel_general(ImagePixel pixel, long x, long y, long z, long width, long height, long depth, std::byte* fn_nonnull contents, long numComponents, PixelComponentType componentType);
ImagePixel _sampleLanczosZ_general(float x, float y, float z, float a, long width, long height, long depth, std::byte* fn_nonnull contents, long numComponents, PixelComponentType componentType, bool renormalize);


struct TemporaryStorageGuard final {
    void* fn_nonnull * fn_nonnull const _source;
    const ImagePixelFormat _sourceFormat;
    const int _sourceWidth;
    const int _sourceHeight;
    const int _sourceDepth;
    long _sourceSize;
    /// Size for the intermediate vertical pass and target size.
    long _currentSourceSize;
    
    /// Buffer for horizontal and (if 3D texture) final pass.
    void* fn_nullable _intermediateBuffer;
    const ImagePixelFormat _intermediateFormat;
    
    // Target image dimensions.
    const int _targetWidth;
    const int _targetHeight;
    const int _targetDepth;
    
    long _horizontalPassSize;
    long _verticalPassSize;
    long _finalPassSize;
    
    TemporaryStorageGuard(void* fn_nonnull * fn_nonnull source, ImagePixelFormat sourceFormat, long sourceWidth, long sourceHeight, long sourceDepth,
                              ImagePixelFormat intermediateFormat, long targetWidth, long targetHeight, long targetDepth):
    _source(source),
    _sourceFormat(sourceFormat),
    _sourceWidth(static_cast<int>(sourceWidth)),
    _sourceHeight(static_cast<int>(sourceHeight)),
    _sourceDepth(static_cast<int>(sourceDepth)),
    _intermediateBuffer(nullptr),
    _intermediateFormat(intermediateFormat),
    _targetWidth(static_cast<int>(targetWidth)),
    _targetHeight(static_cast<int>(targetHeight)),
    _targetDepth(static_cast<int>(targetDepth)) {
        // Calculate size of every pass
        _horizontalPassSize = _sourceDepth * _sourceHeight * _targetWidth * _intermediateFormat.getSize();
        _verticalPassSize = _sourceDepth * _targetHeight * _targetWidth * (_sourceDepth == _targetDepth ? _sourceFormat.getSize() : _intermediateFormat.getSize());
        _finalPassSize = _targetDepth * _targetHeight * _targetWidth * _sourceFormat.getSize();
        
        _sourceSize = _sourceDepth * _sourceHeight * _sourceWidth * _sourceFormat.getSize();
        _currentSourceSize = _sourceSize;
        
        // Allocate intermediate storage enough to store the first (horizontal) and final (depth) if 3D passes
        auto tmpSize = _sourceDepth == _targetDepth ? _horizontalPassSize : std::max(_horizontalPassSize, _finalPassSize);
        _intermediateBuffer = std::malloc(tmpSize);
        if (_intermediateBuffer == nullptr) {
            fputs("Could not allocate memory\n", stderr);
            abort();
            exit(-1);
        }
        
        // Expand the source buffer if target is bigger - for intermediate vertical pass if 3D or final vertical pass if 2D
        auto suggestedIntermediateSourceSize = _sourceDepth == _targetDepth ? _finalPassSize : std::max(_finalPassSize, _verticalPassSize);
        if (suggestedIntermediateSourceSize > _sourceSize) {
            _currentSourceSize = suggestedIntermediateSourceSize;
            *_source = std::realloc(*_source, _finalPassSize);
            if (*_source == nullptr) {
                fputs("Could not allocate memory\n", stderr);
                abort();
                exit(-1);
            }
        }
    }
    
    ~TemporaryStorageGuard() {
        // Release intermediate storage
        std::free(_intermediateBuffer);
        
        // Shrink the source buffer if needed
        if (_finalPassSize < _currentSourceSize) {
            *_source = std::realloc(*_source, _finalPassSize);
            if (*_source == nullptr) {
                fputs("Could not reallocate memory\n", stderr);
                abort();
                exit(-1);
            }
        }
    }
    
    void copyFinalTargetContents() {
        std::memcpy(*_source, _intermediateBuffer, _finalPassSize);
    }
};
