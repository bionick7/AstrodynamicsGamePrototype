import "foreign/quest_base" for Quest
import "foreign/game" for Constants, Game
import "foreign/ship" for Ship

class Tests {
    static assert(x) {
        if (!x) {
            Fiber.abort("assert failed")
        }
    }

    static tests() {
        assert(Game.now == 0)
        //Game.hohmann_tf(Game.as_rid(0, Game.PlanetType), Game.as_rid(1, Game.PlanetType), 0)
        Game.spawn_quest(name)
    }
}