#include "planet.hpp"
#include "global_state.hpp"
#include "ui.hpp"
#include "utils.hpp"
#include "constants.hpp"
#include "ship_modules.hpp"
#include "logging.hpp"
#include "string_builder.hpp"
#include "render_utils.hpp"
#include "debug_drawing.hpp"

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
        resource_delta_node->SetI(GetResourceData(resource_index)->name, economy.native_resource_delta[resource_index]);
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
    RID index = planets->GetIdByName(name);
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
    data->FillBufferWithChild("resource_delta", economy.native_resource_delta, RESOURCE_MAX, resource_names);
    
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
    } else if (GetGlobalState()->focused_planet == id && !IsIdValidTyped(GetGlobalState()->focused_ship, EntityType::SHIP)) {
        GetCamera()->focus_object = id;
    } else {
        GetGlobalState()->focused_planet = id;
    }
}

double Planet::ScreenRadius() const {
    return radius / GetCamera()->space_scale;
    //return fmax(radius / GetCamera()->space_scale, 4);
}

double Planet::GetDVFromExcessVelocity(DVector3 vel) const {
    if (mu == 0) return vel.LengthSquared();
    return sqrt(2*mu / radius + vel.LengthSquared()) - sqrt(mu / radius);
}

double Planet::GetDVFromExcessVelocity(double vel) const {
    if (mu == 0) return vel;
    return sqrt(2*mu / radius + vel*vel) - sqrt(mu / radius);
}

double Planet::GetDVFromExcessVelocityPro(double vel, double parking_orbit, bool aerobreaking) const {
    if (mu == 0) return aerobreaking ? 0 : vel;
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

bool Planet::CanProduce(RID id) const {
    if (!IsIdValid(id)) return false;
    const resource_count_t* construction_resources = NULL;
    switch (IdGetType(id)) {
        case EntityType::SHIP_CLASS: {
            const ShipClass* ship_class = GetShipClassByRID(id);
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
        if (construction_resources[i] > economy.resource_stock[i]) {
            return false;
        }
    }
    return true;
}

void Planet::Conquer(int faction) {
    if (allegiance == faction) return;
    allegiance = faction;
}

void Planet::RecalcStats() {
    // Just call this every frame tbh

    // Not used atm
    for (int i = 0; i < RESOURCE_MAX; i++){
        economy.resource_delta[i] = 0;
    }
}

ShipModuleSlot Planet::GetFreeModuleSlot() const {
    for (int index = 0; index < MAX_PLANET_INVENTORY; index++) {
        if(!IsIdValid(ship_module_inventory[index])) {
            return ShipModuleSlot(id, index, ShipModuleSlot::DRAGGING_FROM_PLANET);
        }
    }
    return ShipModuleSlot(id, -1, ShipModuleSlot::DRAGGING_FROM_PLANET);
}

void Planet::RemoveShipModuleInInventory(int index) {
    ship_module_inventory[index] = GetInvalidId();
}

bool Planet::HasMouseHover(double* min_distance) const {
    // TODO: proximity approximation is pretty shit at flat angles

    OrbitSegment segment = OrbitSegment(&orbit);
    Ray mouse_ray = GetMouseRay(GetMousePosition(), GetCamera()->rl_camera);
    Matrix orbit_transform = MatrixFromColumns((Vector3) orbit.periapsis_dir, (Vector3) orbit.normal, (Vector3) orbit.periapsis_dir.Cross(orbit.normal));
    Matrix inv_orbit_transform = MatrixInvert(orbit_transform);
    //Vector3 mouse_repr = Vector3Add(mouse_ray.position, mouse_ray.direction);
    mouse_ray.position = Vector3Transform(mouse_ray.position, inv_orbit_transform);
    mouse_ray.direction = Vector3Transform(mouse_ray.direction, inv_orbit_transform);
    
    float distance;
    timemath::Time mouseover_time;
    Vector3 local_pos;
    segment.TraceRay(mouse_ray.position, mouse_ray.direction, &distance, &mouseover_time, &local_pos);

    //Vector3 crossing = Vector3Subtract(mouse_ray.position, Vector3Scale(mouse_ray.direction, mouse_ray.position.y / mouse_ray.direction.y));
    //DebugDrawLineRenderSpace(Vector3Transform(local_pos, orbit_transform), Vector3Transform(crossing, orbit_transform));
    //float scale = GameCamera::WorldToRender(orbit.sma);
    //DebugDrawTransform(MatrixMultiply(MatrixScale(scale, scale, scale), orbit_transform));
    Vector2 closest_on_screen = GetWorldToScreen(Vector3Transform(local_pos, orbit_transform), GetCamera()->rl_camera);
    //DebugDrawLineRenderSpace(Vector3Transform(local_pos, orbit_transform), mouse_repr);

    if (Vector2Distance(closest_on_screen, GetMousePosition()) <= 2.0f && distance < *min_distance) {
        *min_distance = distance;
        return true;
    } else {
        return false;
    }
}

void Planet::Update() {
    timemath::Time now = GlobalGetNow();
    position = orbit.GetPosition(now);
    //int index = IdGetIndex(id);
    //position = orbit.FromRightAscention(-PI/2 * index);
    position = orbit.FromRightAscention(-PI/2);
    // RecalcStats();
    economy.Update();
}

void Planet::AdvanceShipProductionQueue() {
    // Update ship production
    if (ship_production_queue.size == 0) return;
    if (!CanProduce(ship_production_queue[0])) {
        // TODO: notify the player
        return;
    }
    ship_production_process++;
    const ShipClass* sc = GetShipClassByRID(ship_production_queue[0]);
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
        if (sc->construction_resources[i] != 0) {
            economy.DrawResource(ResourceTransfer((ResourceType) i, sc->construction_resources[i]));
        }
    }

    ship_production_queue.EraseAt(0);
    ship_production_process = 0;
}

void Planet::AdvanceModuleProductionQueue() {
    if (module_production_queue.size == 0) return;
    if (!CanProduce(module_production_queue[0])) {
        // TODO: notify the player
        ERROR("CANNOT PRODUCE %s on %s", GetModule(module_production_queue[0])->name, name)
        return;
    }
    SHOW_I(module_production_process)
    module_production_process++;
    const ShipModuleClass* smc = GetModule(module_production_queue[0]);
    if (module_production_process < smc->GetConstructionTime()) return;
    
    ShipModuleSlot free_slot = GetFreeModuleSlot();
    if (free_slot.IsValid()) {
        free_slot.SetSlot(module_production_queue[0]);
    }

    for (int i=0; i < RESOURCE_MAX; i++) {
        if (smc->construction_resources[i] != 0) {
            economy.DrawResource(ResourceTransfer((ResourceType) i, smc->construction_resources[i]));
        }
    }

    module_production_queue.EraseAt(0);
    module_production_process = 0;
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
    RID id = GetIdByName(data->Get("name", "UNNAMED"));
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

RID Planets::GetIdByName(const char* planet_name) const {
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

        double inc = planet_data->GetF("Inc") * DEG2RAD;
        inc = inc > PI/2 ? PI : 0;

        nature->mu = planet_data->GetF("mass") * G;
        nature->radius = planet_data->GetF("radius");
        nature->orbit = Orbit(
            sma,
            planet_data->GetF("Ecc"),
            inc,
            planet_data->GetF("LoA") * DEG2RAD,
            planet_data->GetF("AoP") * DEG2RAD,
            parent.mu,
            epoch
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