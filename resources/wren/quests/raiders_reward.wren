import "foreign/quest_base" for Quest
import "foreign/game" for Constants

var class_name = "RaidersReward"

class RaidersReward is Quest {
	static id { "raiders_reward" }
    static challenge_level { 1 }
    static subject { "---" }
    static contractor { "The enclave" }

    static modules {[
        "shpmod_heatshield",
        "shpmod_droptank",
        "shpmod_farm",
        "shpmod_water_extractor",
        "shpmod_small_yard_1",
        "shpmod_small_yard_2",
        "shpmod_small_yard_3",
        "shpmod_small_yard_4",
        "shpmod_railgun",
        "shpmod_missiles",
        "shpmod_armor",
        "shpmod_pdc",
        "shpmod_acs",
        "shpmod_reactor",
    ]}

    construct new () {
        super("init")
        _mods = ["shpmod_droptank", "shpmod_droptank", "shpmod_droptank"]
    }

    reward_choice() {
        for (i in 0...3) {
            var rand_index = rand.int(0, RaidersReward.modules.count-1)
            _mods[i] = RaidersReward.modules[rand_index]
        }
        dialogue(
            "Reward", "Choose your reward",
            _mods,
            ["choice1", "choice2", "choice3"]
        )
    }

    next() {
        super()
        if (state == "init") {
            reward_choice()
        }
        if (state == "choice1") {
            gain_module(_mods[0], Constants.tethys)
            goto("endS")
        }
        if (state == "choice2") {
            gain_module(_mods[1], Constants.tethys)
            goto("endS")
        }
        if (state == "choice3") {
            gain_module(_mods[2], Constants.tethys)
            goto("endS")
        }
	}
}