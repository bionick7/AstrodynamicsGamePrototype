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
    if (id._internal == UINT32_MAX) return false;
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
        return gs->ships.alloc.IsValidIndex(id);
    case EntityType::MODULE:
        return index < gs->ship_modules.shipmodule_count;
    case EntityType::QUEST:
        return gs->quest_manager.available_quests.IsValidIndex(id);
    case EntityType::ACTIVE_QUEST:
        return gs->quest_manager.active_quests.IsValidIndex(id);
    case EntityType::TASK:
        return gs->quest_manager.active_tasks.IsValidIndex(id);
    default:
    case EntityType::INVALID:
        return false;
    }
}
