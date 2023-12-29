class Constants {
    static mimas { 0 }
    static encelladus { 1 }
    static tethys { 2 }
    static rhea { 3 }
    static titan { 4 }
    static iaeptus { 5 }
    static phoebe { 6 }

    // Time is in seconds
    static second { 1 }
    static minute { 60 }
    static hour { 3600 }
    static day { 86400 }
    static year { day*365 }

    // Mass is in kg
    static count { 100000 }
}

class Quest {
    require_transport(payload, departure_planet, arrival_planet, deadline) {
        return {
            "type": "task",
            "payload_mass": payload,
            "departure_planet": departure_planet,
            "arrival_planet": arrival_planet,
            "departure_time_offset": deadline,
            "arrival_time_offset": deadline
        }
    }

    require_transport(payload, departure_planet, arrival_planet, departure_deadline, arrival_deadline) {
        return {
            "type": "task",
            "payload_mass": payload,
            "departure_planet": departure_planet,
            "arrival_planet": arrival_planet,
            "departure_time_offset": departure_deadline,
            "arrival_time_offset": arrival_deadline
        }
    }

    wait_seconds(wait_time) {
        return {
            "type": "wait",
            "wait_time": wait_time
        }
    }

    invalid_task {
        return {
            "type": "invalid"
        }
    }

    dialogue_text(speaker, text) {
        return {
            "type": "dialogue",
            "speaker": speaker,
            "text": text,
        }
    }

    dialogue_text(speaker, text, answer_choices) {
        return {
            "type": "dialogue_choice",
            "speaker": speaker,
            "text": text,
            "answer_choices": answer_choices
        }
    }

    foreign is_task_possible(task)
    foreign pay_money(ammount)
    foreign pay_item(kind)
    foreign pay_reputation(ammount)
    
    ensure_possible(fn) {
        var res = fn.call()
        var counter = 0
        while (!is_quest_possible(quest)) {
            if (counter > 1000) {
                return invalid_task
            }
            res = fn.call()
        }
        return res
    }
}