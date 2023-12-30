#include "planet.hpp"
#include "global_state.hpp"
#include "ui.hpp"
#include "utils.hpp"
#include "constants.hpp"
#include "ship_modules.hpp"

Planet::Planet(const char* p_name, double p_mu, double p_radius) {
    strcpy(name, p_name);
    mu = p_mu;
    radius = p_radius;

    for (int i = 0; i < MAX_PLANET_BUILDINGS; i++) {
        buildings[i] = BuildingInstance(BUILDING_INDEX_INVALID);
    }
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
        resource_node->SetI(GetResourceData(resource_index).name, economy.resource_stock[resource_index]);
        resource_delta_node->SetI(GetResourceData(resource_index).name, economy.resource_delta[resource_index]);
    }

    // buildings
    int last_building_index = 0;
    for(int i=0; i < MAX_PLANET_BUILDINGS; i++) 
        if (buildings[i].IsValid()) last_building_index = i;

    data->SetArray("buildings", last_building_index);
    for(int i=0; i < last_building_index; i++) {
        const BuildingClass* bc = GetBuildingByIndex(buildings[i].class_index);
        data->SetArrayElem("buildings", i, bc->id);
    }
    
    // modules
    data->SetArray("ship_modules", MAX_PLANET_INVENTORY);
    for(int i=0; i < MAX_PLANET_INVENTORY; i++) {
        if (!IsIdValid(ship_module_inventory[i])) continue;
        const ShipModuleClass* smc = GetModuleByIndex(ship_module_inventory[i]);
        data->SetArrayElem("ship_modules", i, smc->id);
    }

    // We assume the orbital info is stored in the ephemerides
}

void Planet::Deserialize(Planets* planets, const DataNode *data) {
    //SettingOverridePush("datanode_quite", true);
    //SettingOverridePop("datanode_quite");
    strcpy(name, data->Get("name", name, true));
    int index = planets->GetIndexByName(name);
    if (index < 0) {
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
            economy.resource_stock[resource_index] = resource_node->GetI(GetResourceData(resource_index).name, 0, true);
        }
    }
    const DataNode* resource_delta_node = data->GetChild("resource_delta", true);
    if (resource_delta_node != NULL) {
        for (int resource_index=0; resource_index < RESOURCE_MAX; resource_index++) {
            economy.resource_delta[resource_index] = resource_delta_node->GetI(GetResourceData(resource_index).name, 0, true);
        }
    }

    if (data->Has("buildings")) {
        int initial_building_count = data->GetArrayLen("buildings", true);
        if (initial_building_count > MAX_PLANET_BUILDINGS) {
            initial_building_count = MAX_PLANET_BUILDINGS;
        }
        
        for (int i = 0; i < initial_building_count; i++) {
            const char* building_id = data->GetArray("buildings", i);
            buildings[i] = BuildingInstance(GetBuildingIndexById(building_id));
        }
    }
    
    if (data->Has("ship_modules")) {
        int ship_module_inventory_count = data->GetArrayLen("ship_modules", true);
        if (ship_module_inventory_count > MAX_PLANET_INVENTORY) {
            ship_module_inventory_count = MAX_PLANET_INVENTORY;
        }
        
        for (int i = 0; i < ship_module_inventory_count; i++) {
            const char* module_id = data->GetArray("ship_modules", i);
            ship_module_inventory[i] = GetBuildingIndexById(module_id);
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
    
    for (int i = 0; i < STAT_MAX; i++){
        stats[i] = 0;
    }

    for (int i = 0; i < MAX_PLANET_BUILDINGS; i++) {
        if (buildings[i].IsValid()) {
            buildings[i].Effect(&economy.resource_delta[0], &stats[0]);
        }
    }
}

void Planet::RequestBuild(int slot, building_index_t building_class) {
    const BuildingClass* bc = GetBuildingByIndex(building_class);
    for (int resource_index=0; resource_index < RESOURCE_MAX; resource_index++) {
        if (bc->build_costs[resource_index] > economy.resource_stock[resource_index]) {
            USER_INFO("Not enough %s (%f available, %f required)", 
                GetResourceData(resource_index).name,
                economy.resource_stock[resource_index],
                bc->build_costs[resource_index]
            )
            return;
        }
    }
    BuildingInstance instance = BuildingInstance(building_class);
    buildings[slot] = instance;

    for (int resource_index=0; resource_index < RESOURCE_MAX; resource_index++) {
        economy.resource_stock[resource_index] -= bc->build_costs[resource_index];
    }
}

bool Planet::AddShipModuleToInventory(entity_id_t module_) {
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

void _UIDrawStats(const resource_count_t stats[]) {
    for (int i=0; i < STAT_MAX; i++) {
        char buffer[50];
        sprintf(buffer, "%-10s %3d", stat_names[i], stats[i]);
        UIContextWrite(buffer);
        //TextBoxLineBreak(&tb);
    }
}

int Planet::UIDrawInventory() {
    // Draw inventory
    const int columns = 4;
    int rows = (int) std::ceil(MAX_PLANET_INVENTORY / columns);
    UIContextPushInset(0, UIContextCurrent().height - UIContextCurrent().y_cursor);
    
    int res = -1;
    for (int i = 0; i < MAX_PLANET_INVENTORY; i++) {
    //for (int column = 0; column < columns; column++)
    //for (int i = column; i < MAX_PLANET_BUILDINGS; i += columns) {
        if (!IsIdValid(ship_module_inventory[i])) continue;
        UIContextPushGridCell(columns, rows, i % columns, i / columns);
        if (DrawShipModule(GetModuleByIndex(ship_module_inventory[i]), false) == ShipModuleClass::SELECT) {
            res = i;
        }
        UIContextPop();  // GridCell
    }
    UIContextPop();  // Inset
    return res;
}

int current_tab = 0;  // Global variable, I suppose
void Planet::DrawUI(const CoordinateTransform* c_transf, bool upper_quadrant, ResourceTransfer transfer, double fuel_draw) {
    if (upper_quadrant) {
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
    //_UIDrawStats(stats);
    switch (current_tab) {
    case 0:
        economy.UIDrawResources(transfer, fuel_draw);
        break;
    case 1:
        economy.UIDrawEconomy(transfer, fuel_draw);
        break;
    case 2:
        UIDrawInventory();
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

entity_id_t Planets::AddPlanet(const DataNode* data) {
    //entity_map.insert({uuid, planet_entity});
    entity_id_t id = GetIndexByName(data->Get("name", "UNNAMED"));
    if (id < 0);
    planet_array[id].Deserialize(this, data);
    planet_array[id].Update();
    planet_array[id].id = id;
    return id;
}

Planet* Planets::GetPlanet(entity_id_t id) const {
    if (id >= planet_count) {
        FAIL("Invalid planet id (%d)", id)
    }
    return &planet_array[(int)id];
}

const PlanetNature* Planets::GetPlanetNature(entity_id_t id) const {
    if (id >= planet_count) {
        FAIL("Invalid planet id (%d)", id)
    }
    return &ephemerides[(int)id];
}

entity_id_t Planets::GetPlanetCount() const {
    return planet_count;
}

entity_id_t Planets::GetIndexByName(const char* planet_name) const {
    // Returns NULL if planet_name not found
    for(int i=0; i < planet_count; i++) {
        if (strcmp(ephemerides[i].name, planet_name) == 0) {
            return i;
        }
    }
    ERROR("No such planet '%s' ", planet_name)
    return -1;
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
    //entity_id_t planets[100];
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

Planet* GetPlanet(entity_id_t id) { return GlobalGetState()->planets.GetPlanet(id); }
int LoadEphemerides(const DataNode* data) { return GlobalGetState()->planets.LoadEphemerides(data); }