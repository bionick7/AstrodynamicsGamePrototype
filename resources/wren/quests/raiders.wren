import "foreign/quest_base" for Quest, Constants, Game

// Basic idea

// move a random ammount from Encelladus to Tethys
// wait 2 days
// dialogue
// branch:
// - end quest
// - move 1kT between to plantes with time pressure, ensure quest is possible
//   on loose: loose money
//   on win: gain heatshield
// - move 1/2kT between to plantes with time pressure
//   on loose: loose reputation
//   on win: gain reputation + gain access to other quest

var class_name = "Raiders"

class Raiders is Quest {
	static id { "raiders" }
    static challenge_level { 1 }
    static subject { "Defend against periodic raiders" }
    static contractor { "THADCON" }

    construct new () {
        super("loop")
        _wave_num = 0
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
            "planet": "Iaeptus",
            "modules": [
                "shpmod_railgun",
                "shpmod_railgun",
                "shpmod_armor",
                "shpmod_reactor",
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
        spawn_ship(ship_data)
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

    next() {
        super()
        if (state == "loop") {
            spawn_enemy()
            //dialogue("???", "Raiders appear")
            goto("wait")
        }
        if (state == "wait") {
            _wave_num = _wave_num + 1
            wait_seconds(Constants.day * 7, "loop")
        }

        if (_step == "endF") {
            return false
        }
        if (_step == "endS") {
            return true
        }
	}
}