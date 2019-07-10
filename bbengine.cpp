/*
  Copyright (C) 2012-2015 - Voidious

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

#include <sstream>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <algorithm>
#include <pthread.h>
#include <limits.h>
#include <platformstl/performance/performance_counter.hpp>
#include <platformstl/synch/sleep_functions.h>
#include "bbconst.h"
#include "bblua.h"
#include "printhandler.h"
#include "zipper.h"
#include "filemanager.h"
#include "replaybuilder.h"
#include "bbengine.h"

BerryBotsEngine::BerryBotsEngine(PrintHandler *printHandler,
    FileManager *fileManager, const char *replayTemplateDir) {
  stage_ = new Stage(DEFAULT_STAGE_WIDTH, DEFAULT_STAGE_HEIGHT);
  consoleHandler_ = new ConsoleEventHandler(this);
  stage_->addEventHandler(consoleHandler_);
  printHandler_ = printHandler;
  fileManager_ = fileManager;

  gameTime_ = 0;
  numTeams_ = numInitializedTeams_ = 0;
  teamSize_ = 1;
  numShips_ = numInitializedShips_ = 0;
  stageRun_ = false;
  stageConfigureComplete_ = false;
  shipInitComplete_ = false;
  battleMode_ = false;
  relativistic_ = true;
  wallCollDamage_ = false;      // These get set to true if battleMode is enabled and
  shipShipCollDamage_ = false;  // have to be turned off "again" if desired.
  roundOver_ = false;
  gameOver_ = false;
  physicsOver_ = false;
  winnerName_[0] = '\0';
  winnerFilename_[0] = '\0';
  hasRanks_ = hasScores_ = false;

  stageState_ = 0;
  stagesDir_ = 0;
  stageFilename_ = 0;
  stageWorld_ = 0;
  teams_ = 0;
  ships_ = 0;
  stageShips_ = 0;
  oldShips_ = 0;
  prevShips_ = 0;
  shipProperties_ = 0;
  worlds_ = 0;
  shipGfxs_ = 0;
  stageGfx_ = 0;
  teamVision_ = 0;
  sensorHandler_ = 0;

  timerSettings_ = new TickTimerSettings;
  timerSettings_->L = 0;
  timerSettings_->timerTick = 0;
  timerSettings_->timerExpiration = LONG_MAX;
  timerSettings_->enabled = true;
  pthread_create(&timerThread_, 0, BerryBotsEngine::timer,
                 (void*) timerSettings_);
  pthread_detach(timerThread_);
  // TODO: how to handle failure to create thread

  replayHandler_ = 0;
  if (replayTemplateDir == 0) {
    replayTemplateDir_ = 0;
  } else {
    replayTemplateDir_ = new char[strlen(replayTemplateDir) + 1];
    strcpy(replayTemplateDir_, replayTemplateDir);
  }
  replayBuilder_ = new ReplayBuilder(replayTemplateDir_);
  deleteReplayBuilder_ = true;
}

BerryBotsEngine::~BerryBotsEngine() {
  timerSettings_->enabled = false;
  if (stagesDir_ != 0) {
    delete stagesDir_;
  }
  if (stageFilename_ != 0) {
    delete stageFilename_;
  }

  for (int x = 0; x < numInitializedShips_; x++) {
    Ship *ship = ships_[x];
    if (ship->properties->ownedByLua) {
      delete ship->properties;
    } else {
      delete ship->properties;
      delete ship;
    }
  }
  delete ships_;
  delete shipProperties_;
  if (oldShips_ != 0) {
    for (int x = 0; x < numShips_; x++) {
      delete oldShips_[x];
    }
    delete oldShips_;
  }
  if (prevShips_ != 0) {
    for (int x = 0; x < numShips_; x++) {
      delete prevShips_[x];
    }
    delete prevShips_;
  }

  for (int x = 0; x < numInitializedTeams_; x++) {
    Team *team = teams_[x];
    TeamResult *teamResult = &(team->result);
    for (int y = 0; y < teamResult->numStats; y++) {
      ScoreStat *stat = teamResult->stats[y];
      delete stat->key;
      delete stat;
    }
    if (team->ownedByLua) {
      lua_close(team->state);
    }
    for (int y = 0; y < team->numRectangles; y++) {
      delete team->gfxRectangles[y];
    }
    for (int y = 0; y < team->numLines; y++) {
      delete team->gfxLines[y];
    }
    for (int y = 0; y < team->numCircles; y++) {
      delete team->gfxCircles[y];
    }
    for (int y = 0; y < team->numTexts; y++) {
      delete team->gfxTexts[y];
    }
    delete team;
  }
  delete teams_;
  if (stageState_ != 0) {
    lua_close(stageState_);
  }

  if (worlds_ != 0) {
    delete worlds_;
  }
  if (shipGfxs_ != 0) {
    delete shipGfxs_;
  }
  if (teamVision_ != 0) {
    for (int x = 0; x < numTeams_; x++) {
      delete teamVision_[x];
    }
    delete teamVision_;
  }
  delete stage_;
  if (sensorHandler_ != 0) {
    delete sensorHandler_;
  }
  if (deleteReplayBuilder_) {
    delete replayBuilder_;
  }
  if (replayHandler_ != 0) {
    delete replayHandler_;
  }
  if (replayTemplateDir_ != 0) {
    delete replayTemplateDir_;
  }
  delete consoleHandler_;
}

Stage* BerryBotsEngine::getStage() {
  return stage_;
}

Ship** BerryBotsEngine::getShips() {
  return ships_;
}

int BerryBotsEngine::getNumShips() {
  return numShips_;
}

Ship* BerryBotsEngine::getStageProgramShip(const char *name) {
  for (int x = 0; x < numShips_; x++) {
    Ship *ship = stageShips_[x];
    if (strcmp(name, ship->properties->name) == 0) {
      return ship;
    }
  }
  return 0;
}

int BerryBotsEngine::getGameTime() {
  return gameTime_;
}

void BerryBotsEngine::setTeamSize(int teamSize) {
  teamSize_ = teamSize;
}

int BerryBotsEngine::getTeamSize() {
  return teamSize_;
}

bool BerryBotsEngine::isStageConfigureComplete() {
  return stageConfigureComplete_;
}

bool BerryBotsEngine::isShipInitComplete() {
  return shipInitComplete_;
}

void BerryBotsEngine::setBattleMode(bool battleMode) {
  battleMode_ = battleMode;
  if (battleMode_) {
    setWallCollDamage(true);
    setShipShipCollDamage(true);
  }
}

bool BerryBotsEngine::isBattleMode() {
  return battleMode_;
}

void BerryBotsEngine::setRelativistic(bool relativistic) {
  relativistic_ = relativistic;
}

bool BerryBotsEngine::isRelativistic() {
  return relativistic_;
}

void BerryBotsEngine::setWallCollDamage(bool wallCollDamage) {
  wallCollDamage_ = wallCollDamage;
}

bool BerryBotsEngine::isWallCollDamage() {
  return wallCollDamage_;
}

void BerryBotsEngine::setShipShipCollDamage(bool shipShipCollDamage) {
  shipShipCollDamage_ = shipShipCollDamage;
}

bool BerryBotsEngine::isShipShipCollDamage() {
  return shipShipCollDamage_;
}

void BerryBotsEngine::setRoundOver(bool roundOver) {
  roundOver_ = roundOver;
}

bool BerryBotsEngine::isRoundOver() {
  return roundOver_;
}

void BerryBotsEngine::setGameOver(bool gameOver) {
  gameOver_ = gameOver;
}

bool BerryBotsEngine::isGameOver() {
  return gameOver_;
}

void BerryBotsEngine::setWinnerName(const char* winnerName) {
  for (int x = 0; x < numTeams_; x++) {
    Team *team = teams_[x];
    if (strcmp(team->name, winnerName) == 0) {
      strcpy(winnerName_, winnerName);
      winnerName_[strlen(winnerName)] = '\0';
      strcpy(winnerFilename_, team->filename);
      winnerFilename_[strlen(winnerFilename_)] = '\0';
    }
  }
}

void BerryBotsEngine::setRank(const char *teamName, int rank) {
  for (int x = 0; x < numTeams_; x++) {
    Team *team = teams_[x];
    if (strcmp(team->name, teamName) == 0) {
      team->result.rank = rank;
      hasRanks_ = true;
      break;
    }
  }
}

void BerryBotsEngine::setScore(const char *teamName, double score) {
  for (int x = 0; x < numTeams_; x++) {
    Team *team = teams_[x];
    if (strcmp(team->name, teamName) == 0) {
      team->result.score = score;
      hasScores_ = true;
      break;
    }
  }
}

void BerryBotsEngine::setStatistic(const char *teamName, const char *key,
                                   double value) {
  for (int x = 0; x < numTeams_; x++) {
    Team *team = teams_[x];
    if (strcmp(team->name, teamName) == 0) {
      TeamResult *teamResult = &(team->result);
      bool found = false;
      for (int y = 0; y < teamResult->numStats; y++) {
        if (strcmp(teamResult->stats[y]->key, key) == 0) {
          teamResult->stats[y]->value = value;
          found = true;
        }
      }
      if (!found && teamResult->numStats < MAX_SCORE_STATS) {
        ScoreStat *stat = new ScoreStat;
        stat->key = new char[strlen(key) + 1];
        strcpy(stat->key, key);
        stat->value = value;
        teamResult->stats[teamResult->numStats++] = stat;
      }
      break;
    }
  }
}

const char* BerryBotsEngine::getWinnerName() {
  if (strlen(winnerName_) == 0) {
    return 0;
  } else {
    return winnerName_;
  }
}

const char* BerryBotsEngine::getWinnerFilename() {
  if (strlen(winnerFilename_) == 0) {
    return 0;
  } else {
    return winnerFilename_;
  }
}

TeamResult** BerryBotsEngine::getTeamResults() {
  processWinnerRanksScores();

  TeamResult** results = new TeamResult*[numTeams_];
  for (int x = 0; x < numTeams_; x++) {
    TeamResult *newResult = new TeamResult;
    TeamResult *result = &(teams_[x]->result);
    *newResult = *result;
    for (int y = 0; y < result->numStats; y++) {
      ScoreStat *newStat = new ScoreStat;
      char *newKey = new char[strlen(result->stats[y]->key) + 1];
      strcpy(newKey, result->stats[y]->key);
      newStat->key = newKey;
      newStat->value = result->stats[y]->value;
      newResult->stats[y] = newStat;
    }
    results[x] = newResult;
  }
  
  return results;
}

bool BerryBotsEngine::hasScores() {
  return hasScores_;
}

void BerryBotsEngine::processWinnerRanksScores() {
  bool hasEffectiveRanks = hasRanks_;
  if (hasScores_ && !hasEffectiveRanks) {
    setTeamRanksByScore();
    hasEffectiveRanks = true;
  } else if (winnerName_ != 0 && !hasEffectiveRanks) {
    setRank(winnerName_, 1);
  }
  if (strlen(winnerName_) == 0 && (hasScores_ || hasEffectiveRanks)) {
    for (int x = 0; x < numTeams_; x++) {
      Team *team = teams_[x];
      if (team->result.rank == 1) {
        setWinnerName(team->name);
        break;
      }
    }
  }
}

void BerryBotsEngine::setTeamRanksByScore() {
  Team** sortedTeams = new Team*[numTeams_];
  for (int x = 0; x < numTeams_; x++) {
    sortedTeams[x] = teams_[x];
  }
  for (int x = 0; x < numTeams_ - 1; x++) {
    for (int y = x + 1; y < numTeams_; y++) {
      Team *team1 = sortedTeams[x];
      Team *team2 = sortedTeams[y];
      if (team1->result.score < team2->result.score) {
        sortedTeams[x] = team2;
        sortedTeams[y] = team1;
      }
    }
  }
  for (int x = 0; x < numTeams_; x++) {
    sortedTeams[x]->result.rank = x + 1;
  }
  delete sortedTeams;
}

Team** BerryBotsEngine::getTeams() {
  return teams_;
}

Team* BerryBotsEngine::getTeam(int teamIndex) {
  return teams_[teamIndex];
}

Team* BerryBotsEngine::getTeam(lua_State *L) {
  for (int x = 0; x < numInitializedTeams_; x++) {
    Team *team = teams_[x];
    if (team->state == L) {
      return team;
    }
  }
  return 0;
}

Team** BerryBotsEngine::getRankedTeams() {
  processWinnerRanksScores();

  Team** sortedTeams = new Team*[numTeams_];
  for (int x = 0; x < numTeams_; x++) {
    sortedTeams[x] = teams_[x];
  }
  for (int x = 0; x < numTeams_ - 1; x++) {
    for (int y = x + 1; y < numTeams_; y++) {
      Team *team1 = sortedTeams[x];
      Team *team2 = sortedTeams[y];
      if ((team1->result.rank > team2->result.rank && team2->result.rank != 0)
          || (team1->result.rank == 0 && team2->result.rank != 0)) {
        sortedTeams[x] = team2;
        sortedTeams[y] = team1;
      }
    }
  }
  return sortedTeams;
}

int BerryBotsEngine::getNumTeams() {
  return numTeams_;
}

// Taken from luajit.c
static int traceback(lua_State *L) {
  if (!lua_isstring(L, 1)) { /* Non-string error object? Try metamethod. */
    if (lua_isnoneornil(L, 1) || !luaL_callmeta(L, 1, "__tostring")
        || !lua_isstring(L, -1)) {
      return 1;  /* Return non-string error object. */
    }
    lua_remove(L, 1);  /* Replace object by result of __tostring metamethod. */
  }
  luaL_traceback(L, L, lua_tostring(L, 1), 1);
  return 1;
}

int BerryBotsEngine::callUserLuaCode(lua_State *L, int nargs,
    const char *errorMsg, int callStyle) throw (EngineException*) {
  int base = lua_gettop(L) - nargs;
  lua_pushcfunction(L, traceback);
  lua_insert(L, base);

  // TODO: I'm not completely convinced we're safe without a mutex here. Do
  //       more tests and maybe add one.
  timerSettings_->timerExpiration = timerSettings_->timerTick + 2;
  timerSettings_->L = L;
  int pcallValue = lua_pcall(L, nargs, 0, base);
  timerSettings_->L = 0;
  timerSettings_->timerExpiration = LONG_MAX;

  lua_remove(L, base);

  if (pcallValue != 0) {
    std::string errorString(errorMsg);
    errorString.append(": %s");
    if (callStyle == PCALL_STAGE) {
      throwForLuaError(L, errorString.c_str());
    } else if (callStyle == PCALL_SHIP) {
      printLuaErrorToShipConsole(L, errorString.c_str());
    }
  }

  return pcallValue;
}

void *BerryBotsEngine::timer(void *vargs) {
  TickTimerSettings *settings = (TickTimerSettings *) vargs;
  while (settings->enabled) {
    platformstl::micro_sleep(PCALL_TIME_LIMIT);
    if (settings->timerExpiration <= ++(settings->timerTick)) {
      if (settings->L != 0) {
        lua_State *killState = settings->L;
        settings->L = 0;
        lua_sethook(killState, killHook, LUA_MASKCOUNT, 1);
      }
    }
  }
  delete settings;
  return 0;
}

// Loads the stage in the file stageName, which may include a relative path,
// from the root directory stagesBaseDir. Note that the file may be either a
// .lua file, in which case we just load it directly; or a stage packaged as a
// .tar.gz file, in which case we extract it to the cache and load from there.
void BerryBotsEngine::initStage(const char *stagesBaseDir,
    const char *stageName, const char *cacheDir) throw (EngineException*) {
  if (stagesDir_ != 0 || stageFilename_ != 0) {
    throw new EngineException(stageName,
                              "Already initialized stage for this engine.");
  }

  try {
    fileManager_->loadStageFileData(
        stagesBaseDir, stageName, &stagesDir_, &stageFilename_, cacheDir);
  } catch (FileNotFoundException *fnfe) {
    EngineException *eie = new EngineException(stageName, fnfe->what());
    delete fnfe;
    throw eie;
  } catch (ZipperException *ze) {
    EngineException *eie = new EngineException(stageName, ze->what());
    delete ze;
    throw eie;
  } catch (PackagedSymlinkException *pse) {
    EngineException *eie = new EngineException(stageName, pse->what());
    delete pse;
    throw eie;
  }
  initStageState(&stageState_, stagesDir_);
  lua_setprinter(stageState_, this);

  if (luaL_loadfile(stageState_, stageFilename_)) {
    throwForLuaError(stageState_, "Cannot load stage file: %s");
  }
  callUserLuaCode(stageState_, 0, "Cannot load stage file", PCALL_STAGE);

  char *stageRootName = fileManager_->stripExtension(stageName);
  char *stageDisplayName = fileManager_->parseFilename(stageRootName);
  stage_->setName(stageDisplayName); // TODO: let stage set name like ship
  std::stringstream msgStream;
  msgStream << "== Stage control program loaded: " << stageDisplayName;
  stagePrint(msgStream.str().c_str());
  delete stageRootName;
  delete stageDisplayName;

  lua_getglobal(stageState_, "configure");
  StageBuilder *stageBuilder = pushStageBuilder(stageState_);
  stageBuilder->engine = this;
  callUserLuaCode(stageState_, 1, "Error calling stage function: 'configure'",
                  PCALL_STAGE);

  stage_->buildBaseWalls();
  stagePrint("");
  stageConfigureComplete_ = true;

  replayBuilder_->addStageProperties(stage_->getName(), stage_->getWidth(),
                                     stage_->getHeight());
  Wall **walls = stage_->getWalls();
  int numWalls = stage_->getWallCount();
  for (int x = 0; x < numWalls; x++) {
    Wall *wall = walls[x];
    replayBuilder_->addWall(wall->getLeft(), wall->getBottom(),
                            wall->getWidth(), wall->getHeight());
  }
  Zone **zones = stage_->getZones();
  int numZones = stage_->getZoneCount();
  for (int x = 0; x < numZones; x++) {
    Zone *zone = zones[x];
    replayBuilder_->addZone(zone->getLeft(), zone->getBottom(),
                            zone->getWidth(), zone->getHeight());
  }
  replayHandler_ = new ReplayEventHandler(replayBuilder_);
  stage_->addEventHandler(replayHandler_);
  
  stage_->setRelativistic(relativistic_);
  stage_->setWallCollDamage(wallCollDamage_);
  stage_->setShipShipCollDamage(shipShipCollDamage_);
  
}

// Loads the teams in the files specified in teamNames from the root directory
// shipsBaseDir. Each team name is a path relative to shipsBaseDir . Note that
// the team file may be either a .lua file, in which case we just load it
// directly; or a ship/team packaged as a .tar.gz file, in which case we extract
// it to the cache and load from there.
void BerryBotsEngine::initShips(const char *shipsBaseDir, char **teamNames,
    int numTeams, const char *cacheDir) throw (EngineException*) {
  int userTeams = numTeams;
  int numStageShips = stage_->getStageShipCount();
  numShips_ = (userTeams * teamSize_) + numStageShips;
  numTeams_ = userTeams + numStageShips;
  replayBuilder_->initShips(numTeams_, numShips_);

  ships_ = new Ship*[numShips_];
  shipProperties_ = new ShipProperties*[numShips_];
  teams_ = new Team*[numTeams_];
  worlds_ = new World*[numTeams_];
  shipGfxs_ = new ShipGfx*[numTeams_];
  int shipIndex = 0;
  for (int x = 0; x < numTeams_; x++) {
    const char *baseDir;
    char *filename;
    bool deleteFilename = false;
    bool stageShip = (x >= userTeams);
    if (stageShip) {
      baseDir = stagesDir_;
      const char *localFilename = stage_->getStageShips()[x - userTeams];
      filename = fileManager_->getStageShipRelativePath(
          stagesDir_, stageFilename_, localFilename);
      if (filename == 0) {
        throw new EngineException(localFilename,
            "Stage ship must be located under stages directory.");
      }
      deleteFilename = true;
    } else {
      baseDir = shipsBaseDir;
      filename = teamNames[x];
    }

    lua_State *teamState;
    char *shipDir = 0;
    char *shipFilename = 0;
    try {
      fileManager_->loadShipFileData(baseDir, filename, &shipDir, &shipFilename,
                                     cacheDir);
    } catch (FileNotFoundException *fnfe) {
      if (deleteFilename) {
        delete filename;
      }
      if (shipDir != 0) {
        delete shipDir;
      }
      if (shipFilename != 0) {
        delete shipFilename ;
      }

      EngineException *eie = new EngineException(filename, fnfe->what());
      delete fnfe;
      throw eie;
    } catch (ZipperException *ze) {
      if (deleteFilename) {
        delete filename;
      }
      if (shipDir != 0) {
        delete shipDir;
      }
      if (shipFilename != 0) {
        delete shipFilename ;
      }
      
      EngineException *eie = new EngineException(filename, ze->what());
      delete ze;
      throw eie;
    } catch (PackagedSymlinkException *pse) {
      if (deleteFilename) {
        delete filename;
      }
      if (shipDir != 0) {
        delete shipDir;
      }
      if (shipFilename != 0) {
        delete shipFilename ;
      }
      
      EngineException *eie = new EngineException(filename, pse->what());
      delete pse;
      throw eie;
    }
    initShipState(&teamState, shipDir);
    lua_setprinter(teamState, this);

    Team *team = new Team;
    team->index = x;
    team->firstShipIndex = shipIndex;
    team->state = teamState;
    team->errored = false;
    team->gfxEnabled = false;
    team->tooManyRectangles = team->tooManyLines = false;
    team->tooManyCircles = team->tooManyTexts = false;
    if (printHandler_ != 0) {
      printHandler_->registerTeam(team, filename);
    }

    int numStateShips = (stageShip ? 1 : teamSize_);
    Ship **stateShips = new Ship*[numStateShips];
    bool disabled;
    if (luaL_loadfile(teamState, shipFilename)) {
      printLuaErrorToShipConsole(teamState, "Error loading file: %s");
      disabled = true;
      team->ownedByLua = false;
    } else if (callUserLuaCode(teamState, 0, "Error loading file",
                               PCALL_SHIP)) {
      disabled = true;
      team->ownedByLua = false;
    } else {
      lua_getglobal(teamState, "init");
      disabled = false;
      team->ownedByLua = true;
    }

    if (!disabled && numStateShips > 1) {
      // If it's a team, pass a table of ships instead of a single ship.
      lua_newtable(teamState);
    }

    team->numShips = numStateShips;
    team->shipsAlive = 0;
    team->stageEventRef = 0;
    for (int y = 0; y < CPU_TIME_TICKS; y++) {
      team->cpuTime[y] = 0;
    }
    team->totalCpuTime = 0;
    team->totalCpuTicks = 0;
    team->disabled = disabled;
    team->numRectangles = 0;
    team->numLines = 0;
    team->numCircles = 0;
    team->numTexts = 0;

    lua_getglobal(teamState, "roundOver");
    team->hasRoundOver = (strcmp(luaL_typename(teamState, -1), "nil") != 0);
    lua_getglobal(teamState, "gameOver");
    team->hasGameOver = (strcmp(luaL_typename(teamState, -1), "nil") != 0);
    lua_pop(teamState, 2);

    char *shipFilenameRoot = fileManager_->stripExtension(filename);
    char *defaultShipName = fileManager_->parseFilename(shipFilenameRoot);
    int defaultNameLength = std::min(MAX_NAME_LENGTH, (int) strlen(defaultShipName));
    strncpy(team->name, defaultShipName, defaultNameLength);
    team->name[defaultNameLength] = '\0';

    int filenameLength = std::min(MAX_NAME_LENGTH, (int) strlen(filename));
    strncpy(team->filename, filename, filenameLength);
    team->filename[filenameLength] = '\0';

    std::stringstream msgStream;
    msgStream << "== Ship control program loaded: " << shipFilename;

    delete shipFilename;
    delete shipFilenameRoot;
    team->stageShip = stageShip;
    team->result.score = team->result.rank = team->result.numStats = 0;
    team->result.showResult = (!stageShip);

    for (int y = 0; y < numStateShips; y++) {
      Ship *ship;
      if (disabled) {
        ship = new Ship;
        ship->properties = new ShipProperties;
        ship->properties->disabled = true;
        ship->properties->ownedByLua = false;
      } else {
        ship = pushShip(teamState);
        if (numStateShips > 1) {
          lua_rawseti(teamState, -2, y + 1);
        }
        ship->properties->disabled = false;
        ship->properties->ownedByLua = true;
      }
      ship->alive = false;
      ship->teamIndex = x;
      ship->index = shipIndex++;
      ships_[ship->index] = stateShips[y] = ship;
    }

    for (int y = 0; y < numStateShips; y++) {
      Ship *ship = stateShips[y];
      ShipProperties *properties = ship->properties;
      ship->laserEnabled = battleMode_;
      ship->torpedoEnabled = battleMode_;
      ship->thrusterEnabled = true;
      ship->energyEnabled = battleMode_;
      ship->powerEnabled = battleMode_;
      ship->shieldsEnabled = battleMode_;
      ship->showName = true;
      ship->kills = ship->damage = 0;
      ship->friendlyKills = ship->friendlyDamage = 0;
      ship->shieldedDamage = 0;
      ship->newColors = true;
      properties->stageShip = stageShip;
      properties->shipR = properties->shipG = properties->shipB = 255;
      properties->laserR = properties->laserB = 0;
      properties->laserG = 255;
      properties->shieldsR = 100;
      properties->shieldsB = 255;
      properties->shieldsG = 200;      
      properties->thrusterG = properties->thrusterB = 0;
      properties->thrusterR = 255;
      properties->engine = this;
  
      strncpy(properties->name, defaultShipName, defaultNameLength);
      properties->name[defaultNameLength] = '\0';
      initShipRound(ship);
    }
    delete defaultShipName;

    teams_[x] = team;
    numInitializedTeams_++;
    for (int y = 0; y < numStateShips; y++) {
      Ship *ship = stateShips[y];
      ships_[ship->index] = ship;
      numInitializedShips_++;
    }
    if (!disabled) {
      shipPrint(teamState, msgStream.str().c_str());
    }

    if (!disabled) {
      worlds_[x] = pushWorld(teamState, stage_, numShips_, teamSize_);
      worlds_[x]->engine = this;
      shipGfxs_[x] = pushShipGfx(teamState);
      shipGfxs_[x]->team = team;
      shipGfxs_[x]->engine = this;

      int r = callUserLuaCode(teamState, 3,
          "Error calling ship function: 'init'", PCALL_SHIP);
      if (r != 0) {
        team->disabled = true;
        for (int y = 0; y < numStateShips; y++) {
          stateShips[y]->alive = false;
          stateShips[y]->properties->disabled = true;
        }
      }
    }
    if (deleteFilename) {
      delete filename;
    }
    if (shipDir != 0) {
      delete shipDir;
    }
    delete stateShips;
  }

  // TODO: print to output console if ship or team name changes
  uniqueShipNames(ships_, numShips_);
  uniqueTeamNames(teams_, numTeams_);
  for (int x = 0; x < numShips_; x++) {
    replayBuilder_->addShipProperties(ships_[x]);
  }
  for (int x = 0; x < numTeams_; x++) {
    replayBuilder_->addTeamProperties(teams_[x]);
  }

  teamVision_ = new bool*[numTeams_];
  for (int x = 0; x < numTeams_; x++) {
    teamVision_[x] = new bool[numShips_];
  }
  sensorHandler_ = new SensorHandler(teams_, numTeams_, teamVision_);
  stage_->addEventHandler((EventHandler*) sensorHandler_);

  lua_getglobal(stageState_, "init");
  stageShips_ = new Ship*[numShips_];
  pushCopyOfShips(stageState_, ships_, stageShips_, numShips_);
  stage_->setTeamsAndShips(teams_, numTeams_, stageShips_, numShips_);
  if (strcmp(luaL_typename(stageState_, -2), "nil") != 0) {
    stageWorld_ = pushWorld(stageState_, stage_, numShips_, teamSize_);
    stageWorld_->engine = this;
    Admin *admin = pushAdmin(stageState_);
    admin->engine = this;
    stageGfx_ = pushStageGfx(stageState_);
    stageGfx_->engine = this;
    callUserLuaCode(stageState_, 4, "Error calling stage function: 'init'",
                    PCALL_STAGE);
  }

  copyShips(stageShips_, ships_, numShips_);
  oldShips_ = new Ship*[numShips_];
  prevShips_ = new Ship*[numShips_];
  for (int x = 0; x < numShips_; x++) {
    oldShips_[x] = new Ship;
    prevShips_[x] = new Ship;
  }
  copyShips(ships_, oldShips_, numShips_);

  replayBuilder_->addShipStates(ships_, gameTime_);

  lua_getglobal(stageState_, "run");
  stageRun_ = (strcmp(luaL_typename(stageState_, -1), "nil") != 0);
  lua_settop(stageState_, 0);

  for (int x = 0; x < numShips_; x++) {
    shipProperties_[x] = ships_[x]->properties;
  }

  for (int x = 0; x < numTeams_; x++) {
    shipPrint(teams_[x]->state, "");
  }
  shipInitComplete_ = true;
}

void BerryBotsEngine::initShipRound(Ship *ship) {
  ship->alive = (!ship->properties->disabled && !ship->properties->stageShip);
  ship->speed = ship->momentum = ship->heading = 
    ship->thrusterAngle = ship->thrusterForce = 0;
  ship->energy = DEFAULT_ENERGY;
  ship->torpedoAmmo = TORPEDO_AMMO;
  ship->laserGunHeat = LASER_HEAT;
  ship->torpedoGunHeat = TORPEDO_HEAT;
  ship->hitWall = ship->hitShip = false;
  ship->power = DEFAULT_POWER;
  ship->shields = 0;

  bool safeStart;
  do {
    Point2D *startPosition = stage_->getStart();
    ship->x = startPosition->getX();
    ship->y = startPosition->getY();
    safeStart = true;
    for (int y = 0; y < ship->index; y++) {
      if (ships_[y]->alive
          && square(ship->x - ships_[y]->x) + square(ship->y - ships_[y]->y)
                < square(SHIP_RADIUS * 2)) {
        safeStart = false;
      }
    }
    delete startPosition;
  } while (!safeStart);
}

void BerryBotsEngine::updateTeamShipsAlive() {
  for (int x = 0; x < numTeams_; x++) {
    Team *team = teams_[x];
    int shipsAlive = 0;
    for (int y = 0; y < team->numShips; y++) {
      Ship *ship = ships_[y + team->firstShipIndex];
      if (ship->alive) {
        shipsAlive++;
      }
    }
    team->shipsAlive = shipsAlive;
  }
}

void BerryBotsEngine::stagePrint(const char *text) {
  if (printHandler_ != 0) {
    printHandler_->stagePrint(text);
  }
  replayBuilder_->addLogEntry(0, gameTime_, text);
}

void BerryBotsEngine::shipPrint(lua_State *L, const char *text) {
  if (printHandler_ != 0) {
    printHandler_->shipPrint(L, text);
  }
  replayBuilder_->addLogEntry(this->getTeam(L), gameTime_, text);
}

void BerryBotsEngine::processTick() throw (EngineException*) {
  gameTime_++;
  physicsOver_ = false;
  updateTeamShipsAlive();    
  stage_->updateTeamVision(teams_, numTeams_, ships_, numShips_, teamVision_);
  stage_->clearStaleUserGfxs(gameTime_);
  copyShips(oldShips_, prevShips_, numShips_);
  copyShips(ships_, oldShips_, numShips_);
  for (int x = 0; x < numTeams_; x++) {
    Team *team = teams_[x];
    if (team->shipsAlive > 0 && !team->disabled) {
      worlds_[x]->time = gameTime_;
      for (int y = 0; y < team->numShips; y++) {
        int shipIndex = y + team->firstShipIndex;
        Ship *ship = ships_[shipIndex];
        ship->thrusterForce = 0;
        ship->laserGunHeat = std::max(0, ship->laserGunHeat - 1);
        ship->torpedoGunHeat = std::max(0, ship->torpedoGunHeat - 1);
        ship->power = std::min(DEFAULT_POWER, ship->power+POWER_REGEN);
        ship->shields *= SHIELDS_DECAY;
      }

      lua_getglobal(team->state, "run");
      pushVisibleEnemyShips(
          team->state, teamVision_[x], x, oldShips_, numShips_);
      Sensors *sensors = pushSensors(team, sensorHandler_, shipProperties_);
      team->counter.start();
      int r = callUserLuaCode(team->state, 2,
                              "Error calling ship function: 'run'", PCALL_SHIP);
      monitorCpuTimer(team, (r != 0 && lua_gethookcount(team->state) > 0));
      cleanupSensorsTables(team->state, sensors);
      lua_settop(team->state, 0);
    }
  }
  stage_->moveAndCheckCollisions(oldShips_, ships_, numShips_, gameTime_);
  physicsOver_ = true;
  replayBuilder_->addShipStates(ships_, gameTime_);

  if (stageRun_) {
    this->setRoundOver(false);
    this->setGameOver(false);
    processStageRun();
  }
}

void BerryBotsEngine::processStageRun() throw (EngineException*) {
  copyShips(ships_, stageShips_, numShips_);
  if (stageWorld_ != 0) {
    stageWorld_->time = gameTime_;
  }
  lua_getglobal(stageState_, "run");
  StageSensors *stageSensors = pushStageSensors(
      stageState_, sensorHandler_, stageShips_, shipProperties_);
  callUserLuaCode(stageState_, 1, "Error calling stage function: 'run'",
                  PCALL_STAGE);
  cleanupStageSensorsTables(stageState_, stageSensors);
  lua_settop(stageState_, 0);
  copyShips(stageShips_, ships_, numShips_);
}

void BerryBotsEngine::processRoundOver() {
  copyShips(stageShips_, ships_, numShips_);
  stage_->reset(gameTime_);
  for (int x = 0; x < numTeams_; x++) {
    Team *team = teams_[x];
    if (team->hasRoundOver) {
      lua_getglobal(team->state, "roundOver");
      team->counter.start();
      int r = callUserLuaCode(team->state, 0,
          "Error calling ship function: 'roundOver'", PCALL_SHIP);
      monitorCpuTimer(team, (r != 0 && lua_gethookcount(team->state) > 0));
    }
    for (int y = 0; y < team->numShips; y++) {
      Ship *ship = ships_[team->firstShipIndex + y];
      if (!ship->properties->stageShip && !ship->properties->disabled) {
        initShipRound(ship);
      }
    }
  }
  copyShips(ships_, stageShips_, numShips_);
}

void BerryBotsEngine::processGameOver() {
  copyShips(stageShips_, ships_, numShips_);
  for (int x = 0; x < numTeams_; x++) {
    Team *team = teams_[x];
    if (team->hasGameOver) {
      lua_getglobal(team->state, "gameOver");
      team->counter.start();
      int r = callUserLuaCode(team->state, 0,
          "Error calling ship function: 'gameOver'", PCALL_SHIP);
      monitorCpuTimer(team, (r != 0 && lua_gethookcount(team->state) > 0));
    }
  }
  copyShips(ships_, stageShips_, numShips_);
}

void BerryBotsEngine::monitorCpuTimer(Team *team, bool fatal) {
  unsigned int cpuTimeSlot = team->totalCpuTicks % CPU_TIME_TICKS;
  team->counter.stop();
  team->totalCpuTime +=
  (team->cpuTime[cpuTimeSlot] = team->counter.get_microseconds());
  team->totalCpuTicks++;

  if (fatal) {
    team->disabled = true;
    for (int x = 0; x < team->numShips; x++) {
      Ship *ship = ships_[x + team->firstShipIndex];
      destroyShip(ship);
      ship->properties->disabled = true;
    }
  }
}

bool BerryBotsEngine::touchedZone(Ship *ship, const char *zoneTag) {
  Ship *oldShip =
      (physicsOver_ ? oldShips_[ship->index] : prevShips_[ship->index]);
  Ship *newShip = (physicsOver_ ? ship : oldShips_[ship->index]);
  return stage_->touchedZone(oldShip, newShip, zoneTag);
}

bool BerryBotsEngine::touchedAnyZone(Ship *ship) {
  Ship *oldShip =
      (physicsOver_ ? oldShips_[ship->index] : prevShips_[ship->index]);
  Ship *newShip = (physicsOver_ ? ship : oldShips_[ship->index]);
  return stage_->touchedAnyZone(oldShip, newShip);
}

void BerryBotsEngine::destroyShip(Ship *ship) {
  stage_->destroyShip(ship, gameTime_);
}

void BerryBotsEngine::uniqueShipNames(Ship** ships, int numShips) {
  for (int x = 1; x < numShips; x++) {
    Ship *ship1 = ships[x];
    int nameNum = 1;
    int nameLen = (int) strlen(ship1->properties->name);
    char numStr[8];
    for (int y = 0; y < x; y++) {
      Ship *ship2 = ships[y];
      if (strcmp(ship1->properties->name, ship2->properties->name) == 0) {
        sprintf(numStr, " %d", ++nameNum);
        strncpy(
            &(ship1->properties->name[std::min(nameLen, MAX_NAME_LENGTH - 7)]),
            numStr, 7);
        y = -1;
      }
    }
  }
}

void BerryBotsEngine::uniqueTeamNames(Team** teams, int numTeams) {
  for (int x = 1; x < numTeams; x++) {
    Team *team1 = teams[x];
    int nameNum = 1;
    int nameLen = (int) strlen(team1->name);
    char numStr[8];
    for (int y = 0; y < x; y++) {
      Team *team2 = teams[y];
      if (strcmp(team1->name, team2->name) == 0) {
        sprintf(numStr, " %d", ++nameNum);
        strncpy(
            &(team1->name[std::min(nameLen, MAX_NAME_LENGTH - 7)]), numStr, 7);
        y = -1;
      }
    }
  }
}

void BerryBotsEngine::copyShips(
    Ship **srcShips, Ship **destShips, int numShips) {
  for (int x = 0; x < numShips; x++) {
    *(destShips[x]) = *(srcShips[x]);
  }
}

ReplayBuilder* BerryBotsEngine::getReplayBuilder() {
  deleteReplayBuilder_ = false;
  return replayBuilder_;
}

// Note: We don't have to log stage errors to the output console because they
//       are considered fatal. We throw exceptions from the engine and the
//       GUI or CLI displays them appropriately.
void BerryBotsEngine::printLuaErrorToShipConsole(lua_State *L,
                                                 const char *formatString) {
  if (printHandler_ != 0) {
    char *errorMessage = formatLuaError(L, formatString);
    if (printHandler_ != 0) {
      printHandler_->shipError(L, errorMessage);
    }
    replayBuilder_->addLogEntry(this->getTeam(L), gameTime_, errorMessage);
    delete errorMessage;
  }
  for (int x = 0; x < numInitializedTeams_; x++) {
    Team *team = teams_[x];
    if (team->state == L) {
      team->errored = true;
    }
  }
}

void BerryBotsEngine::throwForLuaError(lua_State *L, const char *formatString)
    throw (EngineException*) {
  char *errorMessage = formatLuaError(L, formatString);
  EngineException *e = new EngineException(errorMessage);
  delete errorMessage;
  throw e;
}

char* BerryBotsEngine::formatLuaError(lua_State *L, const char *formatString) {
  const char *luaMessage = lua_tostring(L, -1);
  int messageLen = (int) (strlen(formatString) + strlen(luaMessage) - 2);
  char *errorMessage = new char[messageLen + 1];
  sprintf(errorMessage, formatString, luaMessage);
  return errorMessage;
}

EngineException::EngineException(const char *details) {
  message_ = new char[strlen(details) + 41];
  sprintf(message_, "Engine failure: %s", details);
}

EngineException::EngineException(const char *filename, const char *details) {
  std::string errorMessage(filename);
  errorMessage.append(": Engine failure: ");
  errorMessage.append(details);
  message_ = new char[errorMessage.size() + 1];
  strcpy(message_, errorMessage.c_str());
}

const char* EngineException::what() const throw() {
  return message_;
}

EngineException::~EngineException() throw() {
  delete message_;
}

ConsoleEventHandler::ConsoleEventHandler(BerryBotsEngine *engine) {
  engine_ = engine;
  tooManyStageRectangles_ = tooManyStageLines_ = false;
  tooManyStageCircles_ = tooManyStageTexts_ = false;
}

void ConsoleEventHandler::handleShipDestroyed(Ship *destroyedShip, int time,
    Ship **destroyerShips, int numDestroyers) {
  for (int x = 0; x < numDestroyers; x++) {
    printShipDestroyed(destroyedShip, destroyerShips[x], time);
  }
}

void ConsoleEventHandler::tooManyUserGfxRectangles(Team *team) {
  printTooManyUserGfxRectangles(team);
}

void ConsoleEventHandler::tooManyUserGfxLines(Team *team) {
  printTooManyUserGfxLines(team);
}

void ConsoleEventHandler::tooManyUserGfxCircles(Team *team) {
  printTooManyUserGfxCircles(team);
}

void ConsoleEventHandler::tooManyUserGfxTexts(Team *team) {
  printTooManyUserGfxTexts(team);
}

void ConsoleEventHandler::printShipDestroyed(
    Ship *destroyedShip, Ship *destroyerShip, int time) {
  if (destroyedShip->index == destroyerShip->index) {
    lua_State *teamState = engine_->getTeam(destroyerShip->teamIndex)->state;
    std::stringstream msgStream;
    msgStream << "== " << destroyerShip->properties->name
              << " destroyed itself @ " << time;
    engine_->shipPrint(teamState, msgStream.str().c_str());
  } else {
    lua_State *destroyerState =
        engine_->getTeam(destroyerShip->teamIndex)->state;
    std::stringstream destroyerStream;
    destroyerStream << "== " << destroyerShip->properties->name << " destroyed "
        << ((destroyedShip->teamIndex == destroyerShip->teamIndex)
            ? "friendly" : "enemy")
        << " ship: " << destroyedShip->properties->name << " @ " << time;
    engine_->shipPrint(destroyerState, destroyerStream.str().c_str());

    std::stringstream destroyeeStream;
    lua_State *destroyeeState =
        engine_->getTeam(destroyedShip->teamIndex)->state;
    destroyeeStream << "== " << destroyedShip->properties->name
        << " destroyed by: " << destroyerShip->properties->name << " @ "
        << time;
    engine_->shipPrint(destroyeeState, destroyeeStream.str().c_str());
  }
}

void ConsoleEventHandler::printTooManyUserGfxRectangles(Team *team) {
  std::stringstream msgStream;
  msgStream << TOO_MANY_RECTANGLES << TOO_MANY_MORE_INFO;

  if (team == 0) {
    if (!tooManyStageRectangles_) {
      engine_->stagePrint(msgStream.str().c_str());
      tooManyStageRectangles_ = true;
    }
  } else {
    if (!team->tooManyRectangles) {
      engine_->shipPrint(team->state, msgStream.str().c_str());
      team->errored = team->tooManyRectangles = true;
    }
  }
}

void ConsoleEventHandler::printTooManyUserGfxLines(Team *team) {
  std::stringstream msgStream;
  msgStream << TOO_MANY_LINES << TOO_MANY_MORE_INFO;

  if (team == 0) {
    if (!tooManyStageLines_) {
      engine_->stagePrint(msgStream.str().c_str());
      tooManyStageLines_ = true;
    }
  } else {
    if (!team->tooManyLines) {
      engine_->shipPrint(team->state, msgStream.str().c_str());
      team->errored = team->tooManyLines = true;
    }
  }
}

void ConsoleEventHandler::printTooManyUserGfxCircles(Team *team) {
  std::stringstream msgStream;
  msgStream << TOO_MANY_CIRCLES << TOO_MANY_MORE_INFO;

  if (team == 0) {
    if (!tooManyStageCircles_) {
      engine_->stagePrint(msgStream.str().c_str());
      tooManyStageCircles_ = true;
    }
  } else {
    if (!team->tooManyCircles) {
      engine_->shipPrint(team->state, msgStream.str().c_str());
      team->errored = team->tooManyCircles = true;
    }
  }
}

void ConsoleEventHandler::printTooManyUserGfxTexts(Team *team) {
  std::stringstream msgStream;
  msgStream << TOO_MANY_TEXTS << TOO_MANY_MORE_INFO;

  if (team == 0) {
    if (!tooManyStageTexts_) {
      engine_->stagePrint(msgStream.str().c_str());
      tooManyStageTexts_ = true;
    }
  } else {
    if (!team->tooManyTexts) {
      engine_->shipPrint(team->state, msgStream.str().c_str());
      team->errored = team->tooManyTexts = true;
    }
  }
}
