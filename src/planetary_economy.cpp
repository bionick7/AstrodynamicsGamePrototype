#include "planetary_economy.hpp"
#include "global_state.hpp"
#include "ui.hpp"
#include "debug_drawing.hpp"
#include "utils.hpp"
#include "constants.hpp"
#include "string_builder.hpp"

static ResourceData global_resource_data[resources::MAX];

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
        resource_capacity[i] = 100;
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

void PlanetaryEconomy::Update(RID planet) {
    if (global_resource_data[0].resource_index < 0) {
        FAIL("Resources uninitialized")
    }
    for (int i=0; i < resources::MAX; i++) {
        resource_delta[i] = writable_resource_delta[i] + native_resource_delta[i];
        writable_resource_delta[i] = 0;
    }
    if (GetCalendar()->IsNewDay()) {
        GetTechTree()->ReportResourceProduction(resource_delta);
        IDList ship_list;
        GetShips()->GetOnPlanet(&ship_list, planet, 
                                ship_selection_flags::GetAllegianceFlags(GetPlanet(planet)->allegiance));
        int total_storage = 0;
        for(int i=0; i < ship_list.Count(); i++) {
            total_storage += GetShip(ship_list[i])->stats[ship_stats::INDUSTRIAL_STORAGE];
        }
        for (int i=0; i < resources::MAX; i++) {
            bool larger_qtt = i == resources::WATER || i == resources::ROCK;
            resource_capacity[i] = (1 + total_storage) * (larger_qtt ? 500 : 100);
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
        // 2. update noise (independent of natural state)
        // 3. update final
        resource_noise[i] = Clamp(
            randomgen::GetRandomGaussian(-resource_noise[i] * 0.5, GetResourceData(i)->cost_volatility),
            -GetResourceData(i)->max_noise * 0.5,
            GetResourceData(i)->max_noise * 0.5
        );
    }
    for(int i = 0; i < resources::MAX * (PRICE_TREND_SIZE - 1); i++) {
        price_history[i] = price_history[i + resources::MAX];
    }
    RecalculateEconomy();
}

void PlanetaryEconomy::RecalculateEconomy() {
    if (global_resource_data[0].resource_index < 0) {
        FAIL("Resources uninitialized")
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
    if (global_resource_data[0].resource_index < 0) {
        FAIL("Resources uninitialized")
    }

    TransferPlan* plan = NULL;
    const Ship* ship = NULL;
    const ShipClass* ship_class = NULL;
    bool take_input = false;
    resource_count_t absolute_max = 0;

    bool is_in_transferplan = GetTransferPlanUI()->IsActive();

    if (is_in_transferplan) {
        plan = GetTransferPlanUI()->plan;
        ship = GetShip(GetTransferPlanUI()->ship);
        ship_class = GetShipClassByRID(ship->ship_class);

        take_input = is_in_transferplan && (plan->departure_planet == planet || plan->arrival_planet == planet);
        absolute_max = ship->GetRemainingPayloadCapacity(plan->tot_dv);
    }
    
    // Collect available resources and fuels
    int practically_available_fuels = 0;
    List<int> drawn_resources = List<int>();
    for (int i=0; i < resources::MAX; i++) {
        bool skip = true;
        if (resource_stock[i] != 0 || resource_delta[i] != 0) {
            skip = false;
        } else if (take_input && plan->resource_transfer[i] != 0) {
            skip = false;
        }
        if (!skip) {
            drawn_resources.Append(i);
            if (is_in_transferplan && ship_class->CanUseFuel((resources::T) i)) 
                practically_available_fuels++;
        }
    }
    
    int total_size = (DEFAULT_FONT_SIZE + 4) * (drawn_resources.Count());
    int available_size = ui::Current()->height - ui::Current()->y_cursor;
    if (total_size > available_size) {
        ui::PushScrollInset(0, available_size, total_size, &resources_scroll);
    }

    for (int idx=0; idx < drawn_resources.Count(); idx++) {
        static char buffer[50];
        //sprintf(buffer, "%-10s %5d/%5d (%+3d)", GetResourceData(i)->name, qtt, cap, delta);
        /*ui::DrawLimitedSlider(
            resource_stock[i], 0, resource_capacity[i], INT32_MIN, 
            120, 20, Palette::ui_alt, Palette::ui_dark
        );*/

        // Decide when to skip
        int rsc = drawn_resources[idx];

        if (resource_delta[rsc] == 0 || is_in_transferplan) {
            char grow_sign = ' ';
            if (resource_delta[rsc] > 0) grow_sign = '+';
            else if (resource_delta[rsc] < 0) grow_sign = '-';
            sprintf(buffer, "%2s %3d %c", GetResourceUIRep(rsc), resource_stock[rsc], grow_sign);
        } else {
            sprintf(buffer, "%2s %3d : %+2d ", GetResourceUIRep(rsc), resource_stock[rsc], resource_delta[rsc]);
        }        

        ui::PushInset(DEFAULT_FONT_SIZE + 4);
        Vector2 cursor_pos = ui::Current()->GetTextCursor();
        Rectangle button_rect = {cursor_pos.x, cursor_pos.y, 40, 20};
        ui::WriteEx(buffer, text_alignment::CONFORM, false);
        if (CheckCollisionPointRec(GetMousePosition(), button_rect)) {
            StringBuilder sb;
            sb.Add(resources::icons[rsc]).Add(resources::names[rsc]);
            sb.Add("\n").Add(GetUI()->GetConceptDescription(resources::names[rsc]));
            ui::SetMouseHint(sb.c_str);
        }
        if (take_input) {
            // Slider
            ui::Current()->x_cursor = 100;
            if (rsc == plan->fuel_type && plan->departure_planet == planet) {
                resource_count_t current = plan->resource_transfer[rsc] + plan->fuel;
                resource_count_t absolute_max_fuel = ship->GetRemainingPayloadCapacity(0);
                resource_count_t relative_max = absolute_max - plan->GetPayloadMass() + plan->resource_transfer[rsc] + plan->fuel;
                resource_count_t new_current = ui::DrawLimitedSlider(
                    current, 0, absolute_max_fuel, relative_max, 
                    150, 20, Palette::interactable_main, Palette::interactable_alt
                );
                if (new_current < plan->fuel) new_current = plan->fuel;
                plan->resource_transfer[rsc] = new_current - plan->fuel;
            } else {
                resource_count_t relative_max = absolute_max - plan->GetPayloadMass() + plan->resource_transfer[rsc];
                if (relative_max < 0) relative_max = 0;
                plan->resource_transfer[rsc] = ui::DrawLimitedSlider(
                    plan->resource_transfer[rsc], 0, absolute_max, relative_max, 
                    150, 20, Palette::interactable_main, 
                    rsc == plan->fuel_type ? Palette::interactable_alt : Palette::ui_alt
                );
            }
            ui::Current()->x_cursor += 160;
            
            // Fuel selection button
            if (ship_class->CanUseFuel((resources::T) rsc) && practically_available_fuels > 1) {
                ui::PushInline(20, 20);
                button_state_flags::T button_state = ui::AsButton();
                bool is_fuel = plan->fuel_type == rsc;
                if (button_state & button_state_flags::HOVER || is_fuel) {
                    ui::EncloseEx(0, ui::Current()->background_color, Palette::interactable_main, 0);
                } else {
                    ui::EncloseEx(0, ui::Current()->background_color, Palette::ui_main, 0);
                }
                HandleButtonSound(button_state);
                if (button_state & button_state_flags::PRESSED) {
                    //plan->fuel_type = i;  Not yet
                }
                ui::Pop();  // Inline
                ui::HSpace(8);
            } else {
                ui::HSpace(28);
            }

            // Delta text
            resource_count_t qtt = plan->resource_transfer[rsc];
            if (rsc == plan->fuel_type && plan->departure_planet == planet) {
                qtt += plan->fuel;
            }
            if (plan->departure_planet == planet) qtt *= -1;
            if (qtt != 0) {
                sprintf(buffer, "%+3d", qtt);
                ui::WriteEx(buffer, text_alignment::CONFORM, false);
            }
        }
        //ui::FillLine(resource_stock[i] / (double)resource_capacity[i], Palette::ui_main, Palette::bg);
        ui::Pop();  // Inset
        //TextBoxLineBreak(&tb);
    }
    if (total_size > available_size) {
        ui::Pop();  // ScrollInset
    }
}

void _UIDrawResourceGraph(const cost_t price_history[], int resource_index) {
    TextBox* box = ui::Current();
    ResourceData& r_data = global_resource_data[resource_index];
    int graph_height =  r_data.max_cost - r_data.min_cost;

    int current_graph_x = 0;
    int current_graph_y = price_history[resource_index] - r_data.min_cost;
    int current_draw_x = box->x + current_graph_x * box->width / PRICE_TREND_SIZE;
    int current_draw_y = box->y + box->height - current_graph_y * box->height / graph_height;
    for (int i=1; i < PRICE_TREND_SIZE; i++){
        current_graph_x = i;
        current_graph_y = price_history[i * resources::MAX + resource_index] - r_data.min_cost;
        int next_draw_x = box->x + current_graph_x * box->width / PRICE_TREND_SIZE;
        int next_draw_y = box->y + box->height - current_graph_y * box->height / graph_height;
        DrawLine(current_draw_x, current_draw_y, next_draw_x, next_draw_y, Palette::ui_main);
        current_draw_x = next_draw_x;
        current_draw_y = next_draw_y;
    }
    DrawLine(current_draw_x, current_draw_y, box->x, current_draw_y, Palette::interactable_main);
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
        ui::PushInset(DEFAULT_FONT_SIZE+4);
        resources::T resource = (resources::T) i;

        /*if (GetTransferPlanUI()->IsActive()) {
            // Button
            if (ui::ToggleButton(transfer.resource_id == i) & button_state_flags::JUST_PRESSED) {
                GetTransferPlanUI()->SetResources::T(resource);
            }
        }*/

        StringBuilder sb = StringBuilder();
        sb.AddFormat("%-10s", resources::names[i]).AddCost(resource_price[i]);
        //sprintf(buffer, "%-10sM§M  %+3fK /cnt", GetResourceData(i)->name, resource_price[i]/1e3);
        ui::WriteEx(sb.c_str, text_alignment::CONFORM, false);

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
            ui::WriteEx(sb.c_str, text_alignment::CONFORM, false);
        }

        resource_count_t trade_amount = 0;
        if (ui::DirectButton("+ 10", 0) & button_state_flags::JUST_PRESSED) trade_amount = 10;
        if (ui::DirectButton("+ 1", 0) & button_state_flags::JUST_PRESSED) trade_amount = 1;
        if (ui::DirectButton("- 1", 0) & button_state_flags::JUST_PRESSED) trade_amount = -1;
        if (ui::DirectButton("- 10", 0) & button_state_flags::JUST_PRESSED) trade_amount = -10;

        if (trade_amount != 0) {
            TryPlayerTransaction(resource, trade_amount);
        }
            
        ui::Pop();  // Inset
        ui::PushInset(32);
            _UIDrawResourceGraph(price_history, i);
        ui::Pop();  // Inset
        //TextBoxLineBreak(&tb);
    }
}

int LoadResources(const DataNode* data) {
    for (int i=0; i < resources::MAX; i++) {
        GetResourceData(i)->resource_index = i;
        const DataNode* dn = data->GetChild(resources::names[i]);
        if (dn == NULL) {
            continue;
        }
        GetResourceData(i)->description = PermaString(dn->Get("description", "[DESCRIPTION MISSING]"));
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
