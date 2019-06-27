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

void Stage::solveQuadratic(
    double aa, double bb, double cc, double *t1, double *t2) {
  double qq = bb*bb - 4.*aa*cc;
  if(qq < 0.) {
    *t1 = std::numeric_limits<double>::quiet_NaN();
    *t2 = std::numeric_limits<double>::quiet_NaN();
    return;
  } else if(qq > 0. && abs(aa) > 0.) {
    qq = -0.5*(bb + signum(bb)*sqrt(qq));
    *t1 = qq/aa;
    *t2 = cc/qq;
    return;
  } else if(qq == 0. && abs(aa) > 0.) {
    *t1 = -0.5*bb/aa;
    *t2 = std::numeric_limits<double>::quiet_NaN();
    return;
  } else if(aa == 0. && abs(bb) > 0.) {
    *t1 = -cc/bb;
    *t2 = std::numeric_limits<double>::quiet_NaN();
    return;
  } else {
    *t1 = std::numeric_limits<double>::quiet_NaN();
    *t2 = std::numeric_limits<double>::quiet_NaN();
    return;
  }
}

double Stage::getPosMin(
    double aa, double bb) {
  if (aa > 0. and bb > 0.) {
    return fmin(aa,bb);
  } else if(aa > 0.) {
    return aa;
  } else if(bb > 0.){
    return bb;
  } else {
    return std::numeric_limits<double>::quiet_NaN();
  }
}

double Stage::getPosMin(
    double aa, double bb, double cc, double dd) {
  return getPosMin(getPosMin(aa,bb),getPosMin(cc,dd));
}

double Stage::timeToFirstWallCollision(
    ShipMoveData shipDatum, Ship *ship, Line2D* wall) {
  double aa, bb, cc, t1, t2, t3, t4;
  double dx = wall->x2() - wall->x1();
  double dy = wall->y2() - wall->y1();
  double dxy = wall->x2()*wall->y1() - wall->y2()*wall->x1();
  
  aa = 0.5*(dy*shipDatum.dxSpeed - dx*shipDatum.dySpeed);
  bb = dy*shipDatum.xSpeed - dx*shipDatum.ySpeed;
  cc = dy*shipDatum.shipCircle->h() - dx*shipDatum.shipCircle->k() + 
       dxy - SHIP_RADIUS*sqrt(dx*dx+dy*dy);
  
  solveQuadratic(aa, bb, cc, &t1, &t2);

//  std::cout << "t1 " << t1 << '\n';
//  std::cout << "t2 " << t2 << '\n';
//  std::cout << "x " << shipDatum.shipCircle->h() << '\n';
//  std::cout << "y " << shipDatum.shipCircle->k() << '\n';
//  std::cout << "vx " << shipDatum.xSpeed << '\n';
//  std::cout << "vy " << shipDatum.ySpeed << '\n';
//  std::cout << "ax " << shipDatum.dxSpeed << '\n';
//  std::cout << "ay " << shipDatum.dySpeed << '\n';
//  std::cout << "x1 " << wall->x1() << '\n';
//  std::cout << "x2 " << wall->x2() << '\n';
//  std::cout << "y1 " << wall->y1() << '\n';
//  std::cout << "y2 " << wall->y2() << '\n';
//  std::cout << "aa " << aa << '\n';
//  std::cout << "bb " << bb << '\n';
//  std::cout << "cc " << cc << '\n';
//  std::cout << "dx " << dx << '\n';
//  std::cout << "dy " << dy << '\n';
//  std::cout << "dxy " << dxy << '\n';
  
  cc = cc + SHIP_SIZE*sqrt(dx*dx+dy*dy);
  solveQuadratic(aa, bb, cc, &t3, &t4);
  
//  std::cout << "cc " << cc << '\n';
//  std::cout << "t3 " << t3 << '\n';
//  std::cout << "t4 " << t4 << '\n';  
  
  return getPosMin(t1, t2, t3, t4); // TODO better failsafe?
}

double Stage::timeToFirstShipCollision(
    ShipMoveData shipDatum, Ship *ship, ShipMoveData shipDatum2, Ship *ship2) {
  double aa, bb, cc, dd, ee;
  double t1, t2, t3, t4;
  double dx = shipDatum2.shipCircle->h() - shipDatum.shipCircle->h();
  double dy = shipDatum2.shipCircle->k() - shipDatum.shipCircle->k();
  double dxp = shipDatum2.xSpeed - shipDatum.xSpeed;
  double dyp = shipDatum2.ySpeed - shipDatum.ySpeed;
  double dxpp = shipDatum2.dxSpeed - shipDatum.dxSpeed;
  double dypp = shipDatum2.dySpeed - shipDatum.dySpeed;
  
  aa = 0.25*(dxpp*dxpp + dypp*dypp);
  bb = dxp*dxpp + dyp*dypp;
  cc = dxp*dxp + dx*dxpp + dyp*dyp + dy*dypp;
  dd = 2.*(dx*dxp + dy*dyp);
  ee = dx*dx + dy*dy - SHIP_SIZE*SHIP_SIZE;

//  std::cout << "x1y1 " << shipDatum.shipCircle->h() << ' ' << shipDatum.shipCircle->k() << '\n';
//  std::cout << "x2y2 " << shipDatum2.shipCircle->h() << ' ' << shipDatum2.shipCircle->k() << '\n';
//  std::cout << "x1y1next " << shipDatum.nextShipCircle->h() << ' ' << shipDatum.nextShipCircle->k() << '\n';
//  std::cout << "x2y2next " << shipDatum2.nextShipCircle->h() << ' ' << shipDatum2.nextShipCircle->k() << '\n';
//  std::cout << "aa " << aa << '\n';
//  std::cout << "bb " << bb << '\n';
//  std::cout << "cc " << cc << '\n';
//  std::cout << "dd " << dd << '\n';
//  std::cout << "ee " << ee << '\n';
  solveQuartic(aa, bb, cc, dd, ee, &t1, &t2, &t3, &t4);
//  std::cout << "t1 " << t1 << '\n';
//  std::cout << "t2 " << t2 << '\n';
//  std::cout << "t3 " << t3 << '\n';
//  std::cout << "t4 " << t4 << '\n';
  
  return getPosMin(t1, t2, t3, t4);
}

void Stage::solveCubic(
    double aa, double bb, double cc, double dd,
    double *t1, double *t2, double *t3) {
  std::complex<double> ppold, qqold, rrold;
  std::complex<double> pp, qq, rr;
  std::complex<double> i(0.,1.);
  double eps1 = 1.e-12;
  double eps2 = 1.e-9;
  if (aa == 0.) {
    *t3 = std::numeric_limits<double>::quiet_NaN();
    solveQuadratic(bb, cc, dd, t1, t2);
  } else {
    bb = bb/aa;
    cc = cc/aa;
    dd = dd/aa;
    pp = 1.;
    qq = 0.5 + 0.1*i;
    rr = (*t2)*(*t2);
    for (int ii = 0; ii < 1000; ii++) {
      ppold = pp;
      qqold = qq;
      rrold = rr;
      pp = solveCubicHelper(bb, cc, dd, pp, qq, rr);
      qq = solveCubicHelper(bb, cc, dd, qq, rr, pp);
      rr = solveCubicHelper(bb, cc, dd, rr, pp, qq);
      if (abs(pp-ppold) < eps1*abs(pp) + eps1 and abs(qq-qqold) < eps1*abs(qq) + eps1 and
          abs(rr-rrold) < eps1*abs(rr) + eps1) {
        break;
      }
    }
    *t1 = approxReal(pp, eps2);
    *t2 = approxReal(qq, eps2);
    *t3 = approxReal(rr, eps2);
  }
  return;
}

std::complex<double> Stage::solveCubicHelper(
    double bb, double cc, double dd,
    std::complex<double> pp, std::complex<double> qq, std::complex<double> rr) {
  return pp - (dd+pp*(cc+pp*(bb+pp)))/((pp-qq)*(pp-rr));
}

void Stage::solveQuartic(
    double aa, double bb, double cc, double dd, double ee, 
    double *t1, double *t2, double *t3, double *t4) {
  std::complex<double> ppold, qqold, rrold, ssold;
  std::complex<double> pp, qq, rr, ss;
  std::complex<double> i(0.,1.);
  double eps1 = 1.e-12;
  double eps2 = 1.e-9;
  if(aa == 0.) {
    *t4 = std::numeric_limits<double>::quiet_NaN();
    solveCubic(bb, cc, dd, ee, t1, t2, t3);
  } else {
    bb = bb/aa;
    cc = cc/aa;
    dd = dd/aa;
    ee = ee/aa;
    pp = 1.;
    qq = 0.5 + 0.1*i;
    rr = qq*qq;
    ss = qq*rr;
//    std::cout << "pp " << pp << '\n';
//    std::cout << "qq " << qq << '\n';
//    std::cout << "rr " << rr << '\n';
//    std::cout << "ss " << ss << '\n';
    for (int ii = 0; ii < 1000; ii++) {
      ppold = pp;
      qqold = qq;
      rrold = rr;
      ssold = ss;
      pp = solveQuarticHelper(bb, cc, dd, ee, pp, qq, rr, ss);
      qq = solveQuarticHelper(bb, cc, dd, ee, qq, rr, ss, pp);
      rr = solveQuarticHelper(bb, cc, dd, ee, rr, ss, pp, qq);
      ss = solveQuarticHelper(bb, cc, dd, ee, ss, pp, qq, rr);
      if (abs(pp-ppold) < eps1*abs(pp) + eps1 and abs(qq-qqold) < eps1*abs(qq) + eps1 and
          abs(rr-rrold) < eps1*abs(rr) + eps1 and abs(ss-ssold) < eps1*abs(ss) + eps1) {
//        std::cout << "iiend " << ii << '\n';
        break;
      }
    }
    *t1 = approxReal(pp, eps2);
    *t2 = approxReal(qq, eps2);
    *t3 = approxReal(rr, eps2);
    *t4 = approxReal(ss, eps2);
//    std::cout << "pp " << pp << '\n';
//    std::cout << "qq " << qq << '\n';
//    std::cout << "rr " << rr << '\n';
//    std::cout << "ss " << ss << '\n';
  }
  return;
}

std::complex<double> Stage::solveQuarticHelper(
    double bb, double cc, double dd, double ee,
    std::complex<double> pp, std::complex<double> qq, std::complex<double> rr, std::complex<double> ss) {
  return pp - (ee+pp*(dd+pp*(cc+pp*(bb+pp))))/((pp-qq)*(pp-rr)*(pp-ss));
}

double Stage::approxReal(
    std::complex<double> zz, double eps) {
  if (fabs(std::imag(zz)) < eps*abs(zz)) {
    return std::real(zz);
  } else {
    return std::numeric_limits<double>::quiet_NaN();
  }
}

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
//      shipDatum.shipCollisionData = new ShipCollisionData*[numShips];
//      for (int y = 0; y < numShips; y++) {
//        shipDatum.shipCollisionData[y] = 0;
//      }
      shipDatum.stopped = false;
  
      double moveDistance =
          sqrt(square(ship->x - oldShip->x) + square(ship->y - oldShip->y));
      int moveIntervals = ceil(moveDistance / COLLISION_FRAME);
      intervals = std::max(intervals, moveIntervals);
    }
    shipData[x] = shipDatum;
  }

  double dtSub = 1./intervals;  // @ohaas: Time step of sub steps
  
  // Set initual speed
  for (int x = 0; x < numShips; x++) {
    Ship *ship = ships[x];
    ShipMoveData shipDatum = shipData[x];
    if (ship->alive) {
      shipDatum.xSpeed = shipDatum.startXSpeed;
      shipDatum.ySpeed = shipDatum.startYSpeed;
    }
  }


  // Logs for laser hits and alive ships created outside of loop
  // damage, remove dead lasers.
  bool **laserHits = new bool*[numShips];
  for (int x = 0; x < numShips; x++) {
    laserHits[x] = new bool[numShips];
    for (int y = 0; y < numShips; y++) {
      laserHits[x][y] = false;
    }
  }
  bool *wasAlive = new bool[numShips];
  for (int x = 0; x < numShips; x++) {
    wasAlive[x] = ships[x]->alive;
  }
  
  for (int x = 0; x < intervals; x++) {
  
    // Move ships one interval and check for collisions. 
    // Always time push at most until next collision or end of sub-tick
    // time step, then repeat.
    double timeToDo = dtSub;
    while (timeToDo > 1.e-6*dtSub) {
    
      double timeToFirstCollision = std::numeric_limits<double>::infinity();
      int typeFirstCollision = -1;
      int indFirstCollidingShip, indFirstCollidingShip2;
      
      for (int y = 0; y < numShips; y++) {
        if (ships[y]->alive) {
          ShipMoveData *shipDatum = &(shipData[y]);
          double thisX = shipDatum->shipCircle->h();
          double thisY = shipDatum->shipCircle->k();
          shipDatum->nextShipCircle->setPosition(
              thisX + shipDatum->xSpeed*timeToDo + 0.5*shipDatum->dxSpeed*timeToDo*timeToDo,
              thisY + shipDatum->ySpeed*timeToDo + 0.5*shipDatum->dySpeed*timeToDo*timeToDo);
          for (int z = 0; z < numWallLines_; z++) {
            Line2D* line = wallLines_[z];
            Point2D *p1 = 0;
            Point2D *p2 = 0;
            if (shipDatum->nextShipCircle->intersects(line, &p1, &p2)) {
//              std::cout << "gameTime " << gameTime << '\n';
//              std::cout << "indWall " << z << '\n';
//              std::cout << "intersects " << shipDatum->nextShipCircle->intersects(line, &p1, &p2) << '\n';
//              std::cout << "p " << p1->getX() << ' ' << p1->getY() << ' ' << p2->getX() << ' ' << p2->getY()<< '\n';
//              std::cout << '\n';
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
              if (valid) {
                double ttc = timeToFirstWallCollision(*shipDatum, ships[y], line);
                if (ttc < timeToFirstCollision) {
                  timeToFirstCollision = ttc;
                  typeFirstCollision = 0;
                  indFirstCollidingShip = y;  // @ohaas: Maybe better use ship[y]->index
                  shipDatum->wallImpactLine = line;
                }
              }
            }
          }
        }
      }
      
      // Check for ship-ship collisions.
      for (int y = 0; y < numShips; y++) {
        ShipMoveData *shipDatum = &(shipData[y]);
        if (ships[y]->alive) {      
          for (int z = 0; z < y; z++) {
            if (ships[z]->alive) {
              ShipMoveData *shipDatum2 = &(shipData[z]);
              if (shipDatum->nextShipCircle->overlaps(shipDatum2->nextShipCircle)) {
                double ttc = timeToFirstShipCollision(*shipDatum, ships[y], *shipDatum2, ships[z]);
//                std::cout << "ttc " << ttc << '\n';
//                std::cout << '\n';
                if (ttc < timeToFirstCollision) {
                  timeToFirstCollision = ttc;
                  typeFirstCollision = 1;
                  indFirstCollidingShip = y;
                  indFirstCollidingShip2 = z;
                }
              }
            }
          }
        }
      }
      
      // To the time push until first collision any ship does
      double timeDo = fmin(timeToFirstCollision, timeToDo);
      timeToDo -= timeDo;
      for (int kk = 0; kk < numShips; kk++) {
        if (ships[kk]->alive) {
          ShipMoveData *shipDatum = &(shipData[kk]);
          Ship *ship = ships[kk];
          shipDatum->shipCircle->setPosition(
              shipDatum->shipCircle->h() + shipDatum->xSpeed*timeDo + 0.5*shipDatum->dxSpeed*timeDo*timeDo,
              shipDatum->shipCircle->k() + shipDatum->ySpeed*timeDo + 0.5*shipDatum->dySpeed*timeDo*timeDo);  
          shipDatum->xSpeed += shipDatum->dxSpeed*timeDo;
          shipDatum->ySpeed += shipDatum->dySpeed*timeDo;
          ship->x = shipDatum->shipCircle->h();
          ship->y = shipDatum->shipCircle->k();
          setSpeedAndHeading(oldShips[kk], ship, shipDatum);
        }
      }
      
      if (typeFirstCollision == 0) {  // Wall collision
        ShipMoveData *shipDatum = &(shipData[indFirstCollidingShip]);
        Ship *ship = ships[indFirstCollidingShip];
        Line2D *line = shipDatum->wallImpactLine;
        ship->hitWall = true;
        double heading = ship->heading;
        double angle1 = line->theta() - M_PI_2;
        double angleDiff1 = abs(normalRelativeAngle(heading - angle1));
        double angle2 = line->theta() + M_PI_2;
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
        shipDatum->xSpeed += cos(bounceAngle) * bounceForce;
        shipDatum->ySpeed += sin(bounceAngle) * bounceForce;
        for (int z = 0; z < numEventHandlers_; z++) {
          eventHandlers_[z]->handleShipHitWall(ship, bounceAngle, bounceForce, gameTime); // TODO update ship?
        }
        setSpeedAndHeading(oldShips[indFirstCollidingShip], ship, shipDatum);
        
      } else if (typeFirstCollision == 1) {  // Ship-Ship collision
      
        // Update x/y/heading/speeds for ships with ship-ship collisions.
        ShipMoveData *shipDatum = &(shipData[indFirstCollidingShip]);
        ShipMoveData *shipDatum2 = &(shipData[indFirstCollidingShip2]);
        Ship *ship = ships[indFirstCollidingShip];
        Ship *ship2 = ships[indFirstCollidingShip2];

        ship->hitShip = true;
        ship2->hitShip = true;

        double angle = atan2(ship2->y - ship->y, ship2->x - ship->x);
        double force = cos(ship->heading - angle) * ship->speed + 
                       cos(ship2->heading - angle - M_PI) * ship2->speed;
        double xForce = cos(angle) * force;
        double yForce = sin(angle) * force;
        shipDatum->xSpeed -= xForce;
        shipDatum->ySpeed -= yForce;
        setSpeedAndHeading(oldShips[indFirstCollidingShip], ship, shipDatum);
        shipDatum2->xSpeed += xForce;
        shipDatum2->ySpeed += yForce;
        setSpeedAndHeading(oldShips[indFirstCollidingShip2], ship2, shipDatum2);

        for (int z = 0; z < numEventHandlers_; z++) {
          // TODO not sure if both necessary, but I think that's how it was before as well
          eventHandlers_[z]->handleShipHitShip(ship, ship2,
              angle, force,
              normalRelativeAngle(angle-M_PI), force, gameTime);
          eventHandlers_[z]->handleShipHitShip(ship2, ship,
              normalRelativeAngle(angle-M_PI), force,
              angle, force, gameTime);
        }
      }
//      if (typeFirstCollision >= 0) {
//        std::cout << "gameTime " << gameTime << '\n';
//        std::cout << "gameTime Sub " << x*dtSub << ' ' << (x+1)*dtSub << '\n';
//        std::cout << "timeToDo " << timeToDo << '\n';
//        std::cout << "timeDo " << timeDo << '\n';
//        std::cout << "typeFirstCollision " << typeFirstCollision << '\n';
//        std::cout << "timeToFirstCollision " << timeToFirstCollision << '\n';
//        std::cout << "shipData[0] " << shipData[0].shipCircle->h() << " " << shipData[0].shipCircle->k() << '\n';
//        std::cout << "shipData[1] " << shipData[1].shipCircle->h() << " " << shipData[1].shipCircle->k() << '\n';
//        std::cout << '\n';
//      }
    }
  }




  // For lasers fired this tick, check if they intersect any other ships at
  // their initial position (0-25 from origin) before moving the first time.
  checkLaserShipCollisions(ships, shipData, numShips, laserHits, numLasers_,
                           gameTime, true);

  // Move lasers one whole tick.
  for (int x = 0; x < numLasers_; x++) {
    Laser *laser = lasers_[x];
    laser->x += laser->dx;
    laser->y += laser->dy;
    laserLines_[x]->shift(laser->dx, laser->dy);
  }
  
  checkLaserShipCollisions(ships, shipData, numShips, laserHits, numLasers_,
                           gameTime, false);

  // Scoring for destroyed by laser for ships
  // @ohaas: Can be done outside of sub tick move
  for (int x = 0; x < numShips; x++) {
    Ship *ship = ships[x];
    if (wasAlive[x] && !ship->alive) {
      int numDestroyers = 0;
      for (int y = 0; y < numShips; y++) {
        if (laserHits[y][x]) {
          numDestroyers++;
        }
      }
      Ship **destroyers = new Ship*[numDestroyers];
      int destroyerIndex = 0;
      for (int y = 0; y < numShips; y++) {
        if (laserHits[y][x]) {
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


  // Move torpedoes and check for collisions.
  for (int x = 0; x < numShips; x++) {
    Ship *ship = ships[x];
    wasAlive[x] = ship->alive;
  }
  bool **torpedoHits = new bool*[numShips];
  for (int x = 0; x < numShips; x++) {
    torpedoHits[x] = new bool[numShips];
    for (int y = 0; y < numShips; y++) {
      torpedoHits[x][y] = false;
    }
  }
  for (int x = 0; x < numTorpedos_; x++) {
    Torpedo *torpedo = torpedos_[x];
    double distanceRemaining = torpedo->distance - torpedo->distanceTraveled;
    if (distanceRemaining >= TORPEDO_SPEED) {
      torpedo->x += torpedo->dx;
      torpedo->y += torpedo->dy;
      torpedo->distanceTraveled += TORPEDO_SPEED;
    } else {
      torpedo->x += cos(torpedo->heading) * distanceRemaining;
      torpedo->y += sin(torpedo->heading) * distanceRemaining;
      for (int y = 0; y < numShips; y++) {
        Ship *ship = ships[y];
        if (ship->alive) {
          double distSq =
              square(torpedo->x - ship->x) + square(torpedo->y - ship->y);
          if (distSq < square(TORPEDO_BLAST_RADIUS)) {
            int firingShipIndex = torpedo->shipIndex;
            torpedoHits[firingShipIndex][y] = true;
            ShipMoveData *shipDatum = &(shipData[y]);
            double blastDistance = sqrt(distSq);
            double blastFactor =
                square(1.0 - (blastDistance / TORPEDO_BLAST_RADIUS));
            double blastForce = blastFactor * TORPEDO_BLAST_FORCE;
            double blastDamage = blastFactor
                * (ship->energyEnabled ? TORPEDO_BLAST_DAMAGE : 0);
            double blastAngle =
                atan2(ship->y - torpedo->y, ship->x - torpedo->x);
            double damageScore = (blastDamage / DEFAULT_ENERGY);
            if (ship->teamIndex == ships[firingShipIndex]->teamIndex) {
              ships[firingShipIndex]->friendlyDamage += damageScore;
            } else {
              ships[firingShipIndex]->damage += damageScore;
            }
            ship->energy -= blastDamage;
            shipDatum->xSpeed += cos(blastAngle) * blastForce;
            shipDatum->ySpeed += sin(blastAngle) * blastForce;
            setSpeedAndHeading(oldShips[y], ship, shipDatum);

            for (int z = 0; z < numEventHandlers_; z++) {
              eventHandlers_[z]->handleTorpedoHitShip(ships[torpedo->shipIndex],
                  ship, shipDatum->startXSpeed, shipDatum->startYSpeed,
                  blastAngle, blastForce, blastDamage, gameTime);
            }
  
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
        torpedos_[x] = torpedos_[numTorpedos_ - 1];
      }
      delete torpedo;
      numTorpedos_--;
      x--;
    }
  }
  for (int x = 0; x < numShips; x++) {
    Ship *ship = ships[x];
    if (wasAlive[x] && !ship->alive) {
      int numDestroyers = 0;
      for (int y = 0; y < numShips; y++) {
        if (torpedoHits[y][x]) {
          numDestroyers++;
        }
      }
      Ship **destroyers = new Ship*[numDestroyers];
      int destroyerIndex = 0;
      for (int y = 0; y < numShips; y++) {
        if (torpedoHits[y][x]) {
          destroyers[destroyerIndex++] = ships[y];
        }
      }

      for (int y = 0; y < numShips; y++) {
        if (torpedoHits[y][x]) {
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

  for (int x = 0; x < numShips; x++) {
    delete torpedoHits[x];
  }
  delete torpedoHits;
  delete wasAlive;

  for (int x = 0; x < numShips; x++) {
    ShipMoveData *shipDatum = &(shipData[x]);
    if (shipDatum->initialized) {
      delete shipDatum->shipCircle;
      delete shipDatum->nextShipCircle;
//      for (int y = 0; y < numShips; y++) {
//        ShipCollisionData *collisionData = shipDatum->shipCollisionData[y];
//        if (collisionData != 0) {
//          delete collisionData;
//        }
//      }
      delete shipDatum->shipCollisionData;
    }
  }
  delete shipData;
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
        if (((firstTickLasers && laser->fireTime == gameTime
                 && laser->shipIndex != ship->index)
            || !firstTickLasers)
            && shipDatum->shipCircle->intersects(laserLines_[y])) {
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

int Stage::fireLaser(Ship *ship, double heading, int gameTime) {
  if (ship->laserGunHeat > 0 || numLasers_ >= MAX_LASERS) {
    return 0;
  } else {
    double cosHeading = cos(heading);
    double sinHeading = sin(heading);
    double laserX = ship->x + (cosHeading * LASER_SPEED);
    double laserY = ship->y + (sinHeading * LASER_SPEED);
    Line2D laserStartLine(ship->x, ship->y, laserX, laserY);
    if (hasVision(&laserStartLine)) {
      Laser *laser = new Laser;
      laser->id = nextLaserId_++;
      laser->shipIndex = ship->index;
      laser->fireTime = gameTime;
      double dx = cosHeading * LASER_SPEED;
      double dy = sinHeading * LASER_SPEED;
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
    torpedo->x = ship->x + (cosHeading * SHIP_RADIUS);
    torpedo->y = ship->y + (sinHeading * SHIP_RADIUS);
    torpedo->heading = heading;
    torpedo->distance = distance;
    torpedo->dx = dx;
    torpedo->dy = dy;
    torpedo->distanceTraveled = SHIP_RADIUS;
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
