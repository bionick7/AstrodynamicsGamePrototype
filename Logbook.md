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

Focusing on the UX for transfers is still a good idea. Streamline it as much as possible

Maybe ensure that the mission is fair not only in dv, but also time

## Status 26 Dec 23
The main problem with this concept is that a good game can be played intuitively, while this
concept is inherently unintuitive. Especially focusing on chaining complex maneuvers together.

What when we push discretization all the way?
What when we only allow for hohmann/fastest transfers?
nvm, there is no point in not giving the option

Rather make it clear that the optimization is optional.

What drives the conflict/challenge?
challenge was supposed to come from
- chaining maneuvers as efficiently as possible
- anticipating new problems and placing ships in the right places

For 1st, it is crucial to:
- Communicate efficiently what kind of maneuvers are possible (UX)
- Give incentive to take a faster/more efficient route (Quest manager)
For 2nd, it is crucial to:
- Communicate what planets are relevant in which ways (UX/Writing)

what about planning chains => get a to get b to get c to ...
i.e. crafting? look at Dredge e.g.

=> Important quests are unlocked through dialogue & haggling, that become available every X days
Good idea to start implementing wren for story/quest stuff.

## Status 30 Dec 23
Dynamic Quest System works kinda well.
Main question now is when to show the quests.
Maybe some randomly, some for specific triggers.
Really happy with how well the item slot paradigm works. If we make political stability or so a thing, you could get
more or less secure places to store shit.

Could ultimately also incorporate resources into the item-slot paradigm
Fuel would have to be separate though.

## Status 3 Jan 24
Start work on quests
The focus should be on modules, since it's the most elegant (fun?) part of the game.
Modules can produce resources in space-stations and have the same functionality as buildings
make modules => define quests to get them

## Status 6 Jan 24
Work on quest system was kinda to soon and kinda wasted in hindsight.
Focus should be on how to use modules the best i.e. bring X amount of combat power to Y
The problem is that long-term thinking isn't tested right now and this kinda game relies on long-term thinking.

Idea: raiding party, that randomly attacks certain planets (can be within htf windows) and player only knows after party has left.

## Status 8 Jan 24
Combat is really easy to implement? And intel even more so! Simplicity pays off, it seams. We're on gamejam timescales here
Quest system even helps with prototyping this kind of stuff.
Focus on this and we can actually have a game-ready prototype within a few hours.

## Status 10 Jan 24
I am kinda stuck on how exactly the combat system should work tbh.
For threat, I am thinking of introducing a manager (can be in typst) properly schedule the attacks

## Status 14 Jan 24
Raider wave mvp. This system might have potential for core gameplay loop. Just needs balance, really.
Main balance point is how much damage the ships do each 'turn', i.e. offense vs. defense.
Also, how/where are new ships introduced and what are the remaining civilian ships.
Civilian ship also should be damageable.
Ships building only major implementation rn.

## Status 14 Jan 24
A lot of interesting detail going on if we can manage it.
Key decision:
- What to leave behind on longer journeys
- Defending your stations vs. attacking. (natural bias towards defensive)
- Choosing upgrades
Only thing that needs to be done now is ship building queue
Would probably need to be unrealistically fast

Also implemented ship production in an unreasonably short time. The most daunting tasks are always the easiest

## Status 17 Jan 24
Module building implemented. A way to enforce the control of multiple planets is to force the
acquisition of materials from multiple sources for production of ships and modules

## Status 22 Jan 24
Tension really comes from the distance of the resources to where they are needed.
And the fact that there are more places to protect than protection available.
Both facts must be maintained at all times.
For offensive action, ship following must be implemented
The most straightforward way to play the game is as a competition between player and a single AI.
Each starting on one planet. Different planets can have different boni. + Different planets ^= different Factions
- Mimas: ???
- Encelladus: EPE
- Tethys: THADCON & The merchant fleet
- Rhea: ???
- Titan: Monarch larper
- Iaeptus: The Enclave
- Phoebe: ???

## Status 24 Jan 24
Let's start with 3 factions:
The enclave:
    Iaeptus, Phoebe
    High tech, high discipline
    Focuses on military (Access to better shipclasses/modules)
    
    Ships:
        2 express
        2 express (military)
        shipyard on Iaeptus (+repair)
    Resources:
        1 tech on Iaeptus
        1 metal on Iaeptus
        4 water on Iaeptus

Monarch:
    Titan, Rhea
    Low tech, high resources richness
    Focuses on resources
    
    Ships:
        1 big transport
        2 express (military)
        4 small transports
        shipyard on titan
        shipyard on rhea
    Resources:
        2 metal on titan
        8 water on titan
        4 metal on rhea
        5 water on rhea
    
TADCON:
    Mimas, Encelladus, Thetys
    Mid tech, high infrastructure
    Focuses on logistics (what does that mean practically? Low dv transfers)
    
    Ships:
        2 big transports
        2 small transports
        1 express
        2 small transports (military)
        shipyard + workshop in thetys
    Resources:
        1/2 tech on tethys
        2 metal on encelladus
        1 metal on mimas

Also, maybe set construction time always to 1 to feel out the game and focus on logistics
The player should not be bottlenecked by construction time and accumulate resources

## Status 25 Jan 24
Core game rather cool. Question now is, how to keep the player engaged over long timeperiods.
Content, if you will. Oh yeah and provide a trade tax, definetly

## Status 26 Jan 24
AI system requirements:
- work with every faction
- work with every combination of planets
- work as long as there is an opponent
- not take more than 5 ms per frame
- work with arbitrarily many opponents

- shall not cheat (same limitations as the player) (critical)
- shall prevent the player from being complacent (key)
- shall cover all actions that the player can make (critical)

AI is split into high-level and low-level functions
High-level function converts ai state into a set of goals.
Low-level function converts ai state + set of goals into AI output.
AI Output:
- What transfers make due?
- What is the most urgent production for each shipyard? (module and shipclass)
AI Goal set:
- All have a importance (maybe)
- Can be GOAP: Attack tethys -> Collect xxx combat power with xxx dv
- Tree
Node states: 
  Attack planet xxx
  +- Collect xxx combat power at xxx with xxx dv
     +- Build module xxx at planet
     |  +- Bring xxx to plant
     +- Build ship xxx at planet
        +- Bring xxx to plant
  Defend planet xxx

... 

What is the simplest way to potentially do this:
Maximize: 
    the amount of total combat power
    the right ratio between transport potential and combat potential
    
    equal distribution of resources
    the amount of active military pressure on a specific planet

```
Foreach ship:
    if is_transferring():
        pass
    if military:
        if dv >= dv_to_rally_point:
            transfer_to(blackboard.rally_point)
        else:
            try to gain dv, else do nothing <== critical part
    else:
        for planet in reachable_planets():
            capable_mass = ...
            tf = get_optimal_resource_transfer(current_planet, planet)
            if tf_utility > 0:
                make_transfer()
                
Foreach planet:
    if production_queue_isempty():
        queue_production(blackboard.production_request)
```
Civilian transport kinda works, although a more 'official' definition for planet allegiance is needed