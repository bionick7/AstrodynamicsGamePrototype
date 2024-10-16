#include "assets.hpp"
#include "tests.hpp"
#include "string_builder.hpp"
#include "debug_console.hpp"

#define INCLUDE_BAKED  // Comment out to bootstrap baked_data.hpp
#ifdef INCLUDE_BAKED
#include "baked_data.hpp"
#endif

namespace assets {
    Table<Texture2D> texture_table;
    Table<Image> image_table;
    Table<Font> font_table;
    Table<WireframeMesh> mesh_table;
    Table<Shader> shader_table;
    Table<Sound> sound_table;
    Table<DataNode> data_table;
}

// Makes sure the same path is spelled the same
StrHash assets::HashPath(const char* path) {
    ASSERT(strlen(path) < ASSET_PATH_MAX_LENGTH)  // Excluded in final build
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

const bool always_use_embedded_resources = false;  // Debugging setting (cannot make it a 'setting' to avoid recursion)
bool _HasResource(TableKey filepath, bool _is_text) {
    #ifdef INCLUDE_BAKED
    if (FileExists(filepath.txt) && !always_use_embedded_resources) {
        return true;
    }
    for (int i=0; i < BAKED_RESOURCE_COUNT; i++) {
        if (!baked::resources[i].is_text^_is_text && baked::resources[i].path_hash == filepath.h) {
            return true;
        }
    }
    return false;
    #else
    return FileExists(filepath);
    #endif // INCLUDE_BAKED
}

bool assets::HasTextResource(TableKey filepath) {
    return _HasResource(filepath, true);
}

bool assets::HasDataResource(TableKey filepath) {
    return _HasResource(filepath, false);
}

char* _LoadTextFromFileImpl(TableKey filepath, int *byte_size) {
    FILE* file = fopen(filepath.txt, "rt");
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

unsigned char* assets::GetResourceBytes(const char* filepath, int *byte_size) {
    #ifdef INCLUDE_BAKED
    if (FileExists(filepath) && !always_use_embedded_resources) {
        return _LoadBytesFromFileImpl(filepath, byte_size);
    }
    StrHash hash = HashPath(filepath);
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

char* assets::GetResourceText(const char* filepath) {
    int size;
    #ifdef INCLUDE_BAKED
    if (FileExists(filepath) && !always_use_embedded_resources) {
        return _LoadTextFromFileImpl(filepath, &size);
    }
    StrHash hash = HashPath(filepath);
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

template<> Table<Texture2D>*     assets::GetTable<Texture2D>()     { return &assets::texture_table; }
template<> Table<Image>*         assets::GetTable<Image>()         { return &assets::image_table;   }
template<> Table<Font>*          assets::GetTable<Font>()          { return &assets::font_table;    }
template<> Table<WireframeMesh>* assets::GetTable<WireframeMesh>() { return &assets::mesh_table;    }
template<> Table<Shader>*        assets::GetTable<Shader>()        { return &assets::shader_table;  }
template<> Table<Sound>*         assets::GetTable<Sound>()         { return &assets::sound_table;   }
template<> Table<DataNode>*      assets::GetTable<DataNode>()      { return &assets::data_table;    }

Texture2D assets::GetTexture(TableKey path) {
    if (!IsWindowReady()) {
        ERROR("Trying to load shader, yet no window initialized")
        return { 0 };
    }
    int find = texture_table.Find(path);
    if (find >= 0) {
        return texture_table.data[find];
    } else {
        Texture2D texture = LoadTexture(path.txt);
        texture_table.Insert(path, texture);
        return texture;
    }
}

WireframeMesh assets::GetWireframe(TableKey path) {
    int find = mesh_table.Find(path);
    if (find >= 0) {
        return mesh_table.data[find];
    } else {
        WireframeMesh mesh = LoadWireframeMesh(path.txt);
        mesh_table.Insert(path, mesh);
        return mesh;
    }
}

Image assets::GetImage(TableKey path) {
    if (!IsWindowReady()) {
        ERROR("Trying to load image, yet no window initialized")
        return { 0 };
    }
    int find = image_table.Find(path);
    if (find >= 0) {
        return image_table.data[find];
    } else {
        Image image = LoadImage(path.txt);
        image_table.Insert(path, image);
        return image;
    }
}

Font assets::GetFont(TableKey path) {
    if (!IsWindowReady()) {
        ERROR("Trying to load font, yet no window initialized")
        return { 0 };
    }
    int find = font_table.Find(path);
    if (find >= 0) {
        return font_table.data[find];
    } else {
        Font font = LoadFont(path.txt);
        font_table.Insert(path, font);
        return font;
    }
}

Sound assets::GetSound(TableKey path) {
    if (!IsAudioDeviceReady()) {
        ERROR("Trying to load sound, yet no audio device initialized")
        return { 0 };
    }
    int find = sound_table.Find(path);
    if (find >= 0) {
        return sound_table.data[find];
    } else {
        Sound sound = LoadSound(path.txt);
        sound_table.Insert(path, sound);
        return sound;
    }
}

Shader assets::GetShader(TableKey path) {
    if (!IsWindowReady()) {
        ERROR("Trying to load shader, yet no window initialized")
        return { 0 };
    }
    int find = shader_table.Find(path);
    if (find >= 0) {
        return shader_table.data[find];
    } else {
        StringBuilder path_fs;
        StringBuilder path_vs;
        path_fs.Add(path.txt).Add(".fs");
        path_vs.Add(path.txt).Add(".vs");
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
        shader_table.Insert(path, shader);
        return shader;
    }
}

const DataNode* assets::GetData(TableKey path) {
    int find = data_table.Find(path);
    if (find >= 0) {
        return &data_table.data[find];
    } else {
        int index = data_table.Insert(path, DataNode());
        DataNode::FromFile(&data_table.data[index], path.txt);
        return &data_table.data[index];
    }
}

bool assets::IsShaderLoaded(TableKey path) {
    int find = shader_table.Find(path);
    if (find < 0) return false;
    return IsShaderReady(shader_table.data[find]);
}

#define UNLOAD_TABLE(type, table) \
for(int i=0; i < table.size; i++) { \
    Unload##type(table.data[i]); \
} \
INFO("Unloaded %d " #type "s", table.size)\
table.Clear();

void assets::Reload() {
    UNLOAD_TABLE(Texture, texture_table)
    UNLOAD_TABLE(Image, image_table)
    UNLOAD_TABLE(Font, font_table)
    UNLOAD_TABLE(WireframeMesh, mesh_table)
    UNLOAD_TABLE(Shader, shader_table)
    UNLOAD_TABLE(Sound, sound_table)

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
        || IsFileExtension(path, ".fs")
        || IsFileExtension(path, ".obj");
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
    fprintf(out_file, "#define BAKED_DATA_SIZE %zu\n", data_size);
    fprintf(out_file, "#define BAKED_TEXT_SIZE %zu\n", text_size);
    fprintf(out_file, "#define BAKED_RESOURCE_COUNT %d\n\n", resource_count);
    fprintf(out_file, "assets::BakedResource resources[] = {\n");
    for(int i=0; i < resource_count; i++) {
        fprintf(out_file, "    { 0x%016" LONG_STRID "X, %d, %10zu, %10zu }, // %s\n", 
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
        /*if (text_buffer[i] < 0) {
            int resource_index = -1;
            for(int j=0; j < resource_count; j++) {
                if(resources[j].offset < i && resources[j].offset + resources[j].size > i && resources[j].is_text) 
                    resource_index = j;
            }
            FAIL("text at %d (%s, char %d) has a negative char ('%c' - %d)", i, 
                rsc_filepaths[resource_index], 
                i - resources[resource_index].offset,
                text_buffer[i], 
                text_buffer[i]
            )
        }*/
        
        int count = fprintf(out_file, "%2d,", text_buffer[i]);
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
    INFO("Unbake Resources (Not implemented)")
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
