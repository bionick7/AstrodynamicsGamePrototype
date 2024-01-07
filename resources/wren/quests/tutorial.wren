import "foreign/quest_base" for Quest, Constants

var class_name = "Tutorial"
/*
dialogue:
    Alright, so THADCON recently reached out to some survivors on Encelladus. 
    They are in need of a great many things, THADCON could somehow afford to bundle
    together an entire count worth of emergency supplies. So that's our first real
    job. No formal time limit, but hurry up, there's not a lot else to do anyway.

    BR
next: 

*/
class Tutorial is Quest {
	static id { "tutorial" }
    static challenge_level { 0 }
    static subject { "First mission" }
    static contractor { "THADCON" }

    construct new () { super("t1-brief") }
    
    next() {
        super()
        var advisor_name = "PLACEHOLDER_NAME"
        var broker_name = "PLACEHOLDER_NAME 2"
        if (state == "t1-brief") {
            dialogue(advisor_name, 
                """
                Alright, so THADCON recently reached out to some survivors on Encelladus. 
                They are in need of a great many things, THADCON could somehow afford to bundle
                together an entire count worth of emergency supplies. So that's our first real
                job. No formal time limit, but hurry up, there's not a lot else to do anyway.

                BR
                """ + advisor_name
            )
            goto("t1")
        }
        if (state == "t1") {
            var arrival_planet = Constants.mimas
            transport_task(
                1 * Constants.counts,
                Constants.tethys,
                arrival_planet,
                5 * Constants.year,
                "t2-brief","t2-brief"
            )
        }
        if (state == "t2-brief") {
            gain_money(1.0e5)  // Since it's a tutorial, there is no failstate
            dialogue(advisor_name, 
                """
                I understand that we needed to deliver the supply in a hurry, but surely you
                thought about how to get the ship back ? Encelladus is nowhere near maintaining its
                own port and refuling at its current state. Only us and titan really are.

                BR
                """ + advisor_name, 
                [
                    "Uhm fuel?", 
                    "I know what I'm doing, " + advisor_name, 
                    "Delivering the supplies was of utmost importance"
                ],
                ["t2", "t2", "t2"]
            )
        }
        if (state == "t2") {
            // TODO: implement 'move ship' task
            transport_task(
                0,
                arrival_planet,
                Constants.tethys,
                5 * Constants.year,
                "d1-1","d1-1"
            )
        }
        if (state == "d1-1") {
            dialogue(advisor_name, 
                """
                We discussed the possibility with to buy and repair some old water mining ship
                from Bayer-Bosch on encelladus. This way we can extract water from any
                icey moon from the ship.

                BR
                """ + advisor_name, 
                [
                    "Sounds good to me",
                    "I'm not interesed",
                    "Any clue about the price?"
                ],
                [
                    "d1-1.1",
                    "d1-1.2",
                    "d1-1.3",
                ]
            )
        }
        if (state == "d1-1.1") {
            dialogue(advisor_name, "I will get back to you with a selling price in a moment")
            goto("d1-3")

        }
        if (state == "d1-1.2") {
            dialogue(advisor_name, 
                "This has been an ongoing endevour for the last weeks. The price will be decided in a day or two.\n Are you  sure you want to stop the aquisition now?",
                ["I know what I just said", "Fair point, I'll check out the offer then"],
                ["endF", "d1-3"]
            )
            goto("d1-2.1")
        }
        if (state == "d1-2") {
            dialogue(advisor_name, "BB is pretty desperate right now, so I expect it to be cheap. Below 4 million for sure")
            goto("d1-3")
        }
        if (state == "d1-3") {
            wait_seconds(2 * Constants.day)
            goto("d1-4")
        }
        if (state == "d1-4") {
            var current_capital = get_capital()
            var possible_answers = ["Buy it", "Tell them, I'm not interseted", "I'll come back to it later"]
            var possible_routes = ["d1-4.1", "endS", "d1-4.2"]
            if (current_capital < 2e6) {
                possible_answers = ["Can't afford this right now", "I'm not interested, cancel the offer"]
                possible_routes = ["d1-4.3", "endS"]
            }
            dialogue(advisor_name, 
                """
                I have been able to negotiate the price of the ship-ready mining equipment down to M§M 2 M.
                Preliminary forecast predicts that deal to be very advantageous in the long term, but the decision
                is up to you.

                BR
                """ + broker_name, 
                possible_answers,
                possible_routes
            )
        }
        if (state == "d1-4.1") {
            var current_capital = get_capital()

            if (current_capital > 2e6) {
                gain_money(-2e6)
                spawn_ship({
                    "class_id": "shp_spacestation",
                    "is_parked": true,
                    "name": "BB mining ship",
                    "planet": "Encelladus",
                    "modules": ["shpmod_water_extractor", "shpmod_water_extractor"]
                })
            } else {
                Fiber.throwError("Should not reach")
            }
            goto("endS")
        }
        if (state == "d1-4.2") {
            wait_seconds(Constants.day)
            goto("d1-4")
        }
        if (state == "endF") {
            return false
        }
        if (state == "endS") {
            return true
        }
    }
    /*
    main { Fiber.new { |last_result|
        var advisor_name = "PLACEHOLDER_NAME"
        var broker_name = "PLACEHOLDER_NAME 2"
        Fiber.yield(dialogue(advisor_name, 
            """
            Alright, so THADCON recently reached out to some survivors on Encelladus. 
            They are in need of a great many things, THADCON could somehow afford to bundle
            together an entire count worth of emergency supplies. So that's our first real
            job. No formal time limit, but hurry up, there's not a lot else to do anyway.

            BR
            """ + advisor_name
        ))
        
        var arrival_planet = Constants.mimas
        var task1_success = Fiber.yield(transport_task(
            1 * Constants.counts,
            Constants.tethys,
            arrival_planet,
            5 * Constants.year
        ))

        gain_money(1.0e5)  // Since it's a tutorial, there is no failstate
        
        Fiber.yield(dialogue(advisor_name, 
            """
            I understand that we needed to deliver the supply in a hurry, but surely you
            thought about how to get the ship back ? Encelladus is nowhere near maintaining its
            own port and refuling at its current state. Only us and titan really are.

            BR
            """ + advisor_name), 
            [
                "Uhm fuel?", 
                "I know what I'm doing, " + advisor_name, 
                "Delivering the supplies was of utmost importance"
            ]
        )

        // TODO: implement 'move ship' task
        var task2_success = Fiber.yield(transport_task(
            0,
            arrival_planet,
            Constants.tethys,
            5 * Constants.year
        ))

        var mining_ship_answer = Fiber.yield(dialogue(advisor_name, 
            """
            We discussed the possibility with to buy and repair some old water mining ship
            from Bayer-Bosch on encelladus. This way we can extract water from any
            icey moon from the ship.

            BR
            """ + advisor_name), 
            [
                "Sounds good to me",
                "I'm not interesed",
                "Any clue about the price?"
            ]
        )

        if (mining_ship_answer == 0) {
            Fiber.yield(dialogue(advisor_name, "I will get back to you with a selling price in a moment"))
        }
        if (mining_ship_answer == 1) {
            var cancel_confirmation = Fiber.yield(dialogue(advisor_name, 
                "This has been an ongoing endevour for the last weeks. The price will be decided in a day or two.\n Are you  sure you want to stop the aquisition now?"),
                ["I know what I just said", "Fair point, I'll check out the offer then"]
            )
            if (cancel_confirmation == 0) { 
                return true  // End quest
            }
        }
        if (mining_ship_answer == 2) {
            Fiber.yield(dialogue(advisor_name, "BB is pretty desperate right now, so I expect it to be cheap. Below 4 million for sure"))
        }

        Fiber.yield(wait_seconds(2 * Constants.day))
        var choice_keapt_open = true
        while (choice_keapt_open) {
            var current_capital = get_capital()
            var possible_answers = ["Buy it", "Tell them, I'm not interseted", "I'll come back to it later"]
            if (current_capital < 2e6) {
                possible_answers = ["Can't afford this right now", "I'm not interested, cancel the offer"]
            }
            var answer = dialogue(advisor_name, 
                """
                I have been able to negotiate the price of the ship-ready mining equipment down to M§M 2 M.
                Preliminary forecast predicts that deal to be very advantageous in the long term, but the decision
                is up to you.

                BR
                """ + broker_name, 
                possible_answers
            )
            if (answer == 1) {
                return true
            }

            if (answer == 0 && current_capital > 2e6) {
                gain_money(-2e6)
                gain_ship({
                    "class_id": "shp_spacestation",
                    "is_parked": true,
                    "name": "BB mining ship",
                    "planet": "Encelladus",
                    "modules": ["shpmod_water_extractor", "shpmod_water_extractor"]
                })
                return true
            }
        }
    }}
    */
}
