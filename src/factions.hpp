#ifndef FACTIONS_H
#define FACTIONS_H

#include "ai.hpp"

struct Factions {
    int faction_count;
    int player_faction;

    cost_t money[8];
    AIBlackboard ai_information[8];

    void InitializeAI();

    bool CompleteTransaction(int faction, int delta);
    cost_t GetMoney(int faction);
    void Update();

    // Serialization
    void Serialize(DataNode* dn) const;
    void Deserialize(const DataNode* dn);

    bool DoesControlPlanet(int faction, RID planet_id) const;
};

#endif  // FACTIONS_H