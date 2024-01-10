import "random" for Random

class Constants {
    static mimas      { 0 }
    static encelladus { 1 }
    static tethys     { 2 }
    static rhea       { 3 }
    static titan      { 4 }
    static iaeptus    { 5 }
    static phoebe     { 6 }

    // Time is in seconds
    static second      {         1 }
    static minute      {        60 }
    static hour        {      3600 }
    static day         {     86400 }
    static year        { 86400*365 }
    static no_deadline {        -1 }

    // Mass is in kg
    static count { 100000 }
}

class Game {
    static PlanetType {1}
    static ShipType {2}

    foreign static now
    foreign static hohmann_tf(from, to, t0)
    foreign static spawn_ship(ship)

    static as_rid(index, type) {
        return index | (type << 24)
    }
}

class Quest {
    state { _state }
    state=(x) { _state = x }
    query { _query }
    rand { _random }

    construct new (initial_state) {
        _state = initial_state
        _query = []
        _query_it = 0
        _random = Random.new()
    }
       
    serialize() {
        var data = {}
        data["state"] = state
        return data
    }
    
    deserialize(data) {
        state = data["state"]
    }

    yield(action) {
        _query.add(action)
    }

    transport_task(payload, departure_planet, arrival_planet, deadline, on_success, on_fail) {
        yield({
            "type": "task",
            "payload_mass": payload,
            "departure_planet": departure_planet,
            "arrival_planet": arrival_planet,
            "departure_time_offset": deadline,
            "arrival_time_offset": deadline,
            "on_success": on_success,
            "on_fail": on_fail,
        })
    }

    transport_task(payload, departure_planet, arrival_planet, departure_deadline, arrival_deadline, on_success, on_fail) {
        yield({
            "type": "task",
            "payload_mass": payload,
            "departure_planet": departure_planet,
            "arrival_planet": arrival_planet,
            "departure_time_offset": departure_deadline,
            "arrival_time_offset": arrival_deadline,
            "on_success": on_success,
            "on_fail": on_fail,
        })
    }

    wait_seconds(wait_time, next) {
        yield({
            "type": "wait",
            "wait_time": wait_time,
            "next": next
        })
        //System.print((_query))
    }

    invalid_task {
        yield({
            "type": "invalid"
        })
    }

    dialogue(speaker, text) {
        yield({
            "type": "dialogue",
            "speaker": speaker,
            "text": text,
        })
    }

    dialogue(speaker, text, answer_choices, routes) {
        yield({
            "type": "dialogue choice",
            "speaker": speaker,
            "text": text,
            "answer_choices": answer_choices,
            "answer_routes": routes,
        })
    }

    gain_money(ammount) {
        yield({
            "type": "gain money",
            "ammount": ammount
        })
    }

    gain_module(item, location) {
        yield({
            "type": "gain module",
            "module": item,
            "location": location
        })
    }

    gain_reputation(faction, ammount) {
        yield({
            "type": "gain reputation",
            "faction": faction,
            "ammount": ammount
        })
    }

    goto(next) {
        yield({
            "type": "goto",
            "next": next
        })
    }

    next_result() {
        if (_query_it < _query.count) {
            // System.print("result %(_query[_query_it])")
            var res = _query[_query_it]
            _query_it = _query_it + 1
            return res
        }
        return null
    }

    next() {
        _query = []
        _query_it = 0
    }
}