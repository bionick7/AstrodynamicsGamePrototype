#include "ship_modules.hpp"
#include "global_state.hpp"
#include "ship.hpp"
#include "ui.hpp"
#include "constants.hpp"

ShipModuleClass::ShipModuleClass() {
    mass = 0.0;
    name[0] = '\0';
    description[0] = '\0';
}

void ShipModuleClass::Update(Ship* ship) const {
    switch (module_type) {
    case WATER_EXTRACTOR:{
        if (!ship->is_parked) break;
        if (!GlobalGetState()->calendar.IsNewDay()) break;
        Planet* planet = GetPlanet(ship->parent_planet);
        planet->economy.GiveResource(ResourceTransfer(RESOURCE_WATER, 1));
        break;}
    case HEAT_SHIELD:{
        NOT_IMPLEMENTED
        break;}
    default:
    case INVALID_MODULE:
        break;
    }
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
    if (IsIdValid(other.entity) && other.index >= 0) {
        *this = other;
    }
}

bool ShipModuleSlot::IsReachable(ShipModuleSlot other) {
    RID own_planet = GetInvalidId();
    RID other_planet = GetInvalidId();
    if (type == ShipModuleSlot::DRAGGING_FROM_PLANET) {
        own_planet = entity;
    } else if (type == ShipModuleSlot::DRAGGING_FROM_SHIP) {
        own_planet = GetShip(entity)->parent_planet;  // Might be invalid
    }
    
    if (other.type == ShipModuleSlot::DRAGGING_FROM_PLANET) {
        other_planet = other.entity;
    } else if (other.type == ShipModuleSlot::DRAGGING_FROM_SHIP) {
        other_planet = GetShip(other.entity)->parent_planet;  // Might be invalid
    }

    return own_planet == other_planet;
}

int ShipModules::Load(const DataNode* data) {
    shipmodule_count = data->GetArrayChildLen("shipmodules");
    delete[] ship_modules;
    ship_modules = new ShipModuleClass[shipmodule_count];
    for(int i=0; i < shipmodule_count; i++) {
        DataNode* module_data = data->GetArrayChild("shipmodules", i);
        ship_modules[i].mass = module_data->GetF("mass");
        strcpy(ship_modules[i].name, module_data->Get("name"));
        strcpy(ship_modules[i].description, module_data->Get("description"));

        const char* id = module_data->Get("id", "_");
        if (strcmp(id, "shpmod_water_extractor") == 0) 
            ship_modules[i].module_type = ShipModuleClass::WATER_EXTRACTOR;
        if (strcmp(id, "shpmod_heatshield") == 0) 
            ship_modules[i].module_type = ShipModuleClass::HEAT_SHIELD;

        auto pair = shipmodule_ids.insert_or_assign(id, RID(i, EntityType::MODULE));
        ship_modules[i].id = pair.first->first.c_str();  // points to string in dictionary
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
    if (IdGetType(index) != EntityType::MODULE) {
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
        UIContextEnclose(Palette::bg, Palette::ui_dark);
        if (UIContextAsButton() & BUTTON_STATE_FLAG_JUST_PRESSED) {
            // Equipment menu
            return;
        }
    } else {  // filled
        const ShipModuleClass* smc = GetModuleByRID(index);
        UIContextEnclose(Palette::bg, Palette::ui_main);
        ButtonStateFlags button_state = UIContextAsButton();
        if (button_state & BUTTON_STATE_FLAG_HOVER) {
            UISetMouseHint(smc->name);
        }
        if (button_state & BUTTON_STATE_FLAG_PRESSED) {
            return;
        }
        UIContextWrite(smc->name);
    }
}

void ShipModules::InitDragging(ShipModuleSlot slot, Rectangle current_draw_rect) {
    _dragging_origin = slot;
    _dragging = slot.GetSlot();
    slot.SetSlot(GetInvalidId());

    Vector2 draw_pos = {current_draw_rect.x, current_draw_rect.y};
    _dragging_mouse_offset = Vector2Subtract(draw_pos, GetMousePosition());

}

void ShipModules::UpdateDragging() {
    if (!IsIdValid(_dragging)) {
        return;
    }
    Vector2 pos = Vector2Add(GetMousePosition(), _dragging_mouse_offset);
    UIContextPushGlobal(pos.x, pos.y, SHIP_MODULE_WIDTH, SHIP_MODULE_HEIGHT, 16, Palette::ui_main);
    DrawShipModule(_dragging);
    UIContextPop();  // Global

    ShipModuleSlot release_slot = _dragging_origin;

    // TODO
    for (int planet_id = 0; planet_id < GlobalGetState()->planets.GetPlanetCount(); planet_id++) {
        release_slot.AssignIfValid(GetPlanetByIndex(planet_id)->current_slot);
    }
    for (auto it = GlobalGetState()->ships.alloc.GetIter(); it.IsIterGoing(); it++) {
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
    return GlobalGetState()->ship_modules.Load(data);
}

const ShipModuleClass* GetModule(RID id) {
    return GlobalGetState()->ship_modules.GetModuleByRID(id);
}
