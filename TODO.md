
# TODO/Wishlists

## Immediate
- Build buildings (Done)
- Water as fuel (Done)
- Saving state  (Done)
- Quest system (with wren?) ...
- Sound (Done)
- Automatically purchase fuel when possible (Done)
- quantise resources (1 count = 100kT)
- Transfer plan UX
- Implement Ship Docking Rent?
- Build ships in shipyard

## Known Bugs
- Deleting a transfer plan while editing it is a problem  (Done)
- GetCapableDV does not align with GetAvailablePayload (which is prob. right)  (Done)
- Loadsaving loads default wphemerides (Done)
- DATANODE: Can't distinguish between empty child array and empty string array

## UX improvements wishlist
Near-term, realizable only
- modules => buildings (Done)
- "loading"bars to show resource scarcity (Done)
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
