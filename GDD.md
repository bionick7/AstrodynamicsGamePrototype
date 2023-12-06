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
- evaluate situation
- prioritize connections
- plan transfer

//
- evaluate economy
- trade

Loop B (main)

Target period: ???
- aquire colony << motivation what?
- supply colony

//
- take quests

//
- build up fleet

# TODO/Wishlists

## Immediate
- build buildings (Done)
- water as fuel (Done)
- saving state  (Done)
- virtual economy
- quest system (with wren?)
- Sound
- build ships in shipyard

## Known Bugs
- Deleting a transfer plan while editing it is a problem  (Done)
- GetCapableDV does not align with GetAvailablePayload (which is prob. right)  (Done)
- Loadsaving loads default wphemerides

## UX improvements wishlist
Near-term, realizable only
- modules => buildings (Done)
- "loading"bars to show resource scarcity V
- departure time adjusts automatically to the optimum (with option to turn it off)
- "ASAP" button for transfer UI
- front-end info and error log
- different icons for different ships
- icons for buildings
- make clear which buildings are buildable
- warn before committing to an empty transform ("empty" in red or required to choose it explicitly)
- highlight more important statistics

## Graphics improvements wishlist
- UI labels as underlined by thin lines pointing to the thing
- preview small 3D models for flavor
- Post-processing shader

## Tech wishlist
- global settings (with push_override etc.)
- debug console
- Hot reloading
- Integrate buildings into ECS

## Gameplay wishlist
Regardless of implementability
- planet modifiers
- disruption events (can be tied to morale)
- ship buildings
- aerobreaking
- ship maintenance
- tanker fleets
- gravity assists
- segment planets into regions
- crew & population morale
- construct new stations
- combat
- "policies"
- expand to 3D
- tech tree ?
- need for first colony ships
- expand to planets + moons
- lagrange points & 3-body mechanics
