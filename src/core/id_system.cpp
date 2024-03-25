#include "id_system.hpp"
#include "logging.hpp"
#include "global_state.hpp"
#include "datanode.hpp"

RID::RID() {
    *this = GetInvalidId();
}

RID::RID(uint32_t p_internal) {
    _internal = p_internal;
}

RID::RID(uint32_t index, EntityType type) {
    if (index > 0x00fffffful) {
        ERROR("Count for type %ld exceeds overflow (%ld)", type, index);
        *this = GetInvalidId();
    }
    _internal = index | ((uint8_t) type << 24);
}

bool IsIdValid(RID id) {
    if (id.AsInt() == UINT32_MAX) return false;
    EntityType type = IdGetType(id);
    if (type == EntityType::UNINITIALIZED) {
        FAIL("Uninitialized");
    }
    uint32_t index = IdGetIndex(id);

    GlobalState* gs = GetGlobalState();
    switch (type) {
    case EntityType::PLANET:
        return index < gs->planets.GetPlanetCount();
    case EntityType::SHIP:
        return gs->ships.alloc.ContainsID(id);
    case EntityType::SHIP_CLASS:
        return index < gs->ships.ship_classes_count;
    case EntityType::MODULE_CLASS:
        return index < gs->ship_modules.shipmodule_count;
    case EntityType::QUEST:
        return gs->quest_manager.available_quests.ContainsID(id);
    case EntityType::ACTIVE_QUEST:
        return gs->quest_manager.active_quests.ContainsID(id);
    case EntityType::TASK:
        return gs->quest_manager.active_tasks.ContainsID(id);
    case EntityType::DIALOGUE:
        return gs->quest_manager.dialogues.ContainsID(id);
    case EntityType::ICON3D:
        return gs->render_server.icons.ContainsID(id);
    case EntityType::TEXT3D:
        return gs->render_server.text_labels_3d.ContainsID(id);
    case EntityType::TECHTREE_NODE:
        return index < gs->techtree.nodes_count;
    default:
    case EntityType::INVALID:
        return false;
    }
}

bool IsIdValidTyped(RID id, EntityType type) {
    if (IdGetType(id) != type) {
        return false;
    }
    return IsIdValid(id);
}

IDList::IDList() {
    capacity = 5;
    size = 0;
    buffer = new RID[capacity];
}

IDList::IDList(int initial_capacity){
    capacity = initial_capacity;
    size = 0;
    buffer = new RID[capacity];
}

IDList::IDList(const IDList& other) {
    capacity = other.size;
    size = other.size;
    buffer = new RID[capacity];
    for(int i=0; i < size; i++) {
        buffer[i] = other[i];
    }
}

IDList::~IDList() {
    delete[] buffer;
}

void IDList::Resize(int new_capacity) {
    if (new_capacity == 0) {
        delete[] buffer;
        buffer = NULL;
    } else {
        RID* buffer2 = new RID[new_capacity];
        memcpy(buffer2, buffer, sizeof(RID) * size);
        delete[] buffer;
        buffer = buffer2;
    }
    capacity = new_capacity;
}

void IDList::Append(RID id) {
    if (size >= capacity) {
        int extension = capacity/2;
        if (extension < 5) extension = 5;
        capacity += extension;
        Resize(capacity + extension);
    }
    buffer[size] = id;
    size++;
}

void IDList::EraseAt(int index) {
    // O(n) expensive
    if (index >= size) return;
    for(int i=index; i < size; i++) {
        buffer[i] = buffer[i+1];
    }
    size--;
}

RID IDList::Get(int index) const {
    if (index >= size) return GetInvalidId();
    return buffer[index];
}

int IDList::Count() const {
    return size;
}

int IDList::Find(RID id) const {
    for(int i=0; i < size; i++) {
        if (id == buffer[i]) return i;
    }
    return -1;
}

void IDList::Clear() {
    capacity = 5;
    size = 0;
    Resize(5);
}

void IDList::SerializeTo(DataNode* data, const char * key) const {
    data->CreateArray(key, size);
    for(int i=0; i < size; i++) {
        data->InsertIntoArrayI(key, i, buffer[i].AsInt());
    }
}

void IDList::DeserializeFrom(const DataNode *data, const char *key, bool quiet) {
    int p_size = data->GetArrayLen(key, quiet);
    Resize(p_size);
    size = p_size;
    for(int i=0; i < size; i++) {
        buffer[i] = RID(data->GetArrayElemI(key, i, quiet));
    }
}

IDList& IDList::operator=(const IDList& other) {
    capacity = other.size;
    size = other.size;
    if (capacity == 0) {
        buffer = NULL;    
    } else {
        buffer = new RID[capacity];
        for(int i=0; i < size; i++) {
            buffer[i] = other[i];
        }
    }
    return *this;
}

RID IDList::operator[](int index) const {
    return buffer[index];
}

static IDList::SortFn* _current_fn_id = NULL;

int _cmp_func(const void * a, const void * b) {
    if (_current_fn_id == NULL) {
        FAIL("May only be called inside of Sort");
    }
    //SHOW_I(*(RID*)a)
    //SHOW_I(*(RID*)b)
    return _current_fn_id(*(RID*)a, *(RID*)b);
}

void IDList::Sort(SortFn *fn) {
    if (size <= 1) return;
    _current_fn_id = fn;
    qsort(buffer, size, sizeof(RID), _cmp_func);
    _current_fn_id = NULL;
}

void IDList::Inspect() {
    for(int i=0; i < size; i++) {
        printf("%d: type %d, id %d\n", i, (int) IdGetType(buffer[i]), IdGetIndex(buffer[i]));
    }
}
