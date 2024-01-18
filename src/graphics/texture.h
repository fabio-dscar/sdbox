#ifndef __SDBOX_TEXTURE_H__
#define __SDBOX_TEXTURE_H__

#include <sdbox.h>
#include <image.h>

namespace sdbox {

struct FormatInfo;

enum class Wrap { Repeat, Mirrored, ClampEdge, ClampBorder };

enum class Filter {
    Nearest,
    Linear,
    NearestMipNearest,
    LinearMipNearest,
    NearestMipLinear,
    LinearMipLinear
};

struct TexSampler {
    Wrap   s   = Wrap::Repeat;
    Wrap   t   = Wrap::Repeat;
    Wrap   r   = Wrap::Repeat;
    Filter min = Filter::Linear;
    Filter mag = Filter::Linear;
};

class Texture {
public:
    enum class Type : unsigned int { Tex1D, Tex2D, Tex3D, Cube };

    Texture(Type target, ImageFormat fmt, int levels);
    explicit Texture(const Image& image, TexSampler sampler = {});
    explicit Texture(const CubeImage& cube, TexSampler sampler = {});

    ~Texture();

    unsigned int id() const { return handle; }

    void setSampler(TexSampler sampler) const;
    void generateMipmaps() const;

    void upload(const Image& image, int level = 0) const;
    void upload(const CubeImage& cubemap) const;

    ImageFormat format(int level = 0) const;

    std::unique_ptr<Image>     image(int level = 0) const;
    std::unique_ptr<CubeImage> cubemap() const;

    unsigned int handle = 0;
    int          width  = 0;
    int          height = 0;
    int          depth  = 0;
    int          levels = 1;

private:
    void init(ImageFormat format);

    std::size_t sizeBytes(unsigned int level = 0) const;
    std::size_t sizeBytesFace(unsigned int level = 0) const;

    std::unique_ptr<std::byte[]> data(int level) const;
    std::unique_ptr<std::byte[]> data(int face, int level) const;

    Image face(int face, int level = 0) const { return {format(level), data(face, level).get()}; }

    const FormatInfo* info   = nullptr;
    Type              target = Type::Tex2D;
};

inline int MaxMipLevel(int width, int height = 0, int depth = 0) {
    int dim = std::max(width, std::max(height, depth));
    return 1 + static_cast<int>(std::floor(std::log2(dim)));
}

} // namespace sdbox

#endif // __SDBOX_TEXTURE_H__