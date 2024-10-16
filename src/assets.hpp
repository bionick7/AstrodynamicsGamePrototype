#ifndef ASSETS_H
#define ASSETS_H

#include "basic.hpp"
#include "datanode.hpp"
#include "wireframe_mesh.hpp"
#include "table.hpp"

#define ASSET_PATH_MAX_LENGTH 256

namespace assets {
    struct BakedResource {
        StrHash path_hash;
        bool is_text;
        size_t offset;
        size_t size;
    };
    StrHash HashPath(const char* path);

    bool HasTextResource(TableKey path);
    bool HasDataResource(TableKey path);

    // Must be const char*, since its called from raylib
    unsigned char* GetResourceBytes(const char* filepath, int* size);
    char* GetResourceText(const char* filepath);

    Texture2D GetTexture(TableKey path);
    WireframeMesh GetWireframe(TableKey path);
    Image GetImage(TableKey path);
    Font GetFont(TableKey path);
    Shader GetShader(TableKey path);
    Sound GetSound(TableKey path);
    const DataNode* GetData(TableKey path);

    bool IsShaderLoaded(TableKey path);

    void Reload();

    // Used by dev to write header files that include all resource files
    void BakeAllResources();
    // Used by user to unpack user folder for modding/troubleshooting purposes
    void UnBakeAllResources();

    template<typename T> Table<T>* GetTable();

    template <typename T>
    bool Has(TableKey path) {
        return GetTable<T>()->Find(path) >= 0;
    }
}

int AssetTests();

#endif  // ASSETS_H