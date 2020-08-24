#ifndef IUTRIGGERENGINE_H
#define IUTRIGGERENGINE_H

#include<Arduino.h>
#include <ArduinoJson.h>
#include <IUDebugger.h>
//#include "IUFlash.h"

// enum stores the ASCII value of Operand symbols 
enum OPERANDS { 
    MOD         = 37,
    MULTIPLY    = 42,
    ADD         = 43,
    SUBTRACT    = 45,
    DIVIDE      = 47,
    ABS_DIFF    = 99
};

enum COMPARATORS {
    EQUAL_TO              = 0,
    NOT_EQUAL_TO          = 1,
    GREATER_THAN          = 2,
    LESS_THAN             = 3,
    GREATER_THAN_EQUAL_TO = 4,
    LESS_THAN_EQUAL_TO    = 5
};

enum STACK {
    ACTIVE_DIG      = 0,
    REPORTABLE_DIG  = 1
};

class Node {
        public:
         // Linked list 
        const char* digName;
        Node* link;
  };

class IUTriggerComputer
{
    private:
        int value;
        float m_threshold;
        float fout[2];
        const char* m_trgId;
        const char* m_operator ; 
        const char* m_comparator;
        const char* m_feature[2];
        bool m_diagnosticState = false;
        
    public:
        /*** Variables *****/
        static const uint8_t MAX_TRIGGERS_LEN  = 10; 
        static const uint8_t MAX_DIAGNOSTIC_LEN  = 10; 
        uint8_t m_activeTriggerList[MAX_TRIGGERS_LEN];
        uint8_t m_inactiveTriggerList[MAX_TRIGGERS_LEN];
        uint8_t m_activeDiagnosticList[MAX_DIAGNOSTIC_LEN];
        uint8_t m_inactiveDiagnosticList[MAX_DIAGNOSTIC_LEN];
        uint8_t m_firingTriggers[MAX_TRIGGERS_LEN];
        uint8_t m_activeDiagnosticLenght = 0;
        uint8_t m_reportableDiagnosticLength = 0;
        int DIG_COUNT;
        int m_activeTRGCounter;
        int m_inactiveTRGCounter;
        int ACTIVE_TRGCOUNT[MAX_DIAGNOSTIC_LEN];   // store the count of active TRG as per DIG index
        int INACTIVE_TRGCOUNT[MAX_DIAGNOSTIC_LEN];   // store the count of Inactive TRG as per DIG index
        
        bool atleastOneFiringTriggerActive = false;
        String RDIG_LIST[MAX_DIAGNOSTIC_LEN];
        String ACTIVE_TRG[MAX_DIAGNOSTIC_LEN];
        String m_activeTRGList;
        String m_inactiveTRGList;
        String m_totalTRGList;
        const char* diagnosticId;
        char* activeTRG[MAX_DIAGNOSTIC_LEN][MAX_TRIGGERS_LEN];
        char* inactiveTRG[MAX_DIAGNOSTIC_LEN][MAX_TRIGGERS_LEN];
        bool m_triggerStates[MAX_DIAGNOSTIC_LEN][MAX_TRIGGERS_LEN];
        //char* activeTRG[MAX_DIAGNOSTIC_LEN][MAX_TRIGGERS_LEN];
        
        /*** Constructor ****/
        IUTriggerComputer();
        /*** Destructor ****/
        ~IUTriggerComputer();

        /** Operands *****/
        // Implemented template function below
        /*** get container size ****/
        uint8_t getActiveDigContainerlength();
        uint8_t getreportableDigContainerLength();
        // Get Trigger Status
        char* getActiveTriggers(char* dName);
        char* getInactiveTriggers(char* dName);
        uint8_t getCompartorId(const char* comparatorName);
        /**** Feature *****/
        void getFeatures(const char* feature1,float* dest);
        void getFeatures(const char* feature1,const char* feature2,float* dest);
        /**** Validation *****/
        bool validateDigConfigs(JsonObject &config);
        /**** Trigger List ****/
        void maintainActiveTriggers(const char* trgId,uint8_t digIndex,bool trgState);
        void maintainInactiveTriggers(const char* trgId,uint8_t digIndex,bool trgState);
        void maintainAllTriggers(uint8_t digCount,uint8_t trgCount,bool trgstate);
        /****  compute ****/
        virtual void m_specializedCompute();
        void getActiveDiagnostic(JsonObject& digObj);
        void updateActiveDiagnosticList(const char*  m_digName);
        /**** Reset ****/
        void resetBuffer();

        void pushToStack(uint8_t cId,const char* name);
        const char* popFromStack(uint8_t cId);
        void flushPreviousDiagnosticList(uint8_t cId);
        void listDiagnosticContainer(uint8_t cId);
 
};


class IUDiagnosticStateComputer
{
private:
    /* data */
public:
    IUDiagnosticStateComputer();
    ~IUDiagnosticStateComputer();

    void getReportableDiagnostic();

};

/**
 * @brief class IUDiagnosticNotifier
 * manages the publication of the reportable diagnostic list 
 * support the message segmentation with message acknowledment
 * max N no of diagnostic alert can be published in a single o/p JsonObject 
 * 
 */
class IUDiagnosticNotifier:public IUTriggerComputer
{
private:
    /* data */
    uint8_t DEFAULT_BATCH_SIZE = 5;
    const char* digName;
public:
    /**** Constructor *****/
    IUDiagnosticNotifier(/* args */);
    /**** Destructor *****/
    ~IUDiagnosticNotifier();
    const char* getDiagnosticName(uint8_t cId);
    void publishedNotification();
    

};



/**
 * @brief Get the Trigger Output object
 * 
 * @tparam T datatype int, float, double
 * @param feature1 feature1 ID or Name
 * @param feature2 feature2 ID or Name
 * @param operand  Operand to apply on feture data
 * @return T       return the output of the Operand 
 */
template <typename T>
T getTriggerOutput(T feature1, T feature2,const char* operand,bool optionalFeatureFlag)
{
    T result = 0;
    if (strcmp(operand,"NULL") == 0 || optionalFeatureFlag == false )
    {
        result = feature1;
        return result;
    }
    if(strcmp(operand,"--") == 0 && strcmp(operand,"NULL") != 0 ){
        result = feature1 - feature2;
        result = abs(result);
        if(loopDebugMode){
            debugPrint("Abs. Difference :",false);
            debugPrint(result);
        }
    }else {
        switch (operand[0])
        {
        case OPERANDS::ADD :
            result = feature1 + feature2;
            break;
        case OPERANDS::SUBTRACT :
            result = feature1 - feature2;
            break;
        case OPERANDS::MULTIPLY :
            result = feature1 * feature2;
            break;
        case OPERANDS::DIVIDE :
            result = feature1 /feature2;
            break;
        case OPERANDS::MOD :
            result = (int) feature1 % (int)feature2;
            break;
        default:
            break;
        }
    }
    return result;
}

template <typename T>
T computeTriggerState(T result,T threshold,int comparator)
{
    bool tState = 0;
    switch (comparator)
    {
    case COMPARATORS::EQUAL_TO:
         tState = (result == threshold) ? 1:0;
        break;
    case COMPARATORS::NOT_EQUAL_TO :
        tState = (result != threshold) ? 1:0;
        break;
    case COMPARATORS::GREATER_THAN :
        tState = (result > threshold) ? 1:0;
        break;
    case COMPARATORS::GREATER_THAN_EQUAL_TO :
        tState = (result >= threshold) ? 1:0;
        break;
    case COMPARATORS::LESS_THAN :
        tState = (result < threshold) ? 1:0;
        break;
    case COMPARATORS::LESS_THAN_EQUAL_TO :
        tState = (result <= threshold) ? 1:0;
        break;
    default:
        tState = 0;
        if (loopDebugMode)
        {
            debugPrint("Invalid Comparator");
        }
        break;
    }
    return tState;
}

#endif