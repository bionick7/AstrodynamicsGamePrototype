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
#include "render_server.hpp"

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

	Ships ships;
	Planets planets;
	ShipModules ship_modules;
	QuestManager quest_manager;
	Factions factions;

	TransferPlanUI active_transfer_plan;
	BattleLog last_battle_log;

	AudioServer audio_server;
	RenderServer render_server;
	UIGlobals ui;
	WrenInterface wren_interface;

    // Lifecycle
    void Make(timemath::Time time);
    void LoadData();
    void LoadGame(const char* file_path);
    // Update
    void UpdateState(double delta_t);
    // Draw
    void DrawUI();

    bool IsKeyBoardFocused() const;

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

Ships* GetShips();
Planets* GetPlanets();
ShipModules* GetShipModules();
QuestManager* GetQuestManager();
Factions* GetFactions();

TransferPlanUI* GetTransferPlanUI();
BattleLog* GetBattleLog();

AudioServer* GetAudioServer();
RenderServer* GetRenderServer();
WrenInterface* GetWrenInterface();
UIGlobals* GetUI();

#endif // GLOBAL_STATE_H
