import "foreign/quest_base" for Quest, Constants

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

var class_name = "ExampleQuest"

class ExampleQuest is Quest {
	static id { "example" }
    static challenge_level { 1 }
    static subject { "[EXAMPLE QUEST FOR DEBUGGING AND TESTING]" }
    static contractor { "The enclave" }

    construct new () {
        super("init")
    }

    static test() {
        System.print("hello from wren :)")
        var examlpe_quest = ExampleQuest.new()
        var fiber = examlpe_quest.main
        while (!fiber.isDone) {
            System.print(fiber.call(0))
        }
    }
    
    get_random_stuff_task() {
        var departure_planet = rand.int(0, 7)
        var arrival_planet = rand.int(0, 7)
        var departure_time = -1
        var arrival_time = -1
        return transport_task(1000, departure_planet, arrival_planet, departure_time, arrival_planet)
    }
    
    get_random_glory_task() {
        var departure_planet = rand.int(0, 7)
        var arrival_planet = rand.int(0, 7)
        var departure_time = -1
        var arrival_time = -1
        return transport_task(500, departure_planet, arrival_planet, departure_planet, arrival_planet)
    }

    next() {
        super()
        //var ammount = rand.float(1000, 3000)
        if (state == "init") {
            gain_money(1)
            gain_ship({
                "class_id": "shp_spacestation",
                "is_parked": "y",
                "name": "BB mining ship",
                "planet": "Encelladus",
                "modules": ["shpmod_water_extractor", "shpmod_water_extractor"],
            })
            gain_module("shpmod_heatshield", Constants.tethys)
            gain_module("shpmod_farm", Constants.tethys)
            goto("d1")
        }
        if(state == "t1") {
            transport_task(
                2 * Constants.count, 
                Constants.tethys, 
                Constants.encelladus, 
                Constants.day * 4,
                "t1S", "endF"
            )
        }
        if(state == "t1S") {
            gain_money(100000)
            wait_seconds(2 * Constants.day * 4, "d1")
        }

        if (state == "d1") {
            dialogue(
                "The enclave administrator",
                "What do you chose ?",
                ["end quest", "work for stuff", "work for glory"],
                ["endS", "t1.1", "t1.2"]
            )
        }
        if (state == "t1.1") {
            var task = ensure_possible(get_random_stuff_task)
            task["on_success"] = "t1.1S"
            task["on_fail"] = "t1.1F"
            yield(task)
        }
        if (state == "t1.2") {
            var task = ensure_possible(get_random_glory_task)
            task["on_success"] = "t1.2S"
            task["on_fail"] = "t1.2F"
            yield(task)
        }
        if (state == "t1.1S") {
            gain_item("shpmod_heatshield", Constants.titan)
        }
        if (state == "t1.1F") {
            gain_money(-100000)
        }
        if (state == "t1.2S") {
            gain_reputation("The Enclave", 1000)
        }
        if (state == "t1.2F") {
            gain_reputation("The Enclave", -1000)
        }


        if (_step == "endF") {
            return false
        }
        if (_step == "endS") {
            return true
        }
	}
}