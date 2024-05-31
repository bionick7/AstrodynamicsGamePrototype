
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
    foreign static spawn_quest(name)
    
    static as_rid(index, type) {
        return index | (type << 24)
    }
}