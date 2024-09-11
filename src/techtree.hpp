#ifndef TECHTREE_H
#define TECHTREE_H

#include "datanode.hpp"
#include "id_allocator.hpp"
#include "ui.hpp"
#include "planetary_economy.hpp"

struct ResearchCondition {
    enum Type {
        INVALID = -1,
        ANY = 0,
        ALL,
        STAT_CONDITION,
        PRODUCTION_COUNTER,
        VISIT,
        PRODUCE_ITEM,
        ARCHIEVEMENT,
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
        "any", "all", "stat_archieved", "production_counter",
        "visit", "produce_item", "archievement", "free"
    };

    static constexpr const char* comparison_identifiers[COMPARISON_MAX] = {
        "gt", "lt", "geq", "leq", "eq", "neq"
    };

    static constexpr const char* comparison_repr[COMPARISON_MAX] = {
        ">", "<", "\u2265", "\u2264", "=", "\u2260"
    };

    Type type;
    union {
        BranchCondition branch;
        LeafCondition leaf;
    } cond;

    int internal_counter = 0;  // Relevant now only for PRODUCTION_COUNTER

    bool IsBranch() const;
    float GetProgress() const;
    int GetChildCount(bool include_branches) const;
    void GetDescriptiveText(StringBuilder* sb) const;
};

struct TechTreeNode {
    // Primary info
    char name[100] = "UNNAMED";
    const char* str_id;  // Not owning. Is stored in the map GlobalState
    IDList attached_components;
    IDList prerequisites;
    double default_status = -1;
    AtlasPos icon_index;
    int condition_index;

    // Generated info (for drawing)
    int layer = -1;
    int index_in_layer = 0;
    int total_in_layer = 0;
};

struct TechTree {
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
    int archievement_count = 0;
    bool* archeivements = NULL;

    int Load(const DataNode* data);
    int LoadResearchCondition(const DataNode* data, int idx, int child_indices);
    void Update();
    void UpdateTechProgress();
    bool IsUnlocked(RID entity_class) const;
    void ForceUnlockTechnology(RID technode_id);

    void Serialize(DataNode* data) const;
    void Deserialize(const DataNode* data);

    void ReportArchievement(int archievement);
    void ReportVisit(RID planet);
    void ReportProduction(RID item);
    void ReportResourceProduction(const resource_count_t production[]);
    
    void GetAttachedConditions(int condition_index, List<int>* condition_indices) const;

    bool shown;
    void DrawResearchProgressRecursive(int condition_index) const;
    void DrawUI();
};

int LoadTechTree(const DataNode* data);
#endif  // TECHTREE_H