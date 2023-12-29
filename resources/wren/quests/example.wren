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

    construct new () {
        
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
        return require_transport(1000, departure_planet, arrival_planet, departure_time, arrival_planet)
    }
    
    get_random_glory_task() {
        var departure_planet = rand.int(0, 7)
        var arrival_planet = rand.int(0, 7)
        var departure_time = -1
        var arrival_time = -1
        return require_transport(500, departure_planet, arrival_planet, departure_planet, arrival_planet)
    }

    main { Fiber.new { |last_result|
        //var ammount = rand.float(1000, 3000)
        var counts = 2
        var task1_success = Fiber.yield(require_transport(
            counts * Constants.count, 
            Constants.tethys, 
            Constants.encelladus, 
            Constants.day * 4
        ))
        if (task1_success) {
            pay_money(100000)
        } else {
            return false
        }

        Fiber.yield(wait_seconds(2 * Constants.day * 4))

        Fiber.yield(dialogue_text(
            "The enclave administrator",
            "What do you chose ?"
        ))
        dialogue_option = Fiber.yield(dialogue_text(
            "The enclave administrator",
            "What do you chose ?",
            ["end quest", "work for stuff", "work for glory"]
        ))
        if (dialogue_option == 0) {
            var task2_success = Fiber.yield(ensure_possible(get_random_stuff_task))
            if (task2_success) {
                pay_item("heatshield")
            } else {
                pay_money(-100000)
            }
        }
        if (dialogue_option == 1) {
            var task2_success = Fiber.yield(get_random_glory_task())
            if (task2_success) {
                pay_reputation(1000)
            } else {
                pay_reputation(-1000)
            }
        }
        if (dialogue_option == 2) {
            return true
        }
        return false
	}}
}