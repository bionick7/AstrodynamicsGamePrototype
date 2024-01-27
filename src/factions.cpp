#include "factions.hpp"
#include "global_state.hpp"

void Factions::InitializeAI() {
    for (int i=0; i < faction_count; i++) {
        ai_information[i].HighLevelFactionAI();
    }
}

void Factions::Serialize(DataNode *data) const {
    data->CreatChildArray("factions", faction_count);
    for(int i=0; i < faction_count; i++) {
        DataNode* faction_data = data->InsertIntoChildArray("factions", i);
        faction_data->SetI("money", money[i]);
    }
    data->SetI("player_faction", player_faction);
}

void Factions::Deserialize(const DataNode *data) {
    faction_count = data->GetChildArrayLen("factions");
    for(int i=0; i < faction_count; i++) {
        const DataNode* faction_data = data->GetChildArrayElem("factions", i);
        money[i] = faction_data->GetI("money");
    }
    player_faction = data->GetI("player_faction", 0);

}

// delta is the ammount of capital transferred TO the player
bool Factions::CompleteTransaction(int faction, int delta) {
    money[faction] += delta;
    return true;
}

cost_t Factions::GetMoney(int faction) {
    return money[faction];
}

void Factions::Update() {
    if (GetCalendar()->IsNewDay()) {
        for (int i=0; i < faction_count; i++) {
            ai_information[i].HighLevelFactionAI();
        }
    }
    for (int i=0; i < faction_count; i++) {
        ai_information[i].LowLevelFactionAI();
    }
}

bool Factions::DoesControlPlanet(int faction, RID planet_id) const {
    // Unoccupied planets return true for all faction
    return GetPlanet(planet_id)->allegiance == faction;
}
