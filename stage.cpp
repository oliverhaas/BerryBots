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

  Circle2D *shipCircle = new Circle2D(x, y, SHIP_RADIUS);
  for (int z = 0; z < numWallLines_; z++) {
    Line2D* line = wallLines_[z];
    if (shipCircle->intersects(line)) {
      delete shipCircle;
      return true;
    }
  }
  delete shipCircle;
  return false;
}

bool Stage::isShipInShip(int shipIndex, double x, double y) {
  Circle2D *shipCircle = new Circle2D(x, y, SHIP_RADIUS);
  bool shipInShip = false;
  for (int z = 0; z < numShips_ && !shipInShip; z++) {
    if (shipIndex != z && ships_[z]->alive) {
      Ship *otherShip = ships_[z];
      Circle2D *otherShipCircle =
          new Circle2D(otherShip->x, otherShip->y, SHIP_RADIUS);
      if (shipCircle->overlaps(otherShipCircle)) {
        shipInShip = true;
      }
      delete otherShipCircle;
    }
  }
  delete shipCircle;
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

double Stage::timeToFirstShipWallCollision(
    Ship **ships, ShipMoveData *shipData, int numShips, double *timeToFirstEvent,
    int *indexShipWallFirstCollided, int *indexWallShipFirstCollided, 
    int *wallEndpoint, int *typeFirstEvent) {
  for (int ii = 0; ii < numShips; ii++) {
    if (ships[ii]->alive) {
      ShipMoveData *shipDatum = &(shipData[ii]);
      double thisX = shipDatum->shipCircle->h();
      double thisY = shipDatum->shipCircle->k();
      shipDatum->nextShipCircle->setPosition(
          thisX + shipDatum->xSpeed*(*timeToFirstEvent) + 
            0.5*shipDatum->dxSpeed*square(*timeToFirstEvent),
          thisY + shipDatum->ySpeed*(*timeToFirstEvent)+ 
            0.5*shipDatum->dySpeed*square(*timeToFirstEvent));
      for (int jj = 0; jj < numWallLines_; jj++) {
        Line2D* wall = wallLines_[jj];
        Point2D *p1 = 0;
        Point2D *p2 = 0;
        // Check if something went wrong previous time step
        if (shipDatum->shipCircle->intersects(wall)) {
          std::cout << "oldX " << thisX << " oldY " << thisY << "\n";
          std::cout << "newX " << shipDatum->nextShipCircle->h() << 
                       " newY " << shipDatum->nextShipCircle->k() << "\n";
          std::cout << "xSpeed " << shipDatum->xSpeed << 
                       " ySpeed " << shipDatum->ySpeed << "\n";
          std::cout << "wall->x1() " << wall->x1() << " wall->y1() " << wall->y1() << "\n";
          std::cout << "wall->x2() " << wall->x2() << " wall->y2() " << wall->y2() << "\n";
          throw std::runtime_error( "ship-wall collision error" );
        }
        // Rough check if there could been any collision
        if (shipDatum->nextShipCircle->intersects(wall, &p1, &p2)) {
          bool valid = true;
          if (p1 != 0) {
            Line2D vertexSightLine1(thisX, thisY,
                p1->getX() - (signum(p1->getX() - thisX) * VERTEX_FUDGE),
                p1->getY() - (signum(p1->getY() - thisY) * VERTEX_FUDGE));
            valid = hasVision(&vertexSightLine1);
            delete p1;
          }
          if (p2 != 0) {
            if (valid) {
              Line2D vertexSightLine2(thisX, thisY,
                  p2->getX() - (signum(p2->getX() - thisX) * VERTEX_FUDGE),
                  p2->getY() - (signum(p2->getY() - thisY) * VERTEX_FUDGE));
              valid = hasVision(&vertexSightLine2);
            }
            delete p2;
          }
          // If there was a valid collision (e.g. not with wrong side of thin walls)
          if (valid) {
            double t1, t2, t3, t4, tWall, tEndpoint;
            double dx = wall->x2() - wall->x1();
            double dy = wall->y2() - wall->y1();
            double dxy = wall->x2()*wall->y1() - wall->y2()*wall->x1();
            double aa = 0.5*(dy*shipDatum->dxSpeed - dx*shipDatum->dySpeed);
            double bb = dy*shipDatum->xSpeed - dx*shipDatum->ySpeed;
            double cc = dy*shipDatum->shipCircle->h() - dx*shipDatum->shipCircle->k() + 
                        dxy - SHIP_RADIUS*sqrt(dx*dx+dy*dy);
            
            // First assume ship collides with mid segment of wall
            solveQuadratic(aa, bb, cc, &t1, &t2);
            cc = cc + SHIP_SIZE*sqrt(dx*dx+dy*dy);
            solveQuadratic(aa, bb, cc, &t3, &t4);
            tWall = getPosMin(t1, t2, t3, t4);
            double x0 = thisX + shipDatum->xSpeed*tWall + 0.5*shipDatum->dxSpeed*tWall*tWall;
            double y0 = thisY + shipDatum->ySpeed*tWall + 0.5*shipDatum->dySpeed*tWall*tWall;
            // Closest point on infinite line
            double xc = (square(dx)*x0+dx*dy*y0-dxy*dy)/(square(dx)+square(dy)); 
            double yc = (square(dy)*y0+dx*dy*x0+dxy*dx)/(square(dx)+square(dy));
            // Check if actually on line
            if (wall->contains(xc, yc, 1.e-6) and tWall < (*timeToFirstEvent)) { 
              *timeToFirstEvent = tWall;
              *typeFirstEvent = 0;
              *indexShipWallFirstCollided = ii;
              *indexWallShipFirstCollided = jj;            
            } else if (shipDatum->nextShipCircle->contains(wall->x1(), wall->y1())) { 
              dx = shipDatum->shipCircle->h() - wall->x1();
              dy = shipDatum->shipCircle->k() - wall->y1();
              double xp = shipDatum->xSpeed;
              double yp = shipDatum->ySpeed;
              double xpp = shipDatum->dxSpeed;
              double ypp = shipDatum->dySpeed;

              aa = 0.25*(xpp*xpp + ypp*ypp);
              bb = xp*xpp + yp*ypp;
              cc = square(xp) + dx*xpp + square(yp) + dy*ypp;
              double dd = 2.*(dx*xp + dy*yp);
              double ee = square(dx) + square(dy) - square(SHIP_RADIUS);
              solveQuartic(aa, bb, cc, dd, ee, &t1, &t2, &t3, &t4);
              tEndpoint = getPosMin(t1, t2, t3, t4);  
                  
              if (tEndpoint < (*timeToFirstEvent)) {
                *timeToFirstEvent = tEndpoint;
                *typeFirstEvent = 3;
                *indexShipWallFirstCollided = ii;
                *indexWallShipFirstCollided = jj;
                *wallEndpoint = 1;
              }
            } else if (shipDatum->nextShipCircle->contains(wall->x2(), wall->y2())) { 
              dx = shipDatum->shipCircle->h() - wall->x2();
              dy = shipDatum->shipCircle->k() - wall->y2();
              double xp = shipDatum->xSpeed;
              double yp = shipDatum->ySpeed;
              double xpp = shipDatum->dxSpeed;
              double ypp = shipDatum->dySpeed;

              aa = 0.25*(xpp*xpp + ypp*ypp);
              bb = xp*xpp + yp*ypp;
              cc = square(xp) + dx*xpp + square(yp) + dy*ypp;
              double dd = 2.*(dx*xp + dy*yp);
              double ee = square(dx) + square(dy) - square(SHIP_RADIUS);
              solveQuartic(aa, bb, cc, dd, ee, &t1, &t2, &t3, &t4);
              tEndpoint = getPosMin(t1, t2, t3, t4);  
                  
              if (tEndpoint < (*timeToFirstEvent)) {
                *timeToFirstEvent = tEndpoint;
                *typeFirstEvent = 3;
                *indexShipWallFirstCollided = ii;
                *indexWallShipFirstCollided = jj;
                *wallEndpoint = 2;
              }
            }
          }
        }
      }
    }
  }
}
      
double Stage::timeToFirstShipShipCollision(
    Ship **ships, ShipMoveData *shipData, int numShips, double *timeToFirstEvent,
    int *indexShipShipFirstCollided, int *indexShipShipFirstCollided2, int *typeFirstEvent) {
  for (int ii = 0; ii < numShips; ii++) {
    if (ships[ii]->alive) {
      ShipMoveData *shipDatum = &(shipData[ii]);
      for (int jj = 0; jj < ii; jj++) {
        if (ships[jj]->alive) {
          ShipMoveData *shipDatum2 = &(shipData[jj]);
          if (shipDatum->shipCircle->overlaps(shipDatum2->shipCircle)) {
            throw std::runtime_error( "ship-ship collision error" );
          }
          // Predicted rough movement has to be recalculated here because we always
          // update the actual time step
          shipDatum->nextShipCircle->setPosition(
              shipDatum->shipCircle->h() + shipDatum->xSpeed*(*timeToFirstEvent) + 
                0.5*shipDatum->dxSpeed*square(*timeToFirstEvent),
              shipDatum->shipCircle->k() + shipDatum->ySpeed*(*timeToFirstEvent)+ 
                0.5*shipDatum->dySpeed*square(*timeToFirstEvent));
          shipDatum2->nextShipCircle->setPosition(
              shipDatum2->shipCircle->h() + shipDatum2->xSpeed*(*timeToFirstEvent) + 
                0.5*shipDatum2->dxSpeed*square(*timeToFirstEvent),
              shipDatum2->shipCircle->k() + shipDatum2->ySpeed*(*timeToFirstEvent)+ 
                0.5*shipDatum2->dySpeed*square(*timeToFirstEvent));
          if (shipDatum->nextShipCircle->overlaps(shipDatum2->nextShipCircle)) {
            double aa, bb, cc, dd, ee;
            double t1, t2, t3, t4, tmin;
            double dx = shipDatum2->shipCircle->h() - shipDatum->shipCircle->h();
            double dy = shipDatum2->shipCircle->k() - shipDatum->shipCircle->k();
            double dxp = shipDatum2->xSpeed - shipDatum->xSpeed;
            double dyp = shipDatum2->ySpeed - shipDatum->ySpeed;
            double dxpp = shipDatum2->dxSpeed - shipDatum->dxSpeed;
            double dypp = shipDatum2->dySpeed - shipDatum->dySpeed;
            
            aa = 0.25*(dxpp*dxpp + dypp*dypp);
            bb = dxp*dxpp + dyp*dypp;
            cc = dxp*dxp + dx*dxpp + dyp*dyp + dy*dypp;
            dd = 2.*(dx*dxp + dy*dyp);
            ee = dx*dx + dy*dy - SHIP_SIZE*SHIP_SIZE;

            solveQuartic(aa, bb, cc, dd, ee, &t1, &t2, &t3, &t4);
            
            tmin = getPosMin(t1, t2, t3, t4);  
            if (tmin < (*timeToFirstEvent)) {
              *timeToFirstEvent = tmin;
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

//// Alternative implementation which doesn't solve the quartic but rather uses bisection
//// to find collision time. At the moment I think solving the quartic is both faster and
//// similar in terms of safety.    
//double Stage::timeToFirstShipShipCollision(
//    Ship **ships, ShipMoveData *shipData, int numShips, double *timeToFirstEvent,
//    int *indexShipShipFirstCollided, int *indexShipShipFirstCollided2, int *typeFirstEvent) {
//    
//  for (int ii = 0; ii < numShips; ii++) {
//    if (ships[ii]->alive) {
//      ShipMoveData *shipDatum = &(shipData[ii]);
//      double x1old = shipDatum->shipCircle->h();
//      double y1old = shipDatum->shipCircle->k();
//      double xSpeed1 = shipDatum->xSpeed;
//      double ySpeed1 = shipDatum->ySpeed;
//      double dxSpeed1 = shipDatum->dxSpeed;
//      double dySpeed1 = shipDatum->dySpeed;
//      for (int jj = 0; jj < ii; jj++) {
//        if (ships[jj]->alive) {
//          ShipMoveData *shipDatum2 = &(shipData[jj]);
//          
//          if (shipDatum->shipCircle->overlaps(shipDatum2->shipCircle)) {
//            throw std::runtime_error( "ship-ship collision error" );
//          }
//          
//          double x2old = shipDatum2->shipCircle->h();
//          double y2old = shipDatum2->shipCircle->k();
//          double xSpeed2 = shipDatum2->xSpeed;
//          double ySpeed2 = shipDatum2->ySpeed;
//          double dxSpeed2 = shipDatum2->dxSpeed;
//          double dySpeed2 = shipDatum2->dySpeed;
//          double tmin, tmax, tmid, deltat;
//          double x1new, y1new, x2new, y2new;
//          tmin = 0;
//          tmax = (*timeToFirstEvent);
//          deltat = tmax - tmin;

//          shipDatum->nextShipCircle->setPosition(
//              x1old + xSpeed1*tmax + 0.5*dxSpeed1*square(tmax),
//              y1old + ySpeed1*tmax + 0.5*dySpeed1*square(tmax));
//          shipDatum2->nextShipCircle->setPosition(
//              x2old + xSpeed2*tmax + 0.5*dxSpeed2*square(tmax),
//              y2old + ySpeed2*tmax + 0.5*dySpeed2*square(tmax));
//              
//          if ( not shipDatum->nextShipCircle->overlaps(shipDatum2->nextShipCircle)) {
//            continue;
//          }
//          
//          while (deltat > 1.e-9) {
//            if (tmin > (*timeToFirstEvent)) {
//              break;
//            }
//            tmid = 0.5*(tmax + tmin);
//            shipDatum->nextShipCircle->setPosition(
//                x1old + xSpeed1*tmid + 0.5*dxSpeed1*square(tmid),
//                y1old + ySpeed1*tmid + 0.5*dySpeed1*square(tmid));
//            shipDatum2->nextShipCircle->setPosition(
//                x2old + xSpeed2*tmid + 0.5*dxSpeed2*square(tmid),
//                y2old + ySpeed2*tmid + 0.5*dySpeed2*square(tmid));
//            if (shipDatum->nextShipCircle->overlaps(shipDatum2->nextShipCircle)) {
//              tmax = tmid;
//            } else {
//              tmin = tmid;
//            }
//            deltat = tmax - tmin;
//          }

//          if (tmin < (*timeToFirstEvent)) {
//            *timeToFirstEvent = tmin;
//            *typeFirstEvent = 1;
//            *indexShipShipFirstCollided = ii;
//            *indexShipShipFirstCollided2 = jj;
//          }
//        }
//      }
//    }
//  }
//}

void Stage::moveAndCheckCollisions(
    Ship **oldShips, Ship **ships, int numShips, int gameTime) {
  ShipMoveData *shipData = new ShipMoveData[numShips];

  // Calculate non-collision movement and decide on reasonable sub step.
  int intervals = 1;
  for (int x = 0; x < numShips; x++) {
    Ship *ship = ships[x];
    ShipMoveData shipDatum = shipData[x];
    shipDatum.initialized = false;
    shipDatum.shipCircle = 0;
    shipDatum.nextShipCircle = 0;
    if (ship->alive) {
      shipDatum.initialized = true;
      Ship *oldShip = oldShips[x];
  
      double force = ship->thrusterForce;
      double forceAngle = ship->thrusterAngle;
      shipDatum.dxSpeed = cos(forceAngle) * force;
      shipDatum.dySpeed = sin(forceAngle) * force;
      shipDatum.startXSpeed = cos(ship->heading) * ship->speed;
      shipDatum.startYSpeed = sin(ship->heading) * ship->speed;
      shipDatum.xSpeed = shipDatum.startXSpeed + shipDatum.dxSpeed;
      shipDatum.ySpeed = shipDatum.startYSpeed + shipDatum.dySpeed;
      ship->x += shipDatum.startXSpeed + 0.5*shipDatum.dxSpeed;  // @ohaas: Verlet Integration
      ship->y += shipDatum.startYSpeed + 0.5*shipDatum.dySpeed;  // Exact for constant thrust per step
      ship->hitWall = false;
      ship->hitShip = false;
      setSpeedAndHeading(oldShip, ship, &shipDatum);
  
      shipDatum.shipCircle = new Circle2D(oldShip->x, oldShip->y, SHIP_RADIUS);
      shipDatum.nextShipCircle = new Circle2D(0, 0, SHIP_RADIUS);
      shipDatum.wallCollision = false;
      shipDatum.minWallImpactDiff = M_PI;
      shipDatum.wallImpactAngle = 0;
      shipDatum.wallImpactLine = 0;
      shipDatum.shipCollision = false;
      shipDatum.stopped = false;
  
      double moveDistance =
          sqrt(square(ship->x - oldShip->x) + square(ship->y - oldShip->y));
      int moveIntervals = ceil(moveDistance / COLLISION_FRAME);
      intervals = std::max(intervals, moveIntervals);
    }
    shipData[x] = shipDatum;
  }

  double dtSub = 1./intervals;  // @ohaas: Time step of sub steps
  
  // Set inital speed
  for (int x = 0; x < numShips; x++) {
    Ship *ship = ships[x];
    ShipMoveData shipDatum = shipData[x];
    if (ship->alive) {
      Ship *oldShip = oldShips[x];
      ship->x = oldShips[x]->x;
      ship->y = oldShips[x]->y;
      shipDatum.xSpeed = shipDatum.startXSpeed;
      shipDatum.ySpeed = shipDatum.startYSpeed;
    }
  }


  // Logs for laser and torpedo hits and alive ships created outside of sub loop.
  bool **laserHits = new bool*[numShips];
  for (int x = 0; x < numShips; x++) {
    laserHits[x] = new bool[numShips];
    for (int y = 0; y < numShips; y++) {
      laserHits[x][y] = false;
    }
  }
  bool **torpedoHits = new bool*[numShips];
  for (int x = 0; x < numShips; x++) {
    torpedoHits[x] = new bool[numShips];
    for (int y = 0; y < numShips; y++) {
      torpedoHits[x][y] = false;
    }
  }
  bool *wasAlive = new bool[numShips];
  for (int x = 0; x < numShips; x++) {
    wasAlive[x] = ships[x]->alive;
  }

  // For lasers fired this tick only, check if they intersect any OTHER ships at
  // their initial position (@ohaas: [-15,15] from origin) before moving the first time.
  checkLaserShipCollisions(ships, shipData, numShips, laserHits, numLasers_,
                           gameTime, true);
                             
  for (int x = 0; x < intervals; x++) {
  
    // Move ships one interval and check for collisions. 
    // Always time push at most until next collision or end of sub-tick
    // time step, then repeat.
    double timeToDo = dtSub;
    while (timeToDo > 1.e-9*dtSub) {

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
      timeDo *= (1. - 1.e-12);   // Push a little bit less than calculated so we don't get stuck in a wall
      timeToDo -= timeDo;
      
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
    checkLaserShipCollisions(ships, shipData, numShips, laserHits, numLasers_,
                             gameTime, false);

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
    ShipMoveData *shipDatum = &(shipData[x]);
    if (shipDatum->initialized) {
      delete shipDatum->shipCircle;
      delete shipDatum->nextShipCircle;
    }
  }
  delete shipData;
}

void Stage::pushShips(Ship **oldShips, Ship **ships, ShipMoveData *shipData,
    int numShips, double dt) {
  for (int kk = 0; kk < numShips; kk++) {
    if (ships[kk]->alive) {
      ShipMoveData *shipDatum = &(shipData[kk]);
      Ship *ship = ships[kk];
      shipDatum->shipCircle->setPosition(
          shipDatum->shipCircle->h() + shipDatum->xSpeed*dt + 0.5*shipDatum->dxSpeed*dt*dt,
          shipDatum->shipCircle->k() + shipDatum->ySpeed*dt + 0.5*shipDatum->dySpeed*dt*dt);  
      shipDatum->xSpeed += shipDatum->dxSpeed*dt;
      shipDatum->ySpeed += shipDatum->dySpeed*dt;
      ship->x = shipDatum->shipCircle->h();
      ship->y = shipDatum->shipCircle->k();
      setSpeedAndHeading(oldShips[kk], ship, shipDatum);
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
    int numShips, bool **laserHits, int numLasers, int gameTime,
    bool firstTickLasers) {
  for (int x = 0; x < numShips; x++) {
    Ship *ship = ships[x];
    ShipMoveData *shipDatum = &(shipData[x]);
    if (ship->alive) {
      for (int y = 0; y < numLasers; y++) {
        Laser *laser = lasers_[y];
        // @ohaas: Changed this conditionals slightly to make function more usable
        //         from my point of view.
        if (((laser->fireTime == gameTime && laser->shipIndex != ship->index) || 
              (!firstTickLasers && laser->fireTime != gameTime))
            && shipDatum->shipCircle->intersects(laserLines_[y])
            && !laser->dead) {
          int firingShipIndex = laser->shipIndex;
          laserHits[firingShipIndex][x] = true;
          double laserDamage = (ship->energyEnabled ? LASER_DAMAGE : 0);
          double damageScore = (laserDamage / DEFAULT_ENERGY);
          if (ship->teamIndex == ships[firingShipIndex]->teamIndex) {
            ships[firingShipIndex]->friendlyDamage += damageScore;
          } else {
            ships[firingShipIndex]->damage += damageScore;
          }
          ship->energy -= laserDamage;
          for (int z = 0; z < numEventHandlers_; z++) {
            eventHandlers_[z]->handleLaserHitShip(ships[laser->shipIndex],
                ship, laser, shipDatum->startXSpeed, shipDatum->startYSpeed,
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

void Stage::nudgeShip(Ship *oldShip, Ship *ship, ShipMoveData *shipDatum, double angle) {

  // Add a little nudge so we're not inside the wall next time step.
  double epsNudge = 1.e-12*(1. + sqrt(square(width_)+square(height_)));
  shipDatum->shipCircle->setPosition(
      shipDatum->shipCircle->h() + cos(angle)*epsNudge,
      shipDatum->shipCircle->k() + sin(angle)*epsNudge); 
  ship->x = shipDatum->shipCircle->h();
  ship->y = shipDatum->shipCircle->k();
        
  return;
}

void Stage::doWallCollision(Ship *oldShip, Ship *ship, ShipMoveData *shipDatum, Line2D *wall,
                            int gameTime) {

  ship->hitWall = true;
  double heading = ship->heading;
  double angle1 = wall->theta() - M_PI_2;
  double angleDiff1 = abs(normalRelativeAngle(heading - angle1));
  double angle2 = wall->theta() + M_PI_2;
  double angleDiff2 = abs(normalRelativeAngle(heading - angle2));
  double angle, angleDiff = M_PI;
  if (angleDiff1 < angleDiff) {
    angleDiff = angleDiff1;
    angle = angle1;
  }
  if (angleDiff2 < angleDiff) {
    angleDiff = angleDiff2;
    angle = angle2;
  }
  double bounceForce = abs(ship->speed * cos(angleDiff) * (1 + WALL_BOUNCE));
  double bounceAngle = angle + M_PI;
  double energyInitial = 0.5*square(ship->speed); // For collision damage later
  shipDatum->xSpeed += cos(bounceAngle) * bounceForce;
  shipDatum->ySpeed += sin(bounceAngle) * bounceForce;
  for (int ii = 0; ii < numEventHandlers_; ii++) {
    eventHandlers_[ii]->handleShipHitWall(ship, bounceAngle, bounceForce, gameTime);
  }
  setSpeedAndHeading(oldShip, ship, shipDatum);

  // Add a little nudge so we're not inside the wall next time step.
  nudgeShip(oldShip, ship, shipDatum, bounceAngle);
  
  // Collision damage
  ship->energy += WALL_DMG_SCALE*(0.5*square(ship->speed) - energyInitial);;
  if (ship->energy <= 0) {
    ship->alive = false;
  }
  
  return;
}

void Stage::doWallEndpointCollision(Ship *oldShip, Ship *ship, ShipMoveData *shipDatum, Line2D *wall,
                                    int wallEndpoint, int gameTime) {

  ship->hitWall = true;
  double heading = ship->heading;
  double angle;
  if (wallEndpoint == 1) {
    angle = atan2(wall->y1() - shipDatum->shipCircle->k(),
                  wall->x1() - shipDatum->shipCircle->h());
  } else {
    angle = atan2(wall->y2() - shipDatum->shipCircle->k(),
                  wall->x2() - shipDatum->shipCircle->h());
  }
  double angleDiff = abs(normalRelativeAngle(heading - angle));
  double bounceForce = abs(ship->speed * cos(angleDiff) * (1 + WALL_BOUNCE));
  double bounceAngle = angle + M_PI;
  double energyInitial = 0.5*square(ship->speed); // For collision damage later
  shipDatum->xSpeed += cos(bounceAngle) * bounceForce;
  shipDatum->ySpeed += sin(bounceAngle) * bounceForce;
  for (int ii = 0; ii < numEventHandlers_; ii++) {
    eventHandlers_[ii]->handleShipHitWall(ship, bounceAngle, bounceForce, gameTime);
  }
  setSpeedAndHeading(oldShip, ship, shipDatum);

  // Add a little nudge so we're not inside the wall next time step.
  nudgeShip(oldShip, ship, shipDatum, bounceAngle);

  // Collision damage
  ship->energy += WALL_DMG_SCALE*(0.5*square(ship->speed) - energyInitial);
  if (ship->energy <= 0) {
    ship->alive = false;
  }
    
  return;
}

void Stage::doShipShipCollision(Ship **oldShips, Ship **ships, ShipMoveData *shipData, 
                                int indexShipShipFirstCollided, int indexShipShipFirstCollided2,
                                int gameTime) {
                      
  // Update x/y/heading/speeds for ships with ship-ship collisions.
  ShipMoveData *shipDatum = &(shipData[indexShipShipFirstCollided]);
  ShipMoveData *shipDatum2 = &(shipData[indexShipShipFirstCollided2]);
  Ship *ship = ships[indexShipShipFirstCollided];
  Ship *ship2 = ships[indexShipShipFirstCollided2];

  ship->hitShip = true;
  ship2->hitShip = true;

  double angle = atan2(ship2->y - ship->y, ship2->x - ship->x);
  double force = cos(ship->heading - angle) * ship->speed + 
                 cos(ship2->heading - angle - M_PI) * ship2->speed;
  force = force*0.5*(1 + SHIP_SHIP_BOUNCE);
  double xForce = cos(angle) * force;
  double yForce = sin(angle) * force;
  double energyInitial = 0.5*(square(ship->speed) + square(ship2->speed)); // For collision damage later
  shipDatum->xSpeed -= xForce;
  shipDatum->ySpeed -= yForce;
  setSpeedAndHeading(oldShips[indexShipShipFirstCollided], ship, shipDatum);
  shipDatum2->xSpeed += xForce;
  shipDatum2->ySpeed += yForce;
  setSpeedAndHeading(oldShips[indexShipShipFirstCollided2], ship2, shipDatum2);

  for (int z = 0; z < numEventHandlers_; z++) {
    // TODO not sure if both necessary, but I think that's how it was before as well
    eventHandlers_[z]->handleShipHitShip(ship, ship2,
        angle, force,
        normalRelativeAngle(angle-M_PI), force, gameTime);
    eventHandlers_[z]->handleShipHitShip(ship2, ship,
        normalRelativeAngle(angle-M_PI), force,
        angle, force, gameTime);
  }

  // Add a little nudge so we're not inside each other next time step.
  nudgeShip(oldShips[indexShipShipFirstCollided], ship, shipDatum, angle + M_PI);
  nudgeShip(oldShips[indexShipShipFirstCollided2], ship2, shipDatum2, angle); 

  // Collision damage
  double energyFinal = 0.5*(square(ship->speed) + square(ship2->speed)); // For collision damage later
  ship->energy += SHIPSHIP_DMG_SCALE*(energyFinal-energyInitial);
  ship2->energy += SHIPSHIP_DMG_SCALE*(energyFinal-energyInitial);
  if (ship->energy <= 0) {
    ship->alive = false;
  }
  if (ship2->energy <= 0) {
    ship2->alive = false;
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
      double distSq = square(torpedo->x - ship->x) + square(torpedo->y - ship->y);
      if (distSq < square(TORPEDO_BLAST_RADIUS)) {
        int firingShipIndex = torpedo->shipIndex;
        torpedoHits[firingShipIndex][ii] = true;
        ShipMoveData *shipDatum = &(shipData[ii]);
        double blastDistance = sqrt(distSq);
        double blastFactor = square(1.0 - (blastDistance / TORPEDO_BLAST_RADIUS));
        double blastForce = blastFactor * TORPEDO_BLAST_FORCE;
        double blastDamage = blastFactor * (ship->energyEnabled ? TORPEDO_BLAST_DAMAGE : 0);
        double blastAngle = atan2(ship->y - torpedo->y, ship->x - torpedo->x);
        double damageScore = (blastDamage / DEFAULT_ENERGY);
        if (ship->teamIndex == ships[firingShipIndex]->teamIndex) {
          ships[firingShipIndex]->friendlyDamage += damageScore;
        } else {
          ships[firingShipIndex]->damage += damageScore;
        }

        // TODO: Not sure if handler is called correctly, kinda depends on what it exactly does.
        for (int kk = 0; kk < numEventHandlers_; kk++) {
          eventHandlers_[kk]->handleTorpedoHitShip(ships[torpedo->shipIndex],
              ship, shipDatum->xSpeed, shipDatum->ySpeed,
              blastAngle, blastForce, blastDamage, gameTime);
        }

        ship->energy -= blastDamage;
        shipDatum->xSpeed += cos(blastAngle) * blastForce;
        shipDatum->ySpeed += sin(blastAngle) * blastForce;
        setSpeedAndHeading(oldShips[ii], ship, shipDatum);
        
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

void Stage::setSpeedAndHeading(
    Ship *oldShip, Ship *ship, ShipMoveData *shipDatum) {
  // Try to round off rounding errors so bot authors can check speed == 0
  ship->speed =
      round(sqrt(square(shipDatum->xSpeed) + square(shipDatum->ySpeed)), 5);
  ship->heading = ((abs(ship->speed) < 0.000001)
      ? oldShip->heading : atan2(shipDatum->ySpeed, shipDatum->xSpeed));
}

bool Stage::shipStopped(Ship *oldShip, Ship *ship) {
  return (abs(oldShip->x - ship->x) < 0.00001
      && abs(oldShip->y - ship->y) < 0.00001
      && (oldShip->hitWall || oldShip->hitShip));
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
