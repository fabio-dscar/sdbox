#include <texture.h>

#include <glad/glad.h>

#include <map>

using namespace sdbox;

struct sdbox::FormatInfo {
    int         numChannels;
    PixelFormat pxFmt;
    GLuint      intFormat;
    GLuint      format;
    GLuint      type;
};

namespace {

using enum PixelFormat;
const std::map<std::tuple<PixelFormat, int>, FormatInfo> TexFormatMap{
    {{U8, 1},  {1, U8, GL_R8, GL_RED, GL_UNSIGNED_BYTE}    },
    {{U8, 2},  {2, U8, GL_RG8, GL_RG, GL_UNSIGNED_BYTE}    },
    {{U8, 3},  {3, U8, GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE}  },
    {{U8, 4},  {4, U8, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE}},
    {{F16, 1}, {1, F16, GL_R16F, GL_RED, GL_HALF_FLOAT}    },
    {{F16, 2}, {2, F16, GL_RG16F, GL_RG, GL_HALF_FLOAT}    },
    {{F16, 3}, {3, F16, GL_RGB16F, GL_RGB, GL_HALF_FLOAT}  },
    {{F16, 4}, {4, F16, GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT}},
    {{F32, 1}, {1, F32, GL_R32F, GL_RED, GL_FLOAT}         },
    {{F32, 2}, {2, F32, GL_RG32F, GL_RG, GL_FLOAT}         },
    {{F32, 3}, {3, F32, GL_RGB32F, GL_RGB, GL_FLOAT}       },
    {{F32, 4}, {4, F32, GL_RGBA32F, GL_RGBA, GL_FLOAT}     },
};

const std::unordered_map<Wrap, GLenum> OglTexWrap{
    {Wrap::Repeat,      GL_REPEAT         },
    {Wrap::Mirrored,    GL_MIRRORED_REPEAT},
    {Wrap::ClampEdge,   GL_CLAMP_TO_EDGE  },
    {Wrap::ClampBorder, GL_CLAMP_TO_BORDER}
};

const std::unordered_map<Filter, GLenum> OglTexFilter{
    {Filter::Nearest,           GL_NEAREST               },
    {Filter::Linear,            GL_LINEAR                },
    {Filter::NearestMipNearest, GL_NEAREST_MIPMAP_NEAREST},
    {Filter::LinearMipNearest,  GL_LINEAR_MIPMAP_NEAREST },
    {Filter::NearestMipLinear,  GL_NEAREST_MIPMAP_LINEAR },
    {Filter::LinearMipLinear,   GL_LINEAR_MIPMAP_LINEAR  }
};

const std::unordered_map<Texture::Type, GLenum> OglTargets{
    {Texture::Type::Tex1D, GL_TEXTURE_1D      },
    {Texture::Type::Tex2D, GL_TEXTURE_2D      },
    {Texture::Type::Tex3D, GL_TEXTURE_3D      },
    {Texture::Type::Cube,  GL_TEXTURE_CUBE_MAP}
};

} // namespace

Texture::Texture(Type target, ImageFormat fmt, int levels)
    : width(fmt.width), height(fmt.height), depth(fmt.depth), levels(levels), target(target) {
    init(fmt);
    setSampler({});
}

Texture::Texture(const Image& image, TexSampler sampler) {
    const auto fmt = image.format();

    target = Type::Tex1D;
    if (fmt.depth > 0)
        target = Type::Tex3D;
    else if (fmt.height > 0)
        target = Type::Tex2D;

    levels = image.numLevels();

    init(fmt);
    setSampler(sampler);

    for (int lvl = 0; lvl < image.numLevels(); ++lvl)
        upload(image, lvl);
}

Texture::Texture(const CubeImage& cube, TexSampler sampler) {
    const auto fmt = cube.format();

    target = Type::Cube;
    levels = cube.numLevels();

    init(fmt);
    setSampler(sampler);

    upload(cube);
}

Texture::~Texture() {
    if (handle != 0) {
        glDeleteTextures(1, &handle);
    }
}

void Texture::init(ImageFormat fmt) {
    width  = fmt.width;
    height = fmt.height;
    depth  = fmt.depth;

    auto pair = TexFormatMap.find({fmt.pFmt, fmt.nChannels});
    if (pair == TexFormatMap.end())
        FATAL("Unsupported internal format and num channels combination.");

    info = &pair->second;

    glCreateTextures(OglTargets.at(target), 1, &handle);

    if (target == Type::Tex1D)
        glTextureStorage1D(handle, levels, info->intFormat, width);
    else if (target == Type::Tex2D || target == Type::Cube)
        glTextureStorage2D(handle, levels, info->intFormat, width, height);
    else if (target == Type::Tex3D)
        glTextureStorage3D(handle, levels, info->intFormat, width, height, depth);
}

void Texture::setSampler(TexSampler sampler) const {
    switch (target) {
    case Type::Cube:
    case Type::Tex3D:
        glTextureParameteri(handle, GL_TEXTURE_WRAP_R, OglTexWrap.at(sampler.r));
        [[fallthrough]];
    case Type::Tex2D:
        glTextureParameteri(handle, GL_TEXTURE_WRAP_T, OglTexWrap.at(sampler.t));
        [[fallthrough]];
    case Type::Tex1D:
        glTextureParameteri(handle, GL_TEXTURE_WRAP_S, OglTexWrap.at(sampler.s));
        break;
    default:
        LOG_ERROR("Trying to set sampler to unknown target.");
    }

    glTextureParameteri(handle, GL_TEXTURE_MIN_FILTER, OglTexFilter.at(sampler.min));
    glTextureParameteri(handle, GL_TEXTURE_MAG_FILTER, OglTexFilter.at(sampler.mag));
}

std::unique_ptr<std::byte[]> Texture::data(int level) const {
    auto size    = sizeBytes(level);
    auto dataPtr = std::make_unique<std::byte[]>(size);
    glGetTextureImage(handle, level, info->format, info->type, size, dataPtr.get());
    return dataPtr;
}

std::unique_ptr<std::byte[]> Texture::data(int face, int level) const {
    auto size    = sizeBytesFace(level);
    auto dataPtr = std::make_unique<std::byte[]>(size);

    auto oglTarget = OglTargets.at(target);
    glBindTexture(oglTarget, handle);
    glGetTexImage(
        GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, level, info->format, info->type, dataPtr.get());
    glBindTexture(oglTarget, 0);

    return dataPtr;
}

void Texture::upload(const Image& image, int lvl) const {
    auto fmt = image.format(lvl);
    if (target == Type::Tex1D)
        glTextureSubImage1D(handle, lvl, 0, fmt.width, info->format, info->type, image.data());
    else if (target == Type::Tex2D)
        glTextureSubImage2D(
            handle, lvl, 0, 0, fmt.width, fmt.height, info->format, info->type, image.data());
    else if (target == Type::Tex3D)
        glTextureSubImage3D(
            handle, lvl, 0, 0, 0, fmt.width, fmt.height, fmt.depth, info->format, info->type,
            image.data());
}

void Texture::upload(const CubeImage& cubemap) const {
    for (int lvl = 0; lvl < cubemap.numLevels(); ++lvl) {
        for (int face = 0; face < 6; ++face) {
            glTextureSubImage3D(
                handle, lvl, 0, 0, face, ResizeLvl(width, lvl), ResizeLvl(height, lvl), 1,
                info->format, info->type, cubemap[face].data(lvl));
        }
    }
}

std::unique_ptr<Image> Texture::image(int level) const {
    return std::make_unique<Image>(format(level), data(level).get(), 1);
}

std::unique_ptr<CubeImage> Texture::cubemap() const {
    const auto fmt = format();

    auto cube = std::make_unique<CubeImage>(fmt, levels);

    for (int faceIdx = 0; faceIdx < 6; ++faceIdx) {
        Image faceImg{fmt, levels};
        for (int lvl = 0; lvl < levels; ++lvl)
            faceImg.copy(face(faceIdx, lvl), lvl);

        (*cube)[faceIdx] = std::move(faceImg);
    }

    return cube;
}

std::size_t Texture::sizeBytes(unsigned int level) const {
    int faces = target == Type::Cube ? 6 : 1;
    return sizeBytesFace(level) * faces;
}

std::size_t Texture::sizeBytesFace(unsigned int level) const {
    const auto w = ResizeLvl(width, level);
    const auto h = ResizeLvl(height, level);
    const auto d = ResizeLvl(depth, level);
    return ComponentSize(info->pxFmt) * info->numChannels * w * h * d;
}

ImageFormat Texture::format(int lvl) const {
    const auto w = width == 0 ? 0 : ResizeLvl(width, lvl);
    const auto h = height == 0 ? 0 : ResizeLvl(height, lvl);
    const auto d = depth == 0 ? 0 : ResizeLvl(depth, lvl);
    return {info->pxFmt, w, h, d, info->numChannels};
}

void Texture::generateMipmaps() const {
    glGenerateTextureMipmap(handle);
}