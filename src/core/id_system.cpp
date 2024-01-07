#include "id_system.hpp"
#include "logging.hpp"
#include "global_state.hpp"

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

    GlobalState* gs = GlobalGetState();
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
    default:
    case EntityType::INVALID:
        return false;
    }
}

IDList::IDList() {
    capacity = 5;
    size = 0;
    buffer = (RID*) malloc(sizeof(RID) * capacity);
}

IDList::~IDList() {
    free(buffer);
}

void IDList::Append(RID id) {
    if (size >= capacity) {
        int extension = capacity/2;
        if (extension < 5) extension = 5;
        capacity += extension;
        buffer = (RID*) realloc(buffer, sizeof(RID) * capacity);
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

void IDList::Clear() {
    capacity = 5;
    size = 0;
    free(buffer);
    buffer = (RID*) malloc(sizeof(RID) * capacity);
}
