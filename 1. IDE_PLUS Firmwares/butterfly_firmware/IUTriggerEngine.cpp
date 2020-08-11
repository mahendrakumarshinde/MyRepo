#include "IUTriggerEngine.h"
#include "Conductor.h"

extern Conductor conductor;

Node *activeDIGContainer = NULL;
Node *reportableDIGContainer = NULL;
   

IUTriggerComputer::IUTriggerComputer()
{
    if (setupDebugMode){
       debugPrint("Constructor IUTriggerComputer !");
    }
    
}

IUTriggerComputer::~IUTriggerComputer()
{
    if (setupDebugMode)
    {   
        debugPrint("Destructor IUTriggerComputer !");
    }
}

// bool IUTriggerComputer::computeTriggerState(float result,float threshold,const char* comparator){

//       if (result > threshold)
//       {
//           //cout << " Trigger is Active !" << endl;
//           return 1;
//       }else
//       {
//           return 0;
//       }
// }
/**
 * @brief 
 * 
 * @param digObj 
 */
void  IUTriggerComputer::getActiveDiagnostic(JsonObject& digObj){
    
    
}

/**
 * @brief This function stores the list of active triggers per diagnostic
 * 
 * @param digId super index diagnostic ID index
 * @param index Points to the List Index from the TRG config message 
 * @param subindex points to the actual Trigger Index from the TRG congigurations
 * @param trgState state of the computated trigger
 */
void IUTriggerComputer::maintainActiveTriggers(uint8_t digId,uint8_t index, uint8_t subindex, bool trgState){

}
/**
 * @brief 
 * 
 * @param digId 
 * @param index 
 * @param subindex 
 * @param trgState 
 */
void IUTriggerComputer::maintainInactiveTriggers(uint8_t digId,uint8_t index, uint8_t subindex, bool trgState){

}
/**
 * @brief stores all the active /inactive list of triggers as a JSON List
 * 
 * @param trgstate 
 */
void IUTriggerComputer::maintainAllTriggers(uint8_t digCount,uint8_t trgCount, bool trgstate){

    StaticJsonBuffer<400> triggerResultsBuffer;
    JsonObject& triggerResult = triggerResultsBuffer.createObject();
    JsonArray& triggers = triggerResult.createNestedArray("FTR");
    if(triggerResult.success()){ 
    //    for (size_t i = 0; i < digCount; i++)
    //     { 
            JsonArray& TRG = triggers.createNestedArray();
            // for (size_t i = 0; i < trgCount; i++)
            // {   // Append the trigger status
                 TRG.add(trgstate);
            // }
        //}
        triggerResult.printTo(Serial);
    }
}

/**
 * @brief compute the triggers
 * 
 */
void IUTriggerComputer:: m_specializedCompute() {

    
    float featureOutput[2];
    float fout[2];          // allow  max two features can be compared
    JsonObject &m_digObject =  conductor.configureJsonFromFlash("/iuconfig/diagnostic.conf",true);
    // validate the entries available in trigger and diagnostic configs 
    // NB : All the Keys should have a same length of the list 
    bool validFlag = validateDigConfigs(m_digObject); 
    debugPrint("validFlag : ",false);debugPrint(validFlag); 
    if(validFlag){
        bool m_tstate = 0;
        bool optFeature = false;
        int comparatorId;
        // Flush the both stack (container)
        flushPreviousDiagnosticList(STACK::ACTIVE_DIG);
        flushPreviousDiagnosticList(STACK::REPORTABLE_DIG);
        // Trigger Ouptput JOSN List
        StaticJsonBuffer<400> triggerResultsBuffer;
        JsonObject& triggerResult = triggerResultsBuffer.createObject();
        JsonArray& triggers = triggerResult.createNestedArray("FTR");
    
        uint8_t TRG_SIZE = m_digObject["CONFIG"]["TRG"]["FID1"].size();
        uint8_t DIG_SIZE = m_digObject["CONFIG"]["DIG"]["ID"].size();
        
        debugPrint("TRG_SIZE : ",false);debugPrint(TRG_SIZE);
        for (size_t i = 0; i < TRG_SIZE; i++)   
        {   
            JsonArray& TRG = triggers.createNestedArray();
                
            for (size_t j = 0; j < m_digObject["CONFIG"]["TRG"]["FID1"][i].size(); j++)
            {   
                m_feature[0] = m_digObject["CONFIG"]["TRG"]["FID1"][i][j];
                m_feature[1] = m_digObject["CONFIG"]["TRG"]["FID2"][i][j];
                m_operator    = m_digObject["CONFIG"]["TRG"]["OPTR"][i][j];
                m_comparator  = m_digObject["CONFIG"]["TRG"]["COMP"][i][j];
                m_threshold  = m_digObject["CONFIG"]["TRG"]["TRH"][i][j]; 
                
                // debugPrint("Feature Names :",false);
                // debugPrint(m_feature[0],false);debugPrint(",",false);debugPrint(m_feature[1]);
                // debugPrint("OPTR :",false);debugPrint(m_operator);//debugPrint(",",false);debugPrint(m_operand[0],false);
                // debugPrint("TRH :",false); debugPrint(m_threshold);
                // debugPrint("COMP : ",false);debugPrint(m_comparator);
                
                // process 1
                // Get the Feature values from feature output JSON
                comparatorId = getCompartorId(m_comparator);
                //debugPrint("COMP ID :",false);debugPrint(comparatorId);
                if(strcmp(m_feature[1],"NULL") == 0 ){
                    getFeatures(m_feature[0],fout);
                    optFeature = false;
                }else{
                    // get comparator Id
                    getFeatures(m_feature[0],m_feature[1],fout);
                    optFeature = true;
                    //debugPrint("Fout[0]:",false);debugPrint(fout[0]);
                    //debugPrint("Fout[1]:",false);debugPrint(fout[1]);
                }
                float res = getTriggerOutput(fout[0],fout[1],m_operator,optFeature);
                m_tstate =  computeTriggerState(res,m_threshold,comparatorId);
                 if(loopDebugMode){
                     debugPrint("\nTRG Computation : ",false);
                     debugPrint("\nFID1 :",false);debugPrint(m_feature[0],false);debugPrint("->",false);debugPrint(fout[0]);
                     debugPrint("FID2 :",false);debugPrint(m_feature[1],false);debugPrint("->",false);debugPrint(fout[1]);
                     debugPrint("TRG O/P : ",false); debugPrint(res);
                     debugPrint("TRG STATE : ",false);debugPrint(m_tstate);
                     
                }
                // process 2 store active/inactive triggers 
                // TODO : sort the active and inactive trigger list as per diagnostic(optional)
                
                //process 3 manage all active triggers as per diagnostics
                TRG.add(m_tstate);
            }
            debugPrint("........................................");
        }
        // process 4 
        //compute active diagnostics and DIG state
        // Logic : MAND & FTR  = 1 <active> ,0 <inactive>
        bool mandTRG;
        static uint8_t indexCount=0;
        // get active triggers per diagnostic
        for (size_t i = 0; i < TRG_SIZE; i++)
        {
            for (size_t j = 0; j <m_digObject["CONFIG"]["TRG"]["MAND"][i].size(); j++)
            {        
                mandTRG  = m_digObject["CONFIG"]["TRG"]["MAND"][i][j].as<bool>(); 
                if(mandTRG){ 
                    m_activeTriggerList[indexCount] = j;
                    indexCount++; 
                }
                m_firingTriggers[j] = triggerResult["FTR"][i][j].as<bool>();
                if(loopDebugMode){
                    debugPrint("\nMAND : ",false);debugPrint(mandTRG,false);debugPrint(" : ",false);
                    debugPrint(" FTR : ",false);debugPrint(m_firingTriggers[j],false);
                    debugPrint(" AND :",false);debugPrint(mandTRG & m_firingTriggers[j]);
                }
                if(m_firingTriggers[j] == 1){
                    atleastOneFiringTriggerActive = true;
                }
            }
            // check is Diagnostic Active ?
            // Rule 1 : if no mandatory triggers configured but triggers are firing then its Active
            debugPrint("indexCount : ",false); debugPrint(indexCount); 
            if(indexCount == 0 && atleastOneFiringTriggerActive){
                m_diagnosticState = true;
            }else {
             // Rule 2 : All mandatory  triggers should be firing
             // check all Mandatory triggers are firing ?
                for (size_t id = 0; id < indexCount; id++)
                {
                    m_firingTriggers[i] = triggerResult["FTR"][i][m_activeTriggerList[id]].as<bool>();
                    m_diagnosticState = (m_diagnosticState | 1)  &  m_firingTriggers[i];
                    if(loopDebugMode){
                        debugPrint("Active TRG index : ",false);
                        debugPrint(m_activeTriggerList[id],false);
                        debugPrint(" FTR Active : ",false);debugPrint(m_firingTriggers[i],true);
                    }
                    if(m_diagnosticState == false){
                        debugPrint("Not All MAND TRGs are Active, exit loop");
                        break;
                    }   
                }   
            }

            debugPrint(" DIG STATE : ",false);
            debugPrint(m_diagnosticState);
            const char* DIG_NAME = m_digObject["CONFIG"]["DIG"]["ID"][i].as<const char*>();
            updateActiveDiagnosticList(DIG_NAME);
            
            // RESET variables
            indexCount = 0;
            atleastOneFiringTriggerActive = false;
            m_diagnosticState = false;
            resetBuffer();
            debugPrint("*******************************************");
        }

        debugPrint("Active Diagnostic List : ");
        listDiagnosticContainer(STACK::ACTIVE_DIG);
        // TODO : Add Code for Reportable Diagnostic
        
        debugPrint("Reportable Diagnostic List :");    
        pushToStack(STACK::REPORTABLE_DIG,"Vikas");
        pushToStack(STACK::REPORTABLE_DIG,"Darshan");
        pushToStack(STACK::REPORTABLE_DIG,"Swami");

        listDiagnosticContainer(STACK::REPORTABLE_DIG);
        debugPrint("Flush STACK NO : 2");
        flushPreviousDiagnosticList(STACK::REPORTABLE_DIG);

    }
    
    
}

/**
 * @brief Update the the Active Diagnostic List only using the stack with linkedlist
 *        to store the active diagnostic name 
 * @param m_digId diagnostic ID/ name from the configurations
 */
void IUTriggerComputer::updateActiveDiagnosticList(const char*  m_digName){
    if(m_diagnosticState){
        pushToStack(STACK::ACTIVE_DIG, m_digName);    
    }else
    {
        if (loopDebugMode)
        {
            debugPrint("DIG Inactive :",false);
            debugPrint(m_digName);
        }
    }
}

void IUTriggerComputer::pushToStack(uint8_t cId ,const char* name){
    Node *newNode;    
    newNode = new Node();
    newNode->digName = name;
    if(cId == 0) {
        newNode->link = activeDIGContainer;
        activeDIGContainer = newNode;
    }else
    {
        newNode->link = reportableDIGContainer;
        reportableDIGContainer = newNode;
    }
}
void IUTriggerComputer::popFromStack(uint8_t cId){
    Node *temp;
    if(cId == 0){
        temp = activeDIGContainer;
        if (activeDIGContainer == 0)
        {
            debugPrint("Active DIG Container is Empty");
        }else
        {
            // debugPrint("Popped Item is  :",false);
            // debugPrint(activeDIGContainer->digName);
            activeDIGContainer = activeDIGContainer->link;
            free(temp);
        }
    }else{
        temp = reportableDIGContainer;
        if (reportableDIGContainer == 0)
        {
            debugPrint("Active DIG Container is Empty");
        }else
        {
            // debugPrint("Popped Item is  :",false);
            // debugPrint(reportableDIGContainer->digName);
            reportableDIGContainer = reportableDIGContainer->link;
            free(temp);
        }
    }
}
/**
 * @brief Prints all the active stck items
 * 
 */
void IUTriggerComputer::listDiagnosticContainer(uint8_t cId){

    Node *temp;
    if(cId == 0){
        temp = activeDIGContainer;
        if (activeDIGContainer == 0)
        {
            debugPrint(" Active DIG Stack is Empty ");
            return;
        }else
        {
            while (temp != 0)
            {
                //debugPrint("Active DIG Stack Item : ");
                debugPrint(temp->digName);
                temp = temp->link;
            }
        }
    }else{
        temp  = reportableDIGContainer;
        if (reportableDIGContainer == 0)
        {
            debugPrint(" Reportable DIG Stack is Empty ");
            return;
        }else
        {
              while (temp != 0)
            {
                //debugPrint("Reportable Stack Item : ");
                debugPrint(temp->digName);
                temp = temp->link;
            }
        }
    }
}

void IUTriggerComputer::flushPreviousDiagnosticList(uint8_t cId){
    Node *temp;
    if(cId == 0){
        temp = activeDIGContainer;
        if (activeDIGContainer == NULL)
        {
            if(loopDebugMode){
                debugPrint("Active DIG container is Empty");
            }
            return;
        }else
        {
            // delete pop() the complete stack (container)
            while (activeDIGContainer != 0)
            {
                popFromStack(cId);
            }
        }
    }else
    {   temp  = reportableDIGContainer;
        if (reportableDIGContainer == NULL)
        {
            if(loopDebugMode){
                debugPrint("ReportableDIGContaine is empty");
            }
            return;
        }else
        {
            // delete pop() the complete stack (container)
            while (reportableDIGContainer != 0)
            {
                popFromStack(cId);
            }
        }
    }
    
} 

void IUTriggerComputer::resetBuffer(){
    for (size_t i = 0; i < MAX_TRIGGERS_LEN; i++)
    {
        m_activeTriggerList[i] = 0;
    }
}

uint8_t IUTriggerComputer::getCompartorId(const char* comparatorName){
    
    if (strcmp(comparatorName,"==") == 0)
    {   
        return COMPARATORS::EQUAL_TO;
    }
    if (strcmp(comparatorName,"!=") == 0)
    {   
        return COMPARATORS::NOT_EQUAL_TO;
    }
    if (strcmp(comparatorName,">") == 0)
    {   
        return COMPARATORS::GREATER_THAN;
    }
    if (strcmp(comparatorName,">=") == 0)
    {   
        return COMPARATORS::GREATER_THAN_EQUAL_TO;
    }
    if (strcmp(comparatorName,"<") == 0)
    {   
        return COMPARATORS::LESS_THAN;
    }
    if (strcmp(comparatorName,"<=") == 0)
    {   
        return COMPARATORS::LESS_THAN_EQUAL_TO;
    }
}

void IUTriggerComputer::getFeatures(const char* feature1,float* dest){
    // return the mandatory feature output
    const size_t bufferSize = 600;          
    StaticJsonBuffer<bufferSize> jsonBuffer;
    JsonObject &featuerObj = iuFlash.loadConfigJson(IUFlash::CFG_FOUT, jsonBuffer);
    JsonObject &subconfig = featuerObj["FRES"];
    if (subconfig.success()){
        if (subconfig.containsKey(feature1) ){
            // read the feature output value
            dest[0] = subconfig[feature1].as<float>();
            dest[1] = 0; 
        }
    }else
    {
        if (loopDebugMode)
        {
            debugPrint("Feature Result , Invalid JSON");
        }
        
    }
    
}

void IUTriggerComputer::getFeatures(const char* feature1,const char* feature2,float* dest){
    // TODO : remove this once logic once the feature computation are done 
    // NO NEED to read everytime from file 
    // bufferSize will be blocker to allow max no of features 
    const size_t bufferSize = 600;          
    StaticJsonBuffer<bufferSize> jsonBuffer;
    JsonObject &featuerObj = iuFlash.loadConfigJson(IUFlash::CFG_FOUT, jsonBuffer);
    JsonObject &subconfig = featuerObj["FRES"];
    if (subconfig.success()){
        //debugPrint("list all computated features: ");
        // for(JsonObject::iterator it=subconfig.begin(); it!=subconfig.end(); ++it)
        // {
        //     debugPrint(it->key);
        // }
        if (subconfig.containsKey(feature1) ){
            // read the feature output value
            dest[0] = subconfig[feature1].as<float>();
        }if (subconfig.containsKey(feature2) ){
            // read the feature output value
            dest[1] = subconfig[feature2].as<float>();
        }
    }
}
bool IUTriggerComputer::validateDigConfigs(JsonObject &config){
    
     if( config.success() ) {
        const char* msgType = config["MSGTYPE"];
        const int Tsize = config["CONFIG"]["TRG"].size();
        uint8_t Tlen = config["CONFIG"]["TRG"]["FID1"][0].size();
        uint8_t indexSize[Tsize];
        bool validTriggerSize = false;
        debugPrint("Tsize :",false); debugPrint(Tsize);
        debugPrint("Tlen : ",false); debugPrint(Tlen);
        // validate the Trigger paraemters 
        char* type[] = { "FID1","FID2","OPTR","COMP","TRH","MAND"};
        uint8_t temp;
        for (size_t i = 0; i <Tsize ; i++)
        {
           indexSize[i] =  config["CONFIG"]["TRG"][type[i]].size();
        //    debugPrint("Size :",false);
        //    debugPrint(indexSize[i]);
           validTriggerSize = true;
        }
        if (validTriggerSize && strcmp(msgType,"DIG") == 0 )
        {
            for (size_t i = 0; i < Tsize; i++)
            {
                if (indexSize[0] != indexSize[i])
                {
                    validTriggerSize = false;
                    if (loopDebugMode)
                    {  debugPrint("DiG configs length mismatch"); }
                    return false;
                }
            }
        return validTriggerSize;
        }
   
    }
}
/*** Diagnostic State Tracking and Applying the Alert policy using the triplate
 * 
 *   Diagnostic state can be tracked using the 3 state tracking variables 
 * 
 *  1. last_active: most recent timestamp that the diagnostic was found active
 *  2. first_active: first timestamp that the diagnostic was found active for the current activity period
 *  3. last_alert: timestamp of the most recent published alert message
 *
 *  
 */


IUDiagnosticStateComputer::IUDiagnosticStateComputer()
{
    //cout << "diagnosticStateComputer, constructor" << endl;
    if (setupDebugMode)
    {
        debugPrint("Constructor IUDiagnosticStateComputer !");
    }
    
}

IUDiagnosticStateComputer::~IUDiagnosticStateComputer()
{
    if (setupDebugMode)
    {
        debugPrint("Destructor IUDiagnosticStateComputer !");
    }
    
}
