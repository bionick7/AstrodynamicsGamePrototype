#include "assets.hpp"
#include "tests.hpp"
#include "string_builder.hpp"

namespace assets {
    const uint64_t FNV_OFFSET = 14695981039346656037ULL;
    const uint64_t FNV_PRIME = 1099511628211ULL;

    // Return 64-bit FNV-1a hashes for key (NUL-terminated). See description:
    // https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function
    static uint64_t HashesKey(const char* key) {
        uint64_t hash = FNV_OFFSET;
        for (const char* p = key; *p; p++) {
            hash ^= (uint64_t)(unsigned char)(*p);
            hash *= FNV_PRIME;
        }
        return hash;
    }

    Table<Texture2D> texture_table;
    Table<Shader> shader_table;
    Table<Sound> sound_table;
    Table<DataNode> data_table;
}

unsigned char *assets::GetResourceBytes(const char *filepath, int *byte_size) {
    // TODO: enable packing into the app
    return LoadFileData(filepath, byte_size);
}

char *assets::GetResourceText(const char *filepath) {
    return LoadFileText(filepath);
}

Texture2D assets::GetTexture(const char *path) {
    if (!IsWindowReady()) {
        ERROR("Trying to load shader, yet no window initialized")
        return { 0 };
    }
    uint64_t path_hash = HashesKey(path);
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

Sound assets::GetSound(const char* path) {
    if (!IsAudioDeviceReady()) {
        ERROR("Trying to load sound, yet no audio device initialized")
        return { 0 };
    }
    uint64_t path_hash = HashesKey(path);
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
    uint64_t path_hash = HashesKey(path);
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
        if (FileExists(path_vs.c_str)) {file_data_vs = 
            file_data_vs = GetResourceText(path_vs.c_str);
        }
        Shader shader = LoadShaderFromMemory(file_data_vs, file_data_fs);
        free(file_data_fs);
        free(file_data_vs);
        shader_table.Insert(path_hash, shader);
        return shader;
    }
}

DataNode* assets::GetData(const char* path) {
    uint64_t path_hash = HashesKey(path);
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

void assets::Reload() {
    texture_table.Clear();
    shader_table.Clear();
    sound_table.Clear();
    data_table.Clear();
}

int AssetTests() {
    DataNode* d1 = assets::GetData("resources/data/test_data/test_data.yaml");
    TEST_ASSERT_EQUAL(assets::data_table.size, 1)
    assets::GetData("resources/data/test_data/readonly_file.yaml");
    TEST_ASSERT_EQUAL(assets::data_table.size, 2)
    assets::GetData("resources/data/test_data/test_data.yaml");
    assets::GetData("resources/data/test_data/test_data.yaml");
    DataNode* d2 = assets::GetData("resources/data/test_data/test_data.yaml");
    TEST_ASSERT_STREQUAL(d1->Get("key1"), d2->Get("key1"))
    return 0;
}
