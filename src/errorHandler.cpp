#include <Arduino.h>
#include "errorHandler.h"
#include "globals.h"
#include "comms.h"
#include "workFunctions.h"

void raiseError(int code) {
    errorRaised = true;
    errorCode = code;

    outgoingData.errorRaised = true;
    outgoingData.errorCode = code;

DBG_PRINT("errorRaised: ");
DBG_PRINT(outgoingData.errorRaised);
DBG_PRINT(" Code: ");
DBG_PRINT(outgoingData.errorCode);
DBG_PRINT(" errorAck: ");
DBG_PRINTLN(incomingData.errorAck);

}

void clearError() {
    
    if (errorCode == 1 || errorCode == 2) { // codes 1 & 2 are non critical
        
        errorRaised = false;
        errorCode = 0;

        outgoingData.errorRaised = false;
        outgoingData.errorCode = 0;
    
    } else if (errorCode == 3) {  // code 3 is a stalled motor and cannot be cleared unless the work switch is false

        if (!readWorkSwitch()) {

            errorRaised = false;
            errorCode = 0;

            outgoingData.errorRaised = false;
            outgoingData.errorCode = 0;

        }
    }


DBG_PRINT("errorRaised: ");
DBG_PRINT(outgoingData.errorRaised);
DBG_PRINT(" Code: ");
DBG_PRINT(outgoingData.errorCode);
DBG_PRINT(" errorAck: ");
DBG_PRINTLN(incomingData.errorAck);

}

