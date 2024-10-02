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
    name = PermaString();
    description = PermaString();
    has_stat_dependencies = false;

    for (int i=0; i < (int) ship_stats::MAX; i++) {
        delta_stats[i] = 0;
        required_stats[i] = 0;
    }

    for (int i=0; i < (int) resources::MAX; i++) {
        production[i] = 0;
        construction_resources[i] = 0;
    }
}

bool ShipModuleClass::IsPlanetRestricted(RID planet) const {
    return ((1ull << IdGetIndex(planet)) & planets_restriction) == 0;
}

void ShipModuleClass::UpdateCustom(Ship* ship) const {
    bool new_day = GetCalendar()->IsNewDay();
    if (ship->IsParked()) {
        Planet* planet = GetPlanet(ship->GetParentPlanet());
        if (IsPlanetRestricted(planet->id)) {
            return;
        }
        for (int i=0; i < resources::MAX; i++) {
            if (production[i] < -planet->economy.resource_stock[i]) {
                return;
            }
        }
        for (int i=0; i < resources::MAX; i++) {
            if (production[i] != 0) {
                planet->economy.AddResourceDelta((resources::T) i, production[i]);
            }
        }
        planet->module_opinion_delta += opinion_delta;
        planet->module_independence_delta += independence_delta;
    }
}

bool ShipModuleClass::HasStatDependencies() const {
    return has_stat_dependencies;
}

int ShipModuleClass::GetConstructionTime() const {
    return construction_time;
}

void ShipModuleClass::MouseHintWrite(StringBuilder* sb) const {
    sb->AddPerma(name).Add(" ").Add(module_types::str_icons[type]).Add("\n");
    sb->AddPerma(description).Add("\n");

    int consumptions_count = 0;
    int production_rsc_count = 0;
    StringBuilder sb2 = StringBuilder("    ");
    for (int i=0; i < resources::MAX; i++) {
        if (production[i] < 0) {
            if (consumptions_count != 0)
            sb2.Add(" + ");
            sb2.AddFormat("%3d %s", -production[i], GetResourceUIRep(i));
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
    if (consumptions_count + production_rsc_count > 0) {
        sb->Add(sb2.c_str).Add("\n");
    }

    sb2.Clear();
    int delta_stats_num = 0;
    for (int i=0; i < ship_stats::MAX; i++) {
        if (delta_stats[i] == 1) {
            sb2.AddFormat(" %s", ship_stats::icons[i]);
            delta_stats_num++;
        } else if (delta_stats[i] > 1) {
            sb2.AddFormat(" %dx%s", delta_stats[i], ship_stats::icons[i]);
            delta_stats_num++;
        }
    }
    if (delta_stats_num > 0) {
        sb->Add(" Adds").Add(sb2.c_str).Add("\n");
    }
    if (independence_delta != 0) 
        sb->AddFormat("Independence %+3d\n", independence_delta);
    if (opinion_delta != 0)
        sb->AddFormat("Opinion %+3d\n", opinion_delta);

    sb2.Clear();
    int req_stats_num = 0;
    for (int i=0; i < ship_stats::MAX; i++) {
        if (required_stats[i] == 1) {
            sb2.AddFormat(" %s", ship_stats::icons[i]);
            req_stats_num++;
        } else if (required_stats[i] > 1) {
            sb2.AddFormat(" %dx%s", required_stats[i], ship_stats::icons[i]);
            req_stats_num++;
        }
    }
    if (req_stats_num > 0) {
        sb->Add(" Needs ").Add(sb2.c_str).Add("\n");
    }
}

ShipModuleSlot::ShipModuleSlot(RID p_entity, int p_index, ShipModuleSlotType p_origin_type, module_types::T p_module_type) {
    entity = p_entity;
    index = p_index;
    origin_type = p_origin_type;
    module_type = p_module_type;
}

void ShipModuleSlot::SetSlot(RID module_) const {
    if (origin_type == ShipModuleSlot::DRAGGING_FROM_PLANET) {
        GetPlanet(entity)->inventory[index] = module_;
    } else if (origin_type == ShipModuleSlot::DRAGGING_FROM_SHIP) {
        GetShip(entity)->modules[index] = module_;
    }
}

RID ShipModuleSlot::GetSlot() const {
    if (origin_type == ShipModuleSlot::DRAGGING_FROM_PLANET) {
        return GetPlanet(entity)->inventory[index];
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
    //INFO("Fits %s => %s ? %s", module_types::names[smc->type], module_types::names[module_type], module_types::IsCompatible(smc->type, module_type) ? "y":"n")
    return module_types::IsCompatible(smc->type, module_type);
}

bool ShipModuleSlot::IsPlayerAccessible() const {
    if (IsIdValidTyped(entity, EntityType::PLANET)) {
        return GetPlanet(entity)->allegiance == GetFactions()->player_faction;
    }
    if (IsIdValidTyped(entity, EntityType::SHIP)) {
        return GetShip(entity)->IsPlayerControlled();
    }
    return false;
}

void ShipModuleSlot::Draw() const {
    //if (IsSlotFitting(GetShipModules()->_dragging)) {
    //    int enclose_dist = sin(GetRenderServer()->animation_time*5.) > 0 ? 4 : 7;
    //    ui::EncloseEx(-enclose_dist, BLANK, Palette::interactable_main, 5);
    //}
    ui::EncloseEx(0, Palette::bg, Palette::bg, 5);
    int x_cursor = ui::Current()->x_cursor;
    int y_cursor = ui::Current()->y_cursor;
    
    ui::DrawIcon(module_types::icons[module_type], text_alignment::CENTER, Palette::ui_dark, 40);
    ui::Current()->x_cursor = x_cursor;
    ui::Current()->y_cursor = y_cursor;

    RID dragging_object = GetShipModules()->_dragging;
    if (IsIdValid(dragging_object) && 
        (!IsSlotFitting(dragging_object) || !IsReachable(GetShipModules()->_dragging_origin))
    ){
        int center_x = ui::Current()->x + ui::Current()->width/2;
        int center_y = ui::Current()->y + ui::Current()->height/2;
        ui::BeginDirectDraw();
        DrawLine(center_x - 20, center_y - 20, center_x + 20, center_y + 20, Palette::red);
        DrawLine(center_x - 20, center_y + 20, center_x + 20, center_y - 20, Palette::red);
        ui::EndDirectDraw();
    }
}

module_types::T module_types::FromString(const char *name) {
    for(int i=0; i < MAX; i++) {
        if (strcmp(names[i], name) == 0) {
            return (module_types::T) i;
        }
    }
    return INVALID;
}

bool module_types::IsCompatible(module_types::T from, module_types::T to) {
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
        types[i] = module_types::FromString(child->Get("type"));
        for(int j=0; j < MODULE_CONFIG_MAX_NEIGHBORS; j++) {
            int neighbour = child->GetArrayElemI("neighbours", j, -1, true);
            if (neighbour >= module_count) {
                WARNING("Neighbour count exceeds module configuration size")
            } else {
                neighbors[i*MODULE_CONFIG_MAX_NEIGHBORS + j] = neighbour;
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
    WireframeMesh mesh = assets::GetWireframe(mesh_resource_path);
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
void ModuleConfiguration::Draw(Ship* ship, Vector2 anchor_point, text_alignment::T alignment) const {
    const int ADJUST_MARGIN = 10;

    ShipModules* sms = GetShipModules();

    //int width = ui::Current()->width;

    int upper_border = 20;
    int lower_border = SHIP_MODULE_HEIGHT + 10;
    //Rectangle bounding = ui::Current()->GetRect();

    WireframeMesh mesh = assets::GetWireframe(mesh_resource_path);
    float size_x = mesh.bounding_box.max.x - mesh.bounding_box.min.x;
    float size_y = mesh.bounding_box.max.z - mesh.bounding_box.min.z;
    Rectangle adjusted_draw_space = {
        mesh.bounding_box.min.x*100, mesh.bounding_box.min.z*100,
        size_x*100, size_y*100
    };
    adjusted_draw_space.height += upper_border + lower_border;
    int height = adjusted_draw_space.height;

    //Vector2 center = {
    //    bounding.x + bounding.width/2,
    //    bounding.y + bounding.height/2,
    //};

    // Adjust drawing box on mousehover

    //bool fits_naturally = CheckEnclosingRecs(bounding, adjusted_draw_space);
    //bool show_popup = !fits_naturally && 
    //    (CheckCollisionPointRec(GetMousePosition(), bounding) || 
    //    CheckCollisionPointRec(GetMousePosition(), module_config_popup_rect));
    //if (show_popup) {
    adjusted_draw_space.width += ADJUST_MARGIN * 2;
    adjusted_draw_space.height += ADJUST_MARGIN * 2;
    if (adjusted_draw_space.x > GetScreenWidth() - adjusted_draw_space.width - ADJUST_MARGIN * 2)
        adjusted_draw_space.x = GetScreenWidth() - adjusted_draw_space.width - ADJUST_MARGIN * 2;
    if (adjusted_draw_space.y > GetScreenHeight() - adjusted_draw_space.height - ADJUST_MARGIN * 2)
        adjusted_draw_space.y = GetScreenHeight() - adjusted_draw_space.height - ADJUST_MARGIN * 2;

    
    Vector2 pos = ApplyAlignment(anchor_point, {adjusted_draw_space.width, adjusted_draw_space.height}, alignment);
    adjusted_draw_space.x = pos.x;
    adjusted_draw_space.y = pos.y;

    ui::CreateNew(
        adjusted_draw_space.x, adjusted_draw_space.y,
        adjusted_draw_space.width, adjusted_draw_space.height,
        DEFAULT_FONT_SIZE, Palette::ui_main, Palette::bg, z_layers::BASE
    );
    module_config_popup_rect = adjusted_draw_space;

    ui::Enclose();

    //} else {
    //    module_config_popup_rect = {0};
    //}
    Vector2 center = {
        adjusted_draw_space.x + adjusted_draw_space.width/2,
        adjusted_draw_space.y + adjusted_draw_space.height/2,
    };
    ui::WriteEx(GetShipClassByRID(ship->ship_class)->name.GetChar(), text_alignment::HCENTER | text_alignment::TOP, false);

    //bool use_scissor = !CheckEnclosingRecs(ui::Current()->render_rec, bounding);
    //if (use_scissor) {
    //    BeginScissorMode(ui::Current()->render_rec.x, ui::Current()->render_rec.y, ui::Current()->render_rec.width, ui::Current()->render_rec.height);
    //}

    // Draw mesh

    if (IsWireframeReady(mesh)) {
        Rectangle wf_draw_rect = adjusted_draw_space;
        //if (show_popup) {
        wf_draw_rect.x += ADJUST_MARGIN;
        wf_draw_rect.y += ADJUST_MARGIN;
        wf_draw_rect.width -= ADJUST_MARGIN * 2;
        wf_draw_rect.height -= ADJUST_MARGIN * 2;
        //}
        // Make the mesh fit (and adjust to keep the modules aligned)
        center.x -= (mesh.bounding_box.min.x + mesh.bounding_box.max.x) * 50;
        center.y -= (mesh.bounding_box.min.z + mesh.bounding_box.max.z) * 50;
        center.y -= upper_border;
        RenderWireframeMesh2DEx(mesh, center, 100, Palette::bg, Palette::ui_main, ui::Current()->z_layer);
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
        for(int j=0; j < MODULE_CONFIG_MAX_NEIGHBORS; j++) {
            if (neighbors[i*MODULE_CONFIG_MAX_NEIGHBORS + j] >= 0) {
                int neighbour = neighbors[i*MODULE_CONFIG_MAX_NEIGHBORS + j];
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

        button_state_flags::T button_state = GetButtonStateRec(module_rect);
        if (button_state & button_state_flags::HOVER) {
            ship->current_slot = ShipModuleSlot(ship->id, i, ShipModuleSlot::DRAGGING_FROM_SHIP, types[i]);
        }
        if (button_state & button_state_flags::JUST_PRESSED) {
            if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
                sms->DirectSwap(ship->current_slot);
            } else {
                sms->InitDragging(ship->current_slot, module_rect);
            }
        }
    }

    // Draw free modules

    int first_free_module_slot = -1;
    for(int i=module_count; i < SHIP_MAX_MODULES; i++) {
        if (!IsIdValid(ship->modules[i])) {
            first_free_module_slot = i;
            break;
        }
    }

    int free_module_index = 0;
    for(int i=module_count; i < SHIP_MAX_MODULES; i++) {
        if (!IsIdValid(ship->modules[i]) && i != first_free_module_slot) {
            continue;
        }
        Rectangle module_rect = { 
            adjusted_draw_space.x + free_module_index * (SHIP_MODULE_WIDTH + 10) + 5, 
            adjusted_draw_space.y + adjusted_draw_space.height - SHIP_MODULE_HEIGHT - 5, 
            SHIP_MODULE_WIDTH, SHIP_MODULE_HEIGHT 
        };

        // Logic
        button_state_flags::T button_state = GetButtonStateRec(module_rect);
        if (button_state & button_state_flags::HOVER) {
            ship->current_slot = ShipModuleSlot(ship->id, i, ShipModuleSlot::DRAGGING_FROM_SHIP, module_types::FREE);
        }
        if (button_state & button_state_flags::JUST_PRESSED) {
            if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) sms->DirectSwap(ship->current_slot);
            else sms->InitDragging(ship->current_slot, module_rect);
        }
        
        // Drawing
        ui::PushFree(module_rect.x, module_rect.y, module_rect.width, module_rect.height);
        ui::Shrink(3, 3);
        ShipModuleSlot(ship->id, i, ShipModuleSlot::DRAGGING_FROM_SHIP, module_types::FREE).Draw();

        if (IsIdValid(ship->modules[i])) {
            const ShipModuleClass* smc = GetModule(ship->modules[i]);
            sms->DrawShipModule(ship->modules[i], !ship->modules_enabled[i]);
        } else {
            sms->DrawShipModule(ship->modules[i], false);
        }
        ui::Pop();

        free_module_index++;
    }

    //if (use_scissor) {
    //    EndScissorMode();
    //}
    EndRenderInUILayer();
    //BeginRenderInUILayer(50);
    //DrawRectangleLinesEx(adjusted_draw_space, 1, WHITE);
    //DrawRectangleLinesEx(bounding, 1, RED);
    //DrawLine(
    //    adjusted_draw_space.x, adjusted_draw_space.y,
    //    adjusted_draw_space.x+adjusted_draw_space.width, adjusted_draw_space.y+adjusted_draw_space.height, WHITE);
    //DrawLine(
    //    adjusted_draw_space.x+adjusted_draw_space.width, adjusted_draw_space.y, 
    //    adjusted_draw_space.x, adjusted_draw_space.y+adjusted_draw_space.height, WHITE);
    //EndRenderInUILayer();
    for (int i=0; i < module_count; i++) {
        // leave margin, so the rect boundaries from the mesh remain visible
        ui::PushFree(rectangles[i].x, rectangles[i].y, rectangles[i].width, rectangles[i].height);
        ui::Shrink(3, 3);
        ShipModuleSlot(ship->id, i, ShipModuleSlot::DRAGGING_FROM_SHIP, types[i]).Draw();
        if (IsIdValid(ship->modules[i])) {
            const ShipModuleClass* smc = GetModule(ship->modules[i]);
            sms->DrawShipModule(ship->modules[i], !ship->modules_enabled[i]);
        } else {
            sms->DrawShipModule(ship->modules[i], false);
        }
        ui::Pop();
    }
    ui::HelperText(GetUI()->GetConceptDescription("module"));
    //if (show_popup) {
    ui::Pop();
    //}
    //ui::Pop(); // Inset
}

int ShipModules::Load(const DataNode* data) {
    shipmodule_count = data->GetChildArrayLen("shipmodules");
    delete[] ship_modules;
    ship_modules = new ShipModuleClass[shipmodule_count];
    for(int i=0; i < shipmodule_count; i++) {
        DataNode* module_data = data->GetChildArrayElem("shipmodules", i);
        ship_modules[i].mass = module_data->GetF("mass") * KG_PER_COUNT;
        ship_modules[i].name = PermaString(module_data->Get("name"));
        ship_modules[i].description = PermaString(module_data->Get("description"));
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
            //INFO("%lX, %s", (long int)ship_modules[i].planets_restriction, ship_modules[i].name.GetChar())
        } else {
            ship_modules[i].planets_restriction = UINT64_MAX;
        }

        if (module_data->HasChild("construction_requirements")) {
            module_data->DeserializeBuffer("construction_requirements", ship_modules[i].construction_requirements, ship_stats::names, ship_stats::MAX);
            //module_data->GetChild("construction_requirements")->Inspect();
            //SHOW_I(ship_modules[i].construction_requirements[ship_stats::INDUSTRIAL_DOCK])
            //SHOW_I(ship_modules[i].construction_requirements[ship_stats::INDUSTRIAL_ADMIN])
        } else {  // Set default requirements
            for (int j=0; j < ship_stats::MAX; j++) {
                ship_modules[i].construction_requirements[j] = 0;
            }
            ///ship_modules[i].construction_requirements[ship_stats::INDUSTRIAL_ADMIN] = 1;
            ship_modules[i].construction_requirements[ship_stats::INDUSTRIAL_MANUFACTURING] = 1;
        }
        module_data->DeserializeBuffer("add", ship_modules[i].delta_stats, ship_stats::names, ship_stats::MAX);
        module_data->DeserializeBuffer("require", ship_modules[i].required_stats, ship_stats::names, ship_stats::MAX);
        // Add negative stats to requirements
        for (int j=0; j < ship_stats::MAX; j++) {
            if (ship_modules[i].delta_stats[j] < 0) {
                ship_modules[i].required_stats[j] = -ship_modules[i].delta_stats[j];
                ship_modules[i].has_stat_dependencies = true;
            }
        }
        if (module_data->HasChild("add")) {
            ship_modules[i].independence_delta = module_data->GetChild("add")->GetI("independence", 0, true);
            ship_modules[i].opinion_delta = module_data->GetChild("add")->GetI("opinion", 0, true);
        } else {
            ship_modules[i].independence_delta = 0;
            ship_modules[i].opinion_delta = 0;
        }
        
        ship_modules[i].type = module_types::FromString(module_data->Get("type"));

        module_data->DeserializeBuffer("construction_resources", ship_modules[i].construction_resources, resources::names, resources::MAX);
        module_data->DeserializeBuffer("produce", ship_modules[i].production, resources::names, resources::MAX);
        //module_data->DeserializeBuffer("consume", ship_modules[i].consumption, resources::names, resources::MAX);
        if (module_data->GetArrayLen("icon_index") == 2) {
            ship_modules[i].icon_index = AtlasPos(
                module_data->GetArrayElemI("icon_index", 0),
                module_data->GetArrayElemI("icon_index", 1));
        } else {
            ship_modules[i].icon_index = AtlasPos(15, 31);
        }

        const char* string_id = module_data->Get("id", "_");
        RID rid = RID(i, EntityType::MODULE_CLASS);
        ship_modules[i].id = GetGlobalState()->AddStringIdentifier(string_id, rid);

        #define CHECK_FOR_ID(name) else if (strcmp(string_id, "shpmod_"#name) == 0) { expected_modules.name = rid; }
        if(false) {}
        //CHECK_FOR_ID(small_yard_1)
        //CHECK_FOR_ID(small_yard_2)
        //CHECK_FOR_ID(small_yard_3)
        //CHECK_FOR_ID(small_yard_4)
        CHECK_FOR_ID(heatshield)
        CHECK_FOR_ID(droptank_water)
        CHECK_FOR_ID(droptank_hydrogen)
        //CHECK_FOR_ID(shpmod_research_lab)
        #undef CHECK_FOR_ID
    }
    return shipmodule_count;
}

const ShipModuleClass* ShipModules::GetModuleByRID(RID index) const {
    if (ship_modules == NULL) {
        FAIL("Ship modules uninitialized")
        return NULL;
    }
    if (IdGetType(index) != EntityType::MODULE_CLASS) {
        FAIL("Invalid ship module id (%d)", index)
        return NULL;
    }
    if (IdGetIndex(index) >= shipmodule_count) {
        FAIL("Invalid ship module index (%d >= %d or negative)", index, shipmodule_count)
        return NULL;
    }
    return &ship_modules[IdGetIndex(index)];
}

void ShipModules::DrawShipModule(RID index, bool inactive) const {
    if(!IsIdValidTyped(index, EntityType::MODULE_CLASS)) {  // empty
        return;
        //ui::EncloseEx(4, Palette::bg, Palette::ui_dark, 4);
    } else {  // filled
        const ShipModuleClass* smc = GetModuleByRID(index);
        //ui::Enclose();
        button_state_flags::T button_state = ui::AsButton();
        if (button_state & button_state_flags::HOVER) {
            StringBuilder sb;
            smc->MouseHintWrite(&sb);
            sb.AutoBreak(50);
            ui::SetMouseHint(sb.c_str);
        }
        /*if (button_state & ButtonStateFlags::PRESSED) {
            return;
        }*/
        if (inactive) {
            ui::DrawIcon(smc->icon_index, text_alignment::CENTER, Palette::red, 40);
        } else {
            ui::DrawIcon(smc->icon_index, text_alignment::CENTER, Palette::ui_main, 40);
        }
        //ui::Write(smc->name);
    }
}

void ShipModules::InitDragging(ShipModuleSlot slot, Rectangle current_draw_rect) {
    if (!slot.IsPlayerAccessible()) {
        return;
    }
    _dragging_origin = slot;
    _dragging = slot.GetSlot();
    slot.SetSlot(GetInvalidId());

    Vector2 draw_pos = {current_draw_rect.x, current_draw_rect.y};
    _dragging_mouse_offset = Vector2Subtract(draw_pos, GetMousePosition());
}

void ShipModules::DirectSwap(ShipModuleSlot slot) {
    if (!IsIdValid(slot.GetSlot())) {
        return;
    }
    if (!slot.IsPlayerAccessible()) {
        return;
    }
    ShipModuleSlot available = ShipModuleSlot();
    if (slot.origin_type == ShipModuleSlot::DRAGGING_FROM_SHIP
        && IsIdValidTyped(GetGlobalState()->focused_planet, EntityType::PLANET)
    ) {
        const Planet* planet = GetPlanet(GetGlobalState()->focused_planet);
        module_types::T search_type = GetModule(slot.GetSlot())->type;
        available = planet->GetFreeModuleSlot(search_type);
    }
    if (slot.origin_type == ShipModuleSlot::DRAGGING_FROM_PLANET
        && IsIdValidTyped(GetGlobalState()->focused_ship, EntityType::SHIP)
    ) {
        const Ship* ship = GetShip(GetGlobalState()->focused_ship);
        module_types::T search_type = GetModule(slot.GetSlot())->type;
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
    DrawShipModule(_dragging, false);
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

bool ShipModules::IsDropTank(RID module, resources::T rsc) const {
    if ((rsc == resources::WATER    || rsc == resources::NONE) && module == expected_modules.droptank_water) return true;
    if ((rsc == resources::HYDROGEN || rsc == resources::NONE) && module == expected_modules.droptank_hydrogen) return true;
    return false;
}


RID DeserializeModuleInfo(const char* module_info, int* index) {
    static char identifier[100];
    
    if ('0' <= module_info[0] && module_info[0] <= '9') {
        int scan_res = sscanf(module_info, "%d %s", index, identifier);
        if (scan_res != 2) {
            ERROR("Could not parse ship module id '%s'", module_info)
            return GetInvalidId();
        }
    } else {
        strcpy(identifier, module_info);
        *index = -1;
    }

    RID module_rid = GetGlobalState()->GetFromStringIdentifier(identifier);
    if (!IsIdValidTyped(module_rid, EntityType::MODULE_CLASS)) {
        ERROR("No valid module class '%s'", identifier)
        return GetInvalidId();
    }
    
    return module_rid;
}

int LoadShipModules(const DataNode* data) {
    return GetShipModules()->Load(data);
}

const ShipModuleClass* GetModule(RID id) {
    return GetShipModules()->GetModuleByRID(id);
}
