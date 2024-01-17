#include <image.h>

#include <cstring>

using namespace sdbox;

namespace {

float EncodeU8(std::uint8_t u8) {
    return u8 / 255.0f;
}

std::uint8_t DecodeU8(float f32) {
    f32 = std::min(1.0f, std::max(0.0f, f32));
    return static_cast<uint8_t>(f32 * 255.0f + 0.5f);
}

} // namespace

int sdbox::ComponentSize(PixelFormat pFmt) {
    switch (pFmt) {
    case PixelFormat::U8:
        return 1;
    case PixelFormat::F16:
        return 2;
    case PixelFormat::F32:
        return 4;
    default:
        FATAL("Unknown pixel format.");
    }
}

Image::Image(ImageFormat format, int levels) : fmt(format), levels(levels) {
    resizeBuffer();
}

Image::Image(ImageFormat format, PixelVal fillVal, int levels) : fmt(format), levels(levels) {
    resizeBuffer();
    fill(fillVal);
}

Image::Image(ImageFormat format, const std::byte* imgPtr, int levels)
    : fmt(format), levels(levels) {
    auto numElems = TotalPixels(format, levels) * fmt.nChannels;
    auto compSize = ComponentSize(fmt.pFmt);

    resizeBuffer();

    std::copy(imgPtr, imgPtr + numElems * compSize, getPtr());
}

Image::Image(ImageFormat format, const float* imgPtr, int levels) : fmt(format), levels(levels) {
    auto numElems = TotalPixels(format, levels) * fmt.nChannels;

    p32.reserve(numElems);
    p32.assign(imgPtr, imgPtr + numElems);
}

Image::Image(ImageFormat format, Image&& srcImg) : fmt(format), levels(srcImg.levels) {
    auto srcFmt = srcImg.format();

    // Move in if equivalent formats, convert from srcImg otherwise
    if (fmt.pFmt == srcFmt.pFmt && fmt.nChannels == srcFmt.nChannels)
        *this = std::move(srcImg);
    else {
        resizeBuffer();
        for (int lvl = 0; lvl < levels; ++lvl)
            copy(srcImg, lvl);
    }
}

float Image::channel(int x, int y, int z, int c, int lvl) const {
    if (c >= fmt.nChannels)
        return 0;

    auto offset = pixelOffset(x, y, z, lvl) * fmt.nChannels;

    switch (fmt.pFmt) {
    case PixelFormat::U8:
        return EncodeU8(p8[offset + c]);
    case PixelFormat::F16:
        return p16[offset + c];
    case PixelFormat::F32:
        return p32[offset + c];
    default:
        FATAL("Unknown pixel format.");
    }
}

void Image::setChannel(float val, int x, int y, int z, int c, int lvl) {
    assert(c < fmt.nChannels);

    auto offset = pixelOffset(x, y, z, lvl) * fmt.nChannels;

    switch (fmt.pFmt) {
    case PixelFormat::U8:
        p8[offset + c] = DecodeU8(val);
        break;
    case PixelFormat::F16:
        p16[offset + c] = val;
        break;
    case PixelFormat::F32:
        p32[offset + c] = val;
        break;
    default:
        FATAL("Unknown pixel format.");
    }
}

Image::PixelVal Image::pixel(int x, int y, int z, int lvl) const {
    auto chanVals = PixelVal{0.0f, 0.0f, 0.0f, 0.0f};
    for (int c = 0; c < fmt.nChannels; ++c)
        chanVals[c] = channel(x, y, z, c, lvl);
    return chanVals;
}

void Image::setPixel(const PixelVal& px, int x, int y, int z, int lvl) {
    for (int c = 0; c < fmt.nChannels; ++c)
        setChannel(px[c], x, y, z, c, lvl);
}

void Image::fill(PixelVal val) {
    for (int lvl = 0; lvl < levels; ++lvl) {
        const auto lvlFmt = format(lvl);
        for (int z = 0; z < lvlFmt.depth; ++z)
            for (int y = 0; y < lvlFmt.height; ++y)
                for (int x = 0; x < lvlFmt.width; ++x)
                    setPixel(val, x, y, z, lvl);
    }
}

void Image::copy(const Image& srcImg, int toLvl, int fromLvl) {
    auto dims = format(toLvl);
    copy({.sizeX = dims.width, .sizeY = dims.height, .sizeZ = dims.depth}, srcImg, toLvl, fromLvl);
}

void Image::copy(Extents ext, const Image& srcImg, int toLvl, int fromLvl) {
    const auto toX = ext.toX, toY = ext.toY, toZ = ext.toZ;
    const auto fromX = ext.fromX, fromY = ext.fromY, fromZ = ext.fromZ;

    for (int z = 0; z < ext.sizeZ; ++z) {
        for (int y = 0; y < ext.sizeY; ++y) {
            for (int x = 0; x < ext.sizeX; ++x) {
                const auto& px = srcImg.pixel(fromX + x, fromY + y, fromZ + z, fromLvl);
                setPixel(px, toX + x, toY + y, toZ + z, toLvl);
            }
        }
    }
}

Image Image::convertTo(ImageFormat newFmt, int nLvls) const {
    if (fmt.pFmt == newFmt.pFmt && fmt.nChannels == newFmt.nChannels && nLvls == levels)
        return *this;

    Image newImg{newFmt, nLvls};
    for (int lvl = 0; lvl < nLvls; ++lvl)
        newImg.copy(*this, lvl);

    return newImg;
}

void Image::resizeBuffer() {
    auto numElems = TotalPixels(fmt, levels) * fmt.nChannels;
    switch (fmt.pFmt) {
    case PixelFormat::U8:
        p8.resize(numElems);
        break;
    case PixelFormat::F16:
        p16.resize(numElems);
        break;
    case PixelFormat::F32:
        p32.resize(numElems);
        break;
    default:
        FATAL("Unknown pixel format.");
    }
}

std::size_t Image::pixelOffset(int x, int y, int z, int lvl) const {
    const auto nPixels      = TotalPixels(fmt, lvl);
    const auto widthOffset  = ResizeLvl(fmt.width, lvl);
    const auto heightOffset = ResizeLvl(fmt.height, lvl);
    return nPixels + (z * widthOffset * heightOffset + y * widthOffset + x);
}

const std::byte* Image::data(int lvl) const {
    std::size_t prevLvlSize = ImageSize(fmt, lvl);
    return &getPtr()[prevLvlSize];
}

std::byte* Image::data(int lvl) {
    std::size_t prevLvlSize = ImageSize(fmt, lvl);
    return &getPtr()[prevLvlSize];
}

ImageFormat Image::format(int lvl) const {
    const auto w = fmt.width == 0 ? 0 : ResizeLvl(fmt.width, lvl);
    const auto h = fmt.height == 0 ? 0 : ResizeLvl(fmt.height, lvl);
    const auto d = fmt.depth == 0 ? 0 : ResizeLvl(fmt.depth, lvl);
    return {fmt.pFmt, w, h, d, fmt.nChannels};
}

const std::byte* Image::getPtr() const {
    using enum PixelFormat;
    switch (fmt.pFmt) {
    case U8:
        return reinterpret_cast<const std::byte*>(p8.data());
    case F16:
        return reinterpret_cast<const std::byte*>(p16.data());
    case F32:
        return reinterpret_cast<const std::byte*>(p32.data());
    default:
        FATAL("Unknown pixel format.");
    }
}

std::byte* Image::getPtr() {
    using enum PixelFormat;
    switch (fmt.pFmt) {
    case U8:
        return reinterpret_cast<std::byte*>(p8.data());
    case F16:
        return reinterpret_cast<std::byte*>(p16.data());
    case F32:
        return reinterpret_cast<std::byte*>(p32.data());
    default:
        FATAL("Unknown pixel format.");
    }
}

void Image::flipY() {
    Image flipImg{format(), levels};

    for (int lvl = 0; lvl < levels; ++lvl) {
        const auto lvlFmt = format(lvl);

        for (int z = 0; z < lvlFmt.depth; ++z) {
            for (int y = 0; y < lvlFmt.height; ++y) {
                for (int x = 0; x < lvlFmt.width; ++x) {
                    auto px = pixel(x, lvlFmt.height - 1 - y, z, lvl);
                    flipImg.setPixel(px, x, y, z, lvl);
                }
            }
        }
    }

    *this = std::move(flipImg);
}

ImageView::ImageView(const Image& image)
    : img(&image), start(image.data()), viewSize(image.size()), nLevels(image.numLevels()) {}

ImageView::ImageView(const Image& image, int lvl)
    : img(&image), start(image.data(lvl)), viewSize(image.size(lvl)), nLevels(1), viewLevel(lvl) {}

Image ImageView::convertTo(ImageFormat newFmt, int lvl) const {
    Image copyImg{newFmt, 1};
    copyImg.copy(*img, lvl, viewLevel + lvl);
    return copyImg;
}

std::unique_ptr<std::byte[]> sdbox::ExtractChannel(const Image& image, int c, int lvl) {
    const auto imgFmt = image.format(lvl);
    if (c >= imgFmt.nChannels)
        FATAL("Specified channel number is higher than available channels.");

    const auto chSize   = image.size(lvl) / imgFmt.nChannels;
    const auto compSize = ComponentSize(imgFmt.pFmt);
    const auto nPixels  = imgFmt.width * imgFmt.height;

    auto channel = std::make_unique<std::byte[]>(chSize);

    auto imgPtr = image.data(lvl);
    auto dstPtr = channel.get();

    const auto strideCh = compSize * imgFmt.nChannels;

    for (int p = 0; p < nPixels; ++p, imgPtr += strideCh, dstPtr += compSize)
        std::memcpy(dstPtr, imgPtr + compSize * c, compSize);

    return channel;
}

std::unique_ptr<std::byte[]> sdbox::ExtractChannel(const ImageView imgView, int c, int lvl) {
    assert(imgView.numLevels() > lvl);
    return ExtractChannel(*imgView.image(), c, imgView.level() + lvl);
}