#include "factions.hpp"

void Faction::Serialize(DataNode *dn) const {
    dn->SetF("money", money);
}

void Faction::Deserialize(const DataNode *dn) {
    money = dn->GetF("money", 0);
}