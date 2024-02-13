#include "ship_modules.hpp"
#include "global_state.hpp"
#include "ship.hpp"
#include "ui.hpp"
#include "constants.hpp"

ShipModuleClass::ShipModuleClass() {
    mass = 0.0;
    name[0] = '\0';
    description[0] = '\0';
    has_activation_requirements = false;

    for (int i=0; i < (int) ShipStats::MAX; i++) {
        delta_stats[i] = 0;
        required_stats[i] = 0;
    }

    for (int i=0; i < (int) RESOURCE_MAX; i++) {
        production[i] = 0;
        construction_resources[i] = 0;
    }
}

void ShipModuleClass::UpdateStats(Ship* ship) const {
    for (int i=0; i < (int) ShipStats::MAX; i++) {
        if (ship->stats[i] < required_stats[i]) {
            return;
        }
    }
    for (int i=0; i < (int) ShipStats::MAX; i++) {
        ship->stats[i] += delta_stats[i];
    }
}

void ShipModuleClass::UpdateCustom(Ship* ship) const {
    for (int i=0; i < (int) ShipStats::MAX; i++) {
        if (ship->stats[i] < required_stats[i]) {
            return;
        }
    }
    bool new_day = GetCalendar()->IsNewDay();
    if (new_day && ship->IsParked()) {
        Planet* planet = GetPlanet(ship->GetParentPlanet());
        for (int i=0; i < RESOURCE_MAX; i++) {
            if (production[i] != 0) {
                planet->economy.GiveResource(ResourceTransfer((ResourceType) i, production[i]));
            }
        }
    }
}

bool ShipModuleClass::HasDependencies() const {
    return has_activation_requirements;
}

ShipModuleSlot::ShipModuleSlot(RID p_entity, int p_index, ShipModuleSlotType p_type) {
    entity = p_entity;
    index = p_index;
    type = p_type;
}

void ShipModuleSlot::SetSlot(RID module_) const {
    if (type == ShipModuleSlot::DRAGGING_FROM_PLANET) {
        GetPlanet(entity)->ship_module_inventory[index] = module_;
    } else if (type == ShipModuleSlot::DRAGGING_FROM_SHIP) {
        GetShip(entity)->modules[index] = module_;
    }
}

RID ShipModuleSlot::GetSlot() const {
    if (type == ShipModuleSlot::DRAGGING_FROM_PLANET) {
        return GetPlanet(entity)->ship_module_inventory[index];
    } else if (type == ShipModuleSlot::DRAGGING_FROM_SHIP) {
        return GetShip(entity)->modules[index];
    }
    return GetInvalidId();
}

void ShipModuleSlot::AssignIfValid(ShipModuleSlot other) {
    if (other.IsValid()) {
        *this = other;
    }
}

bool ShipModuleSlot::IsValid() const {
    return IsIdValid(entity) && index >= 0;
}

bool ShipModuleSlot::IsReachable(ShipModuleSlot other) const {
    if (!IsValid() || !other.IsValid()) return false;
    RID own_planet = GetInvalidId();
    RID other_planet = GetInvalidId();
    if (type == ShipModuleSlot::DRAGGING_FROM_PLANET) {
        own_planet = entity;
    } else if (type == ShipModuleSlot::DRAGGING_FROM_SHIP) {
        own_planet = GetShip(entity)->GetParentPlanet();  // Might be invalid
        if (!GetShip(entity)->CanDragModule(index)) {
            return false;
        }
    }
    
    if (other.type == ShipModuleSlot::DRAGGING_FROM_PLANET) {
        other_planet = other.entity;
    } else if (other.type == ShipModuleSlot::DRAGGING_FROM_SHIP) {
        other_planet = GetShip(other.entity)->GetParentPlanet();  // Might be invalid
    }

    return own_planet == other_planet;
}

int ShipModules::Load(const DataNode* data) {
    shipmodule_count = data->GetChildArrayLen("shipmodules");
    delete[] ship_modules;
    ship_modules = new ShipModuleClass[shipmodule_count];
    for(int i=0; i < shipmodule_count; i++) {
        DataNode* module_data = data->GetChildArrayElem("shipmodules", i);
        ship_modules[i].mass = module_data->GetF("mass") * KG_PER_COUNT;
        strcpy(ship_modules[i].name, module_data->Get("name"));
        strcpy(ship_modules[i].description, module_data->Get("description"));
        ship_modules[i].is_hidden = strcmp(module_data->Get("hidden", "n", true), "y") == 0;
        ship_modules[i].construction_time = module_data->GetI("construction_time", 20, !ship_modules[i].is_hidden);
        ship_modules[i].construction_batch_size = module_data->GetI("construction_batch_size", 1, true);

        module_data->FillBufferWithChild("add", ship_modules[i].delta_stats, ShipStats::MAX, ship_stat_names);
        module_data->FillBufferWithChild("require", ship_modules[i].required_stats, ShipStats::MAX, ship_stat_names);
        ship_modules[i].has_activation_requirements = module_data->HasChild("require");

        module_data->FillBufferWithChild("construction_resources", ship_modules[i].construction_resources, RESOURCE_MAX, resource_names);
        module_data->FillBufferWithChild("produce", ship_modules[i].production, RESOURCE_MAX, resource_names);
        if (module_data->GetArrayLen("icon_index") == 2) {
            ship_modules[i].icon_index = AtlasPos(
                module_data->GetArrayElemI("icon_index", 0),
                module_data->GetArrayElemI("icon_index", 1));
        } else {
            ship_modules[i].icon_index = AtlasPos(31, 31);
        }

        const char* string_id = module_data->Get("id", "_");
        RID rid = RID(i, EntityType::MODULE_CLASS);
        auto pair = shipmodule_ids.insert_or_assign(string_id, rid);
        ship_modules[i].id = pair.first->first.c_str();  // points to string in dictionary

        #define CHECK_FOR_ID(name) else if (strcmp(string_id, "shpmod_"#name) == 0) { expected_modules.name = rid; }
        if(false) {}
        CHECK_FOR_ID(small_yard_1)
        CHECK_FOR_ID(small_yard_2)
        CHECK_FOR_ID(small_yard_3)
        CHECK_FOR_ID(small_yard_4)
        CHECK_FOR_ID(heatshield)
        CHECK_FOR_ID(droptank)
        #undef CHECK_FOR_ID
    }
    return shipmodule_count;
}

RID ShipModules::GetModuleRIDFromStringId(const char* string_id) const {
    auto find = shipmodule_ids.find(string_id);
    if (find == shipmodule_ids.end()) {
        return GetInvalidId();
    }
    return find->second;
}

const ShipModuleClass* ShipModules::GetModuleByRID(RID index) const {
    if (ship_modules == NULL) {
        ERROR("Ship modules uninitialized")
        return NULL;
    }
    if (IdGetType(index) != EntityType::MODULE_CLASS) {
        return NULL;
    }
    if (IdGetIndex(index) >= shipmodule_count) {
        ERROR("Invalid ship module index (%d >= %d or negative)", index, shipmodule_count)
        return NULL;
    }
    return &ship_modules[IdGetIndex(index)];
}

void ShipModules::DrawShipModule(RID index) const {
    if(!IsIdValid(index)) {  // empty
        ui::EncloseEx(4, Palette::bg, Palette::ui_dark, 4);
    } else {  // filled
        const ShipModuleClass* smc = GetModuleByRID(index);
        ui::Enclose();
        ButtonStateFlags::T button_state = ui::AsButton();
        if (button_state & ButtonStateFlags::HOVER) {
            ui::SetMouseHint(smc->name);
        }
        if (button_state & ButtonStateFlags::PRESSED) {
            return;
        }
        ui::DrawIconSDF(smc->icon_index, Palette::ui_main, 40);
        //ui::Write(smc->name);
    }
}

void ShipModules::InitDragging(ShipModuleSlot slot, Rectangle current_draw_rect) {
    _dragging_origin = slot;
    _dragging = slot.GetSlot();
    slot.SetSlot(GetInvalidId());

    Vector2 draw_pos = {current_draw_rect.x, current_draw_rect.y};
    _dragging_mouse_offset = Vector2Subtract(draw_pos, GetMousePosition());
}

void ShipModules::DirectSwap(ShipModuleSlot slot) {
    ShipModuleSlot available = ShipModuleSlot();
    if (slot.type == ShipModuleSlot::DRAGGING_FROM_SHIP
        && IsIdValidTyped(GetGlobalState()->focused_planet, EntityType::PLANET)
    ) {
        const Planet* planet = GetPlanet(GetGlobalState()->focused_planet);
        available = planet->GetFreeModuleSlot();
    }
    if (slot.type == ShipModuleSlot::DRAGGING_FROM_PLANET
        && IsIdValidTyped(GetGlobalState()->focused_ship, EntityType::SHIP)
    ) {
        const Ship* ship = GetShip(GetGlobalState()->focused_ship);
        available = ship->GetFreeModuleSlot();
    }
    if (!available.IsValid()) {
        return;
    }
    if (!slot.IsReachable(available)) {
        return;
    }
    available.SetSlot(slot.GetSlot());
    slot.SetSlot(GetInvalidId());
}

void ShipModules::UpdateDragging() {
    if (!IsIdValid(_dragging)) {
        return;
    }
    Vector2 pos = Vector2Add(GetMousePosition(), _dragging_mouse_offset);
    ui::PushGlobal(
        pos.x, pos.y, SHIP_MODULE_WIDTH, SHIP_MODULE_HEIGHT, 
        DEFAULT_FONT_SIZE, Palette::ui_main, Palette::bg, 200
    );
    DrawShipModule(_dragging);
    ui::Pop();  // Global

    ShipModuleSlot release_slot = _dragging_origin;

    // TODO
    for (int planet_id = 0; planet_id < GetPlanets()->GetPlanetCount(); planet_id++) {
        release_slot.AssignIfValid(GetPlanetByIndex(planet_id)->current_slot);
    }
    for (auto it = GetShips()->alloc.GetIter(); it; it++) {
        release_slot.AssignIfValid(GetShip(it.GetId())->current_slot);
    }

    if (!_dragging_origin.IsReachable(release_slot)) {
        release_slot = _dragging_origin;
    }

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        _dragging_origin.SetSlot(release_slot.GetSlot());
        release_slot.SetSlot(_dragging);
        _dragging = GetInvalidId();
    }
}

int LoadShipModules(const DataNode* data) {
    return GetShipModules()->Load(data);
}

const ShipModuleClass* GetModule(RID id) {
    return GetShipModules()->GetModuleByRID(id);
}
