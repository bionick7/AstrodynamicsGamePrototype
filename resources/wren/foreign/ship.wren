
foreign class Ship {
    foreign id
    foreign name
    foreign exists

    foreign static spawn(ship)
    foreign kill(call_callback)

    foreign set_stat(stat, value)
    foreign get_stat(stat)
    foreign get_plans()
/*
    set_behaviour(name, x) { _behaviour[name] = x }
    set_behaviour_state(name, x) { _behaviour_state[name] = x }
*/
    
    initiative { get_stat("initiative") }
    initiative=(x) { set_stat("initiative", x) }

    ==(x) {x is Ship && id == x.id}
    !=(x) {!(this == x)}
}