#ifndef TECHTREE_H
#define TECHTREE_H

#include "datanode.hpp"
#include "id_allocator.hpp"
#include "ui.hpp"

struct TechTreeNode {
    // Primary info
    char name[100] = "UNNAMED";
    const char* str_id;
    IDList attached_components;
    IDList prerequisites;
    double defualt_status = -1;
    int research_effort = 1000;
    AtlasPos icon_index;

    // Generated info (for drawing)
    int layer = -1;
    int index_in_layer = 0;
    int total_in_layer = 0;
};


struct TechTree {
    int nodes_count = 0;
    TechTreeNode* nodes = NULL;  // Static
    int* node_progress = NULL;
    int research_focus = -1;

    int layers = 0;
    int max_indecies_in_layer = 0;
    int ui_selected_tech = -1;
    int ui_hovered_tech = -1;

    void Serialize(DataNode* data) const;
    void Deserialize(const DataNode* data);

    int Load(const DataNode* data);
    void Update();
    bool IsUnlocked(RID entity_class) const;
    void ForceUnlockTechnology(const char* tech_id);

    bool shown;
    void DrawUI();
};

int LoadTechTree(const DataNode* data);
#endif  // TECHTREE_H