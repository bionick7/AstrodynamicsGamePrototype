# astrodynamics_concept_3
Need a better name

## Ships
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
Ship modules for:
- engines
- heat shields
-

## Modules

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
- modules breaking
- requiring rare resources from a singular source

Arriving pops should be the only external driver.

# TODO/Wishlists

## Immediate
- build modules (Done)
- water as fuel (Done)
- saving state  (Done)
- build ships in shipyard
- Sound

## Known Bugs
- Deleting a transfer plan 
- Fullscreen doesn't work on linux

## Gameplay issues
- Metal resource loop very lame

## UX improvements wishlist
Near-term, realisable only
- "loading"bars to show resource scarcity
- departure time adjusts automatically to the optimum (with option to turn it off)
- frontend info and error log
- warn before commiting to an empty transform ("empty" in red or required to choose it explicitly)
- "ASAP" button for transfer UI
- highlight more important statistics
- icons for modules

## Graphics improvements wishlist
- Post-processing shader
- different icons for different ships
- preview small 3D models for flavour

## Tech wishlist
- Integrate modules into ECS
- Hot reloading
- cmd console
- global settings (with push_override etc.)

## Gameplay wishlist
Regardless of implementability
- disruption events (can be tied to morale)
- segment planets into regions
- ship modules
- ship maintenance
- planet modifiers
- tech tree
- combat
- expand to 3D
- construct new stations
- aerobreaking
- gravity assists
- lagrange points & 3-body mechanics
- need for first colony ships
- crew & population morale
- "policies"
- expand to planets + moons
