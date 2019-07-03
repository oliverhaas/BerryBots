

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
  with user code, etc. will still stay discretized as up to now. For non-relativistic kinematics (see below)
  the integration/time stepping is exact with Verlet integration, since we only have piecewise constant
  accelerations (except for collisions of course). Collisions are now calculated with analytical formulas,
  since we only have circles and lines this works well. However, if this has to be generalized to relativistic
  kinematics there are not really analytical solutions and it's probably better to change this to something 
  like a bisection search or similar.
- DONE: Include collision damage for both ship-wall and ship-ship collisions. WIP: Hopefully there is an
  easy way to make it switchable from the stage.
- WIP: Maybe change movement physics from non-relativistic to relativistic. This should for one limit
  the maximum velocity, and maybe introduce interesting challenges for fast moving bots, mainly
  in very large stages. If the "speed of light" is chosen large enough, the normal stages wouldn't 
  be affected. Here I would probably need different time stepping algorithms (e.g. Runge-Kutta instead of
  Verlet),





