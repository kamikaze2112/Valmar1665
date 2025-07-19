#include <Arduino.h>
#include "globals.h"
#include "encoder.h"
#include <Preferences.h>

bool prefsValid = false;

Preferences prefs;

void loadPrefs() {

    prefs.begin("valmar_slave", false);

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