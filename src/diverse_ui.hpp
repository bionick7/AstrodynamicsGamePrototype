#ifndef DIVERSE_UI_H
#define DIVERSE_UI_H

#include "global_state.hpp"

namespace Focusables {
    enum T {
        QUEST_MANAGER = 0,
        TIMELINE = 1,
        COMBAT_LOG = 2,
        TECHTREE = 3,
        BUILDING_CONSTRUCTION,
        TRANSFER_PLAN_UI,
        PLANET_SHIP_DETAILS,
        MAP,
    };
}

namespace panel_management {
    void HandleDeselect(GlobalState* gs);
    void DrawUIPanels(GlobalState* gs);
    Focusables::T GetCurrentFocus();

    bool GetUIPanelVisibility(Focusables::T index);
    void SetUIPanelVisibility(Focusables::T index, bool value);
}

void DrawPauseMenu();

#endif  // DIVERSE_UI_H