#include "global_logic.hpp"
#include "id_system.hpp"
#include "ship.hpp"
#include "global_state.hpp"
#include "utils.hpp"

float _GetProgressFromComparison(int variable, int reference, ResearchCondition::Comparison comp) {
    switch (comp) {
        case ResearchCondition::GREATER:
            return Clamp(variable / (float)(reference + 1), 0, 1);
        case ResearchCondition::GREATER_OR_EQUAL:
            return Clamp(variable / (float)reference, 0, 1);
        case ResearchCondition::LESS:
            return variable < reference ? 1:0;
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

bool ResearchCondition::IsValid() const {
    if (IsBranch()) return true;
    int maximum_variable_value = INT32_MAX;
    if (type == PRODUCE_ITEM) {
        RID rid = RID(cond.leaf.variable);
        EntityType rid_type = IdGetType(rid);
        return IsIdValid(rid) && (rid_type == EntityType::SHIP_CLASS || rid_type == EntityType::MODULE_CLASS);
    }
    switch (type) {
    case STAT_CONDITION:{
        maximum_variable_value = ship_stats::MAX - 1; break;
    }
    case PRODUCTION_COUNTER:{
        maximum_variable_value = resources::MAX - 1; break;
    }
    case EXPRESSION:{
        maximum_variable_value = INT32_MAX; break;
    }
    default: break;
    }
    if (cond.leaf.variable < 0 || cond.leaf.variable > maximum_variable_value) {
        return false;
    }
    return true;
}

float ResearchCondition::GetProgress() const {
    if (!IsValid()) return 1;
    switch (type) {
    case ANY:{
        for (int i=0; i < cond.branch.count; i++) {
            if (GetTechTree()->research_conditions[cond.branch.index + i].GetProgress() >= 1) {
                return 1;
            }
        }
        return 0;
    }
    case ALL:{
        for (int i=0; i < cond.branch.count; i++) {
            if (GetTechTree()->research_conditions[cond.branch.index + i].GetProgress() < 1) {
                return 0;
            }
        }
        return 1;
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
    case EXPRESSION:{
        return _GetProgressFromComparison(global_vars::GetByIndex(cond.leaf.variable), cond.leaf.value, cond.leaf.comp);
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

void ResearchCondition::GenLeafFromString(Type p_type, const char *expression) {
    static char var_name[100];
    static char symbol[100];
    int value = 0;
    int sscanf_res = sscanf(expression, "%s %s %d", var_name, symbol, &value);
    if (sscanf_res != 3) {
        ERROR("Could not parse '%s' as a research condition leaf", expression);
    }
    
    cond.leaf.value = value;
    
    cond.leaf.comp = (ResearchCondition::Comparison) FindInArray(
        ResearchCondition::comparison_identifiers, ResearchCondition::COMPARISON_MAX, symbol);

    type = p_type;

    switch (type) {
    case ResearchCondition::STAT_CONDITION:{
        cond.leaf.variable = FindInArray(
            ship_stats::names, ship_stats::MAX, var_name);
        break;
    }
    case ResearchCondition::PRODUCTION_COUNTER:{
        cond.leaf.variable = FindInArray(
            resources::names, resources::MAX, var_name);
        break;
    }
    case ResearchCondition::PRODUCE_ITEM:{
        RID rid = GetGlobalState()->GetFromStringIdentifier(var_name);
        if (!IsIdValidTyped(rid, EntityType::SHIP_CLASS) && !IsIdValidTyped(rid, EntityType::MODULE_CLASS)) {
            cond.leaf.variable = GetInvalidId().AsInt();
            ERROR("No such product '%s'", var_name);
        } else {
            cond.leaf.variable = rid.AsInt();
        }
        break;
    }
    case ResearchCondition::EXPRESSION:{
        cond.leaf.variable = global_vars::GetVarIndex(var_name);
        break;
    }
    default:
        break;
    }
}


void ResearchCondition::GetDescriptiveText(StringBuilder *sb) const {
    if (!IsValid()) {
        sb->Add("INVALID CONDITION");
        return;
    }
    if (cond.leaf.description.offset != 0) {
        sb->AddClock(GetProgress());
        sb->AddPerma(cond.leaf.description);
        return;
    }
    switch (type) {
    case ANY:
    case ALL:{
        break;
    }
    case STAT_CONDITION:{
        sb->AddClock(GetProgress());
        sb->AddFormat(" Have a ship with %s %s %d (%3.0f %%)", ship_stats::icons[cond.leaf.variable], 
                      ResearchCondition::comparison_repr[cond.leaf.comp], cond.leaf.value, GetProgress()*100);
        break;
    }
    case PRODUCTION_COUNTER:{
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
            name = smc->name.GetChar();
        } else if (IsIdValidTyped(object_id, EntityType::SHIP_CLASS)) {
            const ShipClass* sc = GetShipClassByRID(object_id);
            name = sc->name.GetChar();
        } else {
            sb->Add("Invalid stat condition");
            break;
        }
        sb->AddClock(GetProgress());
        sb->AddFormat(" Produce %dx %s (%3.1f %%)", cond.leaf.value, name, GetProgress()*100);
        break;
    }
    case FREE:{
        sb->AddClock(GetProgress());
        sb->Add(" Free Bingo (for testing) :)");
        break;
    }
    default: break;
    }
}

namespace global_vars {   
    List<GlobalVariable> variables;
}


int global_vars::GetVarIndex(const char *name) {
    ASSERT(strlen(name) <= 100)
    StrHash name_hash = HashKey(name);
    for (int i=0; i < variables.Count(); i++) {
        if (variables[i].hash == name_hash) {
            return i;
        }
    }
    int index = variables.AllocForAppend();
    strcpy(variables[index].name, name);
    variables[index].hash = name_hash;
    variables[index].value = 0;
    return index;
}

int global_vars::GetByIndex(int index) {
    if (index < 0 || index >= variables.Count()) {
        return 0;
    }
    return variables[index].value;
}

bool global_vars::HasVar(const char *name) {
    ASSERT(strlen(name) <= 100)
    StrHash name_hash = HashKey(name);
    for (int i=0; i < variables.Count(); i++) {
        if (variables[i].hash == name_hash) {
            return true;
        }
    }
    return false;
}

int global_vars::GetVarCount() {
    return variables.Count();
}

global_vars::GlobalVariable* global_vars::GetVarAt(int index) {
    return &variables[index];
}

int global_vars::Get(const char *name) {
    return GetByIndex(GetVarIndex(name));
}

void global_vars::SetByIndex(int index, int value) {
    variables[index].value = value;
}

void global_vars::Set(const char *name, int value) {
    SetByIndex(GetVarIndex(name), value);
}

void global_vars::IncByIndex(int index, int value) {
    variables[index].value += value;
}

void global_vars::Inc(const char *name, int value) {
    IncByIndex(GetVarIndex(name), value);
}

void global_vars::Serialize(DataNode *data) {
    for (int i=0; i < variables.Count(); i++) {
        data->SetI(variables[i].name, variables[i].value);
    }
}

void global_vars::Deserialize(const DataNode *data) {
    variables.Clear();
    for (int i=0; i < data->GetFieldCount(); i++) {
        const char* name = data->GetKey(i);
        Set(name, data->GetI(name));
    }
}

void global_vars::CompileEffects(const char *cmd, List<Effect> *effect_list) {
    static char var_name[100];
    static char symbol[100];
    int value = 0;
    
    char* current_line = new char[strlen(cmd) + 1];
    strcpy(current_line, cmd);
    while(current_line) {
        int sscanf_res = sscanf(current_line, "%s %s %d\n", var_name, symbol, &value);
        char* next_line = strchr(current_line, '\n');
        if (current_line[0] == '\n' || current_line[0] == '\0') {
            current_line = next_line ? (next_line+1) : NULL;
            continue;
        }
        current_line = next_line ? (next_line+1) : NULL;
        if (sscanf_res != 3) {
            ERROR("Could not interpret '%s' as global variable setting", current_line);
        }

        int index = effect_list->AllocForAppend();
        effect_list->buffer[index].var = GetVarIndex(var_name);

        if (strcmp(symbol, ":=") == 0) {
            effect_list->buffer[index].action = Effect::SET;
            effect_list->buffer[index].value = value;
        } else if (strcmp(symbol, "+=") == 0) {
            effect_list->buffer[index].action = Effect::INC;
            effect_list->buffer[index].value = value;
        } else if (strcmp(symbol, "-=") == 0) {
            effect_list->buffer[index].action = Effect::INC;
            effect_list->buffer[index].value = -value;
        }
    }
    delete[] current_line;
}

void global_vars::InterpretCompiled(Effect effect) {
    switch (effect.action) {
    case Effect::SET:
        SetByIndex(effect.var, effect.value);
        break;
    case Effect::INC:
        IncByIndex(effect.var, effect.value);
        break;
    }
}

void global_vars::Interpret(const char *cmd) {
    List<Effect> effects;
    CompileEffects(cmd, &effects);
    for (int i=0; i < effects.Count(); i++) {
        InterpretCompiled(effects[i]);
    }
}
