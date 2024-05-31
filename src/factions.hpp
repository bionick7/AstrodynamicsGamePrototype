#ifndef FACTIONS_H
#define FACTIONS_H

#include "ai.hpp"

#define MAX_FACTIONS 8

namespace diplomatic_status {
    typedef uint32_t T;
    const static int AT_WAR = 1UL;
    const static int CAN_TRADE = 2UL;
    const static int HAS_TRUCE = 4UL;
    // etc.
}


struct Factions {
    int faction_count;
    int player_faction;

    double reputation_matrix[MAX_FACTIONS*MAX_FACTIONS];
    diplomatic_status::T diplomacy_matrix[MAX_FACTIONS*MAX_FACTIONS];

    cost_t money[MAX_FACTIONS];
    //AIBlackboard ai_information[MAX_FACTIONS];

    constexpr static uint32_t GetAllegianceFlags(int faction) { return 1U << faction; }

    void InitializeAI();

    bool CompleteTransaction(int faction, int delta);
    cost_t GetMoney(int faction);
    void Update();

    // Serialization
    void Serialize(DataNode* dn) const;
    void Deserialize(const DataNode* dn);

    bool CanTradeWithPlanet(int faction, RID planet_id) const;
    bool DoesControlPlanet(int faction, RID planet_id) const;
};

#endif  // FACTIONS_H