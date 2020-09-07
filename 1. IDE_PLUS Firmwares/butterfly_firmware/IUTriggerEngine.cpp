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

uint8_t IUTriggerComputer::getActiveDigContainerlength(){
    return m_activeDiagnosticLenght;
}
uint8_t IUTriggerComputer::getreportableDigContainerLength(){
   return m_reportableDiagnosticLength;
}
/**
 * @brief 
 * 
 * @param digObj 
 */
void  IUTriggerComputer::getActiveDiagnostic(JsonObject& digObj){
    // TODO : Not Implemented   
    
}

/**
 * @brief This function stores the list of active triggers per diagnostic
 * 
 * @param digId super index diagnostic ID index
 * @param index Points to the List Index from the TRG config message 
 * @param subindex points to the actual Trigger Index from the TRG congigurations
 * @param trgState state of the computated trigger
 * 
 * output message format : "DIG_ID:TR1,TR2,TR3;"
 */
void IUTriggerComputer::maintainActiveTriggers(const char* trgId,uint8_t digIndex,bool trgState){
    if (trgState)
    {   // fill all the active triggerIDs to 2D array
        activeTRG[digIndex][m_activeTRGCounter] =(char*) trgId; 
        m_activeTRGCounter++;
        if (loopDebugMode && DEBUG_ENABLE_FLAG)
        {
            debugPrint("Active Trigger:",false);
            debugPrint(trgId);
        }
    }
}
/**
 * @brief 
 * 
 * @param digId 
 * @param index 
 * @param subindex 
 * @param trgState 
 */
void IUTriggerComputer::maintainInactiveTriggers(const char* trgId,uint8_t digIndex,bool trgState){

    if (!trgState)
    {   // fill all the active triggerIDs to 2D array
        inactiveTRG[digIndex][m_inactiveTRGCounter] =(char*) trgId; 
        m_inactiveTRGCounter++;
        if (loopDebugMode && DEBUG_ENABLE_FLAG)
        {
            debugPrint("Active Trigger:",false);
            debugPrint(trgId);
        }
    }    
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

    bool featureFlag = true;
    JsonObject &m_digObject =  conductor.configureJsonFromFlash("/iuRule/diagnostic.conf",true);
    
    // validate the entries available in trigger and diagnostic configs 
    // NB : All the Keys should have a same length of the list
    // TODO : Future Scope send the ack if validFlag is false 
    bool validFlag = validateDigConfigs(m_digObject); 
    if(validFlag){
        bool m_tstate = 0;
        bool optFeature = false;
        int comparatorId;
        // Flush the both stack (container)
        // flushPreviousDiagnosticList(STACK::ACTIVE_DIG);
        // flushPreviousDiagnosticList(STACK::REPORTABLE_DIG);
        
        uint8_t TRG_SIZE = m_digObject["CONFIG"]["TRG"]["FID1"].size();
        uint8_t DIG_SIZE = m_digObject["CONFIG"]["DIG"]["DID"].size();
        for (size_t i = 0; i < TRG_SIZE; i++)   
        {   
            const char*  DIG_ID = m_digObject["CONFIG"]["DIG"]["DID"][i]; 
            
            for (size_t j = 0; j < m_digObject["CONFIG"]["TRG"]["FID1"][i].size(); j++)
            {   
                m_trgId         = m_digObject["CONFIG"]["TRG"]["TID"][i][j];
                m_feature[0]    = m_digObject["CONFIG"]["TRG"]["FID1"][i][j];
                m_feature[1]    = m_digObject["CONFIG"]["TRG"]["FID2"][i][j];
                m_operator      = m_digObject["CONFIG"]["TRG"]["OPTR"][i][j];
                m_comparator    = m_digObject["CONFIG"]["TRG"]["COMP"][i][j];
                m_threshold     = m_digObject["CONFIG"]["TRG"]["TRH"][i][j]; 
                m_mandState     = m_digObject["CONFIG"]["TRG"]["MAND"][i][j].as<bool>();
              
                m_mandFlag[i][j] = m_mandState;     // store the MAND States
                MAND_SIZE[i] = j+1;                 // store the count of MAND Flags
                // process 1
                // Get the Feature values from feature output JSON
                comparatorId = getCompartorId(m_comparator);
                if(strcmp(m_feature[1],"NULL") == 0 ){
                   featureFlag =  getFeatures(m_feature[0],fout);
                    optFeature = false;
                    if(featureFlag == false){
                        if(loopDebugMode && DEBUG_ENABLE_FLAG){
                            debugPrint("Invalid FRES JSON");
                        }
                        return;
                    }
                }else{
                    // get comparator Id
                   featureFlag = getFeatures(m_feature[0],m_feature[1],fout);
                    optFeature = true;
                    if(featureFlag == false){
                        if(loopDebugMode && DEBUG_ENABLE_FLAG){
                            debugPrint("Invalid FRES JSON");
                        }
                        return;
                    }
                }
                float res = getTriggerOutput(fout[0],fout[1],m_operator,optFeature);
                m_tstate =  computeTriggerState(res,m_threshold,comparatorId);
                 if(loopDebugMode && DEBUG_ENABLE_FLAG){
                     debugPrint("\nTRG Computation : ",false);
                     debugPrint("\nFID1 :",false);debugPrint(m_feature[0],false);debugPrint("->",false);debugPrint(fout[0]);
                     debugPrint("FID2 :",false);debugPrint(m_feature[1],false);debugPrint("->",false);debugPrint(fout[1]);
                     debugPrint("TRG O/P : ",false); debugPrint(res);
                     debugPrint("TRG STATE : ",false);debugPrint(m_tstate);
                }
                // process 2 store active/inactive triggers 
                uint8_t DIG_INDEX = i;
                maintainActiveTriggers(m_trgId,DIG_INDEX,m_tstate);
                maintainInactiveTriggers(m_trgId,DIG_INDEX,m_tstate);
                //process 3 manage all active triggers as per diagnostics
                m_triggerStates[DIG_INDEX][j] = m_tstate;        
            }
            // debugPrint("Active Count : ",false);
            // debugPrint(m_activeTRGCounter);
            ACTIVE_TRGCOUNT[i] = m_activeTRGCounter;
            INACTIVE_TRGCOUNT[i] = m_inactiveTRGCounter;
            m_activeTRGCounter = 0;
            m_inactiveTRGCounter = 0;
            featureFlag = true;
           if(loopDebugMode && DEBUG_ENABLE_FLAG){ 
            debugPrint("........................................");
           }
        }
        // process 4 
        // compute active diagnostics and DIG state
        // Logic : MAND & FTR  = 1 <active> ,0 <inactive>
        bool mandTRG;
        static uint8_t indexCount=0;
        // get active triggers per diagnostic
        for (size_t i = 0; i < TRG_SIZE; i++)
        {
            //uint8_t MAND_SIZE   =  m_digObject["CONFIG"]["TRG"]["MAND"][i].size();
            for (size_t j = 0; j < MAND_SIZE[i]; j++)
            {        
                //mandTRG  = m_digObject["CONFIG"]["TRG"]["MAND"][i][j].as<bool>(); 
                mandTRG = m_mandFlag[i][j];
                if(mandTRG){ 
                    m_activeTriggerList[indexCount] = j;    // store the index of MAND TRG
                    indexCount++; 
                }
                m_firingTriggers[j] = m_triggerStates[i][j];
                if(loopDebugMode && DEBUG_ENABLE_FLAG){
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
            if(indexCount == 0 && atleastOneFiringTriggerActive){
                m_diagnosticState = true;
            }else {
             // Rule 2 : All mandatory  triggers should be firing
             // check all Mandatory triggers are firing ?
                for (size_t id = 0; id < indexCount; id++)
                {
                    m_firingTriggers[i] = m_triggerStates[i][m_activeTriggerList[id]];
                    m_diagnosticState = (m_diagnosticState | 1)  &  m_firingTriggers[i];
                    if(loopDebugMode && DEBUG_ENABLE_FLAG){
                        debugPrint("Active TRG index : ",false);
                        debugPrint(m_activeTriggerList[id],false);
                        debugPrint(" FTR Active : ",false);debugPrint(m_firingTriggers[i],true);
                    }
                    if(m_diagnosticState == false){
                        if(loopDebugMode && DEBUG_ENABLE_FLAG){
                            debugPrint("Not All MAND TRGs are Active, exit loop");
                        }
                        break;
                    }   
                }   
            }

            const char* DID = m_digObject["CONFIG"]["DIG"]["DID"][i].as<const char*>();
            String DIG_NAME = DID + String(m_diagnosticState);
            DIG_LIST[i] = DIG_NAME;
            DIG_COUNT++;
            
            // DISABLED container storage 
            //updateActiveDiagnosticList(DIG_NAME.c_str());
            
            // RESET variables
            indexCount = 0;
            atleastOneFiringTriggerActive = false;
            m_diagnosticState = false;
            resetBuffer();
            if(loopDebugMode && DEBUG_ENABLE_FLAG){    
                debugPrint(" DIG STATE : ",false);
                debugPrint(m_diagnosticState);
                debugPrint("*******************************************");
            }
        }

       #if 0
        StaticJsonBuffer<1500> reportableJsonBUffer;
        JsonObject& reportableJson = reportableJsonBUffer.createObject();
        JsonObject& DIGRES = reportableJson.createNestedObject("DIGRES");
        debugPrint("Reportable List : ");
        listDiagnosticContainer(STACK::ACTIVE_DIG);
        int alen = getActiveDigContainerlength();
        debugPrint("R LEN Before : ",false);debugPrint(alen);
        const char* dId;
        for (size_t i = 0; i < alen; i++)
        {
        //Node *temp;
        //temp = activeDIGContainer;
        // while (activeDIGContainer != 0)
        // {
            dId = iuDigNotifier.getDiagnosticName(STACK::ACTIVE_DIG);
            //dId = iuTrigger.popFromStack(STACK::ACTIVE_DIG);
            debugPrint("dID : ",false);debugPrint(dId);
            // construct the RDIG JSON 
            JsonObject& diagnostic = DIGRES.createNestedObject(dId);
            JsonArray& ftr = diagnostic.createNestedArray("FTR");
            JsonObject& desc = diagnostic.createNestedObject("DESC");
            //addFTR(dId.c_str(),ftr);
            //m_reportableDiagnosticLength--;
            //DIGRES.set(dId,1);
            //ftr.add(i);
            //conductor.constructPayload(dId,desc);
            
            m_activeDiagnosticLenght--;
            
        }
        reportableJson.printTo(Serial); 
        int alen1 = getActiveDigContainerlength();
        debugPrint("\nR LEN After : ",false);debugPrint(alen1);
        #endif
        // TODO : Add Code for Reportable Diagnostic 
        // Created dumy stack of reportable diagnostic for Testing

        //debugPrint("Reportable Diagnostic List :");    
        // pushToStack(STACK::REPORTABLE_DIG,"UNBAL");
        // pushToStack(STACK::REPORTABLE_DIG,"BPFO");
        // pushToStack(STACK::REPORTABLE_DIG,"MISALIG");
        // pushToStack(STACK::REPORTABLE_DIG,"RDIG1");
        // pushToStack(STACK::REPORTABLE_DIG,"RDIG2");
        // pushToStack(STACK::REPORTABLE_DIG,"BPFI1");
        // pushToStack(STACK::REPORTABLE_DIG,"BPFI2");
        // pushToStack(STACK::REPORTABLE_DIG,"BPFI3");
        
        //listDiagnosticContainer(STACK::REPORTABLE_DIG);
        //debugPrint("Flush STACK NO : 2");
        //flushPreviousDiagnosticList(STACK::REPORTABLE_DIG);

    }
    
    //return (JsonVariant) diagnosticJson;
    
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
    {   pushToStack(STACK::REPORTABLE_DIG, m_digName);
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
        m_activeDiagnosticLenght++;
    }else
    {
        newNode->link = reportableDIGContainer;
        reportableDIGContainer = newNode;
        m_reportableDiagnosticLength++;
    }
}

const char* IUTriggerComputer::popFromStack(uint8_t cId){
    Node *temp;
    const char* value;
    if(cId == 0){
        temp = activeDIGContainer;
        if (activeDIGContainer == 0)
        {
            debugPrint("\nActive DIG Container is Empty");
        }else
        {
            debugPrint("\nPopped Item is  :",false);
            value = activeDIGContainer->digName;
            activeDIGContainer = activeDIGContainer->link;
            free(temp);
            m_activeDiagnosticLenght--;
        }
    }else{
        temp = reportableDIGContainer;
        if (reportableDIGContainer == 0)
        {
            debugPrint("\nReportable DIG Container is Empty");
        }else
        {
            //debugPrint("\nPopped Item is  :",false);
            value = reportableDIGContainer->digName;
            reportableDIGContainer = reportableDIGContainer->link;
            free(temp);
            m_reportableDiagnosticLength--;
        }
    }
    return value;
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
                //debugPrint("Active DIG Stack Item : ",false);
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
                debugPrint("Reportable Stack Item : ",false);
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
              diagnosticId = popFromStack(cId);
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

bool IUTriggerComputer::getFeatures(const char* feature1,float* dest){
    // return the mandatory feature output
    // TODO : Need to remove feature ouptut file and pass the json in runtime , In Future : Remove Json as well 
    bool success = true;
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
        if (loopDebugMode && DEBUG_ENABLE_FLAG)
        {
            debugPrint("Feature Result , Invalid JSON");
        }
        success = false;
    }
    return success;
}

bool IUTriggerComputer::getFeatures(const char* feature1,const char* feature2,float* dest){
    // TODO : remove this once logic once the feature computation are done 
    // NO NEED to read everytime from file 
    // bufferSize will be blocker to allow max no of features 
    bool success = true;
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
    }else
    {
        success = false;
    }
    
    return success;
}
bool IUTriggerComputer::validateDigConfigs(JsonObject &config){
    
     if( config.success() ) {
        const char* msgType = config["MSGTYPE"];
        const int Tsize = config["CONFIG"]["TRG"].size();
        uint8_t TRG_LEN = config["CONFIG"]["TRG"]["FID1"].size();
        uint8_t Tlen = config["CONFIG"]["TRG"]["FID1"][0].size();
        uint8_t DIG_LEN = config["CONFIG"]["DIG"]["DID"].size();
        uint8_t indexSize[Tsize];
        bool validTriggerSize = false;
        if(loopDebugMode && DEBUG_ENABLE_FLAG){
            debugPrint("Tsize :",false); debugPrint(Tsize);
            debugPrint("Tlen : ",false); debugPrint(Tlen);
            debugPrint("TRG_LEN :",false); debugPrint(TRG_LEN);
            debugPrint("DIG_LEN :",false);  debugPrint(DIG_LEN);
        }
        // validate the Trigger paraemters 
        char* type[] = { "TID","FID1","FID2","OPTR","COMP","TRH","MAND"};   // TODO : Need to Automate this
        uint8_t temp;
        if (DIG_LEN != TRG_LEN)
        {   
            validTriggerSize = false;
            if(loopDebugMode && DEBUG_ENABLE_FLAG){
                debugPrint("Mismatched size of TRG and DIG configs");
            }    
            return validTriggerSize;
        }
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
                    if (loopDebugMode && DEBUG_ENABLE_FLAG)
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



/**
 * @brief Diagnostic fault Notifier methods
 * 
 */
IUDiagnosticNotifier::IUDiagnosticNotifier(/* args */)
{
}

IUDiagnosticNotifier::~IUDiagnosticNotifier()
{
}

const char* IUDiagnosticNotifier::getDiagnosticName(uint8_t cId){
    
    if(cId == 0){
       digName =  popFromStack(STACK::ACTIVE_DIG);
    }else{
           digName =  popFromStack(STACK::REPORTABLE_DIG);
    }
    if(loopDebugMode){
        debugPrint("RDIG NAME :", false);
        debugPrint(digName);
    }   
    return digName;
}