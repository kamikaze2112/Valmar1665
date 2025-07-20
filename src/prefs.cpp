#include <Arduino.h>
#include "globals.h"
#include "encoder.h"
#include "comms.h"
#include <Preferences.h>

bool prefsValid = false;
bool commsValid = false;

Preferences prefs;

void loadComms() {

    prefs.begin("slave_comms", true);

    commsValid = prefs.getBool("commsValid", false);

    if (commsValid) {

        screenPaired = prefs.getBool("screenPaired", false);
        prefs.getBytes("screenAddress", screenAddress, 6);
        DBG_PRINTLN("Loaded comms values from NVS");

    } else {

        DBG_PRINTLN("No stored comms values in NVS.  Using broadcast as defualt.");
    }

    prefs.end();
}

void saveComms() {

    prefs.begin("slave_comms", false);

    if (screenPaired) {

        prefs.putBool("screenPaired", screenPaired);
        prefs.putBytes("screenAddress", screenAddress, 6);
        prefs.putBool("commsValid", true);
    }

    prefs.end();

}

void clearComms() {

    prefs.begin("slave_comms", false);

    prefs.clear();

    prefs.end();

}


void loadPrefs() {

    prefs.begin("valmar_slave", true);

    prefsValid = prefs.getBool("prefsValid", false);

    if (prefsValid) {

        seedPerRev = prefs.getFloat("seedPerRev", 0.0f);
        Encoder::revs = prefs.getFloat("calibrationRevs", 0.0f);
        calibrationWeight = prefs.getFloat("calibrationWeight", 0.0f);

        targetSeedingRate = prefs.getFloat("targetRate", 0.0f);

    }

    prefs.end();
}

void savePrefs() {

    prefs.begin("valmar_slave", false);

    prefs.putFloat("seedPerRev", seedPerRev);
    prefs.putFloat("calibrationRevs", Encoder::revs);
    prefs.putFloat("calibrationWeight", calibrationWeight);

    prefs.putInt("targetRate", targetSeedingRate);
    prefs.putBool("prefsValid", true);

    prefs.end();
}

void clearPrefs() {

    prefs.begin("valmar_slave", false);

    prefs.clear();

    prefs.end();

}