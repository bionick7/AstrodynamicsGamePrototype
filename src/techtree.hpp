#ifndef TECHTREE_H
#define TECHTREE_H

#include "datanode.hpp"
#include "id_allocator.hpp"
#include "ui.hpp"

struct ResearchCondition {
    enum Type {
        INVALID = -1,
        ANY = 0,
        ALL,
        STAT_CONDITION,
        PRODUCTION_COUNTER,
        FREE,

        TYPE_MAX
    };

    enum Comparison {
        GREATER, LESS,
        GREATER_OR_EQUAL, LESS_OR_EQUAL,
        EQUAL, NONEQUAL,

        COMPARISON_MAX
    };

    struct BranchCondition {
        int index;
        int count;
    };

    struct LeafCondition {
        int variable;
        Comparison comp;
        int value;
    };

    static constexpr const char* type_identifiers[TYPE_MAX] = {
        "any", "all", "stat_archieved", "production_counter", "free"
    };

    static constexpr const char* comparison_identifiers[COMPARISON_MAX] = {
        "gt", "lt", "geq", "leq", "eq", "neq"
    };

    Type type;
    union {
        BranchCondition branch;
        LeafCondition leaf;
    } cond;

    float GetProgress();
};

struct TechTreeNode {
    // Primary info
    char name[100] = "UNNAMED";
    const char* str_id;  // Not owning. Is stored in the map GlobalState
    IDList attached_components;
    IDList prerequisites;
    double default_status = -1;
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
    const char** node_names_ptrs = NULL;  // Needed for serialization, not owning
    int research_focus = -1;

    int daily_progress = 0;

    int layers = 0;
    int max_indecies_in_layer = 0;
    int ui_selected_tech = -1;
    int ui_hovered_tech = -1;

    ResearchCondition* research_conditions;
    int research_condition_count;

    void Serialize(DataNode* data) const;
    void Deserialize(const DataNode* data);

    int Load(const DataNode* data);
    int LoadResearchCondition(const DataNode* data, int idx, int child_indices);
    void Update();
    bool IsUnlocked(RID entity_class) const;
    void ForceUnlockTechnology(const char* tech_id);

    bool shown;
    void DrawUI();
};

int LoadTechTree(const DataNode* data);
#endif  // TECHTREE_H