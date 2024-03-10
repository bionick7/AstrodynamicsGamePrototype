#include "ship_modules.hpp"
#include "global_state.hpp"
#include "ship.hpp"
#include "ui.hpp"
#include "constants.hpp"
#include "utils.hpp"
#include "debug_drawing.hpp"

bool ModuleConfiguration::IsAdjacent(ShipModuleSlot lhs, ShipModuleSlot rhs) const {
    return false;
}

ShipModuleClass::ShipModuleClass() {
    mass = 0.0;
    name[0] = '\0';
    description[0] = '\0';
    has_activation_requirements = false;

    for (int i=0; i < (int) ShipStats::MAX; i++) {
        delta_stats[i] = 0;
        required_stats[i] = 0;
    }

    for (int i=0; i < (int) resources::MAX; i++) {
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
    if (ship->IsParked()) {
        Planet* planet = GetPlanet(ship->GetParentPlanet());
        if (((1ull << IdGetIndex(planet->id)) & planets_restriction) == 0) {
            return;
        }
        for (int i=0; i < resources::MAX; i++) {
            if (planet->economy.resource_stock[i] < consumption[i]) {
                return;
            }
        }
        for (int i=0; i < resources::MAX; i++) {
            if (production[i] != 0) {
                planet->economy.AddResourceDelta((resources::T) i, production[i]);
            }
            if (consumption[i] != 0) {
                planet->economy.AddResourceDelta((resources::T) i, -consumption[i]);
            }
        }
    }
}

bool ShipModuleClass::HasDependencies() const {
    return has_activation_requirements;
}

int ShipModuleClass::GetConstructionTime() const {
    return construction_time;
}

void ShipModuleClass::MouseHintWrite(StringBuilder* sb) const {
    sb->Add(name).Add("\n");
    sb->Add(description).Add("\n");

    int consumptions_count = 0;
    int production_rsc_count = 0;
    StringBuilder sb2 = StringBuilder("    ");
    for (int i=0; i < resources::MAX; i++) {
        if (consumption[i] > 0) {
            if (consumptions_count != 0)
            sb2.Add(" + ");
            sb2.AddFormat("%3d %s", consumption[i], GetResourceUIRep(i));
            consumptions_count++;
        }
    }
    sb2.Add("\n => ");
    for (int i=0; i < resources::MAX; i++) {
        if (production[i] > 0) {
            if (production_rsc_count != 0)
            sb2.Add(" + ");
            sb2.AddFormat("%3d %s", production[i], GetResourceUIRep(i));
            production_rsc_count++;
        }
    }
    if (consumptions_count * production_rsc_count > 0) {
        sb->Add(sb2.c_str);
    }
}

ShipModuleSlot::ShipModuleSlot(RID p_entity, int p_index, ShipModuleSlotType p_origin_type, ModuleType::T p_module_type) {
    entity = p_entity;
    index = p_index;
    origin_type = p_origin_type;
    module_type = p_module_type;
}

void ShipModuleSlot::SetSlot(RID module_) const {
    if (origin_type == ShipModuleSlot::DRAGGING_FROM_PLANET) {
        GetPlanet(entity)->ship_module_inventory[index] = module_;
    } else if (origin_type == ShipModuleSlot::DRAGGING_FROM_SHIP) {
        GetShip(entity)->modules[index] = module_;
    }
}

RID ShipModuleSlot::GetSlot() const {
    if (origin_type == ShipModuleSlot::DRAGGING_FROM_PLANET) {
        return GetPlanet(entity)->ship_module_inventory[index];
    } else if (origin_type == ShipModuleSlot::DRAGGING_FROM_SHIP) {
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

bool ShipModuleSlot::IsReachable(ShipModuleSlot target) const {
    if (!IsValid() || !target.IsValid()) return false;
    RID own_planet = GetInvalidId();
    RID other_planet = GetInvalidId();
    if (origin_type == ShipModuleSlot::DRAGGING_FROM_PLANET) {
        own_planet = entity;
    } else if (origin_type == ShipModuleSlot::DRAGGING_FROM_SHIP) {
        own_planet = GetShip(entity)->GetParentPlanet();  // Might be invalid
        if (!GetShip(entity)->CanDragModule(index)) {
            return false;
        }
    }
    
    if (target.origin_type == ShipModuleSlot::DRAGGING_FROM_PLANET) {
        other_planet = target.entity;
    } else if (target.origin_type == ShipModuleSlot::DRAGGING_FROM_SHIP) {
        other_planet = GetShip(target.entity)->GetParentPlanet();  // Might be invalid
    }

    return own_planet == other_planet;
}

bool ShipModuleSlot::IsSlotFitting(RID module) const {
    if (!IsIdValidTyped(module, EntityType::MODULE_CLASS)) {
        return false;
    }
    const ShipModuleClass* smc = GetModule(module);
    //INFO("Fits %s => %s ? %s", ModuleType::names[smc->type], ModuleType::names[module_type], ModuleType::IsCompatible(smc->type, module_type) ? "y":"n")
    return ModuleType::IsCompatible(smc->type, module_type);
}

void ShipModuleSlot::Draw() const {
    //if (IsSlotFitting(GetShipModules()->_dragging)) {
    //    int enclose_dist = sin(GetRenderServer()->screen_time*5.) > 0 ? 4 : 7;
    //    ui::EncloseEx(-enclose_dist, BLANK, Palette::interactable_main, 5);
    //}
    ui::EncloseEx(0, Palette::bg, Palette::bg, 5);
    int x_cursor = ui::Current()->x_cursor;
    int y_cursor = ui::Current()->y_cursor;
    // Center
    ui::Current()->x_cursor = (ui::Current()->width - 40) / 2;
    ui::Current()->y_cursor = (ui::Current()->height - 40) / 2;
    ui::DrawIconSDF(ModuleType::icons[module_type], Palette::ui_dark, 40);
    ui::Current()->x_cursor = x_cursor;
    ui::Current()->y_cursor = y_cursor;

    RID dragging_object = GetShipModules()->_dragging;
    if (IsIdValid(dragging_object) && 
        (!IsSlotFitting(dragging_object) || !IsReachable(GetShipModules()->_dragging_origin))
    ){
        int center_x = ui::Current()->text_start_x + ui::Current()->width/2;
        int center_y = ui::Current()->text_start_y + ui::Current()->height/2;
        ui::BeginDirectDraw();
        DrawLine(center_x - 20, center_y - 20, center_x + 20, center_y + 20, Palette::red);
        DrawLine(center_x - 20, center_y + 20, center_x + 20, center_y - 20, Palette::red);
        ui::EndDirectDraw();
    }
}

ModuleType::T ModuleType::FromString(const char *name) {
    for(int i=0; i < MAX; i++) {
        if (strcmp(names[i], name) == 0) {
            return (ModuleType::T) i;
        }
    }
    return INVALID;
}

bool ModuleType::IsCompatible(ModuleType::T from, ModuleType::T to) {
    switch (to) {
        case LARGE:    return from == LARGE || from == MEDIUM || from == SMALL;
        case MEDIUM:   return from == MEDIUM || from == SMALL;
        case SMALL:    return from == SMALL;
        case FREE:     return from == FREE;
        case ARMOR:    return from == ARMOR;
        case DROPTANK: return from == DROPTANK;
        case ANY:      return from != INVALID;
        default: case INVALID: return false;
    }
    
    return false;
}

void ModuleConfiguration::Load(const DataNode *data, const char* ship_id) {
    module_count = data->GetChildArrayLen(ship_id);
    if (module_count > SHIP_MAX_MODULES) module_count = SHIP_MAX_MODULES;

    // To measure draw size
    Vector2 min_extend = Vector2Zero();
    Vector2 max_extend = Vector2Zero();

    for(int i=0; i < module_count; i++) {
        const DataNode* child = data->GetChildArrayElem(ship_id, i);
        types[i] = ModuleType::FromString(child->Get("type"));
        for(int j=0; j < MODULE_CONFIG_MAX_NEIGHBOURS; j++) {
            int neighbour = child->GetArrayElemI("neighbours", j, -1, true);
            if (neighbour >= module_count) {
                WARNING("Neighbour count exceeds module configuration size")
            } else {
                neighbours[i*MODULE_CONFIG_MAX_NEIGHBOURS + j] = neighbour;
            }
        }
        draw_offset[i].x = child->GetArrayElemF("offset", 0, 0.0);
        draw_offset[i].y = child->GetArrayElemF("offset", 1, 0.0);

        int pos_x = draw_offset[i].x;
        int pos_y = draw_offset[i].y;
        if (pos_x < min_extend.x) min_extend.x = pos_x;
        if (pos_y < min_extend.y) min_extend.y = pos_y;
        if (pos_x > max_extend.x) max_extend.x = pos_x;
        if (pos_y > max_extend.y) max_extend.y = pos_y;
    }

    StringBuilder sb;
    sb.Add("resources/meshes/ships/").Add(ship_id).Add(".obj");
    strcpy(mesh_resource_path, sb.c_str);
    WireframeMesh mesh = assets::GetWirframe(mesh_resource_path);
    if (IsWireframeReady(mesh)) {
        mesh_draw_space = {
            mesh.bounding_box.min.x * 100, mesh.bounding_box.min.z * 100,
            (mesh.bounding_box.max.x - mesh.bounding_box.min.x) * 100,
            (mesh.bounding_box.max.z - mesh.bounding_box.min.z) * 100,
        };
    } else {
        mesh_draw_space = {
            min_extend.x - SHIP_MODULE_WIDTH/2, min_extend.y - SHIP_MODULE_HEIGHT/2,
            max_extend.x - min_extend.x + SHIP_MODULE_WIDTH, max_extend.y - min_extend.y + SHIP_MODULE_HEIGHT,
        };
    }
}

Rectangle module_config_popup_rect = {0};
void ModuleConfiguration::Draw(Ship* ship) const {
    const int ADJUST_MARGIN = 10;

    ShipModules* sms = GetShipModules();

    int width = ui::Current()->width;
    int height = MinInt(width, mesh_draw_space.height) + 20;

    ui::PushInset(0, height);
    Rectangle bounding = ui::Current()->GetRect();
    Rectangle adjusted_draw_space = mesh_draw_space;

    WireframeMesh mesh = assets::GetWirframe(mesh_resource_path);
    float size_x = mesh.bounding_box.max.x - mesh.bounding_box.min.x;
    float size_y = mesh.bounding_box.max.z - mesh.bounding_box.min.z;
    Rectangle draw_rect = {
        mesh.bounding_box.min.x, mesh.bounding_box.min.z,
        size_x, size_y
    };

    Vector2 center = {
        bounding.x + bounding.width/2,
        bounding.y + bounding.height/2,
    };

    adjusted_draw_space.x += center.x;
    adjusted_draw_space.y += center.y;
    bool fits_naturally = CheckEnclosingRecs(bounding, adjusted_draw_space);
    bool show_popup = !fits_naturally && 
        (CheckCollisionPointRec(GetMousePosition(), bounding) || 
        CheckCollisionPointRec(GetMousePosition(), module_config_popup_rect));
    if (show_popup) {
        adjusted_draw_space.width += ADJUST_MARGIN * 2;
        adjusted_draw_space.height += ADJUST_MARGIN * 2;
        if (adjusted_draw_space.x > GetScreenWidth() - adjusted_draw_space.width - ADJUST_MARGIN * 2)
            adjusted_draw_space.x = GetScreenWidth() - adjusted_draw_space.width - ADJUST_MARGIN * 2;
        if (adjusted_draw_space.y > GetScreenHeight() - adjusted_draw_space.height - ADJUST_MARGIN * 2)
            adjusted_draw_space.y = GetScreenHeight() - adjusted_draw_space.height - ADJUST_MARGIN * 2;
        ui::PushGlobal(
            adjusted_draw_space.x, adjusted_draw_space.y,
            adjusted_draw_space.width, adjusted_draw_space.height,
            DEFAULT_FONT_SIZE, Palette::ui_main, Palette::bg, ui::Current()->z_layer + 10
        );
        center = {
            adjusted_draw_space.x + adjusted_draw_space.width/2,
            adjusted_draw_space.y + adjusted_draw_space.height/2,
        };
        module_config_popup_rect = adjusted_draw_space;

        ui::Enclose();
    } else {
        module_config_popup_rect = {0};
    }
    ui::WriteEx(GetShipClassByRID(ship->ship_class)->name, TextAlignment::HCENTER | TextAlignment::TOP, false);

    //DrawRectangleLinesEx(bounding, 1, WHITE);
    bool use_scissor = !CheckEnclosingRecs(ui::Current()->render_rec, bounding);
    if (use_scissor) {
        BeginScissorMode(ui::Current()->render_rec.x, ui::Current()->render_rec.y, ui::Current()->render_rec.width, ui::Current()->render_rec.height);
    }

    if (IsWireframeReady(mesh)) {
        Rectangle wf_draw_rect = adjusted_draw_space;
        if (show_popup) {
            wf_draw_rect.x += ADJUST_MARGIN;
            wf_draw_rect.y += ADJUST_MARGIN;
            wf_draw_rect.width -= ADJUST_MARGIN * 2;
            wf_draw_rect.height -= ADJUST_MARGIN * 2;
        }
        // Make the mesh fit (and adjust to keep the modules aligned)
        center.x -= (mesh.bounding_box.min.x + mesh.bounding_box.max.x) * 50;
        center.y += (mesh.bounding_box.min.z + mesh.bounding_box.max.z) * 50;
        RenderWirframeMesh2DEx(mesh, center, 100, Palette::ui_main, ui::Current()->z_layer);
    }
    BeginRenderInUILayer(ui::Current()->z_layer);
    //DrawRectangle(center.x + 5, center.y-25, 50, 50, WHITE);
    //DrawLine(center.x - 100, center.y +   0, center.x + 100, center.y +   0, WHITE);
    //DrawLine(center.x +   0, center.y - 100, center.x +   0, center.y + 100, WHITE);
    static Rectangle rectangles[SHIP_MAX_MODULES];
    for (int i=0; i < module_count; i++) {
        int center_x = center.x + draw_offset[i].x;
        int center_y = center.y - draw_offset[i].y;
        //DEBUG_SHOW_I(center_x)
        //DEBUG_SHOW_I(center_y)
        for(int j=0; j < MODULE_CONFIG_MAX_NEIGHBOURS; j++) {
            if (neighbours[i*MODULE_CONFIG_MAX_NEIGHBOURS + j] >= 0) {
                int neighbour = neighbours[i*MODULE_CONFIG_MAX_NEIGHBOURS + j];
                int other_center_x = center.x + draw_offset[neighbour].x;
                int other_center_y = center.y + draw_offset[neighbour].y;
                //INFO("%d(%d, %d) => %d(%d, %d)", i, center_x, center_y, neighbour, other_center_x, other_center_y)
                //DrawLine(center_x, center_y, other_center_x, other_center_y, Palette::ui_alt);
            }
        }
        Rectangle module_rect = { center_x - SHIP_MODULE_WIDTH/2, center_y - SHIP_MODULE_HEIGHT/2, SHIP_MODULE_WIDTH, SHIP_MODULE_HEIGHT };
        rectangles[i] = module_rect;
        //DrawRectangleRec(rectangles[i], WHITE);
        //DrawRectangleLinesEx(module_rect, 1, Palette::ui_alt);

        ButtonStateFlags::T button_state = GetButtonStateRec(module_rect);
        if (button_state & ButtonStateFlags::HOVER) {
            ship->current_slot = ShipModuleSlot(ship->id, i, ShipModuleSlot::DRAGGING_FROM_SHIP, types[i]);
        }
        if (button_state & ButtonStateFlags::JUST_PRESSED) {
            if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
                sms->DirectSwap(ship->current_slot);
            } else {
                sms->InitDragging(ship->current_slot, module_rect);
            }
        }
    }
    if (use_scissor) {
        EndScissorMode();
    }
    EndRenderInUILayer();
    for (int i=0; i < module_count; i++) {
        // leave margin, so the rect boundaries from the mesh remain visible
        ui::PushFree(rectangles[i].x, rectangles[i].y, rectangles[i].width, rectangles[i].height);
        ui::Shrink(3, 3);
        ShipModuleSlot(ship->id, i, ShipModuleSlot::DRAGGING_FROM_SHIP, types[i]).Draw();
        sms->DrawShipModule(ship->modules[i]);
        ui::Pop();
    }
    if (show_popup) {
        ui::Pop();
    }

    ui::HelperText(GetUI()->GetConceptDescription("module"));
    ui::Pop(); // Inset
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
        if (module_data->HasArray("planets_restriction")) {
            int planets_restriction_size = module_data->GetArrayLen("planets_restriction");
            ship_modules[i].planets_restriction = 0;
            for(int j=0; j < planets_restriction_size; j++) {
                int index = IdGetIndex(GetPlanets()->GetIdByName(module_data->GetArrayElem("planets_restriction", j)));
                ship_modules[i].planets_restriction |= 1ull << index;
            }
            //INFO("%lX, %s", (long int)ship_modules[i].planets_restriction, ship_modules[i].name)
        } else {
            ship_modules[i].planets_restriction = UINT64_MAX;
        }

        module_data->DeserializeBuffer("add", ship_modules[i].delta_stats, ship_stat_names, ShipStats::MAX);
        module_data->DeserializeBuffer("require", ship_modules[i].required_stats, ship_stat_names, ShipStats::MAX);
        ship_modules[i].has_activation_requirements = module_data->HasChild("require");
        ship_modules[i].type = ModuleType::FromString(module_data->Get("type"));

        module_data->DeserializeBuffer("construction_resources", ship_modules[i].construction_resources, resources::names, resources::MAX);
        module_data->DeserializeBuffer("produce", ship_modules[i].production, resources::names, resources::MAX);
        module_data->DeserializeBuffer("consume", ship_modules[i].consumption, resources::names, resources::MAX);
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
        return;
        //ui::EncloseEx(4, Palette::bg, Palette::ui_dark, 4);
    } else {  // filled
        const ShipModuleClass* smc = GetModuleByRID(index);
        //ui::Enclose();
        ButtonStateFlags::T button_state = ui::AsButton();
        if (button_state & ButtonStateFlags::HOVER) {
            StringBuilder sb;
            smc->MouseHintWrite(&sb);
            sb.AutoBreak(150);
            ui::SetMouseHint(sb.c_str);
        }
        /*if (button_state & ButtonStateFlags::PRESSED) {
            return;
        }*/
        ui::VSpace((ui::Current()->height - 40) / 2);
        ui::HSpace((ui::Current()->width - 40) / 2);
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
    if (slot.origin_type == ShipModuleSlot::DRAGGING_FROM_SHIP
        && IsIdValidTyped(GetGlobalState()->focused_planet, EntityType::PLANET)
    ) {
        const Planet* planet = GetPlanet(GetGlobalState()->focused_planet);
        available = planet->GetFreeModuleSlot();
    }
    if (slot.origin_type == ShipModuleSlot::DRAGGING_FROM_PLANET
        && IsIdValidTyped(GetGlobalState()->focused_ship, EntityType::SHIP)
    ) {
        const Ship* ship = GetShip(GetGlobalState()->focused_ship);
        ModuleType::T search_type = GetModule(slot.GetSlot())->type;
        available = ship->GetFreeModuleSlot(search_type);
    }
    if (!available.IsValid()) {
        return;
    }
    if (!slot.IsReachable(available) || !available.IsSlotFitting(slot.GetSlot())) {
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

    if (!_dragging_origin.IsReachable(release_slot) || !release_slot.IsSlotFitting(_dragging)) {
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
