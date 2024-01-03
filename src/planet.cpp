#include "planet.hpp"
#include "global_state.hpp"
#include "ui.hpp"
#include "utils.hpp"
#include "constants.hpp"
#include "ship_modules.hpp"
#include "logging.hpp"

Planet::Planet(const char* p_name, double p_mu, double p_radius) {
    strcpy(name, p_name);
    mu = p_mu;
    radius = p_radius;

    for (int i = 0; i < MAX_PLANET_INVENTORY; i++) {
        ship_module_inventory[i] = GetInvalidId();
    }
}

void Planet::Serialize(DataNode* data) const {
    data->Set("name", name);
    data->Set("trading_accessible", economy.trading_accessible ? "y" : "n");
    //data->SetF("mass", mu / G);
    //data->SetF("radius", radius);

    DataNode* resource_node = data->SetChild("resource_stock", DataNode());
    DataNode* resource_delta_node = data->SetChild("resource_delta", DataNode());
    for (int resource_index=0; resource_index < RESOURCE_MAX; resource_index++) {
        resource_node->SetI(GetResourceData(resource_index)->name, economy.resource_stock[resource_index]);
        resource_delta_node->SetI(GetResourceData(resource_index)->name, economy.resource_delta[resource_index]);
    }
    
    // modules
    data->SetArray("ship_modules", MAX_PLANET_INVENTORY);
    for(int i=0; i < MAX_PLANET_INVENTORY; i++) {
        if (!IsIdValid(ship_module_inventory[i])) continue;
        const ShipModuleClass* smc = GetModule(ship_module_inventory[i]);
        data->SetArrayElem("ship_modules", i, smc->id);
    }

    // We assume the orbital info is stored in the ephemerides
}

void Planet::Deserialize(Planets* planets, const DataNode *data) {
    //SettingOverridePush("datanode_quite", true);
    //SettingOverridePop("datanode_quite");
    strcpy(name, data->Get("name", name, true));
    RID index = planets->GetIndexByName(name);
    if (!IsIdValid(index)) {
        return;
    }
    //*this = *((Planet*) (void*) &find->second);
    const PlanetNature* nature = planets->GetPlanetNature(index);
    mu = nature->mu;
    radius = nature->radius;
    orbit = nature->orbit;
    economy.trading_accessible = strcmp(data->Get("trading_accessible", economy.trading_accessible ? "y" : "n", true), "y") == 0;

    const DataNode* resource_node = data->GetChild("resource_stock", true);
    if (resource_node != NULL) {
        for (int resource_index=0; resource_index < RESOURCE_MAX; resource_index++) {
            economy.resource_stock[resource_index] = resource_node->GetI(GetResourceData(resource_index)->name, 0, true);
        }
    }
    const DataNode* resource_delta_node = data->GetChild("resource_delta", true);
    if (resource_delta_node != NULL) {
        for (int resource_index=0; resource_index < RESOURCE_MAX; resource_index++) {
            economy.resource_delta[resource_index] = resource_delta_node->GetI(GetResourceData(resource_index)->name, 0, true);
        }
    }
    
    if (data->HasArray("ship_modules")) {
        int ship_module_inventory_count = data->GetArrayLen("ship_modules", true);
        if (ship_module_inventory_count > MAX_PLANET_INVENTORY) {
            ship_module_inventory_count = MAX_PLANET_INVENTORY;
        }
        
        for (int i = 0; i < ship_module_inventory_count; i++) {
            const char* module_id = data->GetArray("ship_modules", i);
            ship_module_inventory[i] = GlobalGetState()->ship_modules.GetModuleRIDFromStringId(module_id);
        }
    }

    economy.RecalcEconomy();
    for (int i=0; i < PRICE_TREND_SIZE; i++) {
        economy.AdvanceEconomy();
    }
}

void Planet::_OnClicked() {
    TransferPlanUI& tp_ui = GlobalGetState()->active_transfer_plan;
    if (
        tp_ui.plan != NULL
        && IsIdValid(tp_ui.ship)
        && IsIdValid(tp_ui.plan->departure_planet)
        && !IsIdValid(tp_ui.plan->arrival_planet)
    ) {
        tp_ui.SetDestination(id);
    } else if (GlobalGetState()->focused_planet == id) {
        GetScreenTransform()->focus = position.cartesian;
    } else {
        GlobalGetState()->focused_planet = id;
    }
}

double Planet::ScreenRadius() const {
    return fmax(GetScreenTransform()->TransformS(radius), 4);
}

double Planet::GetDVFromExcessVelocity(Vector2 vel) const {
    return sqrt(mu / (2*radius) + Vector2LengthSqr(vel));
}

void Planet::RecalcStats() {
    // Just call this every frame tbh
    for (int i = 0; i < RESOURCE_MAX; i++){
        economy.resource_delta[i] = 0;
    }
}

bool Planet::AddShipModuleToInventory(RID module_) {
    for (int index = 0; index < MAX_PLANET_INVENTORY; index++) {
        if(!IsIdValid(ship_module_inventory[index])) {
            ship_module_inventory[index] = module_;
            return true;
        }
    }
    return false;  // No free space
}

void Planet::RemoveShipModuleInInventory(int index) {
    ship_module_inventory[index] = GetInvalidId();
}

bool Planet::HasMouseHover(double* min_distance) const {
    Vector2 screen_pos = GetScreenTransform()->TransformV(position.cartesian);
    double dist = Vector2Distance(GetMousePosition(), screen_pos);
    if (dist <= ScreenRadius() * 1.2 && dist < *min_distance) {
        *min_distance = dist;
        return true;
    } else {
        return false;
    }
}

void Planet::Update() {
    timemath::Time now = GlobalGetNow();
    position = orbit.GetPosition(now);
    // RecalcStats();
    economy.Update();
}

void Planet::Draw(const CoordinateTransform* c_transf) {
    //printf("%f : %f\n", position.x, position.y);
    Vector2 screen_pos = c_transf->TransformV(position.cartesian);
    DrawCircleV(screen_pos, ScreenRadius(), Palette::ui_main);
    
    orbit.Draw(Palette::ui_main);

    int screen_x = (int)screen_pos.x, screen_y = (int)screen_pos.y;
    int text_h = 16;
    Vector2 text_size = MeasureTextEx(GetCustomDefaultFont(), name, text_h, 1);
    DrawTextEx(GetCustomDefaultFont(), name, {screen_x - text_size.x / 2,  screen_y - text_size.y - 5}, text_h, 1, Palette::ui_main);

    if (mouse_hover) {
        // Hover
        DrawCircleLines(screen_x, screen_y, 20, Palette::ship);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            _OnClicked();
        }
    }
}

void _UIDrawStats(const resource_count_t planet_stats[]) {
    for (int i=0; i < PlanetStats::MAX; i++) {
        char buffer[50];
        sprintf(buffer, "%-10s %3d", planet_stat_names[i], planet_stats[i]);
        UIContextWrite(buffer);
        //TextBoxLineBreak(&tb);
    }
}

void Planet::_UIDrawInventory() {
    const int MARGIN = 3;
    int columns = UIContextCurrent().width / (SHIP_MODULE_WIDTH + MARGIN);
    int max_rows = (int) std::ceil(MAX_PLANET_INVENTORY / columns);
    int available_height = UIContextCurrent().height - UIContextCurrent().y_cursor;
    int height = MinInt(available_height, max_rows * (SHIP_MODULE_HEIGHT + MARGIN));
    int rows = height / (SHIP_MODULE_HEIGHT + MARGIN);
    int i_max = MinInt(rows * columns, MAX_PLANET_INVENTORY);
    UIContextPushInset(0, height);
    
    ShipModules* sms = &GlobalGetState()->ship_modules;
    for (int i = 0; i < i_max; i++) {
        //if (!IsIdValid(ship_module_inventory[i])) continue;
        UIContextPushGridCell(columns, rows, i % columns, i / columns);
        UIContextShrink(MARGIN, MARGIN);
        ButtonStateFlags button_state = UIContextAsButton();
        if (button_state & BUTTON_STATE_FLAG_HOVER) {
            current_slot = ShipModuleSlot(id, i, ShipModuleSlot::DRAGGING_FROM_PLANET);
        }
        if (button_state & BUTTON_STATE_FLAG_JUST_PRESSED) {
            sms->InitDragging(current_slot, UIContextCurrent().render_rec);
        }
        sms->DrawShipModule(ship_module_inventory[i]);
        UIContextPop();  // GridCell
    }
    UIContextPop();  // Inset
}

int current_tab = 0;  // Global variable, I suppose
void Planet::DrawUI() {
    // Reset
    current_slot = ShipModuleSlot();

    int show_quadrant = 0;
    ResourceTransfer transfer = ResourceTransfer();
    resource_count_t fuel_draw = -1;  // why not 0?

    const TransferPlan* tp = GlobalGetState()->active_transfer_plan.plan;

    if (GlobalGetState()->active_transfer_plan.IsActive()){
        if (tp->departure_planet == id) {
            show_quadrant = 1;
            transfer = tp->resource_transfer.Inverted();
            fuel_draw = tp->fuel_mass;
        }
        if (tp->arrival_planet == id) {
            show_quadrant = 2;
            transfer = tp->resource_transfer;
        }
    }
    if (mouse_hover || GlobalGetState()->focused_planet == id) {
        show_quadrant = 1;
        transfer = ResourceTransfer();
        fuel_draw = -1;
    }
    if (show_quadrant == 0) {
        return;
    }

    if (show_quadrant == 1) {
        UIContextCreateNew(10, 10, 16*30, GetScreenHeight() / 2 - 20, 16, Palette::ui_main);
    } else {
        UIContextCreateNew(10, GetScreenHeight() / 2 + 10, 16*30, GetScreenHeight() / 2 - 20, 16, Palette::ui_main);
    }
    UIContextCurrent().Enclose(2, 2, Palette::bg, Palette::ui_main);

    UIContextPushInset(4, 20);  // Tab container
    int w = UIContextCurrent().width;
    const int n_tabs = 4;
    const char* tab_descriptions[4] = {
        "resources",
        "economy",
        "inventory",
        "~quests~"
    };
    for (int i=0; i < n_tabs; i++) {
        UIContextPushHSplit(i * w / n_tabs, (i + 1) * w / n_tabs);
        ButtonStateFlags button_state = UIContextAsButton();
        HandleButtonSound(button_state & BUTTON_STATE_FLAG_JUST_PRESSED);
        if (i == 1 && !economy.trading_accessible) {
            UIContextWrite("~economy~");
            UIContextPop();
            continue;
        }
        if (button_state & BUTTON_STATE_FLAG_JUST_PRESSED) {
            current_tab = i;
        }
        if (button_state & BUTTON_STATE_FLAG_HOVER || i == current_tab) {
            UIContextEnclose(Palette::bg, Palette::ui_main);
        }
        UIContextWrite(tab_descriptions[i]);
        UIContextPop();  // HSplit
    }
    UIContextPop();  // Tab container

    UIContextWrite(name);
    UIContextFillline(1, Palette::ui_main, Palette::ui_main);
    //_UIDrawStats(planet_stats);
    switch (current_tab) {
    case 0:
        economy.UIDrawResources(transfer, fuel_draw);
        break;
    case 1:
        economy.UIDrawEconomy(transfer, fuel_draw);
        break;
    case 2:
        _UIDrawInventory();
        break;
    }

    UIContextPop();  // Outside
}

Planets::Planets() {
    planet_array = NULL;
    ephemerides = NULL;
    planet_count = 0;
}

Planets::~Planets() {
    delete[] planet_array;
    delete[] ephemerides;
}

RID Planets::AddPlanet(const DataNode* data) {
    //entity_map.insert({uuid, planet_entity});
    RID id = GetIndexByName(data->Get("name", "UNNAMED"));
    if (!IsIdValid(id)) return id;
    planet_array[IdGetIndex(id)].Deserialize(this, data);
    planet_array[IdGetIndex(id)].Update();
    planet_array[IdGetIndex(id)].id = id;
    return id;
}

Planet* Planets::GetPlanet(RID id) const {
    if (IdGetIndex(id) >= planet_count) {
        FAIL("Invalid planet id (%d)", id)
    }
    return &planet_array[IdGetIndex(id)];
}

const PlanetNature* Planets::GetPlanetNature(RID id) const {
    if (IdGetIndex(id) >= planet_count) {
        FAIL("Invalid planet id (%d)", id)
    }
    return &ephemerides[IdGetIndex(id)];
}

int Planets::GetPlanetCount() const {
    return planet_count;
}

RID Planets::GetIndexByName(const char* planet_name) const {
    // Returns NULL if planet_name not found
    for(int i=0; i < planet_count; i++) {
        if (strcmp(ephemerides[i].name, planet_name) == 0) {
            return RID(i, EntityType::PLANET);
        }
    }
    ERROR("No such planet '%s' ", planet_name)
    return GetInvalidId();
}

const PlanetNature* Planets::GetParentNature() const {
    return &parent;
}

int Planets::LoadEphemerides(const DataNode* data) {
    // Init planets
    parent = {0};
    parent.radius = data->GetF("radius");
    parent.mu = data->GetF("mass") * G;
    planet_count = data->GetArrayChildLen("satellites");
    ephemerides = new PlanetNature[planet_count];
    planet_array = new Planet[planet_count];
    //if (num_planets > 100) num_planets = 100;
    //RID planets[100];
    for(int i=0; i < planet_count; i++) {
        const DataNode* planet_data = data->GetArrayChild("satellites", i);
        PlanetNature* nature = &ephemerides[i];
        strcpy(nature->name, planet_data->Get("name"));
        
        double sma = planet_data->GetF("SMA");
        double ann = planet_data->GetF("Ann") * DEG2RAD;
        timemath::Time epoch = timemath::Time(PosMod(ann, 2*PI) / sqrt(parent.mu / (sma*sma*sma)));

        nature->mu = planet_data->GetF("mass") * G;
        nature->radius = planet_data->GetF("radius");
        nature->orbit = Orbit(
            sma,
            planet_data->GetF("Ecc"),
            (planet_data->GetF("LoA") + planet_data->GetF("AoP")) * DEG2RAD,
            parent.mu,
            epoch, 
            strcmp(planet_data->Get("retrograde", "y", true), "y") != 0
        );
    }
    return planet_count;
}

Planet* GetPlanet(RID id) { return GlobalGetState()->planets.GetPlanet(id); }
Planet* GetPlanetByIndex(int index) { 
    return GlobalGetState()->planets.GetPlanet(RID(index, EntityType::PLANET)); 
}
int LoadEphemerides(const DataNode* data) { return GlobalGetState()->planets.LoadEphemerides(data); }