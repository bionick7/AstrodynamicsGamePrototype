# TODO/Wishlists

## Immediate
- Build buildings (Done)
- Water as fuel (Done)
- Saving state  (Done)
- Quest system (with wren?) (Done)
- Sound (Done)
- Automatically purchase fuel when possible (Done)
- quantize resources (1 count = 100T) (Done)
- water mining ship (or other more techniques to deal with fuel management?,) (Done)
- Transfer plan UX (Half)
- Inventory (Done)
- Quests drop modules & Ships (Done)
- Drag & Drop (Done)
- Dialogue (Done)
- Actual quests
- Build ships in shipyard (Done)
- Group ships and form fleets (Done)
- Let player choose faction (Done)
- counts should be 10 T to allow more flexibility (Done)
- Planets track allegiance independendtly (Done) (updated in battle)
- AI shits :(

## Known Bugs
- Deleting a transfer plan while editing it is a problem  (Done)
- GetCapableDV does not align with GetAvailablePayload (which is prob. right)  (Done)
- Loadsaving loads default ephemerides (Done)
- DATANODE: Can't distinguish between empty child array and empty string array
- ASTRO: departure time optimization is reliable, but not accurate (found A solution)
- QUESTS: First planning a maneuver, then picking up the quest will bypass payload check (Done)
- SAVING: Ships don't reload correctly (Done)
- ASTRO: Transfer plan cancels wen it shouldn't (Done)
- QUESTS: Accepting more than one quest fucks up the order
- UI: measuring inline (conforming) text cannot rely on raylibs MeassureTextEx

## Necessary Incomplete features
- save wren quest state (Done)

## UX improvements wishlist
Near-term, realizable only
- modules => buildings (Done)
- "loading"bars to show resource scarcity (Done)
- departure time adjusts automatically to the optimum (with option to turn it off) (Done)
- "ASAP" button for transfer UI (mathematically tricky problem tbh)
- ability to show hohmann transfer period in transfer window (eh ... hard to make it be useful)
- quest-log window is kinda outdated (repurposed for dialogue)
- Show battle-log after battle (done)

- Select planet when clicking on orbit (Done)
- list ships to select next to planet
- jump from ship to parent ship and plane
- shift-click in inventory to switch inventory
- ships icons
- modules icons (Done)
- warn before committing to an empty transform ("empty" in red or required to choose it explicitly)
- Recursive tooltips

- Show tasks and wait times also in dialogue
- front-end info and error log
- good ui showing quests
- good ui showing tasks
//- make clear which buildings are buildable
- highlight more important statistics

## Graphics improvements wishlist
- Post-processing shader
- preview small 3D models for flavor
- Render to different buffers/layers
- UI labels as underlined by thin lines pointing to the thing

## Tech wishlist
- Implement way to show icons within text/at arbitrary positions
- global settings (with push_override etc.)
- debug console
- Animation tracker system

- Hot reloading

## Gameplay wishlist
Regardless of implementability
- disruption events (can be tied to morale)
- ship buildings (Done)
- construct new stations (Done, it's just a ship duh)
- combat (Done)
- tanker fleets (share dv among fleet)
- aerobreaking (Done)
- expand to 3D
- gravity assists
- crew & population morale
- "policies"
- planet modifiers
- segment planets into regions
- ship maintenance
- tech tree ?
- expand to planets + moons
- need for first colony ships
- lagrange points & 3-body mechanics


## Collective todo for next milestone
- Animation tracker system
- V Implement way to show icons within text/at arbitrary positions  >> ???
    V 3D
    V 2D
- Mesh loading  >> ???
    preview small 3D models for flavor >> partially implemented by raylib
- V global settings (with push_override etc.) / debug console >> known code
- V Renderlayers?  >> ???
  V Post-processing shader  >> known code + custom bloom

3D Conversion
- V 3D camera  >> partially implemented by raylib
- V 3D orbits  >> existing code
- V 3D planets >> existing code
- V 3D billboards >> existing code
- V 3D click handling >> known code
- V Rings >> Get creative
- V Sky >> Get creative

Art:
- V ships icons
- V modules icons
-   ships meshes

UX tweaking:
- V Select planet when clicking on orbit
-   list ships to select next to planet
- V jump from ship to parent ship and planet
- V shift-click in inventory to switch inventory
-   warn before committing to an empty transform ("empty" in red or required to choose it explicitly)

Tutorial:
- V Physical looking booklet that can appear
- V Include helper tooltips