/*
  Copyright (C) 2013-2015 - Voidious

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

#ifndef REPLAY_BUILDER_H
#define REPLAY_BUILDER_H

#include <string.h>
#include <sstream>
#include "bbutil.h"
#include "eventhandler.h"

#define REPLAY_VERSION            1
#define REPLAY_TEMPLATE           "replay_template.html"
#define KINETIC_JS                "kinetic-v5.1.0.min.js"
#define BBREPLAY_JS               "bbreplay-v1.3.js"
#define REPLAY_TITLE_PLACEHOLDER  "{$replayTitle}"
#define REPLAY_DATA_PLACEHOLDER   "{$replayData}"
#define KINETIC_JS_PLACEHOLDER    "{$kineticJs}"
#define BBREPLAY_JS_PLACEHOLDER   "{$bbreplayJs}"

#define CHUNK_SIZE            (1024 * 32 / 4)  // 32 kb of ints
#define MAX_SHIP_TICK_CHUNKS  640              // 20 megs
#define MAX_LASER_CHUNKS      640              // 20 megs lasers/sparks
#define MAX_TEXT_CHUNKS       640              // 20 megs
#define MAX_MISC_CHUNKS       32               // 1 meg each
#define MAX_TORPEDO_SPARKS    30

typedef struct {
  int data[CHUNK_SIZE];
  int size;
} ReplayChunk;

class ReplayData {
  ReplayChunk **chunks_;
  int numChunks_;
  int maxChunks_;

  public:
    ReplayData(int maxChunks);
    ~ReplayData();
    void addInt(int x);
    void addString(const char *s);
    int getSize();
    int getInt(int index);
    void writeChunks(FILE *f);
    std::string toHexString(int blockSize);
};

class ReplayBuilder {
  bool *shipsAlive_;
  bool *shipsShowName_;
  bool *shipsShowEnergy_;
  char **teamNames_;
  int numTeams_;
  int numShips_;
  int numTeamsAdded_;
  char *stageName_;
  ReplayData *stagePropertiesData_;
  ReplayData *wallsData_;
  ReplayData *zonesData_;
  ReplayData *teamPropertiesData_;
  ReplayData *shipPropertiesData_;
  ReplayData *shipAddData_;
  ReplayData *shipRemoveData_;
  ReplayData *shipShowNameData_;
  ReplayData *shipHideNameData_;
  ReplayData *shipShowEnergyData_;
  ReplayData *shipHideEnergyData_;
  ReplayData *shipTickData_;
  ReplayData *laserStartData_;
  ReplayData *laserEndData_;
  ReplayData *laserSparkData_;
  ReplayData *torpedoStartData_;
  ReplayData *torpedoEndData_;
  ReplayData *torpedoBlastData_;
  ReplayData *torpedoDebrisData_;
  ReplayData *shipDestroyData_;
  ReplayData *textData_;
  ReplayData *logData_;
  ReplayData *resultsData_;
  int numTexts_;
  int numLogEntries_;
  char *timestamp_;
  char *templateDir_;
  
  public:
    ReplayBuilder(const char *templateDir);
    ~ReplayBuilder();
    void initShips(int numTeams, int numShips);
    void addStageProperties(const char *name, int width, int height);
    void addWall(int left, int bottom, int width, int height);
    void addZone(int left, int bottom, int width, int height);
    void addTeamProperties(Team *team);
    void addShipProperties(Ship *ship);
    void addShipStates(Ship **ships, int time);
    void addLaserStart(Laser *laser);
    void addLaserEnd(Laser *laser, int time);
    void addLaserSpark(Laser *laser, int time, double x, double y,
                        double dx, double dy);
    void addTorpedoStart(Torpedo *torpedo);
    void addTorpedoEnd(Torpedo *torpedo, int time);
    void addTorpedoBlast(Torpedo *torpedo, int time);
    void addTorpedoDebris(Ship *ship, int time, double dx, double dy,
                           int parts);
    void addShipDestroy(Ship *ship, int time);
    void addText(int time, const char *text, double x, double y, int size,
                  RgbaColor textColor, int duration);
    void addLogEntry(Team *team, int time, const char *logMessage);
    void setResults(Team **rankedTeams, int numTeams);
    const char* getStageName();
    void setTimestamp(const char *timestamp);
    void saveReplay(const char *filename);
    void saveReplay(const char *dir, const char *filename);
  private:
    void addShip(int shipIndex, int time);
    void removeShip(int shipIndex, int time);
    void addShipShowName(int shipIndex, int time);
    void addShipHideName(int shipIndex, int time);
    void addShipShowEnergy(int shipIndex, int time);
    void addShipHideEnergy(int shipIndex, int time);
    void addResult(Team *team);
    void addStat(ScoreStat *stat);
    int round(double f);
    void copyReplayResource(const char *resource, const char *targetDir);
    std::string buildReplayDataString();
    char* readReplayTemplate();
    std::string stagePropertiesDataString();
    std::string teamPropertiesDataString();
    std::string shipPropertiesDataString();
    std::string textDataString();
    std::string logDataString();
    std::string resultsDataString();
    std::string htmlTitle();
    std::string escapeString(std::string s);
    std::string escapeHtml(std::string s);
    void findReplace(std::string &s, char find, const char *replace);
    void appendInt(std::stringstream &hexStream, int i);
    void appendColonString(std::stringstream &hexStream, ReplayData *data,
                          int &i);
    void appendString(std::stringstream &hexStream, ReplayData *data, int &i);

    char* getResourcePath(const char *resourcePath);
};

class ReplayEventHandler : public EventHandler {
  ReplayBuilder *replayBuilder_;

  public:
    ReplayEventHandler(ReplayBuilder *replayBuilder);
    virtual void handleShipFiredLaser(Ship *firingShip, Laser *laser);
    virtual void handleLaserDestroyed(Laser *laser, int time);
    virtual void handleShipFiredTorpedo(Ship *firingShip, Torpedo *torpedo);
    virtual void handleTorpedoDestroyed(Torpedo *torpedo, int time);
    virtual void handleTorpedoExploded(Torpedo *torpedo, int time);
    virtual void handleShipDestroyed(Ship *destroyedShip, int time,
        Ship **destroyerShips, int numDestroyers);
    virtual void handleLaserHitShip(Ship *srcShip, Ship *targetShip,
        Laser *laser, double dx, double dy, int time);
    virtual void handleTorpedoHitShip(Ship *srcShip, Ship *targetShip,
        double dx, double dy, double hitAngle, double hitForce,
        double hitDamage, int time);
    virtual void handleStageText(StageText *stageText);

    virtual void handleShipHitShip(Ship *hittingShip, Ship *targetShip,
        double inAngle, double inForce, double outAngle, double outForce,
        double damage, int time) {};
    virtual void handleShipHitWall(Ship *hittingShip, double bounceAngle,
        double bounceForce, double hitDamage, int time) {};
    virtual void tooManyUserGfxRectangles(Team *team) {};
    virtual void tooManyUserGfxLines(Team *team) {};
    virtual void tooManyUserGfxCircles(Team *team) {};
    virtual void tooManyUserGfxTexts(Team *team) {};
};

#endif
