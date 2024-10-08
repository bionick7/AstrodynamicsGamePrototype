#ifndef GLOBAL_LOGIC_H
#define GLOBAL_LOGIC_H

#include "basic.hpp"
#include "string_builder.hpp"
#include "list.hpp"

namespace comparison {
    enum T {
        GREATER, LESS,
        GREATER_OR_EQUAL, LESS_OR_EQUAL,
        EQUAL, NONEQUAL,

        COMPARISON_MAX
    };

    static constexpr const char* comparison_identifiers[COMPARISON_MAX] = {
        ">", "<", ">=", "<=", "=", "=/="
    };

    static constexpr const char* comparison_repr[COMPARISON_MAX] = {
        ">", "<", "\u2265", "\u2264", "=", "\u2260"
    };
}

struct ResearchCondition {
    enum Type {
        INVALID = -1,
        ANY = 0,
        ALL,
        STAT_CONDITION,
        PRODUCTION_COUNTER,
        PRODUCE_ITEM,
        EXPRESSION,
        FREE,

        TYPE_MAX
    };

    static constexpr const char* type_identifiers[TYPE_MAX] = {
        "any", "all", "stat_achieved", "production_counter",
        "produce_item", "expression", "free"
    };

    Type type;
    int internal_counter = 0;

    union {
        struct {
            int index;
            int count;
        } branch;
        struct {
            int variable;
            int value;
            comparison::T comp;
            PermaString description;
        } leaf;
    } cond {0};

    bool IsBranch() const;
    bool IsValid() const;
    float GetProgress() const;
    int GetChildCount(bool include_branches) const;
    void GenLeafFromString(Type type, const char* expression);
    void GetDescriptiveText(StringBuilder* sb) const;
};

namespace global_vars {
    struct GlobalVariable {
        char name[100];
        StrHash hash;
        int value;
    };

    struct Condition {
        int variable;
        int value;
        comparison::T comp;
    };

    struct Effect {
        int var;
        enum { SET, INC } action;
        int value;
    };

    int GetVarIndex(const char* name);
    bool HasVar(const char *name);
    int TryGetVar(StrHash name_hash);
    bool HasVar(StrHash name_hash);
    int GetVarCount();
    GlobalVariable* GetVarAt(int index);

    int GetByIndex(int index);
    int Get(const char* name);
    void SetByIndex(int index, int value);
    void Set(const char* name, int value);
    void IncByIndex(int index, int value);
    void Inc(const char* name, int value);

    void Serialize(DataNode* data);
    void Deserialize(const DataNode* data);

    Condition InterpretCondition(const char* condition);
    bool CheckCondition(Condition condition);

    void CompileEffects(const char* cmd, List<Effect>* effect_list);
    void InterpretCompiled(Effect effect);
    void Interpret(const char* cmd);
};

struct Event {
    PermaString title;
    PermaString body;
    global_vars::Condition condition;
    List<global_vars::Effect> effect;

    int event_occured_index;
};

struct GlobalLogic {
    List<global_vars::GlobalVariable> variables;
    Event* events;
    int event_count;

    int Load(const DataNode* data);
    void Update();
};

int LoadEvents(const DataNode* dn);

#endif  // GLOBAL_LOGIC_H