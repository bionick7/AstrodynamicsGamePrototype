#include "assets.hpp"
#include "tests.hpp"
#include "string_builder.hpp"
#include "debug_console.hpp"

#define INCLUDE_BAKED  // Comment out to bootstrap baked_data.hpp
#ifdef INCLUDE_BAKED
#include "baked_data.hpp"
#endif

namespace assets {
    const uint64_t FNV_OFFSET = 14695981039346656037ULL;
    const uint64_t FNV_PRIME = 1099511628211ULL;

    Table<Texture2D> texture_table;
    Table<Image> image_table;
    Table<Font> font_table;
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

// Makes sure the same path is spelled the same
uint64_t assets::HashPath(const char* path) {
    if (path[0] == '.' && path[1] == '/')
        return HashKey(&path[2]);
    return HashKey(path);
}

size_t GetFileSize(const char* filepath, bool is_text_file) {
    FILE* file;
    if (is_text_file) {
        file = fopen(filepath, "rt");
    } else {
        file = fopen(filepath, "rb");
    }
    if (file == NULL) {
        ERROR("Could not open file '%s'", filepath)
        return 0;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);

    fclose(file);
    if (is_text_file)
        size += 1;
    return size;
}

unsigned char* _LoadBytesFromFileImpl(const char *filepath, int *byte_size) {
    FILE* file = fopen(filepath, "rb");
    if (file == NULL) {
        ERROR("Could not open file '%s'", filepath)
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    *byte_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    unsigned char* data = (unsigned char*) malloc(*byte_size * sizeof(unsigned char));
    fread(data, sizeof(unsigned char), *byte_size, file);
    fclose(file);
    return data;
}

#ifdef INCLUDE_BAKED
unsigned char* _LoadBytesFromBaked(assets::BakedResource rsc, int* byte_size) {
    *byte_size = rsc.size;
    unsigned char* data = (unsigned char*) malloc(rsc.size * sizeof(unsigned char));
    memcpy(data, &baked::data[rsc.offset], rsc.size);
    return data;
}

char* _LoadTextFromBaked(assets::BakedResource rsc, int* byte_size) {
    *byte_size = rsc.size;
    char* data = (char*) malloc(rsc.size * sizeof(char));
    memcpy(data, &baked::text[rsc.offset], rsc.size);
    return data;
}
#endif  // INCLUDE_BAKED

const bool always_use_embedded_resources = false;  // Debugging setting (cannot make it a 'setting' to avoid recursivity)
bool _HasResource(const char* filepath, bool _is_text) {
    #ifdef INCLUDE_BAKED
    if (FileExists(filepath) && !always_use_embedded_resources) {
        return true;
    }
    uint64_t hash = assets::HashPath(filepath);
    for (int i=0; i < BAKED_RESOURCE_COUNT; i++) {
        if (!baked::resources[i].is_text^_is_text && baked::resources[i].path_hash == hash) {
            return true;
        }
    }
    return false;
    #else
    return FileExists(filepath);
    #endif // INCLUDE_BAKED
}

bool assets::HasTextResource(const char* filepath) {
    return _HasResource(filepath, true);
}

bool assets::HasDataResource(const char* filepath) {
    return _HasResource(filepath, false);
}

char* _LoadTextFromFileImpl(const char *filepath, int *byte_size) {
    FILE* file = fopen(filepath, "rt");
    if (file == NULL) {
        ERROR("Could not open file '%s'", filepath)
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* data = (char*) malloc(size + 1);
    int count = fread(data, 1, size, file);
    if (count < size) {
        data = (char*) realloc(data, count + 1);
    }
    data[count] = '\0';
    *byte_size = count+1;

    fclose(file);
    return data;
}

unsigned char* assets::GetResourceBytes(const char *filepath, int *byte_size) {
    #ifdef INCLUDE_BAKED
    if (FileExists(filepath) && !always_use_embedded_resources) {
        return _LoadBytesFromFileImpl(filepath, byte_size);
    }
    uint64_t hash = HashPath(filepath);
    for (int i=0; i < BAKED_RESOURCE_COUNT; i++) {
        if (!baked::resources[i].is_text && baked::resources[i].path_hash == hash) {
            return _LoadBytesFromBaked(baked::resources[i], byte_size);
        }
    }
    ERROR("No baked resource (data) or file found corresponding to '%s'", filepath);
    *byte_size = 0;
    return NULL;
    #else
    return _LoadBytesFromFileImpl(filepath, byte_size);
    #endif // INCLUDE_BAKED
}

char* assets::GetResourceText(const char *filepath) {
    int size;
    #ifdef INCLUDE_BAKED
    if (FileExists(filepath) && !always_use_embedded_resources) {
        return _LoadTextFromFileImpl(filepath, &size);
    }
    uint64_t hash = HashPath(filepath);
    for (int i=0; i < BAKED_RESOURCE_COUNT; i++) {
        if (baked::resources[i].is_text && baked::resources[i].path_hash == hash) {
            return _LoadTextFromBaked(baked::resources[i], &size);
        }
    }
    ERROR("No baked resource (text) or file found corresponding to '%s'", filepath);
    return NULL;
    #else
    return _LoadTextFromFileImpl(filepath, &size);
    #endif // INCLUDE_BAKED
}

template<> assets::Table<Texture2D>* assets::GetTable<Texture2D>() { return &assets::texture_table; }
template<> assets::Table<Image>*     assets::GetTable<Image>()     { return &assets::image_table;   }
template<> assets::Table<Font>*      assets::GetTable<Font>()      { return &assets::font_table;    }
template<> assets::Table<Shader>*    assets::GetTable<Shader>()    { return &assets::shader_table;  }
template<> assets::Table<Sound>*     assets::GetTable<Sound>()     { return &assets::sound_table;   }
template<> assets::Table<DataNode>*  assets::GetTable<DataNode>()  { return &assets::data_table;    }

Texture2D assets::GetTexture(const char *path) {
    if (!IsWindowReady()) {
        ERROR("Trying to load shader, yet no window initialized")
        return { 0 };
    }
    uint64_t path_hash = HashPath(path);
    int find = texture_table.Find(path_hash);
    if (find >= 0) {
        return texture_table.data[find];
    } else {
        Texture2D texture = LoadTexture(path);
        texture_table.Insert(path_hash, texture);
        return texture;
    }
}

Image assets::GetImage(const char *path) {
    if (!IsWindowReady()) {
        ERROR("Trying to load image, yet no window initialized")
        return { 0 };
    }
    uint64_t path_hash = HashPath(path);
    int find = image_table.Find(path_hash);
    if (find >= 0) {
        return image_table.data[find];
    } else {
        Image image = LoadImage(path);
        image_table.Insert(path_hash, image);
        return image;
    }
}

Font assets::GetFont(const char *path) {
    if (!IsWindowReady()) {
        ERROR("Trying to load font, yet no window initialized")
        return { 0 };
    }
    uint64_t path_hash = HashPath(path);
    int find = font_table.Find(path_hash);
    if (find >= 0) {
        return font_table.data[find];
    } else {
        Font font = LoadFont(path);
        font_table.Insert(path_hash, font);
        return font;
    }
}

Sound assets::GetSound(const char* path) {
    if (!IsAudioDeviceReady()) {
        ERROR("Trying to load sound, yet no audio device initialized")
        return { 0 };
    }
    uint64_t path_hash = HashPath(path);
    int find = sound_table.Find(path_hash);
    if (find >= 0) {
        return sound_table.data[find];
    } else {
        Sound sound = LoadSound(path);
        sound_table.Insert(path_hash, sound);
        return sound;
    }
}

Shader assets::GetShader(const char* path) {
    if (!IsWindowReady()) {
        ERROR("Trying to load shader, yet no window initialized")
        return { 0 };
    }
    uint64_t path_hash = HashPath(path);
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
        if (assets::HasTextResource(path_fs.c_str)) {
            file_data_fs = GetResourceText(path_fs.c_str);
        }
        if (assets::HasTextResource(path_vs.c_str)) {
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
    uint64_t path_hash = HashPath(path);
    int find = data_table.Find(path_hash);
    if (find >= 0) {
        return &data_table.data[find];
    } else {
        int index = data_table.Insert(path_hash, DataNode());
        DataNode::FromFile(&data_table.data[index], path);
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

bool _IsDataFile(const char* path) {
    return IsFileExtension(path, ".png")
        || IsFileExtension(path, ".wav")
        || IsFileExtension(path, ".jpg");
}

bool _IsTextFile(const char* path) {
    return IsFileExtension(path, ".yaml")
        || IsFileExtension(path, ".wren")
        || IsFileExtension(path, ".fnt")
        || IsFileExtension(path, ".vs")
        || IsFileExtension(path, ".fs");
}

void assets::BakeAllResources() {
    INFO("Bake Resource Folder (writes C code)")
    FilePathList fpl = LoadDirectoryFilesEx("resources", NULL, true);
    size_t data_size = 0;
    size_t text_size = 0;
    int resource_count = 0;

    // Counting loop
    for(int i=0; i < fpl.count; i++) {
        if (_IsTextFile(fpl.paths[i])) {
            resource_count++;
        }
        if (_IsDataFile(fpl.paths[i])) {
            resource_count++;
        }
    }
    BakedResource* resources = new BakedResource[resource_count];
    // Not worth integrationg into struct and expires on UnloadDirectory Files
    char** rsc_filepaths = new char*[resource_count];

    // Measuring loop
    int indexer = 0;
    for(int i=0; i < fpl.count; i++) {
        if (_IsTextFile(fpl.paths[i])) {
            size_t size = GetFileSize(fpl.paths[i], true);
            INFO("Baking %s [%lu KB]", fpl.paths[i], size / 1024)
            resources[indexer].is_text = true;
            resources[indexer].offset = text_size;
            rsc_filepaths[indexer] = fpl.paths[i];
            resources[indexer].path_hash = HashPath(fpl.paths[i]);
            resources[indexer].size = size;
            text_size += size;
            indexer++;
        }
        if (_IsDataFile(fpl.paths[i])) {
            size_t size = GetFileSize(fpl.paths[i], false);
            INFO("Baking %s [%lu KB]", fpl.paths[i], size / 1024)
            resources[indexer].is_text = false;
            resources[indexer].offset = data_size;
            rsc_filepaths[indexer] = fpl.paths[i];
            resources[indexer].path_hash = HashPath(fpl.paths[i]);
            resources[indexer].size = size;
            data_size += size;
            indexer++;
        }
    }

    unsigned char* data_buffer = new unsigned char[data_size];
    char* text_buffer = new char[text_size];
    // Copying loop
    for(int i=0; i < resource_count; i++) {
        if (resources[i].is_text) {
            int file_size;
            char* data = _LoadTextFromFileImpl(rsc_filepaths[i], &file_size);
            memcpy(&text_buffer[resources[i].offset], data, resources[i].size);
            free(data);
        } else {
            int file_size;
            unsigned char* data = _LoadBytesFromFileImpl(rsc_filepaths[i], &file_size);
            memcpy(&data_buffer[resources[i].offset], data, resources[i].size);
            free(data);
        }
    }

    const int BYTES_PER_LINE = 100;
    
    FILE* out_file = fopen("src/baked_data.hpp", "wt");
    fprintf(out_file, "/*\n");
    fprintf(out_file, "    This file is auto-generated by the resource baking process\n");
    fprintf(out_file, "    Run the application with the '-bake' argument to regenerate this file\n");
    fprintf(out_file, "*/\n");
    fprintf(out_file, "#ifndef BAKED_DATA_H\n");
    fprintf(out_file, "#define BAKED_DATA_H\n");
    fprintf(out_file, "#include \"basic.hpp\"\n");
    fprintf(out_file, "#include \"assets.hpp\"\n\n");
    fprintf(out_file, "namespace baked {\n");
    fprintf(out_file, "#define BAKED_DATA_SIZE %lu\n", data_size);
    fprintf(out_file, "#define BAKED_TEXT_SIZE %lu\n", text_size);
    fprintf(out_file, "#define BAKED_RESOURCE_COUNT %d\n\n", resource_count);
    fprintf(out_file, "assets::BakedResource resources[] = {\n");
    for(int i=0; i < resource_count; i++) {
        fprintf(out_file, "    { 0x%016llX, %d, %10lu, %10lu }, // %s\n", 
            resources[i].path_hash, 
            resources[i].is_text,
            resources[i].offset,
            resources[i].size,
            rsc_filepaths[i]
        );
    }
    fprintf(out_file, "};\n\n");
    fprintf(out_file, "const unsigned char data[] = {");
    for (int i=0; i < data_size; i++) {
        if (i % BYTES_PER_LINE == 0) {
            fprintf(out_file, "\n");
        }
        fprintf(out_file, "0x%02X,", data_buffer[i]);
    }
    fprintf(out_file, "\n    };\n\n");
    fprintf(out_file, "\n");
    fprintf(out_file, "const char text[] = {");
    for (int i=0; i < text_size; i++) {
        if (i % BYTES_PER_LINE == 0) {
            fprintf(out_file, "\n");
        }
        fprintf(out_file, "0x%02X,", text_buffer[i]);
    }
    fprintf(out_file, "\n    };\n\n");
    fprintf(out_file, "static_assert(sizeof(data) == BAKED_DATA_SIZE);\n");
    fprintf(out_file, "static_assert(sizeof(text) == BAKED_TEXT_SIZE);\n");
    fprintf(out_file, "static_assert(sizeof(resources) == BAKED_RESOURCE_COUNT * sizeof(assets::BakedResource));\n");
    fprintf(out_file, "}  // end namespace\n");
    fprintf(out_file, "#endif  // BAKED_DATA_H\n");
    fclose(out_file);
    UnloadDirectoryFiles(fpl);

    delete[] resources;
    delete[] rsc_filepaths;
    delete[] data_buffer;
    delete[] text_buffer;
}

void assets::UnBakeAllResources() {
    INFO("Unbake Resources")

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
