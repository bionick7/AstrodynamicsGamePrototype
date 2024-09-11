#include "combat.hpp"
#include "ship.hpp"
#include "global_state.hpp"
#include "constants.hpp"

bool ShipBattle(const IDList* ships_aggressor, const IDList* ships_defender, double relative_velocity) {
    // Returns true if aggressor wins
    int velocity_multiplier = relative_velocity / 3000 + 1;

    // Pick ship order (alternating)
    // Foreach ship:
    //     pick opponent (random, strongest, ...)
    //     tally dammages
    //     remove if necaissary
    // Sort by initiative instead

    BattleLog* output_log = GetBattleLog();
    output_log->Clear();

    int total_ships = ships_aggressor->size + ships_defender->size;
    IDList turn_order = IDList(total_ships);
    // Aggressors or defender first?
    for(int i=0; i < ships_aggressor->size; i++) {
        turn_order.Append(ships_aggressor->Get(i));
    }
    for(int i=0; i < ships_defender->size; i++) {
        turn_order.Append(ships_defender->Get(i));
    }
    IDList ships_aggr_strength = IDList(*ships_aggressor);
    IDList ships_defs_strength = IDList(*ships_defender);

    /*turn_order.Sort([](RID ship1, RID ship2) {
        return GetShip(ship2)->stats[ship_stats::INITIATIVE] - GetShip(ship1)->stats[ship_stats::INITIATIVE];
    });*/

    ships_aggr_strength.Sort([](RID ship1, RID ship2){
        return GetShip(ship2)->GetCombatStrength() - GetShip(ship1)->GetCombatStrength();
    });

    ships_defs_strength.Sort([](RID ship1, RID ship2){
        return GetShip(ship2)->GetCombatStrength() - GetShip(ship1)->GetCombatStrength();
    });

    IDList killed = IDList();

    bool agressor_won;
    int leading_turns = 0;
    for (int i=0; i < total_ships; i++) {
        if (GetShip(turn_order[i])->initiative() > leading_turns) {
            leading_turns = GetShip(turn_order[i])->initiative();
        }
    }
    for (int turn_nr=-leading_turns; true; turn_nr++) {
        for (int i=0; i < total_ships; i++) {
            if (killed.Find(turn_order[i]) >= 0) {
                continue;
            }
            Ship* actor_ship = GetShip(turn_order[i]);
            if (actor_ship->initiative() < -turn_nr) {
                continue;
            }
            bool is_agressor = ships_aggr_strength.Find(turn_order[i]) >= 0;
            const IDList* target_ship_list = is_agressor ? &ships_defs_strength : &ships_aggr_strength;
            
            int target_index = 0;
            RID target_id;
            do {
                if (target_index >= target_ship_list->size) {
                    // Kill all encaissary ships
                    for(int i=0; i < killed.size; i++) {
                        RID kill_id = killed.Get(i);
                        GetShips()->KillShip(kill_id, true);
                    }
                    // Force pause
                    GetCalendar()->paused = true;
                    output_log->shown = true;

                    return is_agressor;
                }
                target_id = target_ship_list->Get(target_index++);
            } while (killed.Find(target_id) >= 0);
            Ship* target_ship = GetShip(target_id);

            int kinetic_dammage = actor_ship->kinetic_offense() * velocity_multiplier - actor_ship->kinetic_defense();
            if (kinetic_dammage < 0) kinetic_dammage = 0;
            int ordonance_dammage = actor_ship->ordnance_offense() - actor_ship->ordnance_defense();
            if (ordonance_dammage < 0) ordonance_dammage = 0;

            target_ship->dammage_taken[ship_variables::KINETIC_ARMOR] += kinetic_dammage;
            target_ship->dammage_taken[ship_variables::ENERGY_ARMOR] += ordonance_dammage;

            output_log->AppendAttackLog(turn_nr, actor_ship, target_ship, is_agressor);
            if(
                target_ship->dammage_taken[ship_variables::KINETIC_ARMOR] > target_ship->stats[ship_stats::KINETIC_HP]
                || target_ship->dammage_taken[ship_variables::ENERGY_ARMOR] > target_ship->stats[ship_stats::ENERGY_HP]
            ) {
                output_log->AppendKillLog(turn_nr, actor_ship, target_ship, is_agressor);
                killed.Append(target_id);
            }
        }
    }

    GetTechTree()->ReportAchievement("archvmt_battle");

    //INFO("Party 1 (%d) received %d dammage. Party 2 (%d) received (%d) dammage", ships_1->size, dmg_received_1, ships_2->size, dmg_received_2)
}

void BattleLog::AppendAttackLog(int turn, const Ship* attacker, const Ship* target, bool aggressor_is_attacking) {
    //INFO("Turn %d: %s vs %s", turn, attacker->name, target_ship->name)
    //INFO("    >> %d - %d", 
    //        target_ship->dammage_taken[ship_variables::KINETIC_ARMOR], 
    //        target_ship->dammage_taken[ship_variables::ENERGY_ARMOR]
    //    )
    // '%s (00K, 00E)'
    log.AddFormat("%d: ", turn);
    if (aggressor_is_attacking) {
        log.AddFormat("'%s' <%2dK, %2dE>", attacker->name, attacker->kinetic_offense(), attacker->ordnance_offense());
        log.Add(" >> ");
        log.AddFormat("'%s' [%2dK, %2dE]\n", target->name, target->kinetic_defense(), target->ordnance_defense());
    } else {
        log.AddFormat("'%s' [%2dK, %2dE]", target->name, target->kinetic_defense(), target->ordnance_defense());
        log.Add(" << ");
        log.AddFormat("'%s' <%2dK, %2dE>\n", attacker->name, attacker->kinetic_offense(), attacker->ordnance_offense());
    }
}

void BattleLog::AppendKillLog(int turn, const Ship* attacker, const Ship* target, bool aggressor_is_attacking) {
    INFO("%s killed %s in battle", attacker->name, target->name)
    log.AddFormat("%d: ", turn);
    if (aggressor_is_attacking) {
        log.AddFormat("'%s' DEFEATED\n", target->name);
    } else {
        log.AddFormat("'%s' LOST\n", target->name);
    }
}

void BattleLog::Clear() {

}

void BattleLog::DrawUI() {
    // Draws it's own box. Clears the ui stack
    if (!GetGlobalState()->IsKeyBoardFocused() && IsKeyPressed(KEY_THREE)) {
        shown = !shown;
    }
    if (!shown) return;
    int width = 800;
    int height = 800;
    if (GetScreenWidth() - 2 * 50 < width) width = GetScreenWidth() - 2 * 50;
    if (GetScreenHeight() - 2 * 50 < height) height = GetScreenHeight() - 2 * 50;
    ui::CreateNew(
        (GetScreenWidth() - width) / 2, (GetScreenHeight() - height) / 2, width, height, 
        DEFAULT_FONT_SIZE, Palette::ui_main, Palette::bg, true
    );
    ui::Enclose();
    ui::Write(log.c_str);
}
