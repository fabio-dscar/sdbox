#ifndef __SDBOX_RESOURCE_H__
#define __SDBOX_RESOURCE_H__

#include <sdbox.h>
#include <util.h>
#include <unordered_set>
#include <unordered_map>
#include <type_traits>

namespace sdbox {

using namespace util;

class Shader;
class Program;
class Texture;

static const std::unordered_set BuiltinNames = {
    Hash("main.glsl"), Hash("texture0"), Hash("texture1"), Hash("texture2"), Hash("texture3"),
    Hash("cube0"),     Hash("cube1"),    Hash("cube2"),    Hash("cube3")};

static const std::unordered_set TextureFormats = {
    Hash("exr"), Hash("hdr"), Hash("png"), Hash("jpeg"), Hash("jpg")};

inline bool IsBuiltinName(const std::string& str) {
    return BuiltinNames.find(HashBytes64(str)) != BuiltinNames.end();
}

inline bool IsTexture(const std::string& ext) {
    return TextureFormats.find(HashBytes64(ext)) != TextureFormats.end();
}

template<typename KT, typename VT>
struct Map {
    std::unordered_map<KT, VT> map;
    std::mutex                 mutex;
};

template<typename T>
using Shared = std::shared_ptr<T>;

template<typename T>
using Unique = std::unique_ptr<T>;

template<typename ResType>
struct Resource {
    std::string     name;
    HashResult      nameHash;
    HashResult      hash;
    Shared<ResType> resource;
};

template<class ResType>
Resource(std::string, HashResult, HashResult, Unique<ResType>&&) -> Resource<ResType>;

template<typename T>
using ResourceMap = Map<HashResult, Resource<T>>;

class ResourceRegistry {
public:
    template<typename T>
    void addResource(Resource<T> res) {
        if constexpr (std::is_same<Shader, T>())
            return addResource(std::move(res), shaders);
        else if constexpr (std::is_same<Program, T>())
            return addResource(std::move(res), programs);
        else if constexpr (std::is_same<Texture, T>())
            return addResource(std::move(res), textures);
    }

    template<typename T>
    std::optional<Resource<T>> getResource(const std::string& name) {
        auto hash = util::HashBytes64(name);
        return getResource<T>(hash);
    }

    template<typename T>
    std::optional<Resource<T>> getResource(HashResult hash) {
        if constexpr (std::is_same<Shader, T>())
            return getResource(hash, shaders);
        else if constexpr (std::is_same<Program, T>())
            return getResource(hash, programs);
        else if constexpr (std::is_same<Texture, T>())
            return getResource(hash, textures);
    }

    template<typename T>
    bool exists(HashResult nameHash, HashResult hash) {
        const auto opt = getResource<T>(nameHash);
        return opt && opt.value().hash == hash;
    }

private:
    template<typename T>
    std::optional<Resource<T>> getResource(HashResult hash, ResourceMap<T>& map) {
        std::lock_guard lock{map.mutex};

        auto it = map.map.find(hash);
        if (it != map.map.end())
            return it->second;

        return std::nullopt;
    }

    template<typename T>
    void addResource(Resource<T> res, ResourceMap<T>& map) {
        std::lock_guard reslock{map.mutex};
        map.map[res.nameHash] = std::move(res);
    }

    ResourceMap<Shader>  shaders;
    ResourceMap<Program> programs;
    ResourceMap<Texture> textures;
};

} // namespace sdbox

#endif // __SDBOX_RESOURCE_H__