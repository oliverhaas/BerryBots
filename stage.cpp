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

#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <complex>
#include <limits>
#include <iostream>
#include <cstdio>

#include "bbutil.h"
#include "stage.h"
#include "wall.h"
#include "zone.h"
#include "circle2d.h"
#include "point2d.h"
#include "line2d.h"
#include "filemanager.h"

Stage::Stage(int width, int height) {
  name_ = 0;
  setSize(width, height);
  numWalls_ = 0;
  numWallLines_ = 0;
  numInnerWallLines_ = 0;
  numZones_ = 0;
  numStarts_ = 0;
  numStageShips_ = 0;
  numStageTexts_ = 0;
  startIndex_ = 0;
  for (int x = 0; x < 4; x++) {
    baseWallLines_[x] = 0;
  }
  teams_ = 0;
  numTeams_ = 0;
  ships_ = 0;
  numShips_ = 0;
  numLasers_ = 0;
  numTorpedos_ = 0;
  numEventHandlers_ = 0;
  fileManager_ = new FileManager();
  gfxEnabled_ = false;
  numGfxRectangles_ = 0;
  numGfxLines_ = 0;
  numGfxCircles_ = 0;
  numGfxTexts_ = 0;
  userGfxDisabled_ = false;
  nextLaserId_ = nextTorpedoId_ = 0;
}

void Stage::setName(char *name) {
  if (name_ != 0) {
    delete name_;
  }
  name_ = new char[strlen(name) + 1];
  strcpy(name_, name);
}

const char* Stage::getName() {
  return name_;
}

void Stage::setSize(int width, int height) {
  width_ = width;
  height_ = height;
}

int Stage::getWidth() {
  return width_;
}

int Stage::getHeight() {
  return height_;
}

int Stage::buildBaseWalls() {
  int i = 0;
  i += addWall(-4, -4, 4, height_ + 8, false);
  i += addWall(width_, -4, 4, height_ + 8, false);
  i += addWall(0, -4, width_, 4, false);
  i += addWall(0, height_, width_, 4, false);
  // Checked such that normal vector points away from wall and not inside wall
  baseWallLines_[0] =
      wallLines_[numWallLines_++] = new Line2D(0, 0, width_, 0);
  baseWallLines_[1] =
      wallLines_[numWallLines_++] = new Line2D(width_, 0, width_, height_);
  baseWallLines_[2] =
      wallLines_[numWallLines_++] = new Line2D(width_, height_, 0, height_);
  baseWallLines_[3] =
      wallLines_[numWallLines_++] = new Line2D(0, height_, 0, 0);
  return i;
}

int Stage::addWall(
    int left, int bottom, int width, int height, bool addWallLines) {
  if (numWalls_ >= MAX_WALLS) {
    return 0;
  } else {
    Wall* wall = new Wall(left, bottom, width, height);
    walls_[numWalls_++] = wall;
    if (addWallLines) {
      Line2D** wallLines = wall->getLines();
      for (int x = 0; x < 4; x++) {
        innerWallLines_[numInnerWallLines_++] =
            wallLines_[numWallLines_++] = wallLines[x];
      }
    }
    return 1;
  }
}

Wall** Stage::getWalls() {
  return walls_;
}

int Stage::getWallCount() {
  return numWalls_;
}

int Stage::addZone(
    int left, int bottom, int width, int height, const char *tag) {
  if (numZones_ >= MAX_ZONES) {
    return 0;
  } else {
    if (strlen(tag) > 0) {
      zones_[numZones_++] = new Zone(left, bottom, width, height, tag);
    } else {
      zones_[numZones_++] = new Zone(left, bottom, width, height);
    }
    return 1;
  }
}

Zone** Stage::getZones() {
  return zones_;
}

int Stage::getZoneCount() {
  return numZones_;
}

bool Stage::inZone(Ship *ship, Zone *zone) {
  double shipx = ship->x;
  double shipy = ship->y;
  int zoneLeft = zone->getLeft();
  int zoneBottom = zone->getBottom();
  if (shipx >= zoneLeft && shipx <= zoneLeft + zone->getWidth()
      && shipy >= zoneBottom && shipy <= zoneBottom + zone->getHeight()) {
    return true;
  }
  return false;
}

bool Stage::inZone(Ship *ship, const char *tag) {
  for (int x = 0; x < numZones_; x++) {
    Zone *zone = zones_[x];
    if (strcmp(zone->getTag(), tag) == 0 && inZone(ship, zone)) {
      return true;
    }
  }
  return false;
}

bool Stage::inAnyZone(Ship *ship) {
  for (int x = 0; x < numZones_; x++) {
    if (inZone(ship, zones_[x])) {
      return true;
    }
  }
  return false;
}

bool Stage::touchedZone(Ship *oldShip, Ship *ship, Zone *zone) {
  if (inZone(ship, zone)) {
    return true;
  }

  Line2D *line = new Line2D(oldShip->x, oldShip->y, ship->x, ship->y);
  Line2D **zoneLines = zone->getLines();
  for (int x = 0; x < 4; x++) {
    Line2D *zoneLine = zoneLines[x];
    if (zoneLine->intersects(line)) {
      delete line;
      return true;
    }
  }
  delete line;
  return false;
}

bool Stage::touchedZone(Ship *oldShip, Ship *ship, const char *tag) {
  for (int x = 0; x < numZones_; x++) {
    Zone *zone = zones_[x];
    if (strcmp(zone->getTag(), tag) == 0 && touchedZone(oldShip, ship, zone)) {
      return true;
    }
  }
  return false;
}

bool Stage::touchedAnyZone(Ship *oldShip, Ship *ship) {
  for (int x = 0; x < numZones_; x++) {
    if (touchedZone(oldShip, ship, zones_[x])) {
      return true;
    }
  }
  return false;
}

int Stage::addStart(double x, double y) {
  if (numStarts_ >= MAX_STARTS) {
    return 0;
  } else {
    starts_[numStarts_++] = new Point2D(x, y);
    return 1;
  }
}

Point2D* Stage::getStart() {
  double x, y;
  if (startIndex_ >= numStarts_) {
    x = SHIP_RADIUS + (rand() % (width_ - SHIP_SIZE));
    y = SHIP_RADIUS + (rand() % (height_ - SHIP_SIZE));
  } else {
    Point2D *p = starts_[startIndex_++];
    x = p->getX();
    y = p->getY();
  }

  while (isShipInWall(x, y)) {
    x = limit(SHIP_RADIUS, x + (rand() % SHIP_SIZE) - SHIP_RADIUS,
        width_ - SHIP_RADIUS);
    y = limit(SHIP_RADIUS, y + (rand() % SHIP_SIZE) - SHIP_RADIUS,
        height_ - SHIP_RADIUS);
  }
  return new Point2D(x, y);
}

int Stage::getStartCount() {
  return numStarts_;
}

int Stage::addStageShip(const char *stageShipFilename) {
  if (numStageShips_ >= MAX_STAGE_SHIPS) {
    return 0;
  } else {
    int filenameLen = (int) strlen(stageShipFilename);
    char *newStageShip = new char[filenameLen + 1];
    strcpy(newStageShip, stageShipFilename);
    fileManager_->fixSlashes(newStageShip);
    stageShips_[numStageShips_++] = newStageShip;
    return 1;
  }
}

char** Stage::getStageShips() {
  return stageShips_;
}

int Stage::getStageShipCount() {
  return numStageShips_;
}

bool Stage::isShipInWall(double x, double y) {
  for (int z = 0; z < numWalls_; z++) {
    Wall *wall = walls_[z];
    double left = wall->getLeft();
    double bottom = wall->getBottom();
    if (x > left && x < left + wall->getWidth() && y > bottom
        && y < bottom + wall->getHeight()) {
      return true;
    }
  }

  Circle2D *circ = new Circle2D(x, y, SHIP_RADIUS);
  for (int z = 0; z < numWallLines_; z++) {
    Line2D* line = wallLines_[z];
    if (circ->intersects(line)) {
      delete circ;
      return true;
    }
  }
  delete circ;
  return false;
}

bool Stage::isShipInShip(int shipIndex, double x, double y) {
  Circle2D *circ = new Circle2D(x, y, SHIP_RADIUS);
  bool shipInShip = false;
  for (int z = 0; z < numShips_ && !shipInShip; z++) {
    if (shipIndex != z && ships_[z]->alive) {
      Ship *otherShip = ships_[z];
      Circle2D *otherShipCircle =
          new Circle2D(otherShip->x, otherShip->y, SHIP_RADIUS);
      if (circ->overlaps(otherShipCircle)) {
        shipInShip = true;
      }
      delete otherShipCircle;
    }
  }
  delete circ;
  return shipInShip;
}

int Stage::addStageText(int gameTime, const char *text, double x, double y,
                        int fontSize, RgbaColor textColor, int drawTicks) {
  if (numStageTexts_ >= MAX_STAGE_TEXTS) {
    return 0;
  } else {
    char *newText = new char[strlen(text) + 1];
    strcpy(newText, text);
    StageText *stageText = new StageText;
    stageText->text = newText;
    stageText->x = x;
    stageText->y = y;
    stageText->fontSize = fontSize;
    stageText->textR = textColor.r;
    stageText->textG = textColor.g;
    stageText->textB = textColor.b;
    stageText->textA = textColor.a;
    stageText->startTime = gameTime;
    stageText->drawTicks = drawTicks;
    stageTexts_[numStageTexts_++] = stageText;

    // TODO: Both event handling and having GfxManager fetch these is weird.
    for (int x = 0; x < numEventHandlers_; x++) {
      eventHandlers_[x]->handleStageText(stageText);
    }

    return 1;
  }
}

StageText** Stage::getStageTexts() {
  return stageTexts_;
}

int Stage::getStageTextCount() {
  return numStageTexts_;
}

void Stage::clearStaleStageTexts(int gameTime) {
  for (int x = 0; x < numStageTexts_; x++) {
    StageText *stageText = stageTexts_[x];
    if (gameTime - stageText->startTime >= stageText->drawTicks) {
      if (numStageTexts_ > 1) {
        stageTexts_[x] = stageTexts_[numStageTexts_ - 1];
      }
      delete stageText->text;
      delete stageText;
      numStageTexts_--;
      x--;
    }
  }
}

bool Stage::getGfxEnabled() {
  return gfxEnabled_;
}

void Stage::setGfxEnabled(bool enabled) {
  gfxEnabled_ = enabled;
}

void Stage::disableUserGfx() {
  userGfxDisabled_ = true;
}

void Stage::clearStaleUserGfxs(int gameTime) {
  clearStaleUserGfxRectangles(gameTime);
  clearStaleUserGfxLines(gameTime);
  clearStaleUserGfxCircles(gameTime);
  clearStaleUserGfxTexts(gameTime);
}

int Stage::addUserGfxRectangle(Team *team, int gameTime, double left,
    double bottom, double width, double height, double rotation,
    RgbaColor fillColor, double outlineThickness, RgbaColor outlineColor,
    int drawTicks) {
  if (userGfxDisabled_) {
    return 0;
  } else if ((team == 0 ? numGfxRectangles_ : team->numRectangles)
             >= MAX_USER_RECTANGLES) {
    for (int z = 0; z < numEventHandlers_; z++) {
      eventHandlers_[z]->tooManyUserGfxRectangles(team);
    }
    return 0;
  } else {
    UserGfxRectangle *rectangle = new UserGfxRectangle;
    rectangle->left = left;
    rectangle->bottom = bottom;
    rectangle->width = width;
    rectangle->height = height;
    rectangle->rotation = rotation;
    rectangle->fillR = fillColor.r;
    rectangle->fillG = fillColor.g;
    rectangle->fillB = fillColor.b;
    rectangle->fillA = fillColor.a;
    rectangle->outlineThickness = outlineThickness;
    rectangle->outlineR = outlineColor.r;
    rectangle->outlineG = outlineColor.g;
    rectangle->outlineB = outlineColor.b;
    rectangle->outlineA = outlineColor.a;
    rectangle->startTime = gameTime;
    rectangle->drawTicks = drawTicks;
    if (team == 0) {
      gfxRectangles_[numGfxRectangles_++] = rectangle;
    } else {
      team->gfxRectangles[team->numRectangles++] = rectangle;
    }
    return 1;
  }
}

UserGfxRectangle** Stage::getShipGfxRectangles(int teamIndex) {
  return teams_[teamIndex]->gfxRectangles;
}

int Stage::getShipGfxRectangleCount(int teamIndex) {
  return teams_[teamIndex]->numRectangles;
}

UserGfxRectangle** Stage::getStageGfxRectangles() {
  return gfxRectangles_;
}

int Stage::getStageGfxRectangleCount() {
  return numGfxRectangles_;
}

void Stage::clearStaleUserGfxRectangles(int gameTime) {
  for (int x = 0; x < numTeams_; x++) {
    Team *team = teams_[x];
    team->numRectangles =
        clearStaleUserGfxRectangles(gameTime, team->gfxRectangles,
                                    team->numRectangles);
  }
  numGfxRectangles_ = clearStaleUserGfxRectangles(gameTime, gfxRectangles_,
                                                  numGfxRectangles_);
}

int Stage::clearStaleUserGfxRectangles(int gameTime,
      UserGfxRectangle** gfxRectangles, int numRectangles) {
  for (int y = 0; y < numRectangles; y++) {
    UserGfxRectangle *rectangle = gfxRectangles[y];
    if (gameTime - rectangle->startTime >= rectangle->drawTicks) {
      for (int z = y; z < numRectangles - 1; z++) {
        gfxRectangles[z] = gfxRectangles[z + 1];
      }
      delete rectangle;
      numRectangles--;
      y--;
    }
  }
  return numRectangles;
}

int Stage::addUserGfxLine(Team *team, int gameTime, double x, double y,
    double angle, double length, double thickness, RgbaColor fillColor,
    double outlineThickness, RgbaColor outlineColor, int drawTicks) {
  if (userGfxDisabled_) {
    return 0;
  } else if ((team == 0 ? numGfxLines_ : team->numLines) >= MAX_USER_LINES) {
    for (int z = 0; z < numEventHandlers_; z++) {
      eventHandlers_[z]->tooManyUserGfxLines(team);
    }
    return 0;
  } else {
    UserGfxLine *line = new UserGfxLine;
    line->x = x;
    line->y = y;
    line->angle = angle;
    line->length = length;
    line->thickness = thickness;
    line->fillR = fillColor.r;
    line->fillG = fillColor.g;
    line->fillB = fillColor.b;
    line->fillA = fillColor.a;
    line->outlineThickness = outlineThickness;
    line->outlineR = outlineColor.r;
    line->outlineG = outlineColor.g;
    line->outlineB = outlineColor.b;
    line->outlineA = outlineColor.a;
    line->startTime = gameTime;
    line->drawTicks = drawTicks;
    if (team == 0) {
      gfxLines_[numGfxLines_++] = line;
    } else {
      team->gfxLines[team->numLines++] = line;
    }
    return 1;
  }
}

UserGfxLine** Stage::getShipGfxLines(int teamIndex) {
  return teams_[teamIndex]->gfxLines;
}

int Stage::getShipGfxLineCount(int teamIndex) {
  return teams_[teamIndex]->numLines;
}

UserGfxLine** Stage::getStageGfxLines() {
  return gfxLines_;
}

int Stage::getStageGfxLineCount() {
  return numGfxLines_;
}

void Stage::clearStaleUserGfxLines(int gameTime) {
  for (int x = 0; x < numTeams_; x++) {
    Team *team = teams_[x];
    team->numLines = clearStaleUserGfxLines(gameTime, team->gfxLines,
                                            team->numLines);
  }
  numGfxLines_ = clearStaleUserGfxLines(gameTime, gfxLines_, numGfxLines_);
}

int Stage::clearStaleUserGfxLines(int gameTime, UserGfxLine** gfxLines,
                                  int numLines) {
  for (int y = 0; y < numLines; y++) {
    UserGfxLine *line = gfxLines[y];
    if (gameTime - line->startTime >= line->drawTicks) {
      for (int z = y; z < numLines - 1; z++) {
        gfxLines[z] = gfxLines[z + 1];
      }
      delete line;
      numLines--;
      y--;
    }
  }
  return numLines;
}

int Stage::addUserGfxCircle(Team *team, int gameTime, double x, double y,
    double radius, RgbaColor fillColor, double outlineThickness,
    RgbaColor outlineColor, int drawTicks) {
  if (userGfxDisabled_) {
    return 0;
  } else if ((team == 0 ? numGfxCircles_ : team->numCircles)
             >= MAX_USER_CIRCLES) {
    for (int z = 0; z < numEventHandlers_; z++) {
      eventHandlers_[z]->tooManyUserGfxCircles(team);
    }
    return 0;
  } else {
    UserGfxCircle *circle = new UserGfxCircle;
    circle->x = x;
    circle->y = y;
    circle->radius = radius;
    circle->fillR = fillColor.r;
    circle->fillG = fillColor.g;
    circle->fillB = fillColor.b;
    circle->fillA = fillColor.a;
    circle->outlineThickness = outlineThickness;
    circle->outlineR = outlineColor.r;
    circle->outlineG = outlineColor.g;
    circle->outlineB = outlineColor.b;
    circle->outlineA = outlineColor.a;
    circle->startTime = gameTime;
    circle->drawTicks = drawTicks;
    if (team == 0) {
      gfxCircles_[numGfxCircles_++] = circle;
    } else {
      team->gfxCircles[team->numCircles++] = circle;
    }
    return 1;
  }
}

UserGfxCircle** Stage::getShipGfxCircles(int teamIndex) {
  return teams_[teamIndex]->gfxCircles;
}

int Stage::getShipGfxCircleCount(int teamIndex) {
  return teams_[teamIndex]->numCircles;
}

UserGfxCircle** Stage::getStageGfxCircles() {
  return gfxCircles_;
}

int Stage::getStageGfxCircleCount() {
  return numGfxCircles_;
}

void Stage::clearStaleUserGfxCircles(int gameTime) {
  for (int x = 0; x < numTeams_; x++) {
    Team *team = teams_[x];
    team->numCircles = clearStaleUserGfxCircles(gameTime, team->gfxCircles,
                                                team->numCircles);
  }
  numGfxCircles_ = clearStaleUserGfxCircles(gameTime, gfxCircles_,
                                            numGfxCircles_);
}

int Stage::clearStaleUserGfxCircles(int gameTime, UserGfxCircle** gfxCircles,
                                    int numCircles) {
  for (int y = 0; y < numCircles; y++) {
    UserGfxCircle *circle = gfxCircles[y];
    if (gameTime - circle->startTime >= circle->drawTicks) {
      for (int z = y; z < numCircles - 1; z++) {
        gfxCircles[z] = gfxCircles[z + 1];
      }
      delete circle;
      numCircles--;
      y--;
    }
  }
  return numCircles;
}

int Stage::addUserGfxText(Team *team, int gameTime, const char *text,
    double x, double y, int fontSize, RgbaColor textColor, int drawTicks) {
  if (userGfxDisabled_) {
    return 0;
  } else if ((team == 0 ? numGfxTexts_ : team->numTexts) >= MAX_USER_TEXTS) {
    for (int z = 0; z < numEventHandlers_; z++) {
      eventHandlers_[z]->tooManyUserGfxTexts(team);
    }
    return 0;
  } else {
    UserGfxText *userText = new UserGfxText;
    char *newText = new char[strlen(text) + 1];
    strcpy(newText, text);
    userText->text = newText;
    userText->x = x;
    userText->y = y;
    userText->fontSize = fontSize;
    userText->textR = textColor.r;
    userText->textG = textColor.g;
    userText->textB = textColor.b;
    userText->textA = textColor.a;
    userText->startTime = gameTime;
    userText->drawTicks = drawTicks;
    if (team == 0) {
      gfxTexts_[numGfxTexts_++] = userText;
    } else {
      team->gfxTexts[team->numTexts++] = userText;
    }
    return 1;
  }
}

UserGfxText** Stage::getShipGfxTexts(int teamIndex) {
  return teams_[teamIndex]->gfxTexts;
}

int Stage::getShipGfxTextCount(int teamIndex) {
  return teams_[teamIndex]->numTexts;
}

UserGfxText** Stage::getStageGfxTexts() {
  return gfxTexts_;
}

int Stage::getStageGfxTextCount() {
  return numGfxTexts_;
}

void Stage::clearStaleUserGfxTexts(int gameTime) {
  for (int x = 0; x < numTeams_; x++) {
    Team *team = teams_[x];
    team->numTexts = clearStaleUserGfxTexts(gameTime, team->gfxTexts,
                                            team->numTexts);
  }
  numGfxTexts_ = clearStaleUserGfxTexts(gameTime, gfxTexts_, numGfxTexts_);
}

int Stage::clearStaleUserGfxTexts(int gameTime, UserGfxText** gfxTexts,
                                  int numTexts) {
  for (int y = 0; y < numTexts; y++) {
    UserGfxText *userText = gfxTexts[y];
    if (gameTime - userText->startTime >= userText->drawTicks) {
      for (int z = y; z < numTexts - 1; z++) {
        gfxTexts[z] = gfxTexts[z + 1];
      }
      delete userText;
      numTexts--;
      y--;
    }
  }
  return numTexts;
}

// TODO: It's very confusing that this class takes ownership of ships.
void Stage::setTeamsAndShips(
    Team **teams, int numTeams, Ship **ships, int numShips) {
  teams_ = teams;
  numTeams_ = numTeams;
  ships_ = ships;
  numShips_ = numShips;
}

void Stage::setRelativistic(bool relativistic) {
  relativistic_ = relativistic;
  if (relativistic_) {
    pushFun_ = &relPushFun;
    momentumToVelocity_ = &relMomToVel;
    velocityToMomentum_ = &relVelToMom;
    kineticEnergy_ = &relKinEnergy;  
  } else {
    pushFun_ = &nonRelPushFun;
    momentumToVelocity_ = &nonRelMomToVel;
    velocityToMomentum_ = &nonRelVelToMom;
    kineticEnergy_ = &nonRelKinEnergy;
  }

}

void Stage::setWallCollDamage(bool wallCollDamage) {
  wallCollDamage_ = wallCollDamage;
}

void Stage::setShipShipCollDamage(bool shipShipCollDamage) {
  shipShipCollDamage_ = shipShipCollDamage;
}

double Stage::timeToFirstShipWallCollision(
    Ship **ships, ShipMoveData *shipData, int numShips, double *timeToFirstEvent,
    int *indexShipWallFirstCollided, int *indexWallShipFirstCollided, 
    int *wallEndpoint, int *typeFirstEvent) {
  
  WallEndpointRootStruct wers;
  WallRootStruct wrs;
  wers.pushFun = pushFun_;
  wrs.pushFun = pushFun_;

  for (int ii = 0; ii < numShips; ii++) {
    if (ships[ii]->alive) {
      ShipMoveData *smd = &(shipData[ii]);
      double coordsT[6];
      pushFun_(*timeToFirstEvent, smd->coords, coordsT);
      smd->nextCirc->setPosition(coordsT[0], coordsT[1]);
      for (int jj = 0; jj < numWallLines_; jj++) {
        Line2D* wall = wallLines_[jj];
        // Check end points first
        if (smd->nextCirc->contains(wall->x1(), wall->y1())) { 
          wers.smd = smd; 
          wers.xp = wall->x1();
          wers.yp = wall->y1();
          double tColl;
          double tStart[2] = {0., *timeToFirstEvent};
          newtonBisect(wallEndpointRootFun, tStart, &wers, &tColl, DEFAULT_EPS, DEFAULT_EPS);
          if (tColl < (*timeToFirstEvent)) {
            *timeToFirstEvent = tColl;
            *typeFirstEvent = 3;
            *indexShipWallFirstCollided = ii;
            *indexWallShipFirstCollided = jj;
            *wallEndpoint = 1;
            pushFun_(*timeToFirstEvent, smd->coords, coordsT);
            smd->nextCirc->setPosition(coordsT[0], coordsT[1]);
          }
        } 
        if (smd->nextCirc->contains(wall->x2(), wall->y2())) {
          wers.smd = smd; 
          wers.xp = wall->x2();
          wers.yp = wall->y2();
          double tColl;
          double tStart[2] = {0., *timeToFirstEvent};
          newtonBisect(wallEndpointRootFun, tStart, &wers, &tColl, DEFAULT_EPS, DEFAULT_EPS);           
          if (tColl < (*timeToFirstEvent)) {
            *timeToFirstEvent = tColl;
            *typeFirstEvent = 3;
            *indexShipWallFirstCollided = ii;
            *indexWallShipFirstCollided = jj;
            *wallEndpoint = 2;
            pushFun_(*timeToFirstEvent, smd->coords, coordsT);
            smd->nextCirc->setPosition(coordsT[0], coordsT[1]);
          }
        }
        // Check for middle wall collision
        // And we use line normal/signed distance to check from valid side, i.e.
        // not with wrong side of thin walls
        if (smd->nextCirc->intersects(wall) && 
            wall->signedDistance(smd->nextCirc->h(), smd->nextCirc->k()) > 0.) {
          wrs.smd = smd;
          wrs.wall = wall;
          double tColl;
          double tStart[2] = {0., *timeToFirstEvent};          
          newtonBisect(wallRootFun, tStart, &wrs, &tColl, DEFAULT_EPS, DEFAULT_EPS);
          if (tColl < (*timeToFirstEvent)) { 
            *timeToFirstEvent = tColl;
            *typeFirstEvent = 0;
            *indexShipWallFirstCollided = ii;
            *indexWallShipFirstCollided = jj;
            pushFun_(*timeToFirstEvent, smd->coords, coordsT);
            smd->nextCirc->setPosition(coordsT[0], coordsT[1]);
          }
        }
      }
    }
  }
}
      
double Stage::timeToFirstShipShipCollision(
    Ship **ships, ShipMoveData *shipData, int numShips, double *timeToFirstEvent,
    int *indexShipShipFirstCollided, int *indexShipShipFirstCollided2, int *typeFirstEvent) {
    
  ShipShipRootStruct ssrs;
  
  for (int ii = 0; ii < numShips; ii++) {
    if (ships[ii]->alive) {
      ShipMoveData *smd = &(shipData[ii]);
      for (int jj = 0; jj < ii; jj++) {
        if (ships[jj]->alive) {
          ShipMoveData *smd2 = &(shipData[jj]);
          // Predicted rough movement has to be recalculated here because we always
          // update the actual time step
          double coordsT[6], coordsT2[6];
          pushFun_(*timeToFirstEvent, smd->coords, coordsT);
          pushFun_(*timeToFirstEvent, smd2->coords, coordsT2);
          smd->nextCirc->setPosition(coordsT[0], coordsT[1]);
          smd2->nextCirc->setPosition(coordsT2[0], coordsT2[1]);
          if (smd->nextCirc->overlaps(smd2->nextCirc)) {
            double tColl;
            double tStart[2] = {0., *timeToFirstEvent};
            ssrs.smd = smd;
            ssrs.smd2 = smd2;
            ssrs.pushFun = pushFun_;  
            newtonBisect(shipShipRootFun, tStart, &ssrs, &tColl, DEFAULT_EPS, DEFAULT_EPS);
            if (tColl < (*timeToFirstEvent)) {
              *timeToFirstEvent = tColl;
              *typeFirstEvent = 1;
              *indexShipShipFirstCollided = ii;
              *indexShipShipFirstCollided2 = jj;
            }
          }
        }
      }
    }
  }
}

void Stage::moveAndCheckCollisions(
    Ship **oldShips, Ship **ships, int numShips, int gameTime) {
    
  ShipMoveData *shipData = new ShipMoveData[numShips];

  // Calculate non-collision movement and decide on reasonable sub step.
  int intervals = 1;
  for (int ii = 0; ii < numShips; ii++) {
    Ship *ship = ships[ii];
    ShipMoveData *smd = &shipData[ii];
    smd->initialized = false;
    smd->circ = 0;
    smd->nextCirc = 0;
    if (ship->alive) {
      Ship *oldShip = oldShips[ii];
  
      smd->initialized = true;
      ship->hitWall = false;
      ship->hitShip = false;
      smd->circ = new Circle2D(oldShip->x, oldShip->y, SHIP_RADIUS);
      smd->nextCirc = new Circle2D(0, 0, SHIP_RADIUS);
      
      smd->coords[0] = ship->x;
      smd->coords[1] = ship->y;
      smd->coords[4] = cos(ship->heading) * ship->momentum;
      smd->coords[5] = sin(ship->heading) * ship->momentum;
      smd->coords[6] = cos(ship->thrusterAngle) * ship->thrusterForce;
      smd->coords[7] = sin(ship->thrusterAngle) * ship->thrusterForce;
      momentumToVelocity_(smd->coords[4], smd->coords[5], &smd->coords[2], &smd->coords[3]);

      double coordsT[6];
      pushFun_(1., smd->coords, coordsT);
  
      double moveDistance =
          sqrt(square(coordsT[0] - smd->coords[0]) + square(coordsT[1] - smd->coords[1]));
      int moveIntervals = ceil(moveDistance / COLLISION_FRAME);
      intervals = std::max(intervals, moveIntervals);
    }
  }

  double dtSub = 1./intervals;  // Time step of sub steps
  
  // Logs for laser and torpedo hits and alive ships created outside of sub loop.
  bool **laserHits = new bool*[numShips];
  for (int ii = 0; ii < numShips; ii++) {
    laserHits[ii] = new bool[numShips];
    for (int jj = 0; jj < numShips; jj++) {
      laserHits[ii][jj] = false;
    }
  }
  bool **torpedoHits = new bool*[numShips];
  for (int ii = 0; ii < numShips; ii++) {
    torpedoHits[ii] = new bool[numShips];
    for (int jj = 0; jj < numShips; jj++) {
      torpedoHits[ii][jj] = false;
    }
  }
  bool *wasAlive = new bool[numShips];
  for (int ii = 0; ii < numShips; ii++) {
    wasAlive[ii] = ships[ii]->alive;
  }

  // For lasers fired this tick only, check if they intersect any OTHER ships at
  // their initial position (@ohaas: [-15,15] from origin) before moving the first time.
  checkLaserShipCollisions(ships, shipData, numShips, laserHits, gameTime, true);
                             
  for (int ii = 0; ii < intervals; ii++) {
  
    // Move ships one interval and check for collisions. 
    // Always time push at most until next collision or end of sub-tick
    // time step, then repeat.
    double timeToDo = dtSub;
    while (timeToDo > DEFAULT_EPS*dtSub+DEFAULT_EPS) {

      double timeDo = timeToDo;
      int typeFirstEvent = -1;

      // Check for torpedo explosions
      int indexTorpedoFirstExploded;
      timeToFirstTorpedoExplosion(&timeDo, &indexTorpedoFirstExploded, &typeFirstEvent);

      // Check for ship-ship collisions.
      int indexShipShipFirstCollided, indexShipShipFirstCollided2;
      timeToFirstShipShipCollision(ships, shipData, numShips, &timeDo, &indexShipShipFirstCollided,
                                   &indexShipShipFirstCollided2, &typeFirstEvent);
                                     
      // Check for wall (middle) and wall endpoint collisions
      int wallEndpoint, indexShipWallFirstCollided, indexWallShipFirstCollided;      
      timeToFirstShipWallCollision(ships, shipData, numShips, &timeDo, &indexShipWallFirstCollided,
                                   &indexWallShipFirstCollided, &wallEndpoint, &typeFirstEvent);
      
      // Update remaining time to push until full step
      timeToDo -= timeDo;
      timeDo *= (1.-DEFAULT_EPS);
      
      // Push ships until first event or end of full step
      pushShips(oldShips, ships, shipData, numShips, timeDo);
      
      // Push torpedos
      pushTorpedos(timeDo);
      
      if (typeFirstEvent == 0) {  // First event is wall (middle) collision
      
        doWallCollision(oldShips[indexShipWallFirstCollided], ships[indexShipWallFirstCollided], 
                        &(shipData[indexShipWallFirstCollided]), wallLines_[indexWallShipFirstCollided],
                        gameTime);
        
      } else if (typeFirstEvent == 1) {  // First event is Ship-Ship collision
      
        doShipShipCollision(oldShips, ships, shipData, 
                            indexShipShipFirstCollided, indexShipShipFirstCollided2, gameTime);
                            
      } else if (typeFirstEvent == 2) { // First event is torpedo explosion
      
        explodeTorpedo(oldShips, ships, shipData, numShips, 
                       indexTorpedoFirstExploded, torpedoHits, gameTime);
                       
      } else if (typeFirstEvent == 3) { // First event is wall endpoint collision
      
        doWallEndpointCollision(oldShips[indexShipWallFirstCollided], ships[indexShipWallFirstCollided], 
                                &(shipData[indexShipWallFirstCollided]), wallLines_[indexWallShipFirstCollided],
                                wallEndpoint, gameTime);
                           
      }
      
    }
    
    // Move lasers
    pushLasers(dtSub);
    
    // Check all lasers (only just fired lasers can't collide with the ship they were fired by)
    checkLaserShipCollisions(ships, shipData, numShips, laserHits, gameTime, false);

  }


  // Scoring for destroyed by laser for ships
  // @ohaas: Can be done outside of sub tick move
  //         since it's only for events
  for (int x = 0; x < numShips; x++) {
    Ship *ship = ships[x];
    if (wasAlive[x] && !ship->alive) {
      int numDestroyers = 0;
      for (int y = 0; y < numShips; y++) {
        if (laserHits[y][x] || torpedoHits[y][x]) {
          numDestroyers++;
        }
      }
      Ship **destroyers = new Ship*[numDestroyers];
      int destroyerIndex = 0;
      for (int y = 0; y < numShips; y++) {
        if (laserHits[y][x] || torpedoHits[y][x]) {
          destroyers[destroyerIndex++] = ships[y];
        }
      }
      
      for (int y = 0; y < numShips; y++) {
        if (laserHits[y][x]) {
          double destroyScore = 1.0 / numDestroyers;
          if (ship->teamIndex == ships[y]->teamIndex) {
            ships[y]->friendlyKills += destroyScore;
          } else {
            ships[y]->kills += destroyScore;
          }
        }
      }

      for (int z = 0; z < numEventHandlers_; z++) {
        eventHandlers_[z]->handleShipDestroyed(ship, gameTime, destroyers,
                                               numDestroyers);
      }
      delete destroyers;
    }
  }
  // @ohaas: Kill lasers at walls and clean up dead lasers
  //         Could be done in sub ticks, but once every tick is enough
  for (int x = 0; x < numLasers_; x++) {
    Laser *laser = lasers_[x];
    Line2D *laserLine = laserLines_[x];
    for (int y = 0; y < numWallLines_ && !lasers_[x]->dead; y++) {
      Line2D *wallLine = wallLines_[y];
      if (wallLine->intersects(laserLine)) {
        lasers_[x]->dead = true;
      }
    }
    if (laser->dead) {
      if (numLasers_ > 1) {
        lasers_[x] = lasers_[numLasers_ - 1];
        laserLines_[x] = laserLines_[numLasers_ - 1];
      }
      for (int y = 0; y < numEventHandlers_; y++) {
        eventHandlers_[y]->handleLaserDestroyed(laser, gameTime);
      }
      delete laser;
      delete laserLine;
      numLasers_--;
      x--;
    }
  }

  // Delete laser hit logs
  for (int x = 0; x < numShips; x++) {
    delete laserHits[x];
  }
  delete laserHits;
  // Delete torpedo hit logs
  for (int x = 0; x < numShips; x++) {
    delete torpedoHits[x];
  }
  delete torpedoHits;
  // Delete ship alive logs
  delete wasAlive;  

  // @ohaas: Clean up temporary ship data
  for (int x = 0; x < numShips; x++) {
    ShipMoveData *smd = &(shipData[x]);
    if (smd->initialized) {
      delete smd->circ;
      delete smd->nextCirc;
    }
  }
  delete shipData;
}

void Stage::pushShips(Ship **oldShips, Ship **ships, ShipMoveData *shipData,
    int numShips, double dt) {
  for (int kk = 0; kk < numShips; kk++) {
    if (ships[kk]->alive) {
      ShipMoveData *smd = &(shipData[kk]);
      pushFun_(dt, smd->coords, smd->coords);
      smd->circ->setPosition(smd->coords[0], smd->coords[1]); 
      setShipData(oldShips[kk], ships[kk], smd); 
    }
  }
}
      
void Stage::pushLasers(double dt) {
  for (int kk = 0; kk < numLasers_; kk++) {
    Laser *laser = lasers_[kk];
    laser->x += laser->dx*dt;
    laser->y += laser->dy*dt;
    laserLines_[kk]->shift(laser->dx*dt, laser->dy*dt);
  }
}

void Stage::checkLaserShipCollisions(Ship **ships, ShipMoveData *shipData,
    int numShips, bool **laserHits, int gameTime, bool firstTickLasers) {
    
  for (int ii = 0; ii < numShips; ii++) {
    Ship *ship = ships[ii];
    ShipMoveData *smd = &(shipData[ii]);
    if (ship->alive) {
      for (int jj = 0; jj < numLasers_; jj++) {
        Laser *laser = lasers_[jj];
        // @ohaas: Changed this conditionals slightly to make function more usable
        //         from my point of view.
        if (((laser->fireTime == gameTime && laser->shipIndex != ship->index) || 
              (!firstTickLasers && laser->fireTime != gameTime))
            && smd->circ->intersects(laserLines_[jj])
            && !laser->dead) {
          int firingShipIndex = laser->shipIndex;
          laserHits[firingShipIndex][ii] = true;
          double laserDamage = (ship->energyEnabled ? LASER_DAMAGE : 0);
          double damageScore = (laserDamage / DEFAULT_ENERGY);
          if (ship->teamIndex == ships[firingShipIndex]->teamIndex) {
            ships[firingShipIndex]->friendlyDamage += damageScore;
          } else {
            ships[firingShipIndex]->damage += damageScore;
          }
          ship->energy -= laserDamage;
          for (int kk = 0; kk < numEventHandlers_; kk++) {
            eventHandlers_[kk]->handleLaserHitShip(ships[laser->shipIndex],
                ship, laser, smd->coords[2], smd->coords[3],
                gameTime);
          }

          if (ship->energy <= 0) {
            ship->alive = false;
          }
          laser->dead = true;
        }
      }
    }
  }
}

void Stage::nudgeShip(Ship *oldShip, Ship *ship, ShipMoveData *smd, double angle) {

  // Add a little nudge so we're not inside the wall next time step.
  double epsNudge = 1.e-12*(1. + sqrt(square(width_)+square(height_)));
  smd->circ->setPosition(
      smd->circ->h() + cos(angle)*epsNudge,
      smd->circ->k() + sin(angle)*epsNudge); 
  ship->x = smd->circ->h();
  ship->y = smd->circ->k();
        
  return;
}

void Stage::doWallCollision(Ship *oldShip, Ship *ship, ShipMoveData *smd, Line2D *wall,
                            int gameTime) {

  ship->hitWall = true;
  double energyInitial = kineticEnergy_(smd->coords[4], smd->coords[5]); // For collision damage later
  double normFac = wall->nx()*smd->coords[4] + wall->ny()*smd->coords[5];
  double tangFac = wall->ny()*smd->coords[4] - wall->nx()*smd->coords[5];
  smd->coords[4] = -WALL_BOUNCE*wall->nx()*normFac + wall->ny()*tangFac;
  smd->coords[5] = -WALL_BOUNCE*wall->ny()*normFac - wall->nx()*tangFac;
  momentumToVelocity_(smd->coords[4], smd->coords[5], &smd->coords[2], &smd->coords[3]);
  double angle = atan2(wall->ny(), wall->nx());
  double force = (1.+WALL_BOUNCE)*normFac;
  for (int ii = 0; ii < numEventHandlers_; ii++) {
    eventHandlers_[ii]->handleShipHitWall(ship, angle, force, gameTime);
  }

  // Nudge ship away from collision
  double dist = wall->distance(smd->coords[0], smd->coords[1]) - SHIP_RADIUS;
  if (dist <= DEFAULT_EPS) {
    dist = std::max(-dist,0.) + DEFAULT_EPS*(1. + sqrt(square(smd->coords[0])+square(smd->coords[1])));
    smd->coords[0] += dist*wall->nx();
    smd->coords[1] += dist*wall->ny();
  }
  
  setShipData(oldShip, ship, smd);
  
  // Collision damage
  if (wallCollDamage_) {
    ship->energy += WALL_DMG_SCALE*(kineticEnergy_(smd->coords[4], smd->coords[5]) - energyInitial);;
    if (ship->energy <= 0) {
      ship->alive = false;
    }
  }
  
  return;
}

void Stage::doWallEndpointCollision(Ship *oldShip, Ship *ship, ShipMoveData *smd, Line2D *wall,
                                    int wallEndpoint, int gameTime) {

  ship->hitWall = true;
  double nx, ny, nmaginv;
  if (wallEndpoint == 1) {
    nx = wall->x1() - smd->coords[0];
    ny = wall->y1() - smd->coords[1];
  } else {
    nx = wall->x2() - smd->coords[0];
    ny = wall->y2() - smd->coords[1];
  }
  nmaginv = 1./sqrt(square(nx)+square(ny));
  nx *= nmaginv;
  ny *= nmaginv;
  double energyInitial = kineticEnergy_(smd->coords[4], smd->coords[5]); // For collision damage later
  double normFac = nx*smd->coords[4] + ny*smd->coords[5];
  double tangFac = ny*smd->coords[4] - nx*smd->coords[5];
  smd->coords[4] = -WALL_BOUNCE*nx*normFac + ny*tangFac;
  smd->coords[5] = -WALL_BOUNCE*ny*normFac - nx*tangFac;
  momentumToVelocity_(smd->coords[4], smd->coords[5], &smd->coords[2], &smd->coords[3]);
  double angle = atan2(ny, nx);
  double force = (1.+WALL_BOUNCE)*normFac;
  for (int ii = 0; ii < numEventHandlers_; ii++) {
    eventHandlers_[ii]->handleShipHitWall(ship, angle, force, gameTime);
  }
  
  // Nudge ship away from collision
  double dist = 1./nmaginv - SHIP_RADIUS;
  if (dist <= DEFAULT_EPS) {
    dist = std::max(-dist,0.) + DEFAULT_EPS*(1. + sqrt(square(smd->coords[0])+square(smd->coords[1])));
    smd->coords[0] += dist*nx;
    smd->coords[1] += dist*ny;
  }
  
  setShipData(oldShip, ship, smd);

  // Collision damage
  if (wallCollDamage_) {
    ship->energy += WALL_DMG_SCALE*(kineticEnergy_(smd->coords[4], smd->coords[5]) - energyInitial);;
    if (ship->energy <= 0) {
      ship->alive = false;
    }
  }
  
  return;
  
}

void Stage::doShipShipCollision(Ship **oldShips, Ship **ships, ShipMoveData *shipData, 
                                int indexShipShipFirstCollided, int indexShipShipFirstCollided2,
                                int gameTime) {
                      
  // Update x/y/heading/speeds for ships with ship-ship collisions.
  ShipMoveData *smd = &(shipData[indexShipShipFirstCollided]);
  ShipMoveData *smd2 = &(shipData[indexShipShipFirstCollided2]);
  Ship *ship = ships[indexShipShipFirstCollided];
  Ship *ship2 = ships[indexShipShipFirstCollided2];

  ship->hitShip = true;
  ship2->hitShip = true;

  double angle = atan2(ship2->y - ship->y, ship2->x - ship->x);
  double angle2 = normalRelativeAngle(angle-M_PI);
  double force = cos(ship->heading - angle) * ship->speed + 
                 cos(ship2->heading - angle - M_PI) * ship2->speed;
  force = force*0.5*(1 + SHIP_SHIP_BOUNCE);
  double xForce = cos(angle) * force;
  double yForce = sin(angle) * force;
  double energyInitial = kineticEnergy_(smd->coords[4], smd->coords[5]) +
      kineticEnergy_(smd2->coords[4], smd2->coords[5]); // For collision damage later
  smd->coords[4] -= xForce;
  smd->coords[5] -= yForce;
  momentumToVelocity_(smd->coords[4], smd->coords[5], &smd->coords[2], &smd->coords[3]);
  smd2->coords[4] += xForce;
  smd2->coords[5] += yForce;
  momentumToVelocity_(smd2->coords[4], smd2->coords[5], &smd2->coords[2], &smd2->coords[3]);

  for (int kk = 0; kk < numEventHandlers_; kk++) {
    eventHandlers_[kk]->handleShipHitShip(ship, ship2, angle, force, angle2, force, gameTime);
    eventHandlers_[kk]->handleShipHitShip(ship2, ship, angle2, force, angle, force, gameTime);
  }

  // Nudge ship away from collision if necessary
  double dist = sqrt(square(smd->coords[0]-smd2->coords[0]) + 
    square(smd->coords[1]-smd2->coords[1])) - SHIP_SIZE;
  if (dist <= DEFAULT_EPS) {
    dist = std::max(-dist,0.) + DEFAULT_EPS*(1. + 
      sqrt(square(smd->coords[0])+square(smd->coords[1])) + 
      sqrt(square(smd2->coords[0])+square(smd2->coords[1])));
    smd->coords[0] += -dist*cos(angle);
    smd->coords[1] += -dist*sin(angle);
    smd2->coords[0] += dist*cos(angle);
    smd2->coords[1] += dist*sin(angle);
  }

  setShipData(oldShips[indexShipShipFirstCollided], ship, smd);
  setShipData(oldShips[indexShipShipFirstCollided2], ship2, smd2);

  // Collision damage
  if (shipShipCollDamage_) {
    double energyFinal = kineticEnergy_(smd->coords[4], smd->coords[5]) +
        kineticEnergy_(smd2->coords[4], smd2->coords[5]);
    ship->energy += SHIPSHIP_DMG_SCALE*(energyFinal-energyInitial);
    ship2->energy += SHIPSHIP_DMG_SCALE*(energyFinal-energyInitial);
    if (ship->energy <= 0) {
      ship->alive = false;
    }
    if (ship2->energy <= 0) {
      ship2->alive = false;
    }
  }
  
  return;
}

void Stage::timeToFirstTorpedoExplosion(
    double *timeToFirstEvent, int *torpedoIndex, int* typeFirstEvent) {

  double timeToFirstExplosion;
  for (int ii = 0; ii < numTorpedos_; ii++) {
    Torpedo *torpedo = torpedos_[ii];
    timeToFirstExplosion = (torpedo->distance - torpedo->distanceTraveled)/TORPEDO_SPEED;
    if (timeToFirstExplosion < *timeToFirstEvent) {
      *timeToFirstEvent = timeToFirstExplosion;
      *torpedoIndex = ii;
      *typeFirstEvent = 2;
    }
  }
  return;
}

void Stage::pushTorpedos(double dt) {
  for (int ii = 0; ii < numTorpedos_; ii++) {
    Torpedo *torpedo = torpedos_[ii];
    torpedo->x += torpedo->dx*dt;
    torpedo->y += torpedo->dy*dt;
    torpedo->distanceTraveled += TORPEDO_SPEED*dt;
  }
}

void Stage::explodeTorpedo(Ship **oldShips, Ship **ships, ShipMoveData *shipData, int numShips,
    int torpedoIndex, bool **torpedoHits, int gameTime) {
    
  Torpedo *torpedo = torpedos_[torpedoIndex]; // Torpedos index NOT the same as torpedo id
  
  for (int ii = 0; ii < numShips; ii++) {
    Ship *ship = ships[ii];
    if (ship->alive) {
      ShipMoveData *smd = &(shipData[ii]);
      double distSq = square(torpedo->x - smd->coords[0]) + square(torpedo->y - smd->coords[1]);
      if (distSq < square(TORPEDO_BLAST_RADIUS)) {
        int firingShipIndex = torpedo->shipIndex;
        torpedoHits[firingShipIndex][ii] = true;
        double blastDistance = sqrt(distSq);
        double blastFactor = square(1.0 - (blastDistance / TORPEDO_BLAST_RADIUS));
        double blastForce = blastFactor * TORPEDO_BLAST_FORCE;
        double blastDamage = blastFactor * (ship->energyEnabled ? TORPEDO_BLAST_DAMAGE : 0);
        double blastAngle = atan2(smd->coords[1] - torpedo->y, smd->coords[0] - torpedo->x);
        double damageScore = (blastDamage / DEFAULT_ENERGY);
        if (ship->teamIndex == ships[firingShipIndex]->teamIndex) {
          ships[firingShipIndex]->friendlyDamage += damageScore;
        } else {
          ships[firingShipIndex]->damage += damageScore;
        }

        // TODO: Not sure if handler is called correctly, kinda depends on what it exactly does.
        for (int kk = 0; kk < numEventHandlers_; kk++) {
          eventHandlers_[kk]->handleTorpedoHitShip(ships[torpedo->shipIndex],
              ship, smd->coords[2], smd->coords[3],
              blastAngle, blastForce, blastDamage, gameTime);
        }

        ship->energy -= blastDamage;
        smd->coords[4] += cos(blastAngle) * blastForce;
        smd->coords[5] += sin(blastAngle) * blastForce;
        momentumToVelocity_(smd->coords[4], smd->coords[5], &smd->coords[2], &smd->coords[3]);
        setShipData(oldShips[ii], ship, smd);
        
        if (ship->energy <= 0) {
          ship->alive = false;
        }
      }
    }
  }

  for (int z = 0; z < numEventHandlers_; z++) {
    eventHandlers_[z]->handleTorpedoExploded(torpedo, gameTime);
  }

  if (numTorpedos_ > 1) {
    torpedos_[torpedoIndex] = torpedos_[numTorpedos_ - 1];
  }
  delete torpedo;
  numTorpedos_--;
}

void Stage::setShipData(
    Ship *oldShip, Ship *ship, ShipMoveData *smd) {

  ship->x = smd->coords[0];
  ship->y = smd->coords[1];
  ship->speed = sqrt(square(smd->coords[2]) + square(smd->coords[3]));
  ship->momentum = sqrt(square(smd->coords[4]) + square(smd->coords[5]));
  ship->heading = ((abs(ship->momentum) == 0.)
      ? oldShip->heading : atan2(smd->coords[5], smd->coords[4]));
}

void Stage::updateTeamVision(
    Team **teams, int numTeams, Ship** ships, int numShips, bool** teamVision) {
  for (int x = 0; x < numTeams; x++) {
    Team *team = teams[x];
    for (int y = 0; y < numShips; y++) {
      Ship *ship = ships[y];
      teamVision[x][y] =
          (x != ship->teamIndex) && (team->shipsAlive > 0) && (ship->alive);
    }
  }

  if (numInnerWallLines_ == 0) {
    return;
  }

  for (int x = 0; x < numTeams; x++) {
    Team *team = teams[x];
    for (int y = 0; y < numShips; y++) {
      Ship *visibleShip = ships[y];
      if (x != visibleShip->teamIndex && visibleShip->alive) {
        bool teamHasVision = false;
        for (int z = 0; z < team->numShips && !teamHasVision; z++) {
          int teamShipIndex = team->firstShipIndex + z;
          Ship *teamShip = ships[teamShipIndex];
          if (teamShip->alive) {
            Line2D visionLine(
                teamShip->x, teamShip->y, visibleShip->x, visibleShip->y);
            teamHasVision = hasVision(&visionLine);
          }
        }
        teamVision[x][y] = teamHasVision;
      }
    }
  }
}

bool Stage::hasVision(Line2D *visionLine) {
  for (int z = 0; z < numInnerWallLines_; z++) {
    Line2D* wallLine = innerWallLines_[z];
    if (wallLine->intersects(visionLine)) {
      return false;
    }
  }
  return true;
}

void Stage::updateShipPosition(Ship *ship, double x, double y) {
  while (isShipInWall(x, y) || isShipInShip(ship->index, x, y)) {
    x = limit(SHIP_RADIUS, x + (rand() % SHIP_SIZE) - SHIP_RADIUS,
              width_ - SHIP_RADIUS);
    y = limit(SHIP_RADIUS, y + (rand() % SHIP_SIZE) - SHIP_RADIUS,
              height_ - SHIP_RADIUS);
  }
  ship->x = x;
  ship->y = y;
}

// @ohaas: Changed laser line such that it's centered around firing origin.
//         This makes lasers easier or more intuitive to hit for the user in my opinion.
int Stage::fireLaser(Ship *ship, double heading, int gameTime) {
  if (ship->laserGunHeat > 0 || numLasers_ >= MAX_LASERS) {
    return 0;
  } else {
    double cosHeading = cos(heading);
    double sinHeading = sin(heading);
    double dx = cosHeading * LASER_SPEED;
    double dy = sinHeading * LASER_SPEED;
    double laserX = ship->x + dx*0.5;
    double laserY = ship->y + dy*0.5;
    Line2D laserStartLine(ship->x+dx, ship->y+dy, laserX, laserY);
    if (hasVision(&laserStartLine)) {
      Laser *laser = new Laser;
      laser->id = nextLaserId_++;
      laser->shipIndex = ship->index;
      laser->fireTime = gameTime;
      laser->srcX = ship->x;
      laser->srcY = ship->y;
      laser->x = laserX;
      laser->y = laserY;
      laser->heading = heading;
      laser->dx = dx;
      laser->dy = dy;
      laser->dead = false;
      lasers_[numLasers_] = laser;
      laserLines_[numLasers_++] = new Line2D(
          laser->x - laser->dx, laser->y - laser->dy, laser->x, laser->y);

      for (int z = 0; z < numEventHandlers_; z++) {
        eventHandlers_[z]->handleShipFiredLaser(ship, laser);
      }
    }

    return 1;
  }
}

int Stage::fireTorpedo(
    Ship *ship, double heading, double distance, int gameTime) {
  if (ship->torpedoGunHeat > 0 || numTorpedos_ >= MAX_TORPEDOS) {
    return 0;
  } else {
    Torpedo *torpedo = new Torpedo;
    torpedo->id = nextTorpedoId_++;
    torpedo->shipIndex = ship->index;
    torpedo->fireTime = gameTime;
    double cosHeading = cos(heading);
    double sinHeading = sin(heading);
    double dx = cosHeading * TORPEDO_SPEED;
    double dy = sinHeading * TORPEDO_SPEED;
    torpedo->srcX = ship->x;
    torpedo->srcY = ship->y;
    torpedo->x = ship->x;
    torpedo->y = ship->y;
    torpedo->heading = heading;
//    // @ohaas: Torpedos explode the latest at outer walls.
//    double flightTime = getPosMin(-torpedo->x/dx, (width_-torpedo->x)/dx, 
//                                  -torpedo->y/dy, (height_-torpedo->y)/dy);
//    torpedo->distance = fmin(TORPEDO_SPEED*flightTime, distance);
    torpedo->distance = distance;
    torpedo->dx = dx;
    torpedo->dy = dy;
    torpedo->distanceTraveled = 0.;
    torpedos_[numTorpedos_++] = torpedo;

    for (int z = 0; z < numEventHandlers_; z++) {
      eventHandlers_[z]->handleShipFiredTorpedo(ship, torpedo);
    }

    return 1;
  }
}

Laser** Stage::getLasers() {
  return lasers_;
}

int Stage::getLaserCount() {
  return numLasers_;
}

Torpedo** Stage::getTorpedos() {
  return torpedos_;
}

int Stage::getTorpedoCount() {
  return numTorpedos_;
}

void Stage::destroyShip(Ship *ship, int gameTime) {
  if (ship->alive) {
    ship->alive = false;
    for (int z = 0; z < numEventHandlers_; z++) {
      eventHandlers_[z]->handleShipDestroyed(ship, gameTime, 0, 0);
    }
  }
}

int Stage::addEventHandler(EventHandler *eventHandler) {
  if (numEventHandlers_ >= MAX_EVENT_HANDLERS) {
    return 0;
  } else {
    eventHandlers_[numEventHandlers_++] = eventHandler;
    return 1;
  }
}

void Stage::reset(int time) {
  startIndex_ = 0;
  for (int x = 0; x < numLasers_; x++) {
    for (int y = 0; y < numEventHandlers_; y++) {
      eventHandlers_[y]->handleLaserDestroyed(lasers_[x], time);
    }
    delete lasers_[x];
    delete laserLines_[x];
  }
  numLasers_ = 0;
  for (int x = 0; x < numTorpedos_; x++) {
    for (int y = 0; y < numEventHandlers_; y++) {
      eventHandlers_[y]->handleTorpedoDestroyed(torpedos_[x], time);
    }
    delete torpedos_[x];
  }
  numTorpedos_ = 0;
  for (int x = 0; x < numStageTexts_; x++) {
    delete stageTexts_[x]->text;
    delete stageTexts_[x];
  }
  numStageTexts_ = 0;
}

Stage::~Stage() {
  if (name_ != 0) {
    delete name_;
  }
  for (int x = 0; x < numWalls_; x++) {
    delete walls_[x];
  }
  for (int x = 0; x < numZones_; x++) {
    delete zones_[x];
  }
  for (int x = 0; x < numStarts_; x++) {
    delete starts_[x];
  }
  for (int x = 0; x < 4; x++) {
    if (baseWallLines_[x] != 0) {
      delete baseWallLines_[x];
    }
  }
  for (int x = 0; x < numStageTexts_; x++) {
    delete stageTexts_[x]->text;
    delete stageTexts_[x];
  }
  for (int x = 0; x < numLasers_; x++) {
    delete lasers_[x];
    delete laserLines_[x];
  }
  for (int x = 0; x < numTorpedos_; x++) {
    delete torpedos_[x];
  }
  for (int x = 0; x < numStageShips_; x++) {
    delete stageShips_[x];
  }
  delete ships_;
  delete fileManager_;
}




// Helper functions from here on
void relMomToVel(double px, double py, double *vx, double *vy) {
  double gamInv = 1./sqrt(1.+(square(px)+square(py))/square(SPEED_OF_LIGHT));
  *vx = gamInv*px;
  *vy = gamInv*py;
}

void relVelToMom(double vx, double vy, double *px, double *py) {
  double gam = 1./sqrt(1.-(square(vx)+square(vy))/square(SPEED_OF_LIGHT));
  *px = gam*vx;
  *py = gam*vy;
}

double relKinEnergy(double px, double py) {
  return (sqrt((square(px)+square(py))/square(SPEED_OF_LIGHT) + 1.) - 1.)*square(SPEED_OF_LIGHT);
}

void nonRelMomToVel(double px, double py, double *vx, double *vy) {
  *vx = px;
  *vy = py;
}

void nonRelVelToMom(double vx, double vy, double *px, double *py) {
  *px = vx;
  *py = vy;
}

double nonRelKinEnergy(double px, double py) {
  return 0.5*(square(px) + square(py));
}

void nonRelPushFun(double tt, double* coords0, double* coordsT)  {
  coordsT[0] = coords0[0] + coords0[4]*tt + 0.5*coords0[6]*square(tt);
  coordsT[1] = coords0[1] + coords0[5]*tt + 0.5*coords0[7]*square(tt);
  coordsT[2] = coords0[2] + coords0[6]*tt;
  coordsT[3] = coords0[3] + coords0[7]*tt;
  coordsT[4] = coords0[4] + coords0[6]*tt;
  coordsT[5] = coords0[5] + coords0[7]*tt;
  return;
}

void relPushFun(double tt, double* c0, double* cT)  {

  bool inPlace = (c0 == cT);
  if (inPlace) {
    c0 = new double[8];
    for (int ii = 0; ii < 8; ii++) {
      c0[ii] = cT[ii];
    }
  }

  double p0Sq = square(c0[4]) + square(c0[5]);
  cT[4] = c0[4] + c0[6]*tt;
  cT[5] = c0[5] + c0[7]*tt;
  
  double pTSq = square(cT[4]) + square(cT[5]);
  double gamma = sqrt(1 + pTSq/square(SPEED_OF_LIGHT));
  cT[2] = cT[4]/gamma;
  cT[3] = cT[5]/gamma;
  
  double fmag = sqrt(square(c0[6]) + square(c0[7]));
  if (fmag > DEFAULT_EPS) {
    double root0 = fmag*sqrt(square(SPEED_OF_LIGHT) + p0Sq);
    double rootT = fmag*sqrt(square(SPEED_OF_LIGHT) + pTSq);
    double fac = SPEED_OF_LIGHT/pow(fmag,3);
    double p0f = c0[4]*c0[6] + c0[5]*c0[7];
    double pTf = cT[4]*c0[6] + cT[5]*c0[7];
    double p0Cf = c0[4]*c0[7] - c0[5]*c0[6];
    double term0 = p0Cf*log(p0f + root0);
    double termT = p0Cf*log(pTf + rootT);
    cT[0] = c0[0] + fac*(c0[6]*(rootT-root0) + c0[7]*(termT-term0));
    cT[1] = c0[1] + fac*(c0[7]*(rootT-root0) - c0[6]*(termT-term0));
  } else {
    cT[0] = c0[0] + c0[2]*tt;
    cT[1] = c0[1] + c0[3]*tt;
  }
  
  if (inPlace) {
    delete c0;
  }
  return;
}

int shipShipRootFun(double* tt, void* params, double* out) {

  ShipShipRootStruct *ssrs = static_cast<ShipShipRootStruct*>(params);
  double coordsT[6];
  double coordsT2[6];

  ssrs->pushFun((*tt), ssrs->smd->coords, coordsT);
  ssrs->pushFun((*tt), ssrs->smd2->coords, coordsT2);
    
  out[0] = square(coordsT[0]-coordsT2[0]) + square(coordsT[1]-coordsT2[1]) - square(SHIP_SIZE);
  out[1] = 2.*((coordsT[0]-coordsT2[0])*(coordsT[2]-coordsT2[2]) + 
    (coordsT[1]-coordsT2[1])*(coordsT[3]-coordsT2[3]));
  
  return 0;
}

int wallRootFun(double* tt, void* params, double* out) {

  WallRootStruct *wrs = static_cast<WallRootStruct*>(params);
  double coordsT[6];

  wrs->pushFun((*tt), wrs->smd->coords, coordsT);
    
  out[0] = wrs->wall->signedDistance(coordsT[0], coordsT[1]) - SHIP_RADIUS;
  out[1] = wrs->wall->nx()*coordsT[2] + wrs->wall->ny()*coordsT[3];
  
  return 0;
}

int wallEndpointRootFun(double* tt, void* params, double* out) {

  WallEndpointRootStruct *wers = static_cast<WallEndpointRootStruct*>(params);
  double coordsT[6];

  wers->pushFun((*tt), wers->smd->coords, coordsT);
    
  out[0] = square(coordsT[0]-wers->xp) + square(coordsT[1]-wers->yp) - square(SHIP_RADIUS);
  out[1] = 2.*((coordsT[0]-wers->xp)*coordsT[2] + (coordsT[1]-wers->yp)*coordsT[3]);
  
  return 0;
}


