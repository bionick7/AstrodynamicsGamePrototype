#include "task.hpp"
#include "constants.hpp"
#include "string_builder.hpp"
#include "global_state.hpp"

void _ClearTask(Task* quest) {
    quest->departure_planet = GetInvalidId();
    quest->arrival_planet = GetInvalidId();
    quest->current_planet = GetInvalidId();
    quest->ship = GetInvalidId();
    quest->quest = GetInvalidId();

    quest->payload_mass = 0;
    quest->pickup_expiration_time = timemath::Time::GetInvalid();
    quest->delivery_expiration_time = timemath::Time::GetInvalid();

    quest->payout = 0;
}

Task::Task() {
    _ClearTask(this);
}

/*void Task::CopyFrom(const Task* other) {
    departure_planet = other->departure_planet;
    arrival_planet = other->arrival_planet;
    current_planet = other->current_planet;
    ship = other->ship;

    payload_mass = other->payload_mass;
    pickup_expiration_time = other->pickup_expiration_time;
    delivery_expiration_time = other->delivery_expiration_time;

    payout = other->payout;
}*/

void Task::Serialize(DataNode* data) const {
    data->SetI("departure_planet", departure_planet);
    data->SetI("arrival_planet", arrival_planet);
    data->SetI("current_planet", current_planet);
    data->SetI("ship", ship);
    data->SetI("quest", quest);

    data->SetF("payload_mass", payload_mass);
    data->SetDate("pickup_expiration_time", pickup_expiration_time);
    data->SetDate("delivery_expiration_time", delivery_expiration_time);
    data->SetF("payout", payout);
}

void Task::Deserialize(const DataNode* data) {
    departure_planet =          data->GetI("departure_planet", departure_planet);
    arrival_planet =            data->GetI("arrival_planet", arrival_planet);
    current_planet =            data->GetI("current_planet", current_planet);
    ship =                      data->GetI("ship", ship);
    quest =                     data->GetI("quest", quest);
    payload_mass =              data->GetF("payload_mass", payload_mass);
    pickup_expiration_time =    data->GetDate("pickup_expiration_time", pickup_expiration_time);
    delivery_expiration_time =  data->GetDate("delivery_expiration_time", delivery_expiration_time);
    payout =                    data->GetF("payout", payout);
}

bool Task::IsValid() const {
    return IsIdValid(departure_planet) && IsIdValid(arrival_planet);
}

ButtonStateFlags Task::DrawUI(bool show_as_button, bool highlight) const {
    // Assumes parent UI Context exists
    // Resturns if player wants to accept
    if (!IsValid()) {
        return BUTTON_STATE_FLAG_NONE;
    }

    int height = UIContextPushInset(3, TASK_PANEL_HEIGHT);

    if (height == 0) {
        UIContextPop();
        return BUTTON_STATE_FLAG_NONE;
    }
    if (highlight) {
        UIContextEnclose(Palette::bg, Palette::ship);
    } else {
        UIContextEnclose(Palette::bg, Palette::ui_main);
    }
    UIContextShrink(6, 6);
    ButtonStateFlags button_state = UIContextAsButton();
    if (show_as_button) {
        HandleButtonSound(button_state & (BUTTON_STATE_FLAG_JUST_HOVER_IN | BUTTON_STATE_FLAG_JUST_PRESSED));
    }
    if (height != TASK_PANEL_HEIGHT) {
        UIContextPop();
        return button_state & BUTTON_STATE_FLAG_JUST_PRESSED;
    }

    StringBuilder sb_mouse;
    double dv1, dv2;
    HohmannTransfer(&GetPlanet(departure_planet)->orbit, &GetPlanet(arrival_planet)->orbit, GlobalGetNow(), NULL, NULL, &dv1, &dv2);
    sb_mouse.AddFormat("DV: %.3f km/s\n", (dv1 + dv2) / 1000);

    StringBuilder sb;
    // Line 1
    sb.AddFormat("%s >> %s  %d cts", GetPlanet(departure_planet)->name, GetPlanet(arrival_planet)->name, KGToResourceCounts(payload_mass));
    if (button_state == BUTTON_STATE_FLAG_HOVER) {
        UISetMouseHint(sb_mouse.c_str);
    }
    if (IsIdValid(current_planet)) {
        sb.Add("  Now: [").Add(GetPlanet(current_planet)->name).AddLine("]");
    } else sb.AddLine("");
    // Line 2
    bool is_in_transit = IsIdValid(ship) && !GetShip(ship)->is_parked;
    if (is_in_transit) {
        sb.Add("Expires in ").AddTime(delivery_expiration_time - GlobalGetNow());
    } else {
        sb.Add("Expires in ").AddTime(pickup_expiration_time - GlobalGetNow());
    }
    sb.AddFormat("  => ").AddCost(payout);
    UIContextWrite(sb.c_str);

    UIContextPop();
    return button_state;
}