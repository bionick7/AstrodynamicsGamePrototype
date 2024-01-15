import "foreign/quest_base" for Quest
import "foreign/game" for Constants, Game
import "foreign/ship" for Ship

var class_name = "Raiders"

class Raiders is Quest {
	static id { "raiders" }
    static challenge_level { 1 }
    static subject { "Defend against periodic raiders" }
    static contractor { "THADCON" }

    construct new () {
        super("loop")
        _wave_num = 0
        _available_ships = []
    }

    spawn_enemy() {
        var departure_planet = Constants.titan
        var target_planet = Constants.tethys

        var hohmann_tf_result = Game.hohmann_tf(departure_planet, target_planet, Game.now)
        var return_hohmann_tf_result = Game.hohmann_tf(target_planet, departure_planet, hohmann_tf_result["t2"])

        var ship_data = {
            "class_id": "shp_express",
            "is_parked": true,
            "allegiance": 1,
            "name": "The Blind Rat",
            "planet": "Titan",
            "modules": [
                "shpmod_light_armor",
                "shpmod_missiles",
                "shpmod_missiles",
            ],
            "prepared_plans": [
                {
                    "resource_transfer_id": -1,
                    "resource_transfer_qtt": 0,
                    "departure_planet": Game.as_rid(departure_planet, Game.PlanetType),
                    "arrival_planet": Game.as_rid(target_planet, Game.PlanetType),
                    "departure_time": hohmann_tf_result["t1"],
                    "arrival_time": hohmann_tf_result["t2"],
                },
                {
                    "resource_transfer_id": -1,
                    "resource_transfer_qtt": 0,
                    "departure_planet": Game.as_rid(target_planet, Game.PlanetType),
                    "arrival_planet": Game.as_rid(departure_planet, Game.PlanetType),
                    "departure_time": return_hohmann_tf_result["t1"],
                    "arrival_time": return_hohmann_tf_result["t2"],
                }
            ]
        }
        var ship = Ship.spawn(ship_data)
        //ship.on_death = on_ship_death
        _available_ships.add(ship)
    }
    
    serialize() {
        var data = super()
        data["wave_num"] = _wave_num
        return data
    }
    
    deserialize(data) {
        super(data)
        _wave_num = data["wave_num"]
    }

    notify_event(event, args) {
        super(event, args)
        if (event == "die") {
            on_ship_death(args[0])
        }
    }

    on_ship_death(ship) {
        //if (_available_ships.indexOf(ship) >= 0) {
        if (_available_ships.any {|x| x == ship}) {
            Game.spawn_quest("raiders_reward", _wave_num)
        }
    }

    next() {
        super()
        // Remove redundant
        for(ship in _available_ships) {
            if (ship.exists && ship.get_plans().count == 0) {
                _available_ships.remove(ship)
                ship.kill(false)
            }
        }
        if (state == "loop") {
            spawn_enemy()
            //dialogue("???", "Raiders appear")
            goto("wait")
        }
        if (state == "wait") {
            _wave_num = _wave_num + 1
            wait_seconds(Constants.day * 7, "loop")
        }
	}
}