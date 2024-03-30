import "foreign/quest_base" for Quest
import "foreign/game" for Constants

var class_name = "RaidersReward"

class RaidersReward is Quest {
	static id { "raiders_reward" }
    static challenge_level { 1 }
    static subject { "---" }
    static contractor { "The enclave" }

    static modules_util {[
        "shpmod_heatshield",
        "shpmod_droptank",
        "shpmod_water_extractor",
        "shpmod_acs",
        "shpmod_reactor",
    ]}

    static modules_shipyard {[
        "shpmod_manufacture_offices",
        "shpmod_large_storage",
        "shpmod_workshop",
        "shpmod_wetdock",
    ]}

    static modules_combat {[
        "shpmod_railgun",
        "shpmod_missiles",
        "shpmod_armor",
        "shpmod_pdc",
    ]}

    construct new () {
        super("init")
        _mods = ["shpmod_droptank", "shpmod_droptank", "shpmod_droptank"]
    }

    reward_choice() {
        _mods[0] = RaidersReward.modules_util[rand.int(0, RaidersReward.modules_util.count-1)]
        _mods[1] = RaidersReward.modules_shipyard[rand.int(0, RaidersReward.modules_shipyard.count-1)]
        _mods[2] = RaidersReward.modules_combat[rand.int(0, RaidersReward.modules_combat.count-1)]
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