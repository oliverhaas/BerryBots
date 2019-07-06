

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
  than introducing unintuitive discretization errors (e.g. in collisions). Sensors, interaction
  with user code, etc. will still stay discretized as in original berrybots. For both non-relativistic and relativistic kinematics (see below)
  the integration/time stepping is exact to up to machine precision, since we only have piecewise constant
  accelerations (except for collisions and torpedos of course). I rewrote a little bit more stuff than necessary,
  but the plus side it's both more accurate and faster than the original berrybots calculations.
- DONE: Include collision damage for both ship-wall and ship-ship collisions. It's switchable from stage 
  (setWallCollDamage() and setShipShipCollDamage()). Wall collision damage is especially useful, since it
  limits maximum velocities of ships from a gameplay point of view and discourages colliding with walls,
  which both are abusable from my point of view in the original berrybots (especially combined with the
  wall collision physics as discussed above).
- DONE: Both non-relativistic to relativistic kinematics are available. Relativistic kinematics should 
  for one limit the maximum velocity in a natural/real-life-physical way, and maybe introduce interesting
  challenges for advanced bots. Both the "speed of light" of the game and the laser speed were set to 30
  in this fork, but in principle they can be chosen independently.
- MAYBE: Change the weapon and energy system. I'm thinking about maybe adding another weapon (ion gun, 
  bevaviour somewhere in-between torpedos and lasers) and have "shields" and "hull" instead of just
  energy. And maybe have so ressource system for weapon firing, e.g. firing a laser costs 3 ressource and
  ion gun costs 5, and ressource only replenishes 1 each tick. I'll have to see if it really improves the
  game or not... But as of now I think the damage and weapon part of the game needs a little bit more depth.





