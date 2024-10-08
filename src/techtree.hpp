#ifndef TECHTREE_H
#define TECHTREE_H

#include "datanode.hpp"
#include "id_allocator.hpp"
#include "ui.hpp"
#include "planetary_economy.hpp"
#include "global_logic.hpp"


struct TechTreeNode {
    // Primary info
    PermaString name;
    PermaString description;
    const char* str_id;  // Not owning. Is stored in the map GlobalState
    IDList attached_components;
    IDList prerequisites;
    List<global_vars::Effect> effects;
    double default_status = -1;
    AtlasPos icon_index;
    int condition_index;
    Vector2 draw_pos;
};

struct TechTree {
    const int sidebar_width = 400;

    int nodes_count = 0;
    TechTreeNode* nodes = NULL;  // Static
    int* node_unlocked = NULL;

    int layers = 0;
    int max_indices_in_layer = 0;
    int ui_selected_tech = -1;
    int ui_hovered_tech = -1;

    ResearchCondition* research_conditions;
    int research_condition_count;

    bool* visited_planets = NULL;

    Vector2 ui_camera_offset = {0};

    int Load(const DataNode* data);
    int LoadResearchCondition(const DataNode* data, int idx, int child_indices);
    void Update();
    void UpdateTechProgress();
    bool IsUnlocked(RID entity_class) const;
    void UnlockTechnology(RID technode_id, bool notify);

    void Serialize(DataNode* data) const;
    void Deserialize(const DataNode* data);

    void ReportProduction(RID item);
    void ReportResourceProduction(const resource_count_t production[]);

    void GetAttachedConditions(int condition_index, List<int>* condition_indices) const;

    Vector2 GetNodePos(const TechTreeNode* node) const;
    void DrawResearchProgressRecursive(int condition_index) const;
    void DrawUI();
};

int LoadTechTree(const DataNode* data);
#endif  // TECHTREE_H