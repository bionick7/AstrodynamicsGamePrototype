#ifndef ASSETS_H
#define ASSETS_H

#include "basic.hpp"
#include "datanode.hpp"

namespace assets {
    unsigned char* GetResourceBytes(const char* filepath, int* size);
    char* GetResourceText(const char* filepath);

    Texture2D GetTexture(const char* path);
    Sound GetSound(const char* path);
    Shader GetShader(const char* path);
    DataNode* GetData(const char* path);

    void Reload();

    template<typename T>
    struct Table { 
        uint64_t* hashes = NULL;
        T* data = NULL;
        int size = 0;
        int capacity = 0;

        Table() {}
        ~Table() {
            Clear();
        }

        int Find(uint64_t hash) const {
            if (hashes == NULL) return -1;
            for (int i=0; i < size; i++) {
                if (hash == hashes[i]) {
                    return i;
                }
            }
            return -1;
        }

        int Insert(uint64_t hash, T value) {
            if (hashes == NULL) {
                size = 0;
                capacity = 10;
                hashes = new uint64_t[capacity];
                data = new T[capacity];
            }
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
    };
}

int AssetTests();

#endif  // ASSETS_H