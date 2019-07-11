

BerryBots (sadd fork) README
=========

Original README of BerryBots
--------------


- Main site:   http://berrybots.com
- GitHub repo: http://github.com/Voidious/BerryBots
- Wiki:        http://berrybots.com/wiki
- API docs:    http://berrybots.com/apidoc

BerryBots is a programming game. A player programs a ship that moves around the
stage, sees gameplay events, and (possibly) shoots at other ships. The ship API
and game rules are simple and it's easy to get started in just a few minutes.

The stage is also configured and controlled by a program, so the gameplay on
every stage is different. The sample stages include mazes, jousting, a race
track, a targeting challenge, several classic arcade style games, and a few
battle stages. These only scratch the surface of what can be done with the stage
API, but you could stay busy for quite a while just writing ships for the sample
stages.

Ships and stages are written in Lua, a lightweight, flexible programming
language designed to be embedded into other programs. BerryBots is available for
Mac, Linux, Windows, and Raspberry Pi.




Additions in this fork
--------------

In this fork of BerryBots (and hopefully in the future in the official version) I 
added and changed mainly some game physics, which in my opinion improve the experience.
As of now it's a work-in-progress, but they are mainly:

- DONE: Improve time stepping of movements so they approximate more closely continuous time physics rather
  than introducing unintuitive discretization errors like the original berrybots does (e.g. in collisions 
  it's difficult to tell how exactly and when exactly a wall collision was applied; this can lead up to 
  errors in position of several ship sizes, so it's difficult for a user to predict ship movement after
  a collision). Sensors, interaction
  with user code, etc. will still stay discretized/can be done once per integer time as in original 
  berrybots, obviously. For both non-relativistic and relativistic kinematics (see below)
  the integration/time stepping is exact to up to machine precision if desired (but I chose a more 
  reasonable error of course), since we only have piecewise constant
  force (except for collisions and torpedos of course, but those are treated in an "impulse approximation" 
  as before, but now at the exactly right time). I rewrote a little bit more stuff than necessary,
  but the plus side it's both more accurate and faster than the original berrybots calculations.
- DONE: Include collision damage for both ship-wall and ship-ship collisions. It's switchable from stage 
  (setWallCollDamage() and setShipShipCollDamage()). Wall collision damage is especially useful, since it
  limits maximum velocities of ships from a gameplay point of view and discourages colliding with walls,
  which both are abusable from my point of view in the original berrybots (especially combined with the
  wall collision physics as discussed above). Added some small animations if damage is taken.
- DONE: Both non-relativistic to relativistic kinematics are available. Relativistic kinematics should 
  for one limit the maximum velocity in a natural/real-life-physical way, and maybe introduce interesting
  challenges for advanced bots. Both the "speed of light" of the game and the laser speed in principle 
  can be chosen independently.
- DONE: I introduced "power" for the bots, which is a resource every action -- firing lasers or torpedos 
  and thrusters -- costs. Balance-wise I started such that a beginner bot which fires all the time when 
  gun heat is low enough and uses the thruster at full throttle basically uses almost all power. This 
  means beginners can just start as in the "vanilla" berrybots and don't have to care about power at 
  first. As a side note it's a little bit unlucky berrybots uses the word "energy" for hit points, 
  and now I have "power" as well...
- DONE: Introduced shielding. A bot can charge shields by pumping power into them to mitigate damage from 
  all sources. It's not super easy though, since the power in the shields dissipates fairly quickly 
  (40% each tick as of now), so charging shields has to be timed well. Shielding never completely negates 
  damage either and has diminishing returns if a lot of power is pumped into them. But I want to promote 
  well timed damage predictions as well as short aggressive strategic moves, but not just easily mitigate 
  damage overall just by blindly shielding. The numbers probably need some tuning in the future, but 
  overall I think it's a nice approach. As mentioned above power is already sparse when firing lasers, 
  torpedos and thrusters all the time, and this is intentionally so "old" bots without shielding are 
  still pretty viable. Shield strength is animated as well.
- DONE: I changed torpedos to an ammo system (10 per round) and reduced the torpedo heat (to 40 from 
  100 for now). So in theory one can fire more torpedos in a short time. but is limited in total per 
  round. As I said I want the bots/players to have the option to have short time strategies within 
  a battle, instead of exclusively the continuous orbiting and wave surfing (but obviously not too 
  much eiter). Think of a bot which charges up shields heavily, charges towards the target bots and 
  thus forces most target bots to orbit in a specific direction, then fires multiple torpedoes in 
  the target bot's path. At least I'm hoping that this way different strategies will evolve and 
  responses to those strategies, etc.
- MAYBE: Change the weapon and energy system. I'm thinking about maybe adding another weapon (ion gun, 
  bevaviour somewhere in-between torpedos and lasers) and have "shields" and "hull" instead of just
  energy. And maybe have so ressource system for weapon firing, e.g. firing a laser costs 3 ressource and
  ion gun costs 5, and ressource only replenishes 1 each tick. I'll have to see if it really improves the
  game or not... But as of now I think the damage and weapon part of the game needs a little bit more depth.





