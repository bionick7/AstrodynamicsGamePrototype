# Game Design Document

## Experimentation:
immediatly buy/sell + disable water as fuel (for now)
get economy semi-enjoyable
add urgency with quests

## Ships
```
light freighter:
    1kT
    4km/s H20
bulk freighter:
    3kT
    4km/s H20
nuclear freighter:
    1kT
    10km/s H2
express:
    1kT
    20km/s H2
long-flight ships ... (modules?)
```
Ship modules for:
- engines
- heat shields
- flight duration extension

ship modules stats:
- mass
- building cost
- maintenance toll

## Buildings

## Decisions
### solar-system scale vs. planetary scale

Driving factors are population nd building expansion
Working over terms of years
\+ Proven concept \
\+ Can introduce a lot of different concepts for buildings and ships
\- Operates on a more detached, more distant level \
\- Only one setting (Solar system)

VS

Driving factors are immediate issues
Working over terms of days
Ships could still be upgradable, buildings less so
\+ More close, personal and concrete in vibe \
\+ Each planet it's own setting \
\- Unproven gameplay
\- Risk of becoming boring very quickly

=> Planetary scale

### population/tech driven vs. quest driven
\+ e.g. Stellaris \
\+ tangible power scaling \
\+ can introduce more tech ideas \
\+ more organic storytelling \
\- not realistic for small scales \
\- Would force very large timescales \

VS

\+ e.g. Endless sky \
\+ more personal \
\+ new tech has a background\
\- more manual work \
\- progress less foreseeable \

=> Give lots of quests that can give new tech, or rare resources

### economy/monetary vs. balancing needs
company RP \
\+ natural and organic filler between quests \
\- hard to make it always worth doing something \
\- needs simulated market \
\- who cares about money as an end goal? \

VS

gov. RP \
\+ naturally high stakes \
\- possibly long situations with nothing to do \

=> Allow for trading non-essential items, but end-goal is to sustain colonies

## Design parameters
Scarcity of quests
lots of quests => pick quests to optimize trajectory
few quests => get inventive with quests
lots of quests w. few exceptional ones

Price of fuel
cheap => encourage frequent use of ships
expensive => plan trips carefully. Plays into scarcity

## Gameplay ideas
Player plays a company/org spanning various colonies.
Other colonies exist.
Goal is to expand by acquiring new colonies and sustain the existing ones.


=> How do we make these 'quests/missions' interesting?
- reach hard-to-reach places
- interesting narrative
- incorporate combat
- yield various components


## Gamplay loops
Loop A << current  focus

Target period: ???
- receive quest
- evaluate quest
- plan transfers
- consider placement of additional ships

Progression vague rn. 
How does loop A play into B in a way that the player can plan? (look: urbek, dredge)
Loop B (main)
- implement tech  << more player agency here
- 

//
- take quests

//
- build up fleet

## Status 14 Dec 23
Ship modules implemented.
The logistics part works, but feels pretty stale on it's own.
The game lacks conflict.

It's time to re-explore combat as a mechanic, using modules.
Keep it simple. This is the plan. Every ship has:
- kinetic offensive (speed advantage)
- ordnance offensive
- point-defense (against missiles)
- kinetic defense (against advantage)
- ordnance defense (against ordnance)
modules change these
Unsure if introducing power is a good idea.

I like the elegance of having ships replace buildings for economy stuff.

## Status 15 Dec 23
Game is approaching an RPG more and more and I'm liking it. (quests, discretization, upgrades, ...)

Focussing on the UX for transfers is still a good idea. Streamline it as much as possible

Maybe ensure that the mission is fair not only in dv, but also time

## Status 26 Dec 23
The main problem with this concept is that a good game can be played intuitively, while this
concept is inherently unintuitive. Especially focusing on chaining complex mauenvers together.

What when we push discretization all the way?
What when we only allow for hohmann/fastest transfers?
nvm, there is no point in not giving the option

Rather make it clear that the optimization is optional.

What drives the conflict/challenge?
challenge was supposed to come from
- chaining mauneuvers as efficiently as possible
- anticipating new problems and placing ships in the right places

For 1st, it is crucial to:
- Communicate efficiently what kind of maneuvers are possible (UX)
- Give incentive to take a faster/more efficient route (Quest manager)
For 2nd, it is crucial to:
- Communicate what planets are relevant in which ways (UX/Writing)

what about planning chains => get a to get b to get c to ...
i.e. crafting? look at Dredge e.g.

=> Important quests are unlocked through dialogue & haggeling, that become available every X days
Good idea to start implementing wren for story/quest stuff.