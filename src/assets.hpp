#ifndef ASSETS_H
#define ASSETS_H

#include "basic.hpp"
#include "datanode.hpp"
#include "wireframe_mesh.hpp"

namespace assets {
    struct BakedResource {
        uint64_t path_hash;
        bool is_text;
        size_t offset;
        size_t size;
    };
    uint64_t HashKey(const char* key);
    uint64_t HashPath(const char* path);

    bool HasTextResource(const char* path);
    bool HasDataResource(const char* path);

    unsigned char* GetResourceBytes(const char* filepath, int* size);
    char* GetResourceText(const char* filepath);

    Texture2D GetTexture(const char* path);
    WireframeMesh GetWirframe(const char* path);
    Image GetImage(const char* path);
    Font GetFont(const char* path);
    Shader GetShader(const char* path);
    Sound GetSound(const char* path);
    const DataNode* GetData(const char* path);

    bool IsTextureLoaded(Texture2D instance);
    bool IsShaderLoaded(Shader instance);

    void Reload();

    // Used in dev mode to write header files that include all resource files
    void BakeAllResources();
    // Used on user end to unpack user folder for modding/troubleshooting purposes
    void UnBakeAllResources();

    template<typename T>
    struct Table { 
        uint64_t* hashes = NULL;
        T* data = NULL;

        int size = 0;
        int capacity = 0;

        Table() {
            Init();
        }

        ~Table() {
            Clear();
        }

        int Find(uint64_t hash) const {
            for (int i=0; i < size; i++) {
                if (hash == hashes[i]) {
                    return i;
                }
            }
            return -1;
        }

        int Insert(uint64_t hash, T value) {
            size++;
            if (size > capacity) {
                capacity += 10;
                uint64_t* new_hashes = new uint64_t[capacity];
                T* new_data = new T[capacity];
                for(int i=0; i < size; i++) {
                    new_hashes[i] = hashes[i];
                    new_data[i] = data[i];
                }
                delete[] hashes;
                delete[] data;
                hashes = new_hashes;
                data = new_data;
            }
            hashes[size-1] = hash;
            data[size-1] = value;
            return size-1;
        }

        void Clear() {
            delete[] hashes;
            delete[] data;
            hashes = NULL;
            data = NULL;
            size = 0;
            capacity = 0;
        }

        void Init() {
            size = 0;
            capacity = 10;
            hashes = new uint64_t[capacity];
            data = new T[capacity];
        }

        void Reset() {
            Clear();
            Init();
        }
    };

    template<typename T> Table<T>* GetTable();

    template <typename T>
    bool Has(const char* path) {
        return GetTable<T>()->Find(HashKey(path)) >= 0;
    }
}

int AssetTests();

#endif  // ASSETS_H