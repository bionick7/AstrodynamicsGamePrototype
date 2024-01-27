#include "planet.hpp"
#include "global_state.hpp"
#include "ui.hpp"
#include "utils.hpp"
#include "constants.hpp"
#include "ship_modules.hpp"
#include "logging.hpp"
#include "string_builder.hpp"

Planet::Planet(const char* p_name, double p_mu, double p_radius) {
    strcpy(name, p_name);
    mu = p_mu;
    radius = p_radius;
    ship_production_queue.Clear();
    module_production_queue.Clear();
    ship_production_process = 0;
    module_production_process = 0;

    for (int i = 0; i < MAX_PLANET_INVENTORY; i++) {
        ship_module_inventory[i] = GetInvalidId();
    }
}

void Planet::Serialize(DataNode* data) const {
    data->Set("name", name);
    data->Set("trading_accessible", economy.trading_accessible ? "y" : "n");
    data->SetI("ship_production_process", ship_production_process);
    data->SetI("module_production_process", module_production_process);
    data->SetI("allegiance", allegiance);
    ship_production_queue.SerializeTo(data, "ship_production_queue");
    module_production_queue.SerializeTo(data, "module_production_queue");
    //data->SetF("mass", mu / G);
    //data->SetF("radius", radius);

    DataNode* resource_node = data->SetChild("resource_stock");
    DataNode* resource_delta_node = data->SetChild("resource_delta");
    for (int resource_index=0; resource_index < RESOURCE_MAX; resource_index++) {
        resource_node->SetI(GetResourceData(resource_index)->name, economy.resource_stock[resource_index]);
        resource_delta_node->SetI(GetResourceData(resource_index)->name, economy.resource_delta[resource_index]);
    }
    
    // modules
    data->CreateArray("inventory", MAX_PLANET_INVENTORY);
    for(int i=0; i < MAX_PLANET_INVENTORY; i++) {
        if (IsIdValid(ship_module_inventory[i])) {
            data->InsertIntoArray("inventory", i, GetModule(ship_module_inventory[i])->id);
        } else {
            data->InsertIntoArray("inventory", i, "---");
        }
    }

    // We assume the orbital info is stored in the ephemerides
}

void Planet::Deserialize(Planets* planets, const DataNode *data) {
    //SettingOverridePush("datanode_quite", true);
    //SettingOverridePop("datanode_quite");
    strcpy(name, data->Get("name", name, true));
    ship_production_process = data->GetI("ship_production_process", 0, true);
    module_production_process = data->GetI("module_production_process", 0, true);
    allegiance = data->GetI("allegiance", 0);
    ship_production_queue.DeserializeFrom(data, "ship_production_queue", true);
    module_production_queue.DeserializeFrom(data, "module_production_queue", true);
    RID index = planets->GetIndexByName(name);
    if (!IsIdValid(index)) {
        return;
    }
    //*this = *((Planet*) (void*) &find->second);
    const PlanetNature* nature = planets->GetPlanetNature(index);
    mu = nature->mu;
    radius = nature->radius;
    has_atmosphere = nature->has_atmosphere;
    orbit = nature->orbit;
    economy.trading_accessible = strcmp(data->Get("trading_accessible", economy.trading_accessible ? "y" : "n", true), "y") == 0;

    data->FillBufferWithChild("resource_stock", economy.resource_stock, RESOURCE_MAX, resource_names);
    data->FillBufferWithChild("resource_delta", economy.resource_delta, RESOURCE_MAX, resource_names);
    
    if (data->HasArray("inventory")) {
        int ship_module_inventory_count = data->GetArrayLen("inventory", true);
        if (ship_module_inventory_count > MAX_PLANET_INVENTORY) {
            ship_module_inventory_count = MAX_PLANET_INVENTORY;
        }
        
        for (int i = 0; i < ship_module_inventory_count; i++) {
            const char* module_id = data->GetArrayElem("inventory", i);
            ship_module_inventory[i] = GetShipModules()->GetModuleRIDFromStringId(module_id);
        }
    }

    economy.RecalcEconomy();
    for (int i=0; i < PRICE_TREND_SIZE; i++) {
        economy.AdvanceEconomy();
    }
}

void Planet::_OnClicked() {
    TransferPlanUI* tp_ui = GetTransferPlanUI();
    if (
        tp_ui->plan != NULL
        && IsIdValid(tp_ui->ship)
        && IsIdValid(tp_ui->plan->departure_planet)
        && !IsIdValid(tp_ui->plan->arrival_planet)
    ) {
        tp_ui->SetDestination(id);
    } else if (GetGlobalState()->focused_planet == id) {
        GetCoordinateTransform()->focus = position.cartesian;
    } else {
        GetGlobalState()->focused_planet = id;
    }
}

double Planet::ScreenRadius() const {
    return fmax(GetCoordinateTransform()->TransformS(radius), 4);
}

double Planet::GetDVFromExcessVelocity(Vector2 vel) const {
    return sqrt(2*mu / radius + Vector2LengthSqr(vel)) - sqrt(mu / radius);
}

double Planet::GetDVFromExcessVelocityPro(double vel, double parking_orbit, bool aerobreaking) const {
    double a_intermediate = (parking_orbit + radius) / 2;
    double burn_1 = aerobreaking ? 0 : sqrt(mu / (2*radius) + vel*vel) - sqrt(mu * (2 / radius - 1 / a_intermediate));
    double burn_2 = sqrt(mu / parking_orbit) - sqrt(mu * (2 / parking_orbit - 1 / a_intermediate));
    // dv^2 = v_escape^2 + v_excess^2
    return burn_1 + burn_2;
}

Color Planet::GetColor() const {
    return Palette::ui_main;
    //return allegiance == GetFactions()->player_faction ? Palette::green : Palette::red;
}

void Planet::Conquer(int faction) {
    if (allegiance == faction) return;
    allegiance = faction;
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
    Vector2 screen_pos = GetCoordinateTransform()->TransformV(position.cartesian);
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

bool _CanProduce(RID id, const resource_count_t* planet_resource_array) {
    if (!IsIdValid(id)) return false;
    const resource_count_t* construction_resources = NULL;
    switch (IdGetType(id)) {
        case EntityType::SHIP_CLASS: {
            const ShipClass* ship_class = GetShipClassByIndex(id);
            construction_resources = &ship_class->construction_resources[0];
            break;
        }
        case EntityType::MODULE_CLASS: {
            const ShipModuleClass* module_class = GetModule(id);
            construction_resources = &module_class->construction_resources[0];
            break;
        }
        default: break;
    }

    if (construction_resources == NULL) return false;
    for (int i=0; i < RESOURCE_MAX; i++) {
        if (construction_resources[i] > planet_resource_array[i]) {
            return false;
        }
    }
    return true;
}

void Planet::AdvanceShipProductionQueue() {
    // Update ship production
    if (ship_production_queue.size == 0) return;
    if (!_CanProduce(ship_production_queue[0], &economy.resource_stock[0])) {
        // TODO: notify the player
        return;
    }
    ship_production_process++;
    const ShipClass* sc = GetShipClassByIndex(ship_production_queue[0]);
    if (ship_production_process < sc->construction_time) return;

    IDList list;
    GetShips()->GetOnPlanet(&list, id, UINT32_MAX);
    int allegiance = GetShip(list[0])->allegiance;// Assuming planets can't have split allegiance!

    DataNode ship_data;
    ship_data.Set("name", "TODO: random name");
    ship_data.Set("class_id", sc->id);
    ship_data.SetI("allegiance", allegiance); 
    ship_data.Set("planet", name);
    ship_data.CreateArray("tf_plans", 0);
    ship_data.CreateArray("modules", 0);
    GetShips()->AddShip(&ship_data);

    for (int i=0; i < RESOURCE_MAX; i++) {
        if (sc->construction_resources != 0) {
            economy.DrawResource(ResourceTransfer((ResourceType) i, sc->construction_resources[i]));
        }
    }

    ship_production_queue.EraseAt(0);
    ship_production_process = 0;
}

void Planet::AdvanceModuleProductionQueue() {
    if (module_production_queue.size == 0) return;
    if (!_CanProduce(module_production_queue[0], &economy.resource_stock[0])) {
        // TODO: notify the player
        return;
    }
    module_production_process++;
    const ShipModuleClass* smc = GetModule(module_production_queue[0]);
    if (module_production_process < smc->construction_time) return;

    AddShipModuleToInventory(module_production_queue[0]);
    for (int i=0; i < RESOURCE_MAX; i++) {
        if (smc->construction_resources != 0) {
            economy.DrawResource(ResourceTransfer((ResourceType) i, smc->construction_resources[i]));
        }
    }

    module_production_queue.EraseAt(0);
    module_production_process = 0;
}

void Planet::Draw(const CoordinateTransform* c_transf) {
    //printf("%f : %f\n", position.x, position.y);
    Vector2 screen_pos = c_transf->TransformV(position.cartesian);
    DrawCircleV(screen_pos, ScreenRadius(), GetColor());
    
    orbit.Draw(GetColor());

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

void Planet::_UIDrawInventory() {
    const int MARGIN = 3;
    int columns = UIContextCurrent().width / (SHIP_MODULE_WIDTH + MARGIN);
    int max_rows = (int) std::ceil(MAX_PLANET_INVENTORY / columns);
    int available_height = UIContextCurrent().height - UIContextCurrent().y_cursor;
    int height = MinInt(available_height, max_rows * (SHIP_MODULE_HEIGHT + MARGIN));
    int rows = height / (SHIP_MODULE_HEIGHT + MARGIN);
    int i_max = MinInt(rows * columns, MAX_PLANET_INVENTORY);
    UIContextPushInset(0, height);
    
    ShipModules* sms = GetShipModules();
    for (int i = 0; i < i_max; i++) {
        //if (!IsIdValid(ship_module_inventory[i])) continue;
        UIContextPushGridCell(columns, rows, i % columns, i / columns);
        UIContextShrink(MARGIN, MARGIN);
        ButtonStateFlags::T button_state = UIContextAsButton();
        if (button_state & ButtonStateFlags::HOVER) {
            current_slot = ShipModuleSlot(id, i, ShipModuleSlot::DRAGGING_FROM_PLANET);
        }
        if (button_state & ButtonStateFlags::JUST_PRESSED) {
            sms->InitDragging(current_slot, UIContextCurrent().render_rec);
        }
        sms->DrawShipModule(ship_module_inventory[i]);
        UIContextPop();  // GridCell
    }
    UIContextPop();  // Inset
}

void _ProductionQueueMouseHint(RID id, const resource_count_t* planet_resource_array) {
    if (!IsIdValid(id)) return;
    // Assuming monospace font
    int char_width = MeasureTextEx(
        GetCustomDefaultFont(), 
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789", 
        16, 1.0
    ).x/62;
    StringBuilder sb;
    const resource_count_t* construction_resources = NULL;
    int build_time = 0;
    int batch_size = 0;
    switch (IdGetType(id)) {
        case EntityType::SHIP_CLASS: {
            const ShipClass* ship_class = GetShipClassByIndex(id);
            sb.Add(ship_class->description);
            construction_resources = &ship_class->construction_resources[0];
            build_time = ship_class->construction_time;
            batch_size = ship_class->construction_batch_size;
            break;
        }
        case EntityType::MODULE_CLASS: {
            const ShipModuleClass* module_class = GetModule(id);
            sb.Add(module_class->description);
            construction_resources = &module_class->construction_resources[0];
            build_time = module_class->construction_time;
            batch_size = module_class->construction_batch_size;
            break;
        }
        default: break;
    }

    if (construction_resources == NULL) return;

    sb.AutoBreak(UIContextCurrent().width / char_width);
    UIContextWrite(sb.c_str);
    if (planet_resource_array == NULL) {
        return;
    }
    UIContextWrite("++++++++");
    for (int i=0; i < RESOURCE_MAX; i++) {
        if (construction_resources[i] == 0) {
            continue;
        }
        sb.Clear();
        sb.AddFormat("%s: %d", resource_names[i], construction_resources[i]);
        if (construction_resources[i] <= planet_resource_array[i]) {
            UIContextWrite(sb.c_str);
        } else {
            UIContextCurrent().text_color = Palette::red;
            UIContextWrite(sb.c_str);
            UIContextCurrent().text_color = Palette::ui_main;
        }
    }
    sb.Clear();
    sb.AddI(build_time).Add("D");
    if(batch_size > 1) {
        sb.AddFormat(" (x%d)", batch_size);
    }
    UIContextWrite(sb.c_str);
}

void _UIDrawProduction(int option_size, IDList* queue, resource_count_t resources[], double progress,
    RID id_getter(int), const char* name_getter(RID), bool include_getter(RID)
) {
    // Draw options
    const int COLUMNS = 4;
    int rows = std::ceil(option_size / (double)COLUMNS);
    UIContextPushInset(0, 50*rows);
    UIContextShrink(5, 5);

    RID hovered_id = GetInvalidId();
    for(int i=0; i < option_size; i++) {
        RID id = id_getter(i);
        if (!include_getter(id)) {
            continue;
        }
        UIContextPushGridCell(COLUMNS, rows, i % COLUMNS, i / COLUMNS);
        UIContextShrink(3, 3);
        
        // Possible since Shipclasses get loaded once in continuous mempry
        ButtonStateFlags::T button_state = UIContextAsButton();
        if (button_state & ButtonStateFlags::HOVER) {
            UIContextEnclose(Palette::bg, Palette::ui_main);
            hovered_id = id;
        } else {
            UIContextEnclose(Palette::bg, Palette::blue);
        }
        HandleButtonSound(button_state);
        if ((button_state & ButtonStateFlags::JUST_PRESSED) && _CanProduce(id, &resources[0])) {
            queue->Append(id);
        }

        UIContextWrite(name_getter(id));
        //UIContextFillline(1.0, Palette::ui_main, Palette::bg);
        //UIContextWrite(ship_class->description);
        UIContextPop();  // GridCell
    }
    UIContextPop();  // Inset

    // Draw queue
    bool hover_over_queue = false;
    for(int i=0; i < queue->size; i++) {
        RID id = queue->Get(i);
        UIContextPushInset(0, 50);
        UIContextShrink(3, 3);
        ButtonStateFlags::T button_state = UIContextAsButton();
        if (button_state & ButtonStateFlags::HOVER) {
            UIContextEnclose(Palette::bg, Palette::ui_main);
            hovered_id = id;
            hover_over_queue = true;
        } else {
            UIContextEnclose(Palette::bg, Palette::blue);
        }
        if (button_state & ButtonStateFlags::JUST_PRESSED) {
            queue->EraseAt(i);
            i--;
        }
        UIContextWrite(name_getter(id));
        if (i == 0) {
            UIContextFillline(progress, Palette::ui_main, Palette::bg);
        }
        _ProductionQueueMouseHint(id, NULL);
        UIContextPop();  // Inset
    }
    if (IsIdValid(hovered_id)) {
        UIContextPushMouseHint(400, 400);
        UIContextEnclose(Palette::bg, Palette::ui_main);
        if (hover_over_queue) {
            _ProductionQueueMouseHint(hovered_id, NULL);
        } else {
            _ProductionQueueMouseHint(hovered_id, resources);
        }
        UIContextPop();
    }
}

void Planet::_UIDrawModuleProduction() {
    double progress = 0.0;
    if (module_production_queue.Count() > 0) {
        const ShipModuleClass* module_class = GetModule(module_production_queue[0]);
        progress = Clamp(module_production_process / (float)module_class->construction_time, 0.0f, 1.0f);
    }
    _UIDrawProduction(
        GetShipModules()->shipmodule_count,
        &module_production_queue,
        economy.resource_stock,
        progress,
        [](int i) { return RID(i, EntityType::MODULE_CLASS); },
        [](RID id) { return GetModule(id)->name; },
        [](RID id) { return !GetModule(id)->is_hidden; }
    );
}

void Planet::_UIDrawShipProduction() {
    double progress = 0.0;
    if (ship_production_queue.Count() > 0) {
        const ShipClass* ship_class = GetShipClassByIndex(ship_production_queue[0]);
        progress = Clamp(ship_production_process / (float)ship_class->construction_time, 0.0f, 1.0f);
    }
    _UIDrawProduction(
        GetShips()->ship_classes_count,
        &ship_production_queue,
        economy.resource_stock,
        progress,
        [](int i) { return RID(i, EntityType::SHIP_CLASS); },
        [](RID id) { return GetShipClassByIndex(id)->name; },
        [](RID id) { return !GetShipClassByIndex(id)->is_hidden; }
    );
}

int current_tab = 0;  // Global variable, I suppose
void Planet::DrawUI() {
    // Reset
    current_slot = ShipModuleSlot();

    int y_start = -1;
    int height = -1;
    ResourceTransfer transfer = ResourceTransfer();
    resource_count_t fuel_draw = -1;  // why not 0?

    const TransferPlan* tp = GetTransferPlanUI()->plan;

    if (GetTransferPlanUI()->IsActive()){
        if (tp->departure_planet == id) {
            y_start = 10;
            height = GetScreenHeight() / 2 - 20;
            transfer = tp->resource_transfer.Inverted();
            fuel_draw = tp->fuel_mass;
        } else if (tp->arrival_planet == id) {
            y_start = GetScreenHeight() / 2 + 10;
            height = GetScreenHeight() / 2 - 20;
            transfer = tp->resource_transfer;
        } else {
            return;
        }
    } else if (mouse_hover || GetGlobalState()->focused_planet == id) {
        y_start = 10;
        height = GetScreenHeight() - 20;
        transfer = ResourceTransfer();
        fuel_draw = -1;
    } else {
        return;
    }

    UIContextCreateNew(10, y_start, 16*30, height, 16, Palette::ui_main);
    UIContextCurrent().Enclose(2, 2, Palette::bg, Palette::ui_main);

    UIContextPushInset(4, 20*2);  // Tab container
    int w = UIContextCurrent().width;
    const int n_tabs = 6;
    const char* tab_descriptions[] = {
        "Resources",
        "Economy",
        "Inventory",
        "~Quests~",
        "Ship Production",
        "Module Production",
    };
    static_assert(sizeof(tab_descriptions) / sizeof(tab_descriptions[0]) == n_tabs);

    for (int i=0; i < n_tabs; i++) {
        UIContextPushGridCell(4, 2, i%4, i/4);
        //UIContextPushHSplit(i * w / n_tabs, (i + 1) * w / n_tabs);
        ButtonStateFlags::T button_state = UIContextAsButton();
        HandleButtonSound(button_state & ButtonStateFlags::JUST_PRESSED);
        if (i == 1 && !economy.trading_accessible) {
            UIContextWrite("~Economy~");
            UIContextPop();
            continue;
        }
        if (button_state & ButtonStateFlags::JUST_PRESSED) {
            current_tab = i;
        }
        if (button_state & ButtonStateFlags::HOVER || i == current_tab) {
            UIContextEnclose(Palette::bg, Palette::ui_main);
        }
        UIContextWrite(tab_descriptions[i]);
        UIContextPop();  // GridCell
    }
    UIContextPop();  // Tab container

    UIContextWrite(name);
    UIContextFillline(1, Palette::ui_main, Palette::ui_main);
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
    case 3:
        // Quests
        break;
    case 4:
        _UIDrawShipProduction();
        break;
    case 5:
        _UIDrawModuleProduction();
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
    planet_count = data->GetChildArrayLen("satellites");
    ephemerides = new PlanetNature[planet_count];
    planet_array = new Planet[planet_count];
    //if (num_planets > 100) num_planets = 100;
    //RID planets[100];
    for(int i=0; i < planet_count; i++) {
        const DataNode* planet_data = data->GetChildArrayElem("satellites", i);
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
        nature->has_atmosphere = strcmp(planet_data->Get("has_atmosphere", "n", true), "y") == 0;
    }
    return planet_count;
}

Planet* GetPlanet(RID id) { return GetPlanets()->GetPlanet(id); }

Planet* GetPlanetByIndex(int index) { 
    return GetPlanets()->GetPlanet(RID(index, EntityType::PLANET)); 
}

int LoadEphemerides(const DataNode* data) { return GetPlanets()->LoadEphemerides(data); }