#include "assets.hpp"
#include "tests.hpp"
#include "string_builder.hpp"

namespace assets {
    const uint64_t FNV_OFFSET = 14695981039346656037ULL;
    const uint64_t FNV_PRIME = 1099511628211ULL;

    Table<Texture2D> texture_table;
    Table<Image> image_table;
    Table<Shader> shader_table;
    Table<Sound> sound_table;
    Table<DataNode> data_table;
}

// Return 64-bit FNV-1a hashes for key (NUL-terminated). See description:
// https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function
uint64_t assets::HashKey(const char* key) {
    uint64_t hash = assets::FNV_OFFSET;
    for (const char* p = key; *p; p++) {
        hash ^= (uint64_t)(unsigned char)(*p);
        hash *= assets::FNV_PRIME;
    }
    return hash;
}

unsigned char *assets::GetResourceBytes(const char *filepath, int *byte_size) {
    // TODO: enable packing into the app
    return LoadFileData(filepath, byte_size);
}

char *assets::GetResourceText(const char *filepath) {
    return LoadFileText(filepath);
}

template<> assets::Table<Texture2D>* assets::GetTable<Texture2D>() { return &assets::texture_table; }
template<> assets::Table<Image>*     assets::GetTable<Image>()     { return &assets::image_table;   }
template<> assets::Table<Shader>*    assets::GetTable<Shader>()    { return &assets::shader_table;  }
template<> assets::Table<Sound>*     assets::GetTable<Sound>()     { return &assets::sound_table;   }
template<> assets::Table<DataNode>*  assets::GetTable<DataNode>()  { return &assets::data_table;    }

Texture2D assets::GetTexture(const char *path) {
    if (!IsWindowReady()) {
        ERROR("Trying to load shader, yet no window initialized")
        return { 0 };
    }
    uint64_t path_hash = HashKey(path);
    int find = texture_table.Find(path_hash);
    if (find >= 0) {
        return texture_table.data[find];
    } else {
        Texture2D texture = { 0 };
        int data_size = 0;
        unsigned char *file_data = GetResourceBytes(path, &data_size);
        Image image = LoadImageFromMemory(GetFileExtension(path), file_data, data_size);
        if (image.data == NULL) {
            return texture;
        }
        texture = LoadTextureFromImage(image);
        UnloadImage(image);
        free(file_data);

        texture_table.Insert(path_hash, texture);
        return texture;
    }
}

Image assets::GetImage(const char *path) {
    if (!IsWindowReady()) {
        ERROR("Trying to load shader, yet no window initialized")
        return { 0 };
    }
    uint64_t path_hash = HashKey(path);
    int find = image_table.Find(path_hash);
    if (find >= 0) {
        return image_table.data[find];
    } else {
        int data_size = 0;
        unsigned char *file_data = GetResourceBytes(path, &data_size);
        Image image = LoadImageFromMemory(GetFileExtension(path), file_data, data_size);
        free(file_data);

        image_table.Insert(path_hash, image);
        return image;
    }
}

Sound assets::GetSound(const char* path) {
    if (!IsAudioDeviceReady()) {
        ERROR("Trying to load sound, yet no audio device initialized")
        return { 0 };
    }
    uint64_t path_hash = HashKey(path);
    int find = sound_table.Find(path_hash);
    if (find >= 0) {
        return sound_table.data[find];
    } else {
        int data_size = 0;
        unsigned char *file_data = GetResourceBytes(path, &data_size);
        Wave wave = LoadWaveFromMemory(GetFileExtension(path), file_data, data_size);
        Sound sound = LoadSoundFromWave(wave);
        UnloadWave(wave);
        free(file_data);

        sound_table.Insert(path_hash, sound);
        return sound;
    }
}

Shader assets::GetShader(const char* path) {
    if (!IsWindowReady()) {
        ERROR("Trying to load shader, yet no window initialized")
        return { 0 };
    }
    uint64_t path_hash = HashKey(path);
    int find = shader_table.Find(path_hash);
    if (find >= 0) {
        return shader_table.data[find];
    } else {
        StringBuilder path_fs;
        StringBuilder path_vs;
        path_fs.Add(path).Add(".fs");
        path_vs.Add(path).Add(".vs");
        char *file_data_fs = NULL;
        char *file_data_vs = NULL;
        if (FileExists(path_fs.c_str)) {
            file_data_fs = GetResourceText(path_fs.c_str);
        }
        if (FileExists(path_vs.c_str)) {
            file_data_vs = GetResourceText(path_vs.c_str);
        }
        Shader shader = LoadShaderFromMemory(file_data_vs, file_data_fs);
        free(file_data_fs);
        free(file_data_vs);
        shader_table.Insert(path_hash, shader);
        return shader;
    }
}

const DataNode* assets::GetData(const char* path) {
    uint64_t path_hash = HashKey(path);
    int find = data_table.Find(path_hash);
    if (find >= 0) {
        return &data_table.data[find];
    } else {
        unsigned int data_size = 0;
        char *file_data = GetResourceText(path);
        int index = data_table.Insert(path_hash, DataNode());
        DataNode::FromMemory(&data_table.data[index], path, file_data);
        free(file_data);
        return &data_table.data[index];
    }
}

bool assets::IsTextureLoaded(Texture2D instance) {
    for(int i=0; i < texture_table.size; i++) {
        if (texture_table.data[i].id == instance.id) {
            return IsTextureReady(instance);
        }
    }
    return false;
}

bool assets::IsShaderLoaded(Shader instance) {
    for(int i=0; i < shader_table.size; i++) {
        if (shader_table.data[i].id == instance.id) {
            return IsShaderReady(instance);
        }
    }
    return false;
}

void assets::Reload() {
    for(int i=0; i < texture_table.size; i++) {
        UnloadTexture(texture_table.data[i]);
    }
    for(int i=0; i < image_table.size; i++) {
        UnloadImage(image_table.data[i]);
    }
    for(int i=0; i < shader_table.size; i++) {
        UnloadShader(shader_table.data[i]);
    }
    for(int i=0; i < sound_table.size; i++) {
        UnloadSound(sound_table.data[i]);
    }
    texture_table.Clear();
    image_table.Clear();
    shader_table.Clear();
    sound_table.Clear();
    data_table.Clear();
}

int AssetTests() {
    const DataNode* d1 = assets::GetData("resources/data/test_data/test_data.yaml");
    TEST_ASSERT_EQUAL(assets::data_table.size, 1)
    assets::GetData("resources/data/test_data/readonly_file.yaml");
    TEST_ASSERT_EQUAL(assets::data_table.size, 2)
    assets::GetData("resources/data/test_data/test_data.yaml");
    assets::GetData("resources/data/test_data/test_data.yaml");
    const DataNode* d2 = assets::GetData("resources/data/test_data/test_data.yaml");
    TEST_ASSERT_STREQUAL(d1->Get("key1"), d2->Get("key1"))
    return 0;
}
