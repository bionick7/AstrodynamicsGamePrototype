import "foreign/quest_base" for Quest

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

class ExampleQuest is Quest {
	static id { "example" }
    static challenge_level { 1 }

    static get_random_stuff_task() {
        var departure_planet = rand.int(0, 7)
        var arrival_planet = rand.int(0, 7)
        var departure_time = -1
        var arrival_time = -1
        quest = require_transport(1000, departure_planet, arrival_planet, departure_time, arrival_planet)
    }
    
    static get_random_glory_task() {
        var departure_planet = rand.int(0, 7)
        var arrival_planet = rand.int(0, 7)
        var departure_time = -1
        var arrival_time = -1
        quest = require_transport(500, departure_planet, arrival_planet, departure_planet, arrival_planet)
    }

    static main { Fiber.new { |last_result|
        var ammount = rand.float(1000, 3000)
        task1_success = Fiber.yield(require_transport(ammount, encelladus, tethys, -1, -1))
        if (task1_success) {
            pay_money(100000)
        } else {
            return false
        }

        Fiber.yield(wait_seconds(2 * day_to_seconds))

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
            task2_success = Fiber.yield(ensure_possible(get_random_stuff_task))
            if (task2_success) {
                pay_item("heatshield")
            } else {
                pay_money(-100000)
            }
        }
        if (dialogue_option == 1) {
            task2_success = Fiber.yield(get_random_glory_task())
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