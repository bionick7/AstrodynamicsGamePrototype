#include "techtree.hpp"
#include "global_state.hpp"
#include "event_popup.hpp"
#include "utils.hpp"

int _CountResearchConditionsRecursive(const DataNode *condition_data) {
    if (condition_data == NULL || !condition_data->Has("type")) {
        return 0;  // Invalid condition
    }
    const char* condition_type = condition_data->Get("type");
    if (strcmp(condition_type, "any") != 0 && strcmp(condition_type, "all") != 0) {
        // Neither 'any', nor 'all'
        return 1;
    }
    int res = 1;
    for (int i=0; i < condition_data->GetChildArrayLen("elements"); i++) {
        res += _CountResearchConditionsRecursive(condition_data->GetChildArrayElem("elements", i));
    }
    return res;
}

int TechTree::Load(const DataNode *data) {
    nodes_count = data->GetChildArrayLen("techtree");

    delete[] nodes;
    delete[] node_unlocked;
    delete[] research_conditions;

    nodes = new TechTreeNode[nodes_count];
    node_unlocked = new int[nodes_count];
    research_condition_count = 0;

    delete[] visited_planets;

    visited_planets = new bool[GetPlanets()->GetPlanetCount()];

    for (int i=0; i < GetPlanets()->GetPlanetCount(); i++) { visited_planets[i] = false; }

    // 1st pass: most data
    for(int i=0; i < nodes_count; i++) {
        const DataNode* node_data = data->GetChildArrayElem("techtree", i);
        //nodes[i].name = AddPermaString(node_data->Get("name"));
        nodes[i].name = PermaString(node_data->Get("name"));
        nodes[i].description = PermaString(node_data->Get("description"));
        const char* start_condition_str = node_data->Get("start_condition", "locked");
        node_unlocked[i] = 0;
        if (strcmp(start_condition_str, "unlocked") == 0) node_unlocked[i] = 1;
        if (strcmp(start_condition_str, "hidden") == 0) node_unlocked[i] = -1;
        
        nodes[i].icon_index.x = node_data->GetArrayElemI("icon_index", 0);
        nodes[i].icon_index.y = node_data->GetArrayElemI("icon_index", 1);

        int unlocks_count = node_data->GetArrayLen("unlocks");
        nodes[i].attached_components.Resize(unlocks_count);
        for(int j=0; j < unlocks_count; j++) {
            RID unlock = GetGlobalState()->GetFromStringIdentifier(node_data->GetArrayElem("unlocks", j));
            nodes[i].attached_components.Append(unlock);
        }

        int condition_count = _CountResearchConditionsRecursive(node_data->GetChild("condition"));
        if (condition_count > 0) {
            nodes[i].condition_index = research_condition_count;
        } else {
            nodes[i].condition_index = -1;
        }
        research_condition_count += condition_count;

        RID rid = RID(i, EntityType::TECHTREE_NODE);
        strcpy(nodes[i].str_id, node_data->Get("id"));
        GetGlobalState()->AddStringIdentifier(nodes[i].str_id, rid);
        
        nodes[i].draw_pos.x = node_data->GetArrayElemF("draw_pos", 0);
        nodes[i].draw_pos.y = node_data->GetArrayElemF("draw_pos", 1);
    }

    research_conditions = new ResearchCondition[research_condition_count];

    int research_condition_index = 0;

    // 2nd pass: collect prerequisites, load research conditions
    for(int i=0; i < nodes_count; i++) {
        const DataNode* node_data = data->GetChildArrayElem("techtree", i);
        int prerequisite_count = node_data->GetArrayLen("prerequisites", true);
        nodes[i].prerequisites.Resize(prerequisite_count);
        for(int j=0; j < prerequisite_count; j++) {
            RID prereq = GetGlobalState()->GetFromStringIdentifier(node_data->GetArrayElem("prerequisites", j));
            nodes[i].prerequisites.Append(prereq);
        }

        if (node_data->Has("effect")) {
            global_vars::CompileEffects(node_data->Get("effect"), &nodes[i].effects);
        }

        // Research conditions
        const DataNode* research_condition_data = node_data->GetChild("condition");
        research_condition_index += LoadResearchCondition(research_condition_data, 
            research_condition_index, research_condition_index + 1) + 1;
    }

    UpdateTechProgress();

    return nodes_count;
}

int TechTree::LoadResearchCondition(const DataNode* condition_data, int idx, int child_index) {
    // Returns how many additional conditions are loaded
    if (condition_data == NULL || !condition_data->Has("type")) {
        return -1;
    }
    const char* type_id = condition_data->Get("type");
    research_conditions[idx].type = (ResearchCondition::Type) FindInArray(ResearchCondition::type_identifiers, ResearchCondition::TYPE_MAX, type_id);
    if (research_conditions[idx].IsBranch()) {
        int elements_count = condition_data->GetChildArrayLen("elements");
        int write_index = child_index + elements_count;
        for (int i=0; i < elements_count; i++) {
            write_index += LoadResearchCondition(condition_data->GetChildArrayElem("elements", i), child_index + i, write_index);
        }
        research_conditions[idx].cond.branch.index = child_index;
        research_conditions[idx].cond.branch.count = elements_count;
        return write_index - child_index;
    } else {
        research_conditions[idx].GenLeafFromString(research_conditions[idx].type, condition_data->Get("value"));
        if (condition_data->Has("description")) {
            research_conditions[idx].cond.leaf.description = PermaString(condition_data->Get("description"));
        }
        return 0;
    }
}

void TechTree::UnlockTechnology(RID technode_id, bool notify) {
    int node_index = IdGetIndex(technode_id);
    node_unlocked[node_index] = 1;
    const TechTreeNode* node = &nodes[node_index];
    if (notify) {  // Popup
        Popup* popup = event_popup::AddPopup(500, 500, 300);
        StringBuilder sb;
        sb.Add("Unlocked ").Add(node->name.GetChar());
        strcpy(popup->title, sb.c_str);
        sb.Clear();
        strcpy(popup->description, node->description.GetChar());
    }

    // Run unlock effects
    for (int i=0; i < node->effects.Count(); i++) {
        InterpretCompiled(node->effects[i]);
    }
}

void TechTree::Serialize(DataNode *data) const {
    data->CreateArray("visited_planets", 0);
    for (int i=0; i < GetPlanets()->GetPlanetCount(); i++) {
        if (visited_planets[i]) {
            data->AppendToArray("visited_planets", GetPlanetByIndex(i)->name.GetChar());
        }
    }
    data->CreateArray("condition_internals", research_condition_count);
    for (int i=0; i < research_condition_count; i++) {
        if (research_conditions[i].type == ResearchCondition::PRODUCE_ITEM || 
            research_conditions[i].type == ResearchCondition::PRODUCTION_COUNTER
        ) {
            data->InsertIntoArrayI("condition_internals", i, research_conditions[i].internal_counter);
        } else {
            data->InsertIntoArray("condition_internals", i, "---");
        }
    }
    data->CreateArray("unlocked", 0);
    for (int i=0; i < nodes_count; i++) {
        if (node_unlocked[i] > 0) {
            data->AppendToArray("unlocked", nodes[i].str_id);
        }
    }
}

void TechTree::Deserialize(const DataNode *data) {
    for (int i=0; i < GetPlanets()->GetPlanetCount(); i++) { visited_planets[i] = false; }
    for (int i=0; i < data->GetArrayLen("visited_planets"); i++) {
        int planet_index = IdGetIndex(GetPlanets()->GetIdByName(data->GetArrayElem("visited_planets", i)));
        visited_planets[planet_index] = true;
    }
    ASSERT_EQUAL_INT(data->GetArrayLen("condition_internals"), research_condition_count);
    for (int i=0; i < research_condition_count; i++) {
        research_conditions[i].internal_counter = data->GetArrayElemI("condition_internals", i, 0, true);
    }
    for (int i=0; i < data->GetArrayLen("unlocked"); i++) {
        const char* node_id = data->GetArrayElem("unlocked", i);
        RID node_index = GetGlobalState()->GetFromStringIdentifier(node_id);
        if (IsIdValidTyped(node_index, EntityType::TECHTREE_NODE)) {
            UnlockTechnology(node_index, false);
        }
    }
}

void TechTree::ReportProduction(RID product) {
    for (int node_index=0; node_index < nodes_count; node_index++) {
        if (node_unlocked[node_index] != 0) {
            continue;
        }
        List<int> condition_indices;
        GetAttachedConditions(nodes[node_index].condition_index, &condition_indices);
        
        for(int i=0; i < condition_indices.size; i++) {
            int condition_index = condition_indices[i];
            if (research_conditions[condition_index].type != ResearchCondition::PRODUCE_ITEM) {
                continue;
            }
            if (product.AsInt() == research_conditions[condition_index].cond.leaf.variable) {
                research_conditions[condition_index].internal_counter++;
            }
        }
    }
}

void TechTree::ReportResourceProduction(const resource_count_t production[]) {
    // Report ALL production to the ministry of industry for administration and control
    for (int node_index=0; node_index < nodes_count; node_index++) {
        if (node_unlocked[node_index] != 0) {
            continue;
        }
        List<int> condition_indices;
        GetAttachedConditions(nodes[node_index].condition_index, &condition_indices);
        
        for(int i=0; i < condition_indices.size; i++) {
            int condition_index = condition_indices[i];
            if (research_conditions[condition_index].type != ResearchCondition::PRODUCTION_COUNTER) {
                continue;
            }
            if (!research_conditions[condition_index].IsValid()) {
                continue;
            }
            resource_count_t resource_idx = research_conditions[condition_index].cond.leaf.variable;
            if (production[resource_idx] <= 0) {
                continue;
            }
            research_conditions[condition_index].internal_counter += production[resource_idx];
        }
    }
}

void TechTree::GetAttachedConditions(int condition_index, List<int> *condition_indices) const {
    if (condition_index < 0) return;
    const ResearchCondition* condition = &research_conditions[condition_index];
    if (condition->IsBranch()) {
        for(int i=0; i < condition->cond.branch.count; i++) {
            GetAttachedConditions(condition->cond.branch.index+i, condition_indices);
        }
    } else {
        condition_indices->Append(condition_index);
    }
}

bool TechTree::IsUnlocked(RID entity_class) const {
    for (int i=0; i < nodes_count; i++) {
        if (node_unlocked[i] == 1 && 
            nodes[i].attached_components.Find(entity_class) >= 0
        ) {
            return true;
        }
    }
    return false;
}

void TechTree::Update() {
    UpdateTechProgress();
}

void TechTree::UpdateTechProgress() {
    for (int i=0; i < nodes_count; i++) {
        // Recalculate 
        if (node_unlocked[i] > 0) {
            continue;
        }
        int min_parent_distance = 1;
        for(int j=0; j < nodes[i].prerequisites.size; j++) {
            int parent_index = IdGetIndex(nodes[i].prerequisites[j]);
            if (node_unlocked[parent_index] < min_parent_distance)
                min_parent_distance = node_unlocked[parent_index];
        }
        node_unlocked[i] = min_parent_distance - 1;
        if (nodes[i].condition_index < 0) {
            continue;
        }
        if (node_unlocked[i] == 0 && research_conditions[nodes[i].condition_index].GetProgress() >= 1) {
            UnlockTechnology(RID(i, EntityType::TECHTREE_NODE), true);
        }
    }
}

Vector2 TechTree::GetNodePos(const TechTreeNode* node) const {
    int mid_pos = ui::Current()->x + (ui::Current()->width - sidebar_width) / 2;
    Vector2 res;
    res.x = mid_pos + node->draw_pos.x * 80;
    res.y = ui::Current()->y + 20 + node->draw_pos.y * 100;
    return Vector2Subtract(res, ui_camera_offset);
}

void TechTree::DrawResearchProgressRecursive(int condition_index) const {
    if (condition_index < 0 || condition_index >= research_condition_count) {
        return;
    }
    ResearchCondition condition = research_conditions[condition_index];
    
    int children_count = condition.GetChildCount(false);
    ui::PushInset(children_count * 30);

    const char* type_name = ResearchCondition::type_identifiers[condition.type];

    if (condition.IsBranch()) {
        const int inset_width = 24;
        ui::PushHSplit(0, inset_width);
        ui::BeginDirectDraw();
            int line_x = ui::Current()->x + inset_width - 3;
            DrawLine(line_x, ui::Current()->y + 3,
                     line_x, ui::Current()->y + ui::Current()->height - 3, ui::Current()->text_color);

        // Some lower-abstraction shenanigans to draw text vertically
        const int text_size = DEFAULT_FONT_SIZE;
        Vector2 text_draw_pos;
        text_draw_pos.x = ui::Current()->x + 2;
        text_draw_pos.y = ui::Current()->y + (ui::Current()->height + MeasureText(type_name, text_size)) / 2;
        DrawTextPro(GetCustomDefaultFont(text_size), type_name, text_draw_pos, Vector2Zero(), -90, text_size, 1, ui::Current()->text_color);

        ui::EndDirectDraw();
        ui::Pop();  // HSplit

        ui::PushHSplit(inset_width, ui::Current()->width);
        for (int i=0; i < condition.cond.branch.count; i++) {
            DrawResearchProgressRecursive(condition.cond.branch.index + i);
        }
        ui::Pop();  // HSplit
    } else {
        StringBuilder sb;
        condition.GetDescriptiveText(&sb);
        ui::Write(sb.c_str);
    }
    
    ui::Pop();  // Inset
}

void TechTree::DrawUI() {
    const int min_node_vis = -1;

    ui::CreateNew(
        20, 100, GetScreenWidth() - 40, GetScreenHeight() - 100, 
        DEFAULT_FONT_SIZE, Palette::ui_main, Palette::bg, z_layers::MENU_PANELS
    );
    ui::Enclose();

    // Handle Input
    ui_camera_offset = Vector2Add(ui_camera_offset, Vector2Scale(GetMouseWheelMoveV(), -20));

    // Flavour
    ui::PushAligned(300, 130, text_alignment::TOP | text_alignment::LEFT);
    ui::Current()->text_color = Palette::ui_alt;
    ui::Current()->x += 5;
    ui::Current()->y += 5;
    ui::Enclose();
    ui::WriteEx("Technological\nDevelopment Plan", text_alignment::VCONFORM | text_alignment::HCENTER, false);
    ui::FillLine(1, Palette::ui_alt, Palette::ui_alt);
    ui::Write("");  // Linebreak
    ui::Write("Version N°: 10.b");
    ui::Write("ID: A486F-9");
    ui::Write("Author: Ing. Olav Hobbs");
    ui::Write("Approved by: [Pending]");
    ui::Pop();  // Aligned

    int preview_tech = ui_hovered_tech >= 0 ? ui_hovered_tech : ui_selected_tech;

    // Draw Connections

    ui::BeginDirectDraw();
    for(int i=0; i <= nodes_count; i++) {
        int node_index = i < nodes_count ? i : preview_tech;  // Re-render preview_tech on top of everything

        if (node_index < 0) {  // Possible if preview tech is empty
            continue;
        }
        if (node_unlocked[node_index] < min_node_vis) {
            continue;
        }

        Color connection_color = Palette::ui_dark;
                    
        if (node_unlocked[node_index] >= 0) {
            connection_color = Palette::ui_alt;
        }

        if (node_index == preview_tech) {
            connection_color = Palette::interactable_main;
        }

        Vector2 node_pos = Vector2Add(GetNodePos(&nodes[node_index]), { 25, 0 });
        for(int j=0; j < nodes[node_index].prerequisites.size; j++) {
            const int arrow_width = 8;
            const int arrow_height = 10;

            Vector2 prereq_pos = Vector2Add(GetNodePos(&nodes[IdGetIndex(nodes[node_index].prerequisites[j])]), { 25, 50 });
            int mid_point_y = prereq_pos.y + 30 + nodes[node_index].draw_pos.x * 4;
            Vector2 p1 = { prereq_pos.x, mid_point_y };
            Vector2 p2 = { node_pos.x, mid_point_y };
            DrawLineV(prereq_pos, p1, connection_color);
            DrawLineV(p1, p2, connection_color);
            DrawLineV(p2, node_pos, connection_color);
            DrawTriangle(node_pos, Vector2Add(node_pos, { -arrow_width/2, -arrow_height }), 
                         Vector2Add(node_pos, { arrow_width/2, -arrow_height }), connection_color);
        }
    }
    ui::EndDirectDraw();

    ui_hovered_tech = -1;
    for(int i=0; i < nodes_count; i++) {
        if (node_unlocked[i] < min_node_vis) {
            continue;
        }
        Vector2 node_pos = GetNodePos(&nodes[i]);
        Color color = Palette::ui_dark;
            
        if (node_unlocked[i] == 1) {
            color = Palette::ui_main;
        } else if (node_unlocked[i] == 0) {
            color = Palette::ui_alt;
        }

        if (i == preview_tech) {
            color = Palette::interactable_main;
        }

        ui::PushFree(node_pos.x, node_pos.y, 50, 50);
        button_state_flags::T button_state = ui::AsButton();
        HandleButtonSound(button_state);
        if (button_state & button_state_flags::HOVER) {
            ui::SetMouseHint(nodes[i].name.GetChar());
            ui_hovered_tech = i;
        }

        ui::EncloseEx(0, Palette::bg, color, 0);
        ui::DrawIcon(nodes[i].icon_index, text_alignment::CENTER, color, 40);
        ui::Pop();  // Global
    }

    Rectangle clickable_selection_rect = ui::Current()->GetRect();
    if (preview_tech >= 0) clickable_selection_rect.width -= sidebar_width;  // Exclude side-bar

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), clickable_selection_rect)) {
        ui_selected_tech = ui_hovered_tech;
    }

    // Draw side-bar

    if (preview_tech >= 0) {
        ui::PushHSplit(ui::Current()->width - sidebar_width, ui::Current()->width);
        ui::Enclose();

        // Name
        ui::WriteEx(nodes[preview_tech].name.GetChar(), text_alignment::HCENTER | text_alignment::VCONFORM, true);
        ui::FillLine(1, Palette::ui_alt, Palette::bg);
        ui::VSpace(10);

        // Prerequisites
        if (nodes[preview_tech].prerequisites.size > 0) {
            ui::WriteEx("Prerequisites: ", text_alignment::HCENTER | text_alignment::VCONFORM, true);
            for (int i=0; i < nodes[preview_tech].prerequisites.size; i++) {
                int prereq_index = IdGetIndex(nodes[preview_tech].prerequisites[i]);
                if (node_unlocked[prereq_index] < 1) {
                    ui::Current()->text_color = Palette::red;
                }
                ui::Write(TextFormat("- %s", nodes[prereq_index].name.GetChar()));
                ui::Current()->text_color = Palette::ui_main;
            }
        }
        ui::FillLine(1, Palette::ui_alt, Palette::bg);
        ui::VSpace(8);

        ui::WriteEx("Unlocks: ", text_alignment::HCENTER | text_alignment::VCONFORM, true);
        ui::HSpace(6);
        int width = ui::Current()->width;
        int columns = nodes[preview_tech].attached_components.size;
        if (columns < 4) columns = 4;
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
        ui::FillLine(1, Palette::ui_alt, Palette::bg);
        ui::VSpace(8);
        
        // Requirements
        ui::WriteEx("Requirements: ", text_alignment::HCENTER | text_alignment::VCONFORM, true);
        ui::VSpace(6);
        if (node_unlocked[preview_tech] < 0) {
            // Gray out if not active
            ui::Current()->text_color = Palette::ui_alt;
        }
        DrawResearchProgressRecursive(nodes[preview_tech].condition_index);
        ui::Current()->text_color = Palette::ui_main;
        ui::FillLine(1, Palette::ui_alt, Palette::bg);
        ui::VSpace(8);

        // Description
        ui::Write(nodes[preview_tech].description.GetChar());

        ui::Pop();  // Side-bar
    }
    ui::Pop();  // Global
}

int LoadTechTree(const DataNode *data) {
    return GetTechTree()->Load(data);
}
