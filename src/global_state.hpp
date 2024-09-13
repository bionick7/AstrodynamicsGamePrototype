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
#include "techtree.hpp"

struct GlobalState {

    RID focused_planet;
    RID focused_ship;
    RID hover;

    GameCamera camera;
	Calendar calendar;

	Ships ships;
	Planets planets;
	ShipModules ship_modules;
	QuestManager quest_manager;
	Factions factions;
    TechTree techtree;

	TransferPlanUI active_transfer_plan;
	BattleLog last_battle_log;

	AudioServer audio_server;
	RenderServer render_server;
	UIGlobals ui;
	WrenInterface wren_interface;

    std::map<std::string, RID> string_identifiers = std::map<std::string, RID>();

    // Lifecycle
    void Make(timemath::Time time);
    void LoadData();
    // Update
    void UpdateState(double delta_t);
    // Draw
    void DrawUI();

    bool IsKeyBoardFocused() const;

    const char* AddStringIdentifier(const char* string_id, RID rid);
    RID GetFromStringIdentifier(const char* string_id);


    // Serialization
    void LoadGame(const char* file_path);
    void SaveGame(const char* file_path) const;
    void Serialize(DataNode* dn) const;
    void Deserialize(const DataNode* dn);
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
TechTree* GetTechTree();

TransferPlanUI* GetTransferPlanUI();
BattleLog* GetBattleLog();

AudioServer* GetAudioServer();
RenderServer* GetRenderServer();
WrenInterface* GetWrenInterface();
UIGlobals* GetUI();

#endif // GLOBAL_STATE_H
