#ifndef GLOBAL_STATE_H
#define GLOBAL_STATE_H

#include "id_system.hpp"
#include "planet.hpp"
#include "ship.hpp"
#include "transfer_plan.hpp"
#include "datanode.hpp"
#include "coordinate_transform.hpp"
#include "quest_manager.hpp"
#include "audio_server.hpp"
#include "wren_interface.hpp"
#include "combat.hpp"
#include "factions.hpp"

struct GlobalState {
    enum FocusablesPanels {
        COMBAT_LOG,
        QUEST_MANAGER,
        TIMELINE,
        BUILDING_CONSTRUCTION,
        TRANSFER_PLAN_UI,
        PLANET_SHIP_DETAILS,
        MAP,
    };

    FocusablesPanels current_focus;
    RID focused_planet;
    RID focused_ship;

    GameCamera camera;
	Calendar calendar;
	TransferPlanUI active_transfer_plan;
	QuestManager quest_manager;
	Ships ships;
	Planets planets;
	ShipModules ship_modules;
	AudioServer audio_server;
	WrenInterface wren_interface;
	UIGlobals ui;
	BattleLog last_battle_log;
	Factions factions;

    // Lifecycle
    void Make(timemath::Time time);
    void LoadData();
    void LoadGame(const char* file_path);
    // Update
    void UpdateState(double delta_t);
    // Draw
    void DrawState();

    // Interaction
    bool CompleteTransaction(int faction, int delta);
    cost_t GetMoney(int faction);

    // Serialization
    void Serialize(DataNode* dn) const;
    void Deserialize(const DataNode* dn);

    //entt::registry registry;
    void _InspectState();
};

GlobalState* GetGlobalState();
timemath::Time GlobalGetNow();
GameCamera* GetCamera();
Calendar* GetCalendar();
TransferPlanUI* GetTransferPlanUI();
QuestManager* GetQuestManager();
Ships* GetShips();
Planets* GetPlanets();
ShipModules* GetShipModules();
AudioServer* GetAudioServer();
WrenInterface* GetWrenInterface();
UIGlobals* GetUI();
BattleLog* GetBattleLog();
Factions* GetFactions();

#endif // GLOBAL_STATE_H
