#include "ai.hpp"
#include "global_state.hpp"
#include "debug_drawing.hpp"

double _GetMinDVTo(const Ship* ship, const Planet* to) {
    double dv1, dv2;
    const Planet* from = GetPlanet(ship->GetParentPlanet());
    HohmannTransfer(
        &from->orbit, &to->orbit,
        GlobalGetNow(), NULL, NULL,
        &dv1, &dv2
    );
    if (to->has_atmosphere && ship->CountModulesOfClass(GetShipModules()->expected_modules.heatshield) > 0) {
        return from->GetDVFromExcessVelocity({0, (float) dv1});
    }
    return from->GetDVFromExcessVelocity({0, (float) dv1}) + to->GetDVFromExcessVelocity({0, (float) dv2});
}

TransferPlan _HohmannTransferPlan(const Ship* ship, const Planet* to) {
    timemath::Time t1, t2;
    const Planet* from = GetPlanet(ship->GetParentPlanet());
    HohmannTransfer(
        &from->orbit, &to->orbit,
        GlobalGetNow(), &t1, &t2,
        NULL, NULL
    );
    TransferPlan res;
    res.departure_planet = ship->GetParentPlanet();
    res.arrival_planet = to->id;
    res.departure_time = t1;
    res.arrival_time = t2;
    TransferPlanSolve(&res);
    return res;
}

AIBlackboard::~AIBlackboard() {
    delete[] resource_transfer_tensor;
}

double AIBlackboard::CalcTransferUtility(AbstractTransfer atf) const {
    if (resource_transfer_tensor == NULL) {
        return -1;
    }
    if (!GetFactions()->DoesControlPlanet(faction, atf.arrival_planet)) {
        return -1;
    }
    //DebugPrintText("faction %d controls %s", faction, GetPlanet(atf.arrival_planet)->name);
    if (atf.fuel > GetPlanet(atf.departure_planet)->economy.resource_stock[RESOURCE_WATER]) {
        return -1;
    }
    //DebugPrintText("%d <= %d", atf.fuel, GetPlanet(atf.departure_planet)->economy.resource_stock[RESOURCE_WATER]);
    int departure_planet_tensor_lookup = IdGetIndex(atf.departure_planet);
    int arrival_planet_tensor_lookup = IdGetIndex(atf.arrival_planet);
    int planets_count = GetPlanets()->GetPlanetCount();
    int tensor_index = departure_planet_tensor_lookup*planets_count*RESOURCE_MAX
      + arrival_planet_tensor_lookup*RESOURCE_MAX
      + atf.resource_transfer.resource_id;
    double utility_per_count = resource_transfer_tensor[tensor_index];
    return utility_per_count * atf.resource_transfer.quantity;

    // TODO: take into account fuel cost
}

RID AIBlackboard::ShipProductionRequest() const {
    return GetInvalidId();
}

RID AIBlackboard::ModuleProductionRequest() const {
    return GetInvalidId();
}

void AIBlackboard::HighLevelFactionAI() {
    //INFO("High level AI")
    // fill resource transfer tensor
    int planets_count = GetPlanets()->GetPlanetCount();
    if (resource_transfer_tensor == NULL) {
        // Assuming the ammount of planets is invariant
        resource_transfer_tensor = new double [planets_count*planets_count*RESOURCE_MAX];
    }

    // Tightening the triple loop
    IDList owned_planets;
    for(int i=0; i < planets_count; i++){
        Planet* planet = GetPlanetByIndex(i);
        if (GetFactions()->DoesControlPlanet(faction, planet->id)) {
            owned_planets.Append(RID(i, EntityType::PLANET));
        }
    }
    // Precomputing the worth of different resources
    // Worth(resource, planet) = MaxStock(resource) - Stock(resource, planet)
    double* resource_worth = new double[owned_planets.size*RESOURCE_MAX];
    for(int j=0; j < RESOURCE_MAX; j++) {
        resource_count_t max_amount = 0;
        for(int i=0; i < owned_planets.size; i++) {
            Planet* planet = GetPlanet(owned_planets[i]);
            if (planet->economy.resource_stock[j] > max_amount) {
                max_amount = planet->economy.resource_stock[j];
            }
        }
        for(int i=0; i < owned_planets.size; i++) {
            Planet* planet = GetPlanet(owned_planets[i]);
            if (max_amount == 0) {
                resource_worth[i*RESOURCE_MAX + j] = 0;    
            } else {
                double t = planet->economy.resource_stock[j] / (double)max_amount;
                resource_worth[i*RESOURCE_MAX + j] = Lerp(1/(t+.333), 1-t, t);
            }
        }
    }

    // Print out thing, if necaissary
    for(int i=0; i < owned_planets.size; i++) {
        Planet* planet = GetPlanet(owned_planets[i]);
        StringBuilder sb;
        sb.Add(planet->name).Add(": (");
        for(int j=0; j < RESOURCE_MAX; j++) {
            sb.Add(resource_names[j]).Add(" = ").AddF(resource_worth[i*RESOURCE_MAX + j]);
            if (j < RESOURCE_MAX-1) sb.Add(", ");
        }
        sb.Add(")");
        //INFO(sb.c_str)
    }

    // Iterate over each possibility
    for(int i=0; i < owned_planets.size; i++) {
        int departure_planet_tensor_lookup = IdGetIndex(owned_planets[i])*planets_count*RESOURCE_MAX;
        const Planet* departure_planet = GetPlanet(owned_planets[i]);
        for(int j=0; j < owned_planets.size; j++) {
            int arrival_planet_tensor_lookup = IdGetIndex(owned_planets[j])*RESOURCE_MAX;
            const Planet* arrival_planet = GetPlanet(owned_planets[j]);
            for(int k=0; k < owned_planets.size; k++) {
                int tensor_index = departure_planet_tensor_lookup + arrival_planet_tensor_lookup + k;
                //double utility_per_count = GetUtilityOfTransfer(departure_planet, arrival_planet, (ResourceType) k);
                double utility_per_count = resource_worth[j*RESOURCE_MAX + k] - resource_worth[i*RESOURCE_MAX + k];
                resource_transfer_tensor[tensor_index] = utility_per_count;
            }
        }
    }

    delete[] resource_worth;
    
    // decide on rally point
    if (owned_planets.size > 0) {
        //rally_point_planet = owned_planets[0];
    }
    // launch attack if certain
}

void AIBlackboard::_LowLevelHandleMilitaryShip(Ship* ship) const {
    // Military
    if (!IsIdValidTyped(rally_point_planet, EntityType::PLANET)) {
        return;
    }
    if (ship->GetCapableDV() > _GetMinDVTo(ship, GetPlanet(rally_point_planet))) {
        TransferPlan tp = _HohmannTransferPlan(ship, GetPlanet(rally_point_planet));
        ship->prepared_plans_count = 1;  // Always looks 1 plan ahead
        ship->prepared_plans[0] = tp;
    } else {
        INFO("deltaV (%f < %f) does not allow '%s' : %s => %s", 
            ship->GetCapableDV(), 
            _GetMinDVTo(ship, GetPlanet(rally_point_planet)),
            ship->name,
            GetPlanet(ship->GetParentPlanet())->name,
            GetPlanet(rally_point_planet)->name
        )
        NOT_IMPLEMENTED
    }
}

void AIBlackboard::_LowLevelHandleCivilianShip(Ship* ship) const {
    // Civialian/Transport
    if (!IsIdValidTyped(ship->GetParentPlanet(), EntityType::PLANET)) {
        return;
    }
    AbstractTransfer best_transfer = {
        .departure_planet = GetInvalidId(),
        .arrival_planet = GetInvalidId(),
        .resource_transfer = ResourceTransfer(RESOURCE_NONE, 0),
        .fuel = 0
    };
    double best_utility = -INFINITY;
    for(int i=0; i < GetPlanets()->GetPlanetCount(); i++){
        const Planet* planet = GetPlanetByIndex(i);
        if (planet->id == ship->GetParentPlanet()) {
            continue;
        }
        double dv = _GetMinDVTo(ship, planet);
        if (dv > GetShipClassByIndex(ship->ship_class)->max_dv) {
            continue;
        }
        resource_count_t capacity = ship->GetRemainingPayloadCapacity(dv);  // Calculates capacity for every planet
        resource_count_t fuel = ship->GetFuelRequiredFull(dv);
        for(int i=0; i < RESOURCE_MAX; i++) {
            AbstractTransfer atf = {
                .departure_planet  = ship->GetParentPlanet(),
                .arrival_planet = planet->id,
                .resource_transfer = ResourceTransfer((ResourceType) i, capacity),
                .fuel = fuel
            };
            double utility = CalcTransferUtility(atf);
            if (utility > best_utility) {
                best_utility = utility;
                best_transfer = atf;
            }
        }
    }
    if (best_utility <= 0) {
        return;
    }
    INFO("'%s' (faction %d) : %s => %s", ship->name, faction, 
        GetPlanet(best_transfer.departure_planet)->name,
        GetPlanet(best_transfer.arrival_planet)->name
    );
    TransferPlan tp = _HohmannTransferPlan(ship, GetPlanet(best_transfer.arrival_planet));
    tp.resource_transfer = best_transfer.resource_transfer;
    tp.fuel_mass = best_transfer.fuel;

    ship->prepared_plans_count = 1;  // Always looks 1 plan ahead
    ship->prepared_plans[0] = tp;
}

void AIBlackboard::_LowLevelHandlePlanet(Planet* planet) const {
    if (!GetFactions()->DoesControlPlanet(faction, planet->id)) {
        return;
    }
    if (planet->ship_production_queue.size == 0) {
        RID requested_ship = ShipProductionRequest();
        if (IsIdValid(requested_ship)) {
            planet->ship_production_queue.Append(requested_ship);
        }
    }
    if (planet->module_production_queue.size == 0) {
        RID requested_module = ModuleProductionRequest();
        if (IsIdValid(requested_module)) {
            planet->ship_production_queue.Append(requested_module);
        }
    }
}

void AIBlackboard::LowLevelFactionAI() const {
    for(auto it = GetShips()->alloc.Begin(); it; it++) {
        Ship* ship = GetShip(it.GetId());
        if (ship->allegiance != faction) {
            continue;
        }
        if (ship->prepared_plans_count > 0) {
            continue;
        }
        if (ship->GetCombatStrength() > 0) {
            _LowLevelHandleMilitaryShip(ship);
        } else {
            _LowLevelHandleCivilianShip(ship);
        }
    }
    for(int i=0; i < GetPlanets()->GetPlanetCount(); i++){
        Planet* planet = GetPlanetByIndex(i);
        _LowLevelHandlePlanet(planet);
    }
}
