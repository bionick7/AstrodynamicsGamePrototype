# astrodynamics_concept_3
Need a better name

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

## Gameplay ideas
Key questions ranked in order of importance
- What is the core driver to expand to other planets / specialize industry?
- What is the optimal forced frequency (2M?)

Other ideas:
- research and acquisition of engines limits and controls expansion
- find a way to force exploration ?


## Gameplay issues
- Metal resource loop very lame

## Gamplay loops
Loop A << current  focus

Target period: ???
- evaluate situation
- prioritize connections
- plan transfer

Loop B (main)

Target period: ???
- found colony  << motivation what?
- supply colony
- make self-sufficient
- accomodate people << 2M; Forced frequency good?
- make productive

Loop C (sketch)

Target period: ???
- Unlock resource/tech
- Use resource

Disruptive events
- buildings breaking
- requiring rare resources from a singular source

Arriving pops should be the only external driver.


# TODO/Wishlists

## Immediate
- build buildings (Done)
- water as fuel (Done)
- saving state  (Done)
- build ships in shipyard
- Sound

## Known Bugs
- Deleting a transfer plan while editing it is a problem

## UX improvements wishlist
Near-term, realizable only
- modules => buildings (Done)
- "loading"bars to show resource scarcity V
- departure time adjusts automatically to the optimum (with option to turn it off)
- "ASAP" button for transfer UI
- frontend info and error log
- different icons for different ships
- icons for buildings
- make clear which buildings are buildable
- warn before comitting to an empty transform ("empty" in red or required to choose it explicitly)
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
