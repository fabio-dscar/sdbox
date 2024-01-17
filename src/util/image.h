#ifndef __SDBOX_IMAGE_H__
#define __SDBOX_IMAGE_H__

#include <sdbox.h>
#include <variant>
// #include <half/half.hpp>

namespace sdbox {

using Half = short;
// using Half = half_float::half;

enum class PixelFormat : std::uint32_t { U8, F16, F32 };

struct ImageFormat {
    PixelFormat pFmt      = PixelFormat::F32;
    int         width     = 0;
    int         height    = 0;
    int         depth     = 0;
    int         nChannels = 0;
};

struct Extents {
    int toX   = 0;
    int toY   = 0;
    int toZ   = 0;
    int fromX = 0;
    int fromY = 0;
    int fromZ = 0;
    int sizeX, sizeY, sizeZ;
};

int ComponentSize(PixelFormat pFmt);

inline int ResizeLvl(int dim, int lvl) {
    return std::max(dim >> lvl, 1);
}

// Total pixels across levels
inline std::size_t TotalPixels(ImageFormat fmt, int levels = 1) {
    std::size_t totalPx = 0;
    for (int l = 0; l < levels; ++l)
        totalPx += ResizeLvl(fmt.width, l) * ResizeLvl(fmt.height, l) * ResizeLvl(fmt.depth, l);
    return totalPx;
}

inline std::size_t ImageSize(ImageFormat fmt, int levels = 1) {
    return TotalPixels(fmt, levels) * ComponentSize(fmt.pFmt) * fmt.nChannels;
}

// Mipmapped image
class Image {
public:
    using PixelVal = std::array<float, 4>;

    Image() = default;
    Image(ImageFormat format, int levels);
    Image(ImageFormat format, PixelVal fillVal, int levels = 1);
    Image(ImageFormat format, const std::byte* imgPtr, int levels = 1);
    Image(ImageFormat format, const float* imgPtr, int levels = 1);
    Image(ImageFormat format, Image&& srcImg);

    Image convertTo(ImageFormat newFmt, int nLvls = 1) const;

    void copy(const Image& srcImg) { *this = srcImg.convertTo(fmt); }
    void copy(const Image& srcImg, int toLvl, int fromLvl = 0);
    void copy(Extents ext, const Image& srcImg, int toLvl = 0, int fromLvl = 0);

    PixelVal pixel(int x, int y, int z, int lvl = 0) const;
    void     setPixel(const PixelVal& px, int x, int y, int z, int lvl = 0);

    float channel(int x, int y, int z, int c, int lvl = 0) const;
    void  setChannel(float val, int x, int y, int z, int c, int lvl = 0);

    void flipY();

    const std::byte* data(int lvl = 0) const;
    std::byte*       data(int lvl = 0);

    ImageFormat format(int level = 0) const;

    std::size_t size(int lvl) const { return ImageSize(format(lvl)); }
    std::size_t size() const { return ImageSize(fmt, levels); }

    int numLevels() const { return levels; }

private:
    void fill(PixelVal val);

    void        resizeBuffer();
    std::size_t pixelOffset(int x, int y, int z, int lvl = 0) const;

    const std::byte* getPtr() const;
    std::byte*       getPtr();

    std::vector<std::uint8_t> p8;
    std::vector<Half>         p16;
    std::vector<float>        p32;

    ImageFormat fmt;
    int         levels = 1;
};

class CubeImage {
public:
    CubeImage() = default;
    CubeImage(ImageFormat fmt, int levels) : levels(levels) {
        for (auto& img : faces)
            img = {fmt, levels};
    }

    const Image& operator[](int idx) const { return faces[idx]; }
    Image&       operator[](int idx) { return faces[idx]; }

    int         numLevels() const { return levels; }
    ImageFormat format(int lvl = 0) const { return faces[0].format(lvl); }

private:
    std::array<Image, 6> faces;
    int                  levels = 1;
};

// Non owning reference to an image (including all levels) or just one image level
class ImageView {
public:
    ImageView(const Image& image);
    ImageView(const Image& image, int lvl);

    ImageFormat format(int lvl = 0) const { return img->format(viewLevel + lvl); }

    const std::byte* data() const { return start; }
    std::size_t      size() const { return viewSize; }

    Image convertTo(ImageFormat newFmt, int lvl = 0) const;

    const Image* image() const { return img; }

    int level() const { return viewLevel; }
    int numLevels() const { return nLevels; }

private:
    const Image*     img;
    const std::byte* start;
    std::size_t      viewSize;
    int              nLevels   = 1;
    int              viewLevel = 0;
};

std::unique_ptr<std::byte[]> ExtractChannel(const Image& image, int c, int lvl = 0);
std::unique_ptr<std::byte[]> ExtractChannel(const ImageView imgView, int c, int lvl = 0);

} // namespace sdbox

#endif // __SDBOX_IMAGE_H__