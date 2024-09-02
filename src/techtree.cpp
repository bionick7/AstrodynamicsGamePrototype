#include "techtree.hpp"
#include "global_state.hpp"
#include "logging.hpp"
#include "debug_drawing.hpp"
#include "debug_console.hpp"

void TechTree::Serialize(DataNode *data) const {
    data->SerializeBuffer("techtree_status", node_progress, node_names_ptrs, nodes_count, false);
    if (research_focus < 0) {
        data->Set("tech_focus", "none");
    } else {
        data->Set("tech_focus", nodes[research_focus].str_id);
    }
}

void TechTree::Deserialize(const DataNode *data) {
    data->DeserializeBuffer("techtree_status", node_progress, node_names_ptrs, nodes_count);
    for(int i=0; i < nodes_count; i++) {
        if (node_progress[i] > nodes[i].research_effort) 
            node_progress[i] = nodes[i].research_effort;
        //INFO("%s: %d", node_names_ptrs[i], node_progress[i])
    }
    RID research_focus_id = GetGlobalState()->GetFromStringIdentifier(data->Get("tech_focus"));
    research_focus = IsIdValid(research_focus_id) ? IdGetIndex(research_focus_id) : -1;
}

int _SetNodeLayer(TechTreeNode* nodes, int index) {
    if (nodes[index].layer >= 0) {
        return nodes[index].layer;
    }
    nodes[index].layer = 0;
    for (int i=0; i < nodes[index].prerequisites.size; i++) {
        int prerequisite_index = _SetNodeLayer(nodes, IdGetIndex(nodes[index].prerequisites[i]));
        if (prerequisite_index + 1 > nodes[index].layer) {
            nodes[index].layer = prerequisite_index + 1;
        }
    }
    return nodes[index].layer;
}

int TechTree::Load(const DataNode *data) {
    nodes_count = data->GetChildArrayLen("techtree");
    delete[] nodes;
    delete[] node_progress;
    delete[] node_names_ptrs;
    nodes = new TechTreeNode[nodes_count];
    node_progress = new int[nodes_count];
    node_names_ptrs = new const char*[nodes_count];
    // 1st pass: most data
    for(int i=0; i < nodes_count; i++) {
        const DataNode* node_data = data->GetChildArrayElem("techtree", i);
        strcpy(nodes[i].name, node_data->Get("name"));
        const char* start_condition_str = node_data->Get("start_condition");
        nodes[i].research_effort = node_data->GetI("effort", 1000);
        if (strcmp(start_condition_str, "unlocked") == 0) {
            node_progress[i] = nodes[i].research_effort;
        }
        else if (strcmp(start_condition_str, "available") == 0) {
            node_progress[i] = 0;
        } else {
            node_progress[i] = -nodes[i].research_effort;
        }
        
        nodes[i].icon_index.x = node_data->GetArrayElemI("icon_index", 0);
        nodes[i].icon_index.y = node_data->GetArrayElemI("icon_index", 1);

        int unlocks_count = node_data->GetArrayLen("unlocks");
        nodes[i].attached_components = IDList(unlocks_count);
        for(int j=0; j < unlocks_count; j++) {
            RID unlock = GetGlobalState()->GetFromStringIdentifier(node_data->GetArrayElem("unlocks", j));
            nodes[i].attached_components.Append(unlock);
        }

        RID rid = RID(i, EntityType::TECHTREE_NODE);
        nodes[i].str_id = GetGlobalState()->AddStringIdentifier(node_data->Get("id"), rid);
        node_names_ptrs[i] = nodes[i].str_id;
    }
    // 2nd pass: prerequisites
    for(int i=0; i < nodes_count; i++) {
        const DataNode* node_data = data->GetChildArrayElem("techtree", i);
        int prerequisite_count = node_data->GetArrayLen("prerequisites", true);
        nodes[i].prerequisites = IDList(prerequisite_count);
        for(int j=0; j < prerequisite_count; j++) {
            RID prereq = GetGlobalState()->GetFromStringIdentifier(node_data->GetArrayElem("prerequisites", j));
            nodes[i].prerequisites.Append(prereq);
        }
    }

    layers = 1;

    // 3rd pass: Calculate layers
    for(int i=0; i < nodes_count; i++) {
        int layer = _SetNodeLayer(nodes, i);
        if(layer + 1 > layers) {
            layers = layer + 1;
        }
    }

    int* layer_offsets = new int[layers];
    for(int i=0; i < layers; i++) layer_offsets[i] = 0;
    // 4th pass: Calculate layer indices (knowing max layers)
    max_indecies_in_layer = 1;
    for(int i=0; i < nodes_count; i++) {
        nodes[i].index_in_layer = layer_offsets[nodes[i].layer]++;
        if (nodes[i].index_in_layer + 1 > max_indecies_in_layer){
            max_indecies_in_layer = nodes[i].index_in_layer + 1;
        }
        //INFO("%s: %d, %d", nodes[i].name, nodes[i].layer, nodes[i].index_in_layer)
    }
    // 5th pass: Set total in layer
    for(int i=0; i < nodes_count; i++) {
        nodes[i].total_in_layer = layer_offsets[nodes[i].layer];
    }
    delete[] layer_offsets;
    return nodes_count;
}

void TechTree::ForceUnlockTechnology(const char *tech_id) {
    RID technode_id = GetGlobalState()->GetFromStringIdentifier(tech_id);
    INFO("'%s' => %d", tech_id, technode_id.AsInt())
    if (IsIdValidTyped(technode_id, EntityType::TECHTREE_NODE)) {
        int index = IdGetIndex(technode_id);
        node_progress[index] = nodes[index].research_effort;
    }
}

bool TechTree::IsUnlocked(RID entity_class) const {
    for (int i=0; i < nodes_count; i++) {
        if (node_progress[i] < nodes[i].research_effort) {
            continue;
        }
        if (nodes[i].attached_components.Find(entity_class) >= 0) {
            return true;
        }
    }
    return false;
}

void TechTree::Update() {
    return;
    daily_progress = GetSettingNum("base_tech_progress");
    IDList all_ships;
    GetShips()->GetAll(&all_ships, ship_selection_flags::GetAllegianceFlags(0));
    //RID lab = GetShipModules()->expected_modules.shpmod_research_lab;
    for(int i=0; i < all_ships.size; i++) {
        //daily_progress += GetShip(all_ships[i])->CountModulesOfClass(lab) * GetSettingNum("tech_progress_per_lab");
    }
    if (GetCalendar()->IsNewDay() && research_focus >= 0) {
        // TODO: calculate research
        node_progress[research_focus] += daily_progress;

        if (node_progress[research_focus] >= nodes[research_focus].research_effort) {
            node_progress[research_focus] = nodes[research_focus].research_effort;
            research_focus = -1;
        }
    }
}

Vector2 _GetNodePos(const TechTreeNode* node) {
    int total_height = ui::Current()->height;
    Vector2 res;
    res.x = ui::Current()->x + 20 + node->layer * 75;
    if (node->total_in_layer == 1) {
        res.y = ui::Current()->y + 20 + (total_height - 90) / 2;
        res.y += node->layer * 5;
    } else {
        res.y = ui::Current()->y + 20 + node->index_in_layer * (total_height - 90) / (node->total_in_layer - 1);
    }
    return res;
}

void TechTree::DrawUI() {
    // Manage viewing
    if (!GetGlobalState()->IsKeyBoardFocused() && IsKeyPressed(KEY_FOUR)) {
        shown = !shown;
    }
    if (!shown) return;

    ui::CreateNew(
        20, 100, GetScreenWidth() - 40, GetScreenHeight() - 100, 
        DEFAULT_FONT_SIZE, Palette::ui_main, Palette::bg, true
    );
    ui::Enclose();

    int preview_tech = ui_hovered_tech >= 0 ? ui_hovered_tech : ui_selected_tech;

    BeginRenderInUILayer(ui::Current()->z_layer);
    for(int i=0; i < nodes_count; i++) {
        if (node_progress[i] < 0) {
            continue;
        }

        Color connection_color = Palette::ui_alt;

        if (i == preview_tech) {
            connection_color = Palette::interactable_main;
        }

        Vector2 node_pos = _GetNodePos(&nodes[i]);
        for(int j=0; j < nodes[i].prerequisites.size; j++) {
            Vector2 prereq_pos = _GetNodePos(&nodes[IdGetIndex(nodes[i].prerequisites[j])]);
            DrawLine(node_pos.x + 25, node_pos.y + 25, prereq_pos.x + 25, prereq_pos.y + 25, connection_color);
        }
    }
    EndRenderInUILayer();

    ui_hovered_tech = -1;
    for(int i=0; i < nodes_count; i++) {
        if (node_progress[i] < 0) {
            continue;
        }
        Vector2 node_pos = _GetNodePos(&nodes[i]);
        Color color = Palette::ui_alt;
            
        if (node_progress[i] == nodes[i].research_effort) {
            color = Palette::ui_main;
        }

        if (i == preview_tech) {
            color = Palette::interactable_main;
        }

        ui::PushGlobal(node_pos.x, node_pos.y, 50, 50, DEFAULT_FONT_SIZE, color, Palette::bg, ui::Current()->z_layer);
        button_state_flags::T button_state = ui::AsButton();
        HandleButtonSound(button_state);
        if (button_state & button_state_flags::HOVER) {
            ui::SetMouseHint(nodes[i].name);
            ui_hovered_tech = i;
        }

        if (i == research_focus) {
            int enclose_radius = Lerp(4, 8, sin(GetRenderServer()->animation_time * 6.0)*.5 + .5);
            ui::EncloseEx(-enclose_radius, Palette::bg, Palette::interactable_main, 4);
        }

        ui::Enclose();
        ui::DrawIcon(nodes[i].icon_index, color, 40);
        ui::Pop();  // Global
        if (node_progress[i] > 0 && node_progress[i] < nodes[i].research_effort) {
            ui::FilllineEx(
                node_pos.x, node_pos.x + 50, node_pos.y + 57, 
                node_progress[i] / (double)nodes[i].research_effort, 
                Palette::ui_main, Palette::ui_alt
            );
        }
    }

    Rectangle clickable_selection_rect = ui::Current()->GetRect();
    if (preview_tech >= 0) clickable_selection_rect.width -= 200;  // Exclude side-bar

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), clickable_selection_rect)) {
        ui_selected_tech = ui_hovered_tech;
    }

    bool can_research_preview = false;
    if(preview_tech >= 0) {
        can_research_preview = node_progress[preview_tech] < nodes[preview_tech].research_effort;
        for (int i=0; i < nodes[preview_tech].prerequisites.size; i++) {
            int prereq = IdGetIndex(nodes[preview_tech].prerequisites[i]);
            if (node_progress[prereq] < nodes[prereq].research_effort) {
                can_research_preview = false;
            }
        }
    }

    // Draw side-bar

    if (preview_tech >= 0) {
        ui::PushHSplit(ui::Current()->width - 200, ui::Current()->width);
        ui::Enclose();

        ui::Write(nodes[preview_tech].name);
        ui::Write(TextFormat("to unlock: %d/%d", node_progress[preview_tech], nodes[preview_tech].research_effort));
        if (nodes[preview_tech].prerequisites.size > 0) {
            ui::Write("Prerequisites: ");
            for (int i=0; i < nodes[preview_tech].prerequisites.size; i++) {
                int prereq_index = IdGetIndex(nodes[preview_tech].prerequisites[i]);
                if (node_progress[prereq_index] < nodes[prereq_index].research_effort) {
                    ui::Current()->text_color = Palette::red;
                }
                ui::Write(TextFormat("- %s", nodes[prereq_index].name));
                ui::Current()->text_color = Palette::ui_main;
            }
        }
        ui::HSpace(6);
        int width = ui::Current()->width;
        int columns = nodes[preview_tech].attached_components.size;
        if (columns < 2) columns = 2;
        int icon_size = width / columns;
        ui::PushInset(icon_size - 8);
        for (int i=0; i < nodes[preview_tech].attached_components.size; i++) {
            ui::PushGridCell(columns, 1, i, 0);
            RID component_id = nodes[preview_tech].attached_components[i];
            if (IsIdValidTyped(component_id, EntityType::SHIP_CLASS)) {
                GetShips()->DrawShipClassUI(component_id);
            } else if (IsIdValidTyped(component_id, EntityType::MODULE_CLASS)) {
                GetShipModules()->DrawShipModule(component_id, false);
            }
            ui::Pop();  // GridCell
        }
        ui::Pop();  // Inset

        // Confirm button
        if (can_research_preview) {
            ui::PushAligned(ui::Current()->width - 10, 20, text_alignment::HCENTER | text_alignment::BOTTOM);
            button_state_flags::T confirm_button = ui::AsButton();
            if (confirm_button & button_state_flags::HOVER) {
                ui::EncloseEx(0, Palette::bg, Palette::interactable_main, 4);
                ui::Shrink(4, 4);
            } else {
                ui::Enclose();
            }
            if (confirm_button & button_state_flags::JUST_PRESSED) {
                research_focus = preview_tech;
            }
            ui::WriteEx("Focus", text_alignment::CENTER, false);
            ui::Pop();
        }

        ui::Pop();  // Side-bar
    }
    ui::Pop();  // Global
}

int LoadTechTree(const DataNode *data) {
    return GetTechTree()->Load(data);
}

