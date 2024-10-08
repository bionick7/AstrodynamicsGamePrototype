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
    }
    for (int i=0; i < resources::MAX; i++) {
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

int LoadResources(const DataNode* data) {
    for (int i=0; i < resources::MAX; i++) {
        GetResourceData(i)->resource_index = i;
        const DataNode* dn = data->GetChild(resources::names[i]);
        if (dn == NULL) {
            continue;
        }
        GetResourceData(i)->description = PermaString(dn->Get("description", "[DESCRIPTION MISSING]"));
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
    return resources::icons[resource_index];
}
