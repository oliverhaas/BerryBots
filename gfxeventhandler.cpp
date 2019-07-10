/*
  Copyright (C) 2012-2013 - Voidious

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <cstdio>
#include "gfxeventhandler.h"

GfxEventHandler::GfxEventHandler() {
  numLaserHits_ = 0;
  numTorpedoHits_ = 0;
  numShipDeaths_ = 0;
  numTorpedoBlasts_ = 0;
  numWallColls_ = 0;
  numShipShipColls_ = 0;
}

void GfxEventHandler::handleLaserHitShip(Ship *srcShip, Ship *targetShip,
    Laser *laser, double dx, double dy, int time) {
  if (numLaserHits_ < MAX_LASERS) {
    LaserHitShipGraphic *hitGraphic = new LaserHitShipGraphic;
    hitGraphic->srcShipIndex = srcShip->index;
    hitGraphic->hitShipIndex = targetShip->index;
    hitGraphic->time = time;
    hitGraphic->x = targetShip->x;
    hitGraphic->y = targetShip->y;
    hitGraphic->dx = dx;
    hitGraphic->dy = dy;
    for (int x = 0; x < NUM_LASER_SPARKS; x++) {
      hitGraphic->offsets[x] = rand() % 360;
    }
    laserHits_[numLaserHits_++] = hitGraphic;
  }
}

void GfxEventHandler::handleTorpedoExploded(Torpedo *torpedo, int time) {
  if (numTorpedoBlasts_ < MAX_TORPEDOS) {
    TorpedoBlastGraphic *blastGraphic = new TorpedoBlastGraphic;
    blastGraphic->x = torpedo->x;
    blastGraphic->y = torpedo->y;
    blastGraphic->time = time;
    torpedoBlasts_[numTorpedoBlasts_++] = blastGraphic;
  }
}

void GfxEventHandler::handleTorpedoHitShip(Ship *srcShip, Ship *targetShip,
    double dx, double dy, double hitAngle, double hitForce, double hitDamage,
    int time) {
  if (numTorpedoHits_ < MAX_TORPEDOS) {
    TorpedoHitShipGraphic *hitGraphic = new TorpedoHitShipGraphic;
    hitGraphic->srcShipIndex = srcShip->index;
    hitGraphic->hitShipIndex = targetShip->index;
    hitGraphic->time = time;
    hitGraphic->x = targetShip->x;
    hitGraphic->y = targetShip->y;
    hitGraphic->dx = dx;
    hitGraphic->dy = dy;
    short numSparks =
        ceil((hitDamage / TORPEDO_BLAST_DAMAGE) * MAX_TORPEDO_SPARKS);
    for (int x = 0; x < numSparks; x++) {
      hitGraphic->offsets[x] = rand() % 360;
      hitGraphic->speeds[x] = 50 + (rand() % 50);
    }
    hitGraphic->numTorpedoSparks = numSparks;
    torpedoHits_[numTorpedoHits_++] = hitGraphic;
  }
}

void GfxEventHandler::handleShipHitWall(Ship *hittingShip,
  double bounceAngle, double bounceForce, double hitDamage, int time) {
  if (numWallColls_ < MAX_WALLCOLLS && hitDamage > 0.) {
    ShipHitWallGraphic *hitGraphic = new ShipHitWallGraphic;
    hitGraphic->shipIndex = hittingShip->index;
    hitGraphic->time = time;
    hitGraphic->x = hittingShip->x - cos(bounceAngle)*SHIP_RADIUS;
    hitGraphic->y = hittingShip->y - sin(bounceAngle)*SHIP_RADIUS;
    short numSparks =
        std::min(MAX_WALLCOLL_SPARKS, (int) ceil(2 + hitDamage/20.*MAX_WALLCOLL_SPARKS));
    for (int ii = 0; ii < numSparks; ii++) {
      hitGraphic->offsets[ii] = (-bounceAngle-0.5*M_PI)*180/M_PI + 20 + (rand() % 140);
      hitGraphic->speeds[ii] = (1 + (rand() % 1))*(-bounceForce);
    }
    hitGraphic->numWallCollSparks = numSparks;
    wallColls_[numWallColls_++] = hitGraphic;
  }
}

void GfxEventHandler::handleShipHitShip(Ship *hittingShip, Ship *targetShip,
    double inAngle, double inForce, double outAngle, double outForce,
    double damage, int time) {
  if (numShipShipColls_ < MAX_SHIPSHIPCOLLS && damage > 0.) {
    ShipHitShipGraphic *hitGraphic = new ShipHitShipGraphic;
    hitGraphic->shipIndex = hittingShip->index;
    hitGraphic->time = time;
    hitGraphic->x = hittingShip->x + cos(inAngle)*SHIP_RADIUS;
    hitGraphic->y = hittingShip->y + sin(inAngle)*SHIP_RADIUS;
    short numSparks =
        std::min(MAX_SHIPSHIPCOLL_SPARKS, (int) ceil(2 + damage/20.*MAX_SHIPSHIPCOLL_SPARKS));
    for (int ii = 0; ii < numSparks; ii++) {
      hitGraphic->offsets[ii] = (-inAngle+0.5*M_PI)*180/M_PI + 20 + (rand() % 140);
      hitGraphic->speeds[ii] = (1 + (rand() % 1))*outForce;
    }
    hitGraphic->numShipShipCollSparks = numSparks;
    shipShipColls_[numShipShipColls_++] = hitGraphic;
  }
}

void GfxEventHandler::handleShipDestroyed(Ship *destroyedShip, int time,
    Ship **destroyerShips, int numDestroyers) {
  if (numShipDeaths_ < MAX_SHIP_DEATHS) {
    ShipDeathGraphic *deathGraphic = new ShipDeathGraphic;
    deathGraphic->shipIndex = destroyedShip->index;
    deathGraphic->x = destroyedShip->x;
    deathGraphic->y = destroyedShip->y;
    deathGraphic->time = time;
    shipDeaths_[numShipDeaths_++] = deathGraphic;
  }
}

LaserHitShipGraphic** GfxEventHandler::getLaserHits() {
  return laserHits_;
}

int GfxEventHandler::getLaserHitCount() {
  return numLaserHits_;
}

void GfxEventHandler::removeLaserHits(int time) {
  for (int x = 0; x < numLaserHits_; x++) {
    LaserHitShipGraphic *laserHit = laserHits_[x];
    if (laserHit->time <= time) {
      if (numLaserHits_ > 1) {
        laserHits_[x] = laserHits_[numLaserHits_ - 1];
      }
      x--;
      numLaserHits_--;
      delete laserHit;
    }
  }
}

TorpedoHitShipGraphic** GfxEventHandler::getTorpedoHits() {
  return torpedoHits_;
}

int GfxEventHandler::getTorpedoHitCount() {
  return numTorpedoHits_;
}

void GfxEventHandler::removeTorpedoHits(int time) {
  for (int x = 0; x < numTorpedoHits_; x++) {
    TorpedoHitShipGraphic *torpedoHit = torpedoHits_[x];
    if (torpedoHit->time <= time) {
      if (numTorpedoHits_ > 1) {
        torpedoHits_[x] = torpedoHits_[numTorpedoHits_ - 1];
      }
      x--;
      numTorpedoHits_--;
      delete torpedoHit;
    }
  }
}

ShipHitWallGraphic** GfxEventHandler::getWallColls() {
  return wallColls_;
}

int GfxEventHandler::getWallCollsCount() {
  return numWallColls_;
}

void GfxEventHandler::removeWallColls(int time) {
  for (int ii = 0; ii < numWallColls_; ii++) {
    ShipHitWallGraphic *wallColl = wallColls_[ii];
    if (wallColl->time <= time) {
      if (numWallColls_ > 1) {
        wallColls_[ii] = wallColls_[numWallColls_ - 1];
      }
      ii--;
      numWallColls_--;
      delete wallColl;
    }
  }
}

ShipHitShipGraphic** GfxEventHandler::getShipShipColls() {
  return shipShipColls_;
}

int GfxEventHandler::getShipShipCollsCount() {
  return numShipShipColls_;
}

void GfxEventHandler::removeShipShipColls(int time) {
  for (int ii = 0; ii < numShipShipColls_; ii++) {
    ShipHitShipGraphic *shipShipColl = shipShipColls_[ii];
    if (shipShipColl->time <= time) {
      if (numShipShipColls_ > 1) {
        shipShipColls_[ii] = shipShipColls_[numShipShipColls_ - 1];
      }
      ii--;
      numShipShipColls_--;
      delete shipShipColl;
    }
  }
}

ShipDeathGraphic** GfxEventHandler::getShipDeaths() {
  return shipDeaths_;
}

int GfxEventHandler::getShipDeathCount() {
  return numShipDeaths_;
}

void GfxEventHandler::removeShipDeaths(int time) {
  for (int x = 0; x < numShipDeaths_; x++) {
    ShipDeathGraphic *shipDeath = shipDeaths_[x];
    if (shipDeath->time <= time) {
      if (numShipDeaths_ > 1) {
        shipDeaths_[x] = shipDeaths_[numShipDeaths_ - 1];
      }
      x--;
      numShipDeaths_--;
      delete shipDeath;
    }
  }
}

TorpedoBlastGraphic** GfxEventHandler::getTorpedoBlasts() {
  return torpedoBlasts_;
}

int GfxEventHandler::getTorpedoBlastCount() {
  return numTorpedoBlasts_;
}

void GfxEventHandler::removeTorpedoBlasts(int time) {
  for (int x = 0; x < numTorpedoBlasts_; x++) {
    TorpedoBlastGraphic *torpedoBlast = torpedoBlasts_[x];
    if (torpedoBlast->time <= time) {
      if (numTorpedoBlasts_ > 1) {
        torpedoBlasts_[x] = torpedoBlasts_[numTorpedoBlasts_ - 1];
      }
      x--;
      numTorpedoBlasts_--;
      delete torpedoBlast;
    }
  }
}

GfxEventHandler::~GfxEventHandler() {
  for (int x = 0; x < numLaserHits_; x++) {
    delete laserHits_[x];
  }
  for (int x = 0; x < numShipDeaths_; x++) {
    delete shipDeaths_[x];
  }
  for (int x = 0; x < numTorpedoBlasts_; x++) {
    delete torpedoBlasts_[x];
  }
  for (int x = 0; x < numWallColls_; x++) {
    delete wallColls_[x];
  }
  for (int x = 0; x < numShipShipColls_; x++) {
    delete shipShipColls_[x];
  }
}
