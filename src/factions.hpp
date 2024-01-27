#ifndef FACTIONS_H
#define FACTIONS_H

#include "ai.hpp"

struct Faction {
    cost_t money;
    int id;
    AIBlackboard ai_information;

    void AssignID(int p_id);

    // Serialization
    void Serialize(DataNode* dn) const;
    void Deserialize(const DataNode* dn);

    bool DoesControlPlanet(RID planet_id) const;
};

#endif  // FACTIONS_H