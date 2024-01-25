#ifndef FACTIONS_H
#define FACTIONS_H

#include "planetary_economy.hpp"

struct Faction {
    cost_t money;

    // Serialization
    void Serialize(DataNode* dn) const;
    void Deserialize(const DataNode* dn);
};

#endif  // FACTIONS_H