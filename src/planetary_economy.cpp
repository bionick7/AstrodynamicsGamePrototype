#include "planetary_economy.hpp"
#include "global_state.hpp"
#include "ui.hpp"
#include "debug_drawing.hpp"
#include "utils.hpp"
#include "constants.hpp"
#include "string_builder.hpp"

static ResourceData global_resource_data[resources::MAX] = {{""}};

double ResourceCountsToKG(resource_count_t counts) {
    return counts * KG_PER_COUNT;
}

resource_count_t KGToResourceCounts(double mass) {
    return mass / KG_PER_COUNT;
}

PlanetaryEconomy::PlanetaryEconomy(){
    for (int i=0; i < resources::MAX; i++) {
        resource_stock[i] = 0;
        resource_delta[i] = 0;
        native_resource_delta[i] = 0;
        writable_resource_delta[i] = 0;
        resource_capacity[i] = 1000;
        resource_price[i] = GetResourceData(i)->default_cost;
    }
    for (int i=0; i < resources::MAX*PRICE_TREND_SIZE; i++) {
        price_history[i] = resource_price[i % resources::MAX];
        delivered_resources_today[i] = 0;
    }
}

resource_count_t PlanetaryEconomy::TakeResource(resources::T resource_id, resource_count_t quantity) {
    // Returns how much of the resource the planet was able to give
    if (resource_id < 0 || resource_id >= resources::MAX) {
        return 0;
    }
    
    resource_count_t transferred_resources = ClampInt(quantity, 0, resource_stock[resource_id]);
    resource_stock[resource_id] -= transferred_resources;

    return transferred_resources;
}

resource_count_t PlanetaryEconomy::GiveResource(resources::T resource_id, resource_count_t quantity) {
    // Returns how much of the resource the planet was able to take
    if (resource_id < 0 || resource_id >= resources::MAX) {
        return 0;
    }

    resource_count_t transferred_resources = ClampInt(quantity, 0, resource_capacity[resource_id] - resource_stock[resource_id]);
    resource_stock[resource_id] += transferred_resources;
    delivered_resources_today[resource_id] += transferred_resources;

    return transferred_resources;
}

void PlanetaryEconomy::AddResourceDelta(resources::T resource_id, resource_count_t quantity) {
    writable_resource_delta[resource_id] += quantity;
}

cost_t PlanetaryEconomy::GetPrice(resources::T resource, resource_count_t quantity) const {
    return quantity * resource_price[resource];
}

resource_count_t PlanetaryEconomy::GetForPrice(resources::T resource, cost_t price) const {
    return price / resource_price[resource];
}

void PlanetaryEconomy::Update() {
    if (*global_resource_data[0].name == '\n') {
        FAIL("Resources uninititalized")
    }
    for (int i=0; i < resources::MAX; i++) {
        resource_delta[i] = writable_resource_delta[i] + native_resource_delta[i];
        writable_resource_delta[i] = 0;
    }
    if (GetCalendar()->IsNewDay()) {
        for (int i=0; i < resources::MAX; i++) {
            resource_stock[i] = Clamp(resource_stock[i] + resource_delta[i], 0, resource_capacity[i]);
            delivered_resources_today[i] = 0;
        }
        AdvanceEconomy();
    }
}

void PlanetaryEconomy::AdvanceEconomy() {
    // Call every day
    for (int i=0; i < resources::MAX; i++){
        // 1. calculate 'natural state' according to supply and demand
        // 2. update noise (independant of natural state)
        // 3. update final
        resource_noise[i] = Clamp(
            GetRandomGaussian(-resource_noise[i] * 0.5, GetResourceData(i)->cost_volatility),
            -GetResourceData(i)->max_noise * 0.5,
            GetResourceData(i)->max_noise * 0.5
        );
    }
    for(int i = 0; i < resources::MAX * (PRICE_TREND_SIZE - 1); i++) {
        price_history[i] = price_history[i + resources::MAX];
    }
    RecalcEconomy();
}

void PlanetaryEconomy::RecalcEconomy() {
    if (global_resource_data[0].name[0] == 0) {
        FAIL("Resources uninititalized")
    }
    for (int i = 0; i < resources::MAX; i++){
        cost_t natural_cost = GetResourceData(i)->default_cost;  // TBD
        resource_price[i] = Clamp(
            resource_noise[i] + natural_cost,
            GetResourceData(i)->min_cost,
            GetResourceData(i)->max_cost
        );
        price_history[resources::MAX*(PRICE_TREND_SIZE-1) + i] = resource_price[i];
    }
}

int resources_scroll = 0;
void PlanetaryEconomy::UIDrawResources(RID planet) {
    if (global_resource_data[0].name[0] == 0) {
        FAIL("Resources uninititalized")
    }
    int total_size = (DEFAULT_FONT_SIZE + 4 + 8) * (resources::MAX + 1);
    int available_size = ui::Current()->height - ui::Current()->y_cursor;
    if (total_size > available_size) {
        ui::PushScrollInset(0, available_size, total_size, &resources_scroll);
    }
    TransferPlan* plan = NULL;
    bool take_input = false;
    resource_count_t absolute_max = 0;
    if (GetTransferPlanUI()->IsActive()) {
        plan = GetTransferPlanUI()->plan;
        take_input = GetTransferPlanUI()->IsActive() && (plan->departure_planet == planet || plan->arrival_planet == planet);
        const Ship* ship = GetShip(GetTransferPlanUI()->ship);
        absolute_max = ship->GetRemainingPayloadCapacity(plan->tot_dv);
    }
    for (int i=0; i < resources::MAX; i++) {
        char buffer[50];
        //sprintf(buffer, "%-10s %5d/%5d (%+3d)", GetResourceData(i)->name, qtt, cap, delta);
        ui::DrawLimitedSlider(
            resource_stock[i], 0, resource_capacity[i], INT32_MIN, 
            120, 20, Palette::ui_alt, Palette::ui_dark
        );
        if (resource_delta[i] == 0) {
            sprintf(buffer, "%2s %3d       ", GetResourceUIRep(i), resource_stock[i]);
        } else {
            sprintf(buffer, "%2s %3d : %+2d ", GetResourceUIRep(i), resource_stock[i], resource_delta[i]);
        }        

        ui::PushInset(0, DEFAULT_FONT_SIZE + 4);
        Vector2 cursor_pos = ui::Current()->GetTextCursor();
        Rectangle button_rect = {cursor_pos.x, cursor_pos.y, 16, 16};
        ui::WriteEx(buffer, TextAlignment::CONFORM, false);
        if (CheckCollisionPointRec(GetMousePosition(), button_rect)) {
            //ui::SetMouseHint(GetUI()->GetConceptDescription(resources::names[i]));
            ui::SetMouseHint(resources::names[i]);
        }
        if (take_input) {
            // Slider
            ui::Current()->x_cursor = 140;
            if (i == plan->fuel_type && plan->departure_planet == planet) {
                const Ship* ship = GetShip(GetTransferPlanUI()->ship);
                resource_count_t current = plan->resource_transfer[i] + plan->fuel;
                resource_count_t absolute_max_fuel = ship->GetRemainingPayloadCapacity(0);
                resource_count_t relative_max = absolute_max - plan->GetPayloadMass() + plan->resource_transfer[i] + plan->fuel;
                resource_count_t new_current = ui::DrawLimitedSlider(
                    current, 0, absolute_max_fuel, relative_max, 
                    120, 20, Palette::interactable_main, Palette::interactable_alt
                );
                if (new_current < plan->fuel) new_current = plan->fuel;
                plan->resource_transfer[i] = new_current - plan->fuel;
            } else {
                resource_count_t relative_max = absolute_max - plan->GetPayloadMass() + plan->resource_transfer[i];
                if (relative_max < 0) relative_max = 0;
                if (i == plan->fuel_type) {
                    plan->resource_transfer[i] = ui::DrawLimitedSlider(
                        plan->resource_transfer[i], 0, absolute_max, relative_max, 
                        120, 20, Palette::interactable_main, Palette::interactable_alt
                    );
                } else {
                    plan->resource_transfer[i] = ui::DrawLimitedSlider(
                        plan->resource_transfer[i], 0, absolute_max, relative_max, 
                        120, 20, Palette::ui_main, Palette::ui_alt
                    );
                }
            }
        }

        if (plan != NULL) {
            resource_count_t qtt = plan->resource_transfer[i];
            if (i == plan->fuel_type && plan->departure_planet == planet) {
                qtt += plan->fuel;
            }
            if (plan->departure_planet == planet) qtt *= -1;
            if (qtt != 0) {
                sprintf(buffer, "   %+3d", qtt);
                ui::WriteEx(buffer, TextAlignment::CONFORM, false);
            }
        }
        //ui::Fillline(resource_stock[i] / (double)resource_capacity[i], Palette::ui_main, Palette::bg);
        ui::Pop();  // Inset
        //TextBoxLineBreak(&tb);
    }
    if (total_size > available_size) {
        ui::Pop();  // ScrollInset
    }
}

void _UIDrawResourceGrpah(const cost_t price_history[], int resource_index) {
    TextBox* box = ui::Current();
    ResourceData& r_data = global_resource_data[resource_index];
    int graph_height =  r_data.max_cost - r_data.min_cost;

    int current_graph_x = 0;
    int current_graph_y = price_history[resource_index] - r_data.min_cost;
    int current_draw_x = box->text_start_x + current_graph_x * box->width / PRICE_TREND_SIZE;
    int current_draw_y = box->text_start_y + box->height - current_graph_y * box->height / graph_height;
    for (int i=1; i < PRICE_TREND_SIZE; i++){
        current_graph_x = i;
        current_graph_y = price_history[i * resources::MAX + resource_index] - r_data.min_cost;
        int next_draw_x = box->text_start_x + current_graph_x * box->width / PRICE_TREND_SIZE;
        int next_draw_y = box->text_start_y + box->height - current_graph_y * box->height / graph_height;
        DrawLine(current_draw_x, current_draw_y, next_draw_x, next_draw_y, Palette::ui_main);
        current_draw_x = next_draw_x;
        current_draw_y = next_draw_y;
    }
    DrawLine(current_draw_x, current_draw_y, box->text_start_x, current_draw_y, Palette::interactable_main);
}

void PlanetaryEconomy::TryPlayerTransaction(resources::T resource_id, resource_count_t quantity) {
    int faction = GetFactions()->player_faction;
    if (quantity < 0) {  // Sell
        resource_count_t actual = -quantity;
        GetFactions()->CompleteTransaction(faction, GetPrice(resource_id, quantity));
    }
    else if (quantity > 0) {  // Buy
        quantity = fmin(GetForPrice(resource_id, GetFactions()->GetMoney(faction)), quantity);
        resource_count_t actual = GiveResource(resource_id, quantity);
        GetFactions()->CompleteTransaction(faction, -GetPrice(resource_id, actual));
    }
}

void PlanetaryEconomy::UIDrawEconomy(RID planet) {
    for (int i=0; i < resources::MAX; i++) {
        //char buffer[50];
        //sprintf(buffer, "%-10s %5d/%5d (%+3d)", GetResourceData(i)->name, qtt, cap, delta);
        ui::PushInset(0, DEFAULT_FONT_SIZE+4);
        resources::T resource = (resources::T) i;

        /*if (GetTransferPlanUI()->IsActive()) {
            // Button
            if (ui::ToggleButton(transfer.resource_id == i) & ButtonStateFlags::JUST_PRESSED) {
                GetTransferPlanUI()->Setresources::T(resource);
            }
        }*/

        StringBuilder sb = StringBuilder();
        sb.AddFormat("%-10s", GetResourceData(i)->name).AddCost(resource_price[i]);
        //sprintf(buffer, "%-10sM§M  %+3fK /cnt", GetResourceData(i)->name, resource_price[i]/1e3);
        ui::WriteEx(sb.c_str, TextAlignment::CONFORM, false);

        resource_count_t qtt = 0;
        /*if (transfer.resource_id == i) {
            qtt += transfer.quantity;
        }
        if (fuel.quantity > 0 && i == fuel.resource_id) {
            qtt -= fuel.quantity;
        }*/
        if (qtt != 0) {
            sb.Clear();
            sb.AddI(qtt).Add("(").AddCost(GetPrice(resource, qtt)).Add(")");
            ui::WriteEx(sb.c_str, TextAlignment::CONFORM, false);
        }

        resource_count_t trade_ammount = 0;
        if (ui::DirectButton("+ 10", 0) & ButtonStateFlags::JUST_PRESSED) trade_ammount = 10;
        if (ui::DirectButton("+ 1", 0) & ButtonStateFlags::JUST_PRESSED) trade_ammount = 1;
        if (ui::DirectButton("- 1", 0) & ButtonStateFlags::JUST_PRESSED) trade_ammount = -1;
        if (ui::DirectButton("- 10", 0) & ButtonStateFlags::JUST_PRESSED) trade_ammount = -10;

        if (trade_ammount != 0) {
            TryPlayerTransaction(resource, trade_ammount);
        }
            
        ui::Pop();  // Inset
        ui::PushInset(0, 32);
            _UIDrawResourceGrpah(price_history, i);
        ui::Pop();  // Inset
        //TextBoxLineBreak(&tb);
    }
}

int LoadResources(const DataNode* data) {
    for (int i=0; i < resources::MAX; i++) {
        strcpy(GetResourceData(i)->name, resources::names[i]);
    }
    for (int i=0; i < resources::MAX; i++) {
        const DataNode* dn = data->GetChild(GetResourceData(i)->name);
        if (dn == NULL) {
            continue;
        }
        strncpy(GetResourceData(i)->descrption, dn->Get("description", "[DESCRITION MISSING]"), RESOURCE_DESCRIPTION_MAX_SIZE);
        GetResourceData(i)->min_cost = dn->GetF("min_cost", 0, true);
        GetResourceData(i)->max_cost = dn->GetF("max_cost", GetResourceData(i)->min_cost, true);
        GetResourceData(i)->default_cost = dn->GetF("default_cost", (GetResourceData(i)->max_cost + GetResourceData(i)->min_cost) / 2, true);
        GetResourceData(i)->cost_volatility = dn->GetF("cost_volatility", GetResourceData(i)->default_cost - GetResourceData(i)->min_cost, true);
        GetResourceData(i)->max_noise = dn->GetF("max_noise", 0, true);
    }
    return resources::MAX;
}

ResourceData* GetResourceData(int resource_index) {
    return &global_resource_data[resource_index];
}

int FindResource(const char* name, int default_) {
    for(int i=0; i < resources::MAX; i++) {
        if (strcmp(resources::names[i], name) == 0) {
            return i;
        }
    }
    return default_;
}

const char* GetResourceUIRep(int resource_index) {
    //return resources::icons + 3*resource_index;
    return resources::icons[resource_index];
    //return resources::names[resource_index];
}
