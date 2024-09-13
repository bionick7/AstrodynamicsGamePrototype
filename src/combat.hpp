#ifndef COMBAT_H
#define COMBAT_H

#include "basic.hpp"
#include "id_system.hpp"
#include "string_builder.hpp"
#include <vector>

struct Ship;

bool ShipBattle(const IDList* ships_1, const IDList* ships_2, double relative_velocity);

/*
  -1:: The 'Blind Rat' (K1, O3) => 'Military Ship' (K1, O2), 1 dmg
  ...
  !! The 'Blind Rat' has been destroyed
*/

struct BattleLogEntryAttack {
    int turn;
    char* attacker_name;
    char* target_name;
};

struct BattleLogEntryKill {
    
};

union BattleLogEntry {
    BattleLogEntryAttack attack;
    BattleLogEntryKill kill;
};

struct BattleLog {
    StringBuilder log;
    std::vector<BattleLogEntry> battle_logs = std::vector<BattleLogEntry>();

    void AppendAttackLog(int turn, const Ship* attacker, const Ship* target, bool aggressor_is_attacking);
    void AppendKillLog(int turn, const Ship* attacker, const Ship* target, bool aggressor_is_attackingo);
    void Clear();

    void DrawUI();
};

#endif  // COMBAT_H