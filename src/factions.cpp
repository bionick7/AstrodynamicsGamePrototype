#include "factions.hpp"
#include "global_state.hpp"

void Faction::AssignID(int p_id) {
    id = p_id;
    ai_information.faction = p_id;
}

void Faction::Serialize(DataNode *dn) const {
    dn->SetF("money", money);
}

void Faction::Deserialize(const DataNode *dn) {
    money = dn->GetF("money", 0);
}

bool Faction::DoesControlPlanet(RID planet_id) const {
    // Unoccupied planets return true for all faction
    for (auto it = GlobalGetState()->ships.alloc.GetIter(); it; it++) {
        Ship* ship = GetShip(it.GetId());
        if (planet_id == ship->GetParentPlanet() && ship->allegiance != id) {
            return false;
        }
    }
    return true;
}
