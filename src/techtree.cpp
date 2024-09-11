#include "techtree.hpp"
#include "global_state.hpp"
#include "logging.hpp"
#include "debug_drawing.hpp"
#include "debug_console.hpp"
#include "global_state.hpp"

float _GetProgressFromComparison(int variable, int reference, ResearchCondition::Comparison comp) {
    switch (comp) {
        case ResearchCondition::GREATER:
            return variable / (float)(reference + 1);
        case ResearchCondition::LESS:
            return variable < reference ? 1:0;
        case ResearchCondition::GREATER_OR_EQUAL:
            return variable / (float)reference;
        case ResearchCondition::LESS_OR_EQUAL:
            return variable <= reference ? 1:0;
        case ResearchCondition::EQUAL :
            return variable == reference ? 1:0;
        case ResearchCondition::NONEQUAL:
            return variable != reference ? 1:0;
        default:
            return 0;
    }
}

bool ResearchCondition::IsBranch() const {
    switch (type) {
    case ANY:
    case ALL: return true;
    default: return false;
    }
}

float ResearchCondition::GetProgress() const {
    switch (type) {
    case ANY:{
        for (int i=0; i < cond.branch.count; i++) {
            if (GetTechTree()->research_conditions[cond.branch.index + i].GetProgress() < 1) {
                return 0;
            }
        }
        return 1;
    }
    case ALL:{
        for (int i=0; i < cond.branch.count; i++) {
            if (GetTechTree()->research_conditions[cond.branch.index + i].GetProgress() >= 1) {
                return 1;
            }
        }
        return 0;
    }
    case STAT_CONDITION:{
        int max_stat = 0;
        for (auto it = GetShips()->alloc.GetIter(); it; it++) {
            const Ship* ship = GetShip(it.GetId());
            if (ship->stats[cond.leaf.variable] > max_stat) {
                max_stat = ship->stats[cond.leaf.variable];
            }
        }
        return _GetProgressFromComparison(max_stat, cond.leaf.value, cond.leaf.comp);
    }
    case PRODUCTION_COUNTER:{
        return _GetProgressFromComparison(internal_counter, cond.leaf.value, cond.leaf.comp);
    }
    case PRODUCE_ITEM:{
        return _GetProgressFromComparison(internal_counter, cond.leaf.value, cond.leaf.comp);
    }
    case VISIT:{
        return GetTechTree()->visited_planets[cond.leaf.variable] ? 1 : 0;
    }
    case ARCHIEVEMENT:{
        return 0;
    }
    case FREE:{
        return 1;
    }
    default: {
        return 0;
    }
    }
    return 0;
}

int ResearchCondition::GetChildCount(bool include_branches) const {
    // Includes self
    if (!IsBranch()) {
        return 1;
    }
    int res = include_branches ? 1 : 0;
    for (int i=0; i < cond.branch.count; i++) {
        res += GetTechTree()->research_conditions[cond.branch.index + i].GetChildCount(include_branches);
    }
    return res;
}

void ResearchCondition::GetDescriptiveText(StringBuilder *sb) const {
    switch (type) {
    case ANY:
    case ALL:{
        break;
    }
    case STAT_CONDITION:{
        if (cond.leaf.variable < 0 || cond.leaf.comp < 0) {
            sb->Add("Invalid stat condition");
            break;
        }
        sb->AddClock(GetProgress());
        sb->AddFormat(" Have a ship with %s %s %d (%3.0f %%)", ship_stats::icons[cond.leaf.variable], 
                      ResearchCondition::comparison_repr[cond.leaf.comp], cond.leaf.value, GetProgress()*100);
        break;
    }
    case PRODUCTION_COUNTER:{
        if (cond.leaf.variable < 0) {
            sb->Add("Invalid stat condition");
            break;
        }
        sb->AddClock(GetProgress());
        sb->AddFormat(" Produce %d counts of %s (%3.1f %%)", cond.leaf.value, 
                        resources::icons[cond.leaf.variable], GetProgress()*100);
        break;
    }
    case PRODUCE_ITEM:{
        RID object_id = RID(cond.leaf.variable);
        const char* name = "UNKNOWN";
        if (IsIdValidTyped(object_id, EntityType::MODULE_CLASS)) {
            const ShipModuleClass* smc = GetModule(object_id);
            name = smc->name;
        } else if (IsIdValidTyped(object_id, EntityType::SHIP_CLASS)) {
            const ShipClass* sc = GetShipClassByRID(object_id);
            name = sc->name;
        } else {
            sb->Add("Invalid stat condition");
            break;
        }
        sb->AddClock(GetProgress());
        sb->AddFormat(" Produce %dx %s (%3.1f %%)", cond.leaf.value, name, GetProgress()*100);
        break;
    }
    case VISIT:{
        if (cond.leaf.variable < 0) {
            sb->Add("Invalid stat condition");
            break;
        }
        const Planet* planet = GetPlanetByIndex(cond.leaf.variable);
        sb->AddClock(GetProgress());
        sb->AddFormat("Visit %s", planet->name);
        break;
    }
    case ARCHIEVEMENT:{
        if (cond.leaf.variable < 0) {
            sb->Add("Invalid stat condition");
            break;
        }
        sb->AddClock(GetProgress());
        sb->Add("TODO: archievment description");
        break;
    }
    case FREE:{
        sb->AddClock(GetProgress());
        sb->Add(" Free Bingo (for testing) :)");
        break;
    }
    }
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
    delete[] archeivements;
    visited_planets = new bool[GetPlanets()->GetPlanetCount()];
    archeivements = new bool[archievement_count];
    for (int i=0; i < GetPlanets()->GetPlanetCount(); i++) { visited_planets[i] = false; }
    for (int i=0; i < archievement_count; i++) { archeivements[i] = false; }

    // 1st pass: most data
    for(int i=0; i < nodes_count; i++) {
        const DataNode* node_data = data->GetChildArrayElem("techtree", i);
        strcpy(nodes[i].name, node_data->Get("name"));
        const char* start_condition_str = node_data->Get("start_condition", "locked");
        node_unlocked[i] = 0;
        if (strcmp(start_condition_str, "unlocked") == 0) node_unlocked[i] = 1;
        if (strcmp(start_condition_str, "hidden") == 0) node_unlocked[i] = -1;
        
        nodes[i].icon_index.x = node_data->GetArrayElemI("icon_index", 0);
        nodes[i].icon_index.y = node_data->GetArrayElemI("icon_index", 1);

        int unlocks_count = node_data->GetArrayLen("unlocks");
        nodes[i].attached_components = IDList(unlocks_count);
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
        nodes[i].str_id = GetGlobalState()->AddStringIdentifier(node_data->Get("id"), rid);
    }

    research_conditions = new ResearchCondition[research_condition_count];

    int research_condition_index = 0;

    // 2nd pass: collect prerequisites and load research conditions
    for(int i=0; i < nodes_count; i++) {
        const DataNode* node_data = data->GetChildArrayElem("techtree", i);
        int prerequisite_count = node_data->GetArrayLen("prerequisites", true);
        nodes[i].prerequisites = IDList(prerequisite_count);
        for(int j=0; j < prerequisite_count; j++) {
            RID prereq = GetGlobalState()->GetFromStringIdentifier(node_data->GetArrayElem("prerequisites", j));
            nodes[i].prerequisites.Append(prereq);
        }

        // Research conditions
        const DataNode* research_condition_data = node_data->GetChild("condition");
        research_condition_index += LoadResearchCondition(research_condition_data, 
            research_condition_index, research_condition_index + 1) + 1;
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
    max_indices_in_layer = 1;
    for(int i=0; i < nodes_count; i++) {
        nodes[i].index_in_layer = layer_offsets[nodes[i].layer]++;
        if (nodes[i].index_in_layer + 1 > max_indices_in_layer){
            max_indices_in_layer = nodes[i].index_in_layer + 1;
        }
        //INFO("%s: %d, %d", nodes[i].name, nodes[i].layer, nodes[i].index_in_layer)
    }
    // 5th pass: Set total in layer
    for(int i=0; i < nodes_count; i++) {
        nodes[i].total_in_layer = layer_offsets[nodes[i].layer];
    }
    delete[] layer_offsets;

    UpdateTechProgress();

    return nodes_count;
}

int _FindInArray(const char* const search[], int search_count, const char* identifier) {
    int res = -1;
    for (int i=0; i < search_count; i++){
        if (strcmp(identifier, search[i]) == 0) {
            res = i;
            break;
        }
    }
    if (res < 0) {
        ERROR("'%s' not found in respective array ('%s', ...)", identifier, search)
    }
    return res;
}

int TechTree::LoadResearchCondition(const DataNode* condition_data, int idx, int child_index) {
    // Returns how many additional conditions are loaded
    if (condition_data == NULL || !condition_data->Has("type")) {
        return -1;
    }
    const char* type_id = condition_data->Get("type");
    research_conditions[idx].type = (ResearchCondition::Type) _FindInArray(ResearchCondition::type_identifiers, ResearchCondition::TYPE_MAX, type_id);
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
        research_conditions[idx].cond.leaf.value = condition_data->GetI("value");
        
        const char* comp_id = condition_data->Get("comp");
        research_conditions[idx].cond.leaf.comp = (ResearchCondition::Comparison) _FindInArray(
            ResearchCondition::comparison_identifiers, ResearchCondition::COMPARISON_MAX, comp_id);

        switch (research_conditions[idx].type) {
        case ResearchCondition::STAT_CONDITION:{
            const char* var_id = condition_data->Get("stat");
            research_conditions[idx].cond.leaf.variable = _FindInArray(
                ship_stats::names, ship_stats::MAX, var_id);
            break;
        }
        case ResearchCondition::PRODUCTION_COUNTER:{
            const char* var_id = condition_data->Get("rsc");
            research_conditions[idx].cond.leaf.variable = _FindInArray(
                resources::names, resources::MAX, var_id);
            break;
        }
        case ResearchCondition::VISIT:{
            const char* var_id = condition_data->Get("planet");
            research_conditions[idx].cond.leaf.variable = IdGetIndex(GetPlanets()->GetIdByName(var_id));
            break;
        }
        case ResearchCondition::PRODUCE_ITEM:{
            const char* var_id = condition_data->Get("item");
            research_conditions[idx].cond.leaf.variable = GetGlobalState()->GetFromStringIdentifier(var_id).AsInt();
            break;
        }
        case ResearchCondition::ARCHIEVEMENT:{
            NOT_IMPLEMENTED
            break;
        }
        default:{
            ERROR("Unrecognized research condition type '%s'", type_id)
            break;
        }
        }
        return 0;
    }
}

void TechTree::ForceUnlockTechnology(RID technode_id) {
    //INFO("'%s' => %d", tech_id, technode_id.AsInt())
    if (IsIdValidTyped(technode_id, EntityType::TECHTREE_NODE)) {
        int index = IdGetIndex(technode_id);
        node_unlocked[index] = 1;
    }
}

/*
    tech:
      visited planets:
      - Tethys
      - Rhea
      archievements:
      - A
      - B
      conditions:
      - index: 0
        value: 5
*/

void TechTree::Serialize(DataNode *data) const {
    data->CreateArray("visited_planets", 0);
    for (int i=0; i < GetPlanets()->GetPlanetCount(); i++) {
        if (visited_planets[i]) {
            data->AppendToArray("visited_planets", GetPlanetByIndex(i)->name);
        }
    }
    data->CreateArray("archievements", 0);
    for (int i=0; i < archievement_count; i++) {
        if (visited_planets[i]) {
            data->AppendToArray("visited_planets", GetPlanetByIndex(i)->name);
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
    for (int i=0; i < research_condition_count; i++) {
        if (node_unlocked[i]) {
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
    for (int i=0; i < archievement_count; i++) { archeivements[i] = false; }
    for (int i=0; i < data->GetArrayLen("archievements"); i++) {
        int archievement_index = IdGetIndex(GetGlobalState()->GetFromStringIdentifier(data->GetArrayElem("archievements", i)));
        archeivements[archievement_index] = true;
    }
    ASSERT_EQUAL_INT(data->GetArrayLen("condition_internals"), research_condition_count);
    for (int i=0; i < research_condition_count; i++) {
        research_conditions[i].internal_counter = data->GetArrayElemI("condition_internals", i, 0, true);
    }
    for (int i=0; i < data->GetArrayLen("unlocked"); i++) {
        const char* node_id = data->GetArrayElem("unlocked", i);
        RID node_index = GetGlobalState()->GetFromStringIdentifier(node_id);
        if (IsIdValidTyped(node_index, EntityType::TECHTREE_NODE)) {
            node_unlocked[IdGetIndex(node_index)] = 1;
        }
    }
}

void TechTree::ReportArchievement(int archievement) {
    // Not called

    // TODO
}

void TechTree::ReportVisit(RID planet) {
    // Not called
    if (!IsIdValidTyped(planet, EntityType::PLANET)) {
        return;
    }
    visited_planets[IdGetIndex(planet)] = true;
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
        if (node_unlocked[i] > 0)
            continue;
        int min_parent_distance = 1;
        for(int j=0; j < nodes[i].prerequisites.size; j++) {
            int parent_index = IdGetIndex(nodes[i].prerequisites[j]);
            if (node_unlocked[parent_index] < min_parent_distance)
                min_parent_distance = node_unlocked[parent_index];
        }
        node_unlocked[i] = min_parent_distance - 1;
        if (node_unlocked[i] == 0 && research_conditions[nodes[i].condition_index].GetProgress() >= 1) {
            node_unlocked[i] = 1;
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
                     line_x, ui::Current()->y + ui::Current()->height - 3, Palette::ui_main);

        // Some lower-abstraction shenanigans to draw text vertically
        const int text_size = DEFAULT_FONT_SIZE;
        Vector2 text_draw_pos;
        text_draw_pos.x = ui::Current()->x + 2;
        text_draw_pos.y = ui::Current()->y + (ui::Current()->height + MeasureText(type_name, text_size)) / 2;
        DrawTextPro(GetCustomDefaultFont(text_size), type_name, text_draw_pos, Vector2Zero(), -90, text_size, 1, Palette::ui_main);

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

    // Draw Connections

    BeginRenderInUILayer(ui::Current()->z_layer);
    for(int i=0; i < nodes_count; i++) {
        if (node_unlocked[i] < min_node_vis) {
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
        if (node_unlocked[i] < min_node_vis) {
            continue;
        }
        Vector2 node_pos = _GetNodePos(&nodes[i]);
        Color color = Palette::ui_alt;
            
        if (node_unlocked[i] == 1) {
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

        ui::Enclose();
        ui::DrawIcon(nodes[i].icon_index, color, 40);
        ui::Pop();  // Global
        /*
        if (node_progress[i] > 0 && node_progress[i] < nodes[i].research_effort) {
            ui::FillLineEx(
                node_pos.x, node_pos.x + 50, node_pos.y + 57, 
                node_progress[i] / (double)nodes[i].research_effort, 
                Palette::ui_main, Palette::ui_alt
            );
        }*/
    }

    const int sidebar_width = 400;

    Rectangle clickable_selection_rect = ui::Current()->GetRect();
    if (preview_tech >= 0) clickable_selection_rect.width -= sidebar_width;  // Exclude side-bar

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), clickable_selection_rect)) {
        ui_selected_tech = ui_hovered_tech;
    }

    // Draw side-bar

    if (preview_tech >= 0) {
        ui::PushHSplit(ui::Current()->width - sidebar_width, ui::Current()->width);
        ui::Enclose();

        // Prerequisites
        ui::Write(nodes[preview_tech].name);
        if (nodes[preview_tech].prerequisites.size > 0) {
            ui::Write("Prerequisites: ");
            for (int i=0; i < nodes[preview_tech].prerequisites.size; i++) {
                int prereq_index = IdGetIndex(nodes[preview_tech].prerequisites[i]);
                if (node_unlocked[prereq_index] < 1) {
                    ui::Current()->text_color = Palette::red;
                }
                ui::Write(TextFormat("- %s", nodes[prereq_index].name));
                ui::Current()->text_color = Palette::ui_main;
            }
        }

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
        
        // Draw Progress
        ui::Write("Unlock conditions: ");
        ui::VSpace(6);
        DrawResearchProgressRecursive(nodes[preview_tech].condition_index);

        ui::Pop();  // Side-bar
    }
    ui::Pop();  // Global
}

int LoadTechTree(const DataNode *data) {
    return GetTechTree()->Load(data);
}
