#ifndef CONFIGURATOR_H
#define CONFIGURATOR_H

#include <ArduinoJson.h>

#include "FeatureGraph.h"
#include "Conductor.h"


class Configurator
{
    public:
        /***** Preset values and default settings *****/
        static char START_CONFIRM[12];
        static char END_CONFIRM[10];
        /***** Constructors & destructor *****/
        Configurator(int placeholder) {}
        virtual ~Configurator() {}
        /***** Serial Reading *****/
        void readFromSerial(IUSerial *iuSerial);
        /***** Command Processing *****/
        void processConfiguration(char *json);
        void configureDeviceFunctions(JsonVariant &config);
        void configureAllFeatures(JsonVariant &config);
        void configureAllSensors(JsonVariant &config);
        void processLegacyUSBCommands(char *buff);
        void processLegacyBLECommands(char *buff);
        /***** Debugging *****/
        void exposeAllConfigurations();


    protected:
        // Static JSON buffer to parse config
        StaticJsonBuffer<1600> jsonBuffer;
        // eg: can hold the following config (remove the space and line breaks)
//        {
//          "features": {
//            "A93": {
//              "STREAM": 1,
//              "OPS": 1,
//              "TRH": [100, 110, 120]
//            },
//            "VAX": {
//              "STREAM": 1,
//              "OPS": 0,
//              "TRH": [0.5, 1.2, 1.5]
//            },
//            "VAY": {
//              "STREAM": 1,
//              "OPS": 0,
//              "TRH": [0.5, 1.2, 1.5]
//            },
//            "VAZ": {
//              "STREAM": 1,
//              "OPS": 0,
//              "TRH": [0.5, 1.2, 1.5]
//            },
//           "TMP": {
//              "STREAM": 1,
//              "OPS": 0,
//              "TRH": [40, 80, 120]
//            },
//            "S12": {
//              "STREAM": 1,
//              "OPS": 0,
//              "TRH": [500, 100, 1500]
//            },
//            "XXX": {
//              "STREAM": 1,
//              "OPS": 0,
//              "TRH": [500, 100, 1500]
//            },
//            "XXX": {
//              "STREAM": 1,
//              "OPS": 0,
//              "TRH": [500, 100, 1500]
//            },
//            "XXX": {
//              "STREAM": 1,
//              "OPS": 0,
//              "TRH": [500, 100, 1500]
//            },
//            "XXX": {
//              "STREAM": 1,
//              "OPS": 0,
//              "TRH": [500, 100, 1500]
//            }
//          }
//        }

};


/***** Instanciation *****/

extern Configurator configurator;

#endif // CONFIGURATOR_H
