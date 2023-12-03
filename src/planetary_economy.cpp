#include "planetary_economy.hpp"
#include "global_state.hpp"
#include "ui.hpp"
#include "utils.hpp"
#include "constants.hpp"

static ResourceData global_resource_data[RESOURCE_MAX] = {{""}};

PlanetaryEconomy::PlanetaryEconomy(){
    for (int i=0; i < RESOURCE_MAX; i++) {
        resource_stock[i] = 0;
        resource_delta[i] = 0;
        resource_capacity[i] = 1e7;
        resource_price[i] = GetResourceData(i).default_cost;
    }
    for (int i=0; i < RESOURCE_MAX*PRICE_TREND_SIZE; i++) {
        price_history[i] = resource_price[i % RESOURCE_MAX];
    }
    for (int i=0; i < PRICE_TREND_SIZE; i++) {
        AdvanceEconomy();
    }
}

resource_count_t PlanetaryEconomy::DrawResource(ResourceType resource, resource_count_t quantity) {
    // Returns how much of the resource the planet was able to give
    if (resource < 0 || resource >= RESOURCE_MAX) {
        return 0;
    }
    
    resource_count_t transferred_resources = Clamp(quantity, 0, resource_stock[resource]);
    resource_stock[resource] -= transferred_resources;
    //GlobalGetState()->CompleteTransaction(-GetPrice(resource, quantity), "purchased resource");

    return transferred_resources;
}

resource_count_t PlanetaryEconomy::GiveResource(ResourceType resource, resource_count_t quantity) {
    // Returns how much of the resource the planet was able to take
    if (resource < 0 || resource >= RESOURCE_MAX) {
        return 0;
    }

    resource_count_t transferred_resources = Clamp(quantity, 0, resource_capacity[resource]);
    resource_stock[resource] += transferred_resources;
    //GlobalGetState()->CompleteTransaction(-GetPrice(resource, quantity), "sold resource");

    return transferred_resources;
}

cost_t PlanetaryEconomy::GetPrice(ResourceType resource, resource_count_t quantity) const {
    return quantity * resource_price[resource];
}

resource_count_t PlanetaryEconomy::GetForPrice(ResourceType resource, cost_t price) const {
    return price / resource_price[resource];
}

void PlanetaryEconomy::Update() {
    if (global_resource_data[0].name == 0) {
        FAIL("Resources uninititalized")
    }
    Time now = GlobalGetNow();
    Time prev = GlobalGetPreviousFrameTime();
    double delta_T = TimeDays(TimeSub(now, prev));
    for (int i=0; i < RESOURCE_MAX; i++) {
        resource_stock[i] = Clamp(resource_stock[i] + resource_delta[i] * delta_T, 0, resource_capacity[i]);
    }
    if ((int)TimeDays(prev) != (int)TimeDays(now)) {
        AdvanceEconomy();
    }
}

void PlanetaryEconomy::AdvanceEconomy() {
    // Call every day
    for (int i = 0; i < RESOURCE_MAX; i++){
        // 1. calculate 'natural state' according to supply and demand
        // 2. update noise (independant of natural state)
        // 3. update final
        resource_noise[i] = Clamp(
            GetRandomGaussian(-resource_noise[i] * 0.5, GetResourceData(i).cost_volatility),
            -GetResourceData(i).max_noise * 0.5,
            GetResourceData(i).max_noise * 0.5
        );
    }
    for(int i = 0; i < RESOURCE_MAX * (PRICE_TREND_SIZE - 1); i++) {
        price_history[i] = price_history[i + RESOURCE_MAX];
    }
    RecalcEconomy();
}

void PlanetaryEconomy::RecalcEconomy() {
    if (global_resource_data[0].name[0] == 0) {
        FAIL("Resources uninititalized")
    }
    for (int i = 0; i < RESOURCE_MAX; i++){
        cost_t natural_cost = GetResourceData(i).default_cost;  // TBD
        resource_price[i] = Clamp(
            resource_noise[i] + natural_cost,
            GetResourceData(i).min_cost,
            GetResourceData(i).max_cost
        );
        price_history[RESOURCE_MAX*(PRICE_TREND_SIZE-1) + i] = resource_price[i];
    }
}

void PlanetaryEconomy::UIDrawResources(const ResourceTransfer& transfer, double fuel_draw) {
    if (global_resource_data[0].name[0] == 0) {
        FAIL("Resources uninititalized")
    }
    for (int i=0; i < RESOURCE_MAX; i++) {
        char buffer[50];
        //sprintf(buffer, "%-10s %5d/%5d (%+3d)", GetResourceData(i).name, qtt, cap, delta);
        sprintf(buffer, "%-10s %3.1fT (%+3d T/d)", GetResourceData(i).name, resource_stock[i] / 1e3, (int)(resource_delta[i]/1e3));
        UIContextPushInset(0, 18);
        if (GlobalGetState()->active_transfer_plan.IsActive()) {
            // Button
            if (UIContextDirectButton(transfer.resource_id == i ? "X" : " ", 2) & BUTTON_STATE_FLAG_JUST_PRESSED) {
                GlobalGetState()->active_transfer_plan.SetResourceType((ResourceType) i);
            }
        }
        UIContextWrite(buffer, false);

        double qtt = 0;
        if (transfer.resource_id == i) {
            qtt += transfer.quantity;
        }
        if (fuel_draw > 0 && i == RESOURCE_WATER) {
            qtt -= fuel_draw;
        }
        if (qtt != 0) {
            sprintf(buffer, "   %+3.1fK", qtt / 1000);
            UIContextWrite(buffer, false);
        }
        UIContextFillline(resource_stock[i] / resource_capacity[i], MAIN_UI_COLOR, BG_COLOR);
        UIContextPop();  // Inset
        //TextBoxLineBreak(&tb);
    }
}

void _UIDrawResourceGrpah(const double price_history[], int resource_index) {
    TextBox& box = UIContextCurrent();
    ResourceData& r_data = global_resource_data[resource_index];
    int graph_height =  r_data.max_cost - r_data.min_cost;

    int current_graph_x = 0;
    int current_graph_y = price_history[resource_index] - r_data.min_cost;
    int current_draw_x = box.text_start_x + current_graph_x * box.width / PRICE_TREND_SIZE;
    int current_draw_y = box.text_start_y + box.height - current_graph_y * box.height / graph_height;
    for (int i=1; i < PRICE_TREND_SIZE; i++){
        current_graph_x = i;
        current_graph_y = price_history[i * RESOURCE_MAX + resource_index] - r_data.min_cost;
        int next_draw_x = box.text_start_x + current_graph_x * box.width / PRICE_TREND_SIZE;
        int next_draw_y = box.text_start_y + box.height - current_graph_y * box.height / graph_height;
        DrawLine(current_draw_x, current_draw_y, next_draw_x, next_draw_y, MAIN_UI_COLOR);
        current_draw_x = next_draw_x;
        current_draw_y = next_draw_y;
    }
    DrawLine(current_draw_x, current_draw_y, box.text_start_x, current_draw_y, PALETTE_BLUE);
}

void PlanetaryEconomy::TryPlayerTransaction(ResourceTransfer tf) {
    if (tf.quantity < 0) {  // Sell
        resource_count_t actual_ammount = DrawResource(tf.resource_id, -tf.quantity);
        GlobalGetState()->CompleteTransaction(GetPrice(tf.resource_id, actual_ammount), "Sold on market");
    }
    else if (tf.quantity > 0) {  // Buy
        tf.quantity = fmin(GetForPrice(tf.resource_id, GlobalGetState()->capital), tf.quantity);
        resource_count_t actual_ammount = GiveResource(tf.resource_id, tf.quantity);
        GlobalGetState()->CompleteTransaction(-GetPrice(tf.resource_id, actual_ammount), "Bought on market");
    }
}

void PlanetaryEconomy::UIDrawEconomy(const ResourceTransfer& transfer, double fuel_draw) {
    for (int i=0; i < RESOURCE_MAX; i++) {
        char buffer[50];
        //sprintf(buffer, "%-10s %5d/%5d (%+3d)", GetResourceData(i).name, qtt, cap, delta);
        UIContextPushInset(0, 18);
        ResourceType resource = (ResourceType) i;

        if (GlobalGetState()->active_transfer_plan.IsActive()) {
            // Button
            if (UIContextDirectButton(transfer.resource_id == i ? "X" : " ", 2) & BUTTON_STATE_FLAG_JUST_PRESSED) {
                GlobalGetState()->active_transfer_plan.SetResourceType(resource);
            }
        }
        sprintf(buffer, "%-10s %+3f $/T", GetResourceData(i).name, resource_price[i]);
        UIContextWrite(buffer, false);

        double qtt = 0;
        if (transfer.resource_id == i) {
            qtt += transfer.quantity;
        }
        if (fuel_draw > 0 && i == RESOURCE_WATER) {
            qtt -= fuel_draw;
        }
        if (qtt != 0) {
            sprintf(buffer, "   %+3.1fT (%3.1f K$)", qtt / 1000, GetPrice(resource, qtt) / 1000);
            UIContextWrite(buffer, false);
        }

        double trade_ammount = 0;
        if (UIContextDirectButton("-100T", 0) & BUTTON_STATE_FLAG_JUST_PRESSED) trade_ammount =-1e5;
        if (UIContextDirectButton( "-10T", 0) & BUTTON_STATE_FLAG_JUST_PRESSED) trade_ammount =-1e4;
        if (UIContextDirectButton( "+10T", 0) & BUTTON_STATE_FLAG_JUST_PRESSED) trade_ammount = 1e4;
        if (UIContextDirectButton("+100T", 0) & BUTTON_STATE_FLAG_JUST_PRESSED) trade_ammount = 1e5;

        if (trade_ammount != 0) {
            TryPlayerTransaction({resource, trade_ammount});
        }
            
        UIContextPop();  // Inset
        UIContextPushInset(0, 32);
            _UIDrawResourceGrpah(price_history, i);
        UIContextPop();  // Inset
        //TextBoxLineBreak(&tb);
    }
}

int LoadResources(const DataNode* data) {
    for (int i=0; i < RESOURCE_MAX; i++) {
        strcpy(GetResourceData(i).name, resource_names[i]);
    }
    for (int i=0; i < RESOURCE_MAX; i++) {
        const DataNode* dn = data->GetChild(GetResourceData(i).name);
        strncpy(GetResourceData(i).descrption, dn->Get("description", "[DESCRITION MISSING]"), RESOURCE_DESCRIPTION_MAX_SIZE);
        GetResourceData(i).min_cost = dn->GetF("min_cost", 0);
        GetResourceData(i).max_cost = dn->GetF("max_cost", GetResourceData(i).min_cost);
        GetResourceData(i).default_cost = dn->GetF("default_cost", (GetResourceData(i).max_cost + GetResourceData(i).min_cost) / 2);
        GetResourceData(i).cost_volatility = dn->GetF("cost_volatility", GetResourceData(i).default_cost - GetResourceData(i).min_cost);
        GetResourceData(i).max_noise = dn->GetF("max_noise", 0);
    }
    return 0;
}

ResourceData& GetResourceData(int resource_index) {
    return global_resource_data[resource_index];
}
