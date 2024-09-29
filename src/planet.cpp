#include "planet.hpp"
#include "global_state.hpp"
#include "ui.hpp"
#include "utils.hpp"
#include "constants.hpp"
#include "ship_modules.hpp"
#include "logging.hpp"
#include "string_builder.hpp"
#include "debug_drawing.hpp"

Planet::Planet(PermaString p_name, double p_mu, double p_radius) {
    name = p_name;
    mu = p_mu;
    radius = p_radius;
    //ship_production_queue.Clear();
    //module_production_queue.Clear();

    for (int i = 0; i < MAX_PLANET_INVENTORY; i++) {
        inventory[i] = GetInvalidId();
    }
}

void Planet::Serialize(DataNode* data) const {
    data->Set("name", name.GetChar());
    data->Set("trading_accessible", economy.trading_accessible ? "y" : "n");
    data->SetI("allegiance", allegiance);
    data->SetI("independence", independence);
    data->SetI("base_independence_delta", base_independence_delta);
    data->SetI("opinion", opinion);

    data->SerializeBuffer("resource_stock", economy.resource_stock, resources::names, resources::MAX);
    data->SerializeBuffer("resource_delta", economy.native_resource_delta, resources::names, resources::MAX);
    
    // modules
    for(int i=0; i < MAX_PLANET_INVENTORY; i++) {
        if (IsIdValid(inventory[i])) {
            data->AppendToArray("inventory", TextFormat("%d %s%", i, GetModule(inventory[i])->id));
        }
    }

    // We assume the orbital info is stored in the ephemeris
}

void Planet::Deserialize(Planets* planets, const DataNode *data) {
    //SettingOverridePush("datanode_quite", true);
    //SettingOverridePop("datanode_quite");
    name = PermaString(data->Get("name", name.GetChar(), true));
    allegiance = data->GetI("allegiance", 0);

    independence = data->GetI("independence", 0, true);
    base_independence_delta = data->GetI("base_independence_delta", 0);
    opinion = data->GetI("opinion", 0, true);

    RID index = planets->GetIdByName(name.GetChar());
    if (!IsIdValid(index)) {
        return;
    }
    // Cursed way to do it
    //*this = *((Planet*) (void*) planets->GetPlanetNature(index));
    
    const PlanetNature* nature = planets->GetPlanetNature(index);
    mu = nature->mu;
    radius = nature->radius;
    has_atmosphere = nature->has_atmosphere;
    rotation_period = nature->rotation_period;
    orbit = nature->orbit;
    economy.trading_accessible = strcmp(data->Get("trading_accessible", economy.trading_accessible ? "y" : "n", true), "y") == 0;

    data->DeserializeBuffer("resource_stock", economy.resource_stock, resources::names, resources::MAX);
    data->DeserializeBuffer("resource_delta", economy.native_resource_delta, resources::names, resources::MAX);
        
    int inventory_count = data->GetArrayLen("inventory", true);
    if (inventory_count > MAX_PLANET_INVENTORY) {
        inventory_count = MAX_PLANET_INVENTORY;
    }
    for(int i=0; i < inventory_count; i++) {
        const char* identifier_package = data->GetArrayElem("inventory", i);

        int index;
        RID module_rid = DeserializeModuleInfo(identifier_package, &index);
        if (!IsIdValid(module_rid)) continue;
        
        module_types::T module_type = GetShipModules()->GetModuleByRID(module_rid)->type;
        int proper_index = GetFreeModuleSlot(module_type).index;
        if (index < 0) {
            // Unspecified index
            index = proper_index;
        } else if (IsIdValidTyped(inventory[index], EntityType::MODULE_CLASS)) {
            // Index not available
            WARNING("Ship already has module at %d", index)
            index = proper_index;
        }

        inventory[index] = module_rid;
    }
    
    RecalculateStats();
    economy.RecalculateEconomy();
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
    return radius / GetCamera()->macro_scale;
    //return fmax(radius / GetCamera()->macro_scale, 4);
}

double Planet::GetSOI() const {
    return orbit.sma * pow(mu / GetPlanets()->parent.mu, 0.4);
}

double Planet::GetDVFromExcessVelocity(DVector3 vel) const {
    if (mu == 0) return vel.Length();
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

bool Planet::CanProduce(RID id, bool check_resources, bool check_stats) const {
    if (!IsIdValid(id)) return false;
    for (int i=0; i < cached_ship_list.size; i++) {
        const Ship* ship = GetShip(cached_ship_list[i]);
        if (ship->CanProduce(id, check_resources, check_stats)) {
            return true;
        }
    }
    return false;
}

void Planet::Conquer(int faction, bool include_ships) {
    if (include_ships) {
        cached_ship_list.Clear();
        GetShips()->GetOnPlanet(&cached_ship_list, id, ship_selection_flags::ALL);
        for (int i=0; i < cached_ship_list.size; i++) {
            GetShip(cached_ship_list[i])->allegiance = faction;
        }
    }
    if (allegiance == faction) return;
    allegiance = faction;
}

void Planet::RecalculateStats() {
    // Just call this every frame tbh
    independence_delta_log.Clear();
    independence_delta = base_independence_delta;
    int delta_from_delivery = 0;
    for(int i=0; i < resources::MAX; i++) {
        if (economy.delivered_resources_today[i] != 0) {
            //delta_from_delivery -= economy.delivered_resources_today[i] * GetResourceData(i)->default_cost / 1000000;
            delta_from_delivery -= economy.delivered_resources_today[i] / 10;

            //DebugPrintText("%s: %d", resources::names[i], economy.delivered_resources_today[i]);
            //INFO("%s: %d", resources::names[i], economy.delivered_resources_today[i])
        }
    }


    independence_delta += delta_from_delivery;
    independence_delta += module_independence_delta;
    independence_delta_log.AddFormat("Base: %+d\n", base_independence_delta);
    if (delta_from_delivery != 0)
        independence_delta_log.AddFormat("From delivery: %+d\n", delta_from_delivery);
    if (module_independence_delta != 0)
        independence_delta_log.AddFormat("From modules: %+d\n", module_independence_delta);
    module_independence_delta = 0;

    opinion_delta_log.Clear();
    opinion_delta_log.Add("Base: +0");
    opinion_delta = 0;
    opinion_delta += module_opinion_delta;

    if (module_opinion_delta != 0)
        opinion_delta_log.AddFormat("From modules: %+d\n", module_opinion_delta);
    module_opinion_delta = 0;

    for (int i = 0; i < resources::MAX; i++){
        economy.resource_delta[i] = 0;
    }
}

ShipModuleSlot Planet::GetFreeModuleSlot(module_types::T type) const {
    for (int index = 0; index < MAX_PLANET_INVENTORY; index++) {
        module_types::T slot_type = module_types::ANY;  // For now
        if(!IsIdValid(inventory[index]) && module_types::IsCompatible(type, slot_type)) {
            return ShipModuleSlot(id, index, ShipModuleSlot::DRAGGING_FROM_PLANET, module_types::ANY);
        }
    }
    return ShipModuleSlot(id, -1, ShipModuleSlot::DRAGGING_FROM_PLANET, module_types::ANY);
}

void Planet::RemoveShipModuleInInventory(int index) {
    inventory[index] = GetInvalidId();
}

/*Planet::ProductionOrder Planet::MakeProductionOrder(RID id) const {
    // TODO: untested
    int current_order_count = INT32_MAX;
    ProductionOrder res;
    res.product = id;
    res.worker = GetInvalidId();
    const List<ProductionOrder>* production_orders;
    if (IsIdValidTyped(id, EntityType::SHIP_CLASS)) {
        production_orders = &ship_production_queue;
    } else {
        production_orders = &module_production_queue;
    }
    for (int i=0; i < cached_ship_list.size; i++) {
        const Ship* ship = GetShip(cached_ship_list[i]);
        if (!ship->CanProduce(id, true, true)) {
            continue;
        }
        int ship_order_count = 0;
        for (int j=0; j < production_orders->size; j++) {
            if (production_orders->Get(j).worker == cached_ship_list[i]) ship_order_count++;
        }
        if (ship_order_count < current_order_count) {
            current_order_count = ship_order_count;
            res.worker = cached_ship_list[i];
        }
    }
    return res;
}*/

double Planet::GetMousePixelDistance() const {
    // TODO: proximity approximation is pretty shit at flat angles

    OrbitSegment segment = OrbitSegment(&orbit);
    Ray mouse_ray = GetMouseRay(GetMousePosition(), GetCamera()->macro_camera);
    Matrix orbit_transform = MatrixFromColumns((Vector3) orbit.periapsis_dir, (Vector3) orbit.normal, (Vector3) orbit.periapsis_dir.Cross(orbit.normal));
    Matrix inv_orbit_transform = MatrixInvert(orbit_transform);
    //Vector3 mouse_representation = Vector3Add(mouse_ray.position, mouse_ray.direction);
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
    Vector2 closest_on_screen = GetWorldToScreen(Vector3Transform(local_pos, orbit_transform), GetCamera()->macro_camera);
    float screen_distance = Vector2Distance(closest_on_screen, GetMousePosition());
    //DebugDrawLineRenderSpace(Vector3Transform(local_pos, orbit_transform), mouse_mouse_representation);
    return screen_distance;
}

void Planet::GetRandomOrbit(int index, Orbit *orbit) const {
    randomgen::SetRandomSeed(index + (IdGetIndex(id) << 16));
    double soi = GetSOI();
    double min_radius = radius * 1.05;
    double max_radius = soi * 0.8;

    enum {
        LOW,
        MEDIUM,
        HIGH,
        STATIONARY,
        HALF_STATIONARY,
    } orbit_altitude_cat;
    
    enum {
        EQUATORIAL,
        POLAR,
        ANY,
    } orbit_orientation_cat;

    enum {
        CIRCULAR,
        ELLIPTIC,
    } orbit_ecc_cat = CIRCULAR;

    // rotation_period = 2 PI sqrt(sma³ / mu);
    // sma = cbrt((rotation_period / 2PI)² * mu)
    const double GEOSTAT_ALTITUDE_FACTOR = 0.6299605249;  // (1/2) ^ (2/3)
    double geostat_radius = cbrt(rotation_period*rotation_period / (2*PI*PI) * mu);
    double semi_geostat_radius = GEOSTAT_ALTITUDE_FACTOR * geostat_radius;

    if (index < 2) {
        // Put the 'main' stations on index 0,1 which are low equatorial orbits
        orbit_altitude_cat = LOW;
        orbit_orientation_cat = EQUATORIAL;
        orbit_ecc_cat = CIRCULAR;
    } else {
        double selector_alt = randomgen::GetRandomUniform(0, 1);
        if (selector_alt < 0.4) {
            orbit_altitude_cat = LOW;
        } else if (selector_alt < 0.6) {
            orbit_altitude_cat = LOW;
        } else {
            orbit_altitude_cat = HIGH;
        }
        if (min_radius < geostat_radius && geostat_radius < max_radius && 0.8 < selector_alt) {
            orbit_altitude_cat = STATIONARY;
        }
        if (min_radius < semi_geostat_radius && semi_geostat_radius < max_radius && 0.9 < selector_alt) {
            orbit_altitude_cat = HALF_STATIONARY;
        }

        double selector_orientation = randomgen::GetRandomUniform(0, 1);
        if (selector_alt < 0.2 || orbit_altitude_cat == STATIONARY || orbit_altitude_cat == HALF_STATIONARY) {
            orbit_orientation_cat = EQUATORIAL;
        } else if (selector_alt < 0.4) {
            orbit_orientation_cat = POLAR;
        } else {
            orbit_orientation_cat = ANY;
        }
        if (0.9 < selector_alt) {
            orbit_ecc_cat = ELLIPTIC;
        }
    }

    switch (orbit_altitude_cat) {
    case LOW:
        do orbit->sma = min_radius + fabs(randomgen::GetRandomGaussian(0, min_radius * 0.66));
        while (orbit->sma >= max_radius);
        break;
    case MEDIUM:
        do orbit->sma = randomgen::GetRandomGaussian(min_radius * 3, min_radius * 2);
        while (orbit->sma >= max_radius || orbit->sma < min_radius);
        break;
    case HIGH:
        do orbit->sma = randomgen::GetRandomUniform(min_radius * 3, max_radius);
        while (orbit->sma >= max_radius || orbit->sma < min_radius);
        break;
    case STATIONARY:
        do orbit->sma = randomgen::GetRandomGaussian(geostat_radius, 1000);
        while (orbit->sma >= max_radius || orbit->sma < min_radius);
        break;
    case HALF_STATIONARY:
        do orbit->sma = randomgen::GetRandomGaussian(semi_geostat_radius, 1000);
        while (orbit->sma >= max_radius || orbit->sma < min_radius);
        break;
    }
    
    switch (orbit_orientation_cat) {
    case EQUATORIAL:
        orbit->normal = { 0.0f, 1.0f, 0.0f }; break;
    case POLAR: {
        double longuitude = randomgen::GetRandomUniform(0, 2*PI);
        orbit->normal = DVector3(cos(longuitude), 0, sin(longuitude)); 
        break;}
    case ANY:
        orbit->normal = randomgen::RandomOnSphere(); break;
    }
    
    switch (orbit_ecc_cat) {
    case CIRCULAR:
        orbit->ecc = fabs(randomgen::GetRandomUniform(0, 0.1)); break;
    case ELLIPTIC:{
        // periapsis: p / (1 + e), apoapsis: p / (1 - e)
        // e < p / min_r - 1; e < 1 - p / max_r
        double p = orbit->sma * (1 - orbit->ecc*orbit->ecc);
        double max_ecc = fmin(p / min_radius - 1, 1 - p / max_radius);
        orbit->ecc = fabs(randomgen::GetRandomUniform(0, max_ecc)); 
        break;}
    }
    
    orbit->mu = mu;
    orbit->periapsis_dir = orbit->normal.AnyOrthogonalDirection();
    orbit->periapsis_dir.Rotated(orbit->normal, randomgen::GetRandomUniform(0, 2*PI));
    orbit->epoch = timemath::Time(randomgen::GetRandomUniform(0, orbit->GetPeriod().Seconds()));
}

void Planet::Update() {
    timemath::Time now = GlobalGetNow();
    position = orbit.GetPosition(now);
    cached_ship_list.Clear();
    GetShips()->GetOnPlanet(&cached_ship_list, id, ship_selection_flags::ALL);
    //int index = IdGetIndex(id);
    //position = orbit.FromRightAscension(-PI/2 * index);
    //position = orbit.FromRightAscension(-PI/2);
    RecalculateStats();
    if (GetCalendar()->IsNewDay()) {
        independence += independence_delta;
        if (independence > 150) independence = 150;
        if (independence < 0) independence = 0;
        if (independence >= 100 && allegiance != 0) {
            Conquer(1, true);
        }
    }
    
    economy.Update();
}

void Planet::Draw3D() const {
    GetRenderServer()->QueueConicDraw(ConicRenderInfo::FromOrbit(
        &orbit, GlobalGetNow(), DVector3::Zero(), orbit_render_mode::Gradient, GetColor()));
    GetRenderServer()->QueueSphereDraw(SphereRenderInfo::FromWorldPos(
        position.cartesian, radius, GetColor()));

    if (radius > 0 && mu > 0 && GetGlobalState()->focused_planet == id) {
        IDList ship_list;
        GetShips()->GetOnPlanet(&ship_list, id, ship_selection_flags::ALL);

        // Draw surrounding orbits (visual)
        Orbit ship_orbit;
        for(int i=0; i < 20; i++) {
            GetRandomOrbit(i, &ship_orbit);
            Color color = Palette::ui_alt;
            if (i == 0){
                color = GetShip(ship_list.Get(i))->GetColor();
            }
            GetRenderServer()->QueueConicDraw(ConicRenderInfo::FromOrbit(
                &ship_orbit, GlobalGetNow(), position.cartesian, orbit_render_mode::Solid, color));
        }
        
        // Show SOI
        GetRenderServer()->QueueConicDraw(ConicRenderInfo::FromCircle(
            position.cartesian, MatrixIdentity(), GetSOI(), orbit_render_mode::Solid, Palette::ui_main));
    }

    RID preview_ship = GetInvalidId();
    if (IsIdValidTyped(GetGlobalState()->focused_ship, EntityType::SHIP) && GetShip(GetGlobalState()->focused_ship)->GetParentPlanet() == id) {
        preview_ship = GetGlobalState()->focused_ship;
    }
    if (IsIdValidTyped(GetGlobalState()->hover, EntityType::SHIP) && GetShip(GetGlobalState()->hover)->GetParentPlanet() == id) {
        preview_ship = GetGlobalState()->hover;
    }
    if (IsIdValid(preview_ship)) {
        Orbit ship_orbit;
        int index = 0;
        GetRandomOrbit(index, &ship_orbit);
        Color color = GetShip(preview_ship)->GetColor();
        GetRenderServer()->QueueConicDraw(ConicRenderInfo::FromOrbit(
            &ship_orbit, GlobalGetNow(), position.cartesian, orbit_render_mode::Solid, color));
    }
}

Planets::Planets() {
    planet_array = NULL;
    ephemeris = NULL;
    planet_count = 0;
}

Planets::~Planets() {
    delete[] planet_array;
    delete[] ephemeris;
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
    return &ephemeris[IdGetIndex(id)];
}

int Planets::GetPlanetCount() const {
    return planet_count;
}

RID Planets::GetIdByName(const char* planet_name) const {
    // Returns NULL if planet_name not found
    for(int i=0; i < planet_count; i++) {
        if (strcmp(ephemeris[i].name.GetChar(), planet_name) == 0) {
            return RID(i, EntityType::PLANET);
        }
    }
    ERROR("No such planet '%s' ", planet_name)
    return GetInvalidId();
}

const PlanetNature* Planets::GetParentNature() const {
    return &parent;
}

int Planets::LoadEphemeris(const DataNode* data) {
    // Init planets
    parent.radius = data->GetF("radius");
    parent.mu = data->GetF("mass") * G;
    planet_count = data->GetChildArrayLen("satellites");
    ephemeris = new PlanetNature[planet_count];
    planet_array = new Planet[planet_count];
    //if (num_planets > 100) num_planets = 100;
    //RID planets[100];
    for(int i=0; i < planet_count; i++) {
        const DataNode* planet_data = data->GetChildArrayElem("satellites", i);
        PlanetNature* nature = &ephemeris[i];
        nature->name = PermaString(planet_data->Get("name"));
        
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
        nature->has_atmosphere = strcmp(planet_data->Get("has_atmosphere", "n", true), "y") == 0;;
        nature->rotation_period = planet_data->GetF("rotation_period", 0) * 3600;
    }
    return planet_count;
}

void Planets::Clear() {
    // Clean up everything associated with the planets
    for (int i=0; i < planet_count; i++) {
        GetRenderServer()->text_labels_3d.EraseAt(planet_array[i].text3d);
    }

    // Code duplication from constructor/destructor :(
    delete[] planet_array;
    delete[] ephemeris;

    planet_array = NULL;
    ephemeris = NULL;
    planet_count = 0;
}

Planet* GetPlanet(RID id) { return GetPlanets()->GetPlanet(id); }

Planet* GetPlanetByIndex(int index) { 
    return GetPlanets()->GetPlanet(RID(index, EntityType::PLANET)); 
}

double PlanetsMinDV(RID planet_a, RID planet_b, bool aerobrake) {
    timemath::Time departure, arrival;

    const Orbit* orbit_a = &GetPlanet(planet_a)->orbit;
    const Orbit* orbit_b = &GetPlanet(planet_b)->orbit;
    
    double mu = orbit_a->mu;
    double a_hohmann = (orbit_a->sma + orbit_b->sma) * 0.5;
    
    DVector3 normal = DVector3::Up();  // TODO: how do we know?

    double circ_vel_a = sqrt(mu / orbit_a->sma) * Sign(normal.Dot(orbit_a->normal));
    double circ_vel_b = sqrt(mu / orbit_b->sma) * Sign(normal.Dot(orbit_b->normal));
    double raw_dv1 = fabs(sqrt(mu * (2 / orbit_a->sma - 1 / a_hohmann)) - circ_vel_a);
    double raw_dv2 = fabs(circ_vel_b - sqrt(mu * (2 / orbit_b->sma - 1 / a_hohmann)));

    double dv1 = GetPlanet(planet_a)->GetDVFromExcessVelocity(raw_dv1);
    double dv2 = GetPlanet(planet_b)->GetDVFromExcessVelocity(raw_dv2);

    return aerobrake ? dv1 : dv1 + dv2;
}

int LoadEphemeris(const DataNode* data) { return GetPlanets()->LoadEphemeris(data); }

