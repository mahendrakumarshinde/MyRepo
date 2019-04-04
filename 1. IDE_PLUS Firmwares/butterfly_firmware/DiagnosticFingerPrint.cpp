#include "DiagnosticFingerPrint.h"
#include "IUESP8285.h"


extern bool sync_fingerprint_lock;
extern const char* fingerprintData ;
const char* iuFingerprintOutput;//[500];

/*
 * Checks weather diagnostic configuration data present in file/memory 
 * return True - available else False
 *      
 * 
 */


bool DiagnosticEngine::isDiagnosticConfigAvailable() {


    if( true ){

      m_configAvailable = true;
      
      return m_configAvailable;
    }
    else {            // Diagnostic Configuration not present 

      return m_configAvailable;
    }
    
}


/*
 * Function Name : setDiagnosticFingerPrint
 * @conditionCode - Represent the feature codes like velocity /Acceleration along the selected axis for n-diagnostic feature.
 * @parameterId - each Velocity and accelration axis assigned the ids in the received message.
 * @speed - combination of speed X constant (constant as per feature/diagnostic finger print configured
 * @band - allowable band to compute the data in Step function.
 * @ amplitudes - type q15 
 * return bool on sucess
 * 
 */

void DiagnosticEngine :: setDiagnosticFingerPrint( char* conditionCode,uint8_t parameterId, float speed, uint8_t band,size_t sampleCount, q15_t *amplitudes  ){


      // ConditionCode - FV<featureNo>  --> Apply on Velocity  (m/s) 
      //               - FA<featureNo>  --> Apply on Acceleration (m/s2)

     /* Serial.print("Pre-processing DIagnostic Finger Print  :\n");
      Serial.print("Condition Code :");Serial.println(conditionCode);//Serial.print("\t\n");
      Serial.print("Parameter ID :");Serial.println(parameterId);//Serial.print("\t\n");
      Serial.print("Speed :"); Serial.println(speed);
      Serial.print("Band :");Serial.println(band);
      Serial.print("Amplitude Count :");Serial.println(sampleCount);
      
      Serial.print("[");
      for (int i =0 ;i<sampleCount; ++i) {

        Serial.print( amplitudes[i] );Serial.print(",");     // converting q15 to float 
      }
      Serial.println("]");
  
  Serial.println("Data Pre-processing Done ...");
  */
}

void DiagnosticEngine :: setDiagnosticFingerPrint( char* conditionCode,uint8_t parameterId, float speed, uint8_t band  ){


      // ConditionCode - FV<featureNo>  --> Apply on Velocity  (m/s) 
      //               - FA<featureNo>  --> Apply on Acceleration (m/s2)

   /*   Serial.print("Received Diagnostic Config MSG :");
      Serial.print(conditionCode);
      Serial.print("\t");
      Serial.print(parameterId);
      Serial.print("\t");
      Serial.print(speed);Serial.print("\t\t\t");
      Serial.println(band);

     */ 
    

  
}



float* DiagnosticEngine:: readCurrentDiagnosticFingerPrintData(bool m_mode,float idx, float idy, float idz){
  
}

//void DiagnosticEngine::configureAllfingerprints(JsonVariant &config){
//
//  
//}

//
//char* DiagnosticEngine::getDiagnosticFingerPrint(){
//
//  
// }


/*
 * configureFingerPrintsFromFlash(char* filename,bool isSet)
 * 
 * 
 */
JsonObject& DiagnosticEngine::configureFingerPrintsFromFlash(String filename,bool isSet){

  if(isSet != true){

    //return false;
  }
  
 // Open the configuration file
 
  File myFile = DOSFS.open(filename,"r");
  
  //StaticJsonBuffer<1024> jsonBuffer;
  //const size_t bufferSize = JSON_ARRAY_SIZE(8) + JSON_OBJECT_SIZE(300) + 60;        // dynamically allociated memory
  const size_t bufferSize = JSON_OBJECT_SIZE(1) + 50*JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(50) + 2430;
  
  DynamicJsonBuffer jsonBuffer(bufferSize);
 // Serial.print("JSON 2 SIZE :");Serial.println(bufferSize);
  // Parse the root object
  JsonObject &root = jsonBuffer.parseObject(myFile);
  //JsonObject& root2 = root["fingerprints"];
  
  if (!root.success()){
    // debugPrint(F("Failed to read fingerprints.conf file, using default configuration"));
   
  }
 else {
  // close file
 // Serial.println("Closing the fingerprints.conf file.....");
  myFile.close();

 }
    
 return root;     // JSON Object
}

/*
 * get Lower Cutoff
 */

float DiagnosticEngine::lowerCutoff(float m_speedMultiplier,int m_bandValue,float m_freqResolution){
  
   m_speedMultiplier = m_speedMultiplier/m_freqResolution;                         // divid by frequency resolution
    
  return  m_speedMultiplier - m_speedMultiplier*m_bandValue/100.0;
  
 }

 /*
 * get Higher Cutoff
 */

float DiagnosticEngine::higherCutoff(float m_speedMultiplier,int m_bandValue,float m_freqResolution){
  
  m_speedMultiplier = m_speedMultiplier/m_freqResolution;                         // divid by frequency resolution
    
  return  m_speedMultiplier + m_speedMultiplier*m_bandValue/100.0;
  
 }
/*
 * m_specializedCompute - Diagnostic Fingerprints 
 * 
 * return type - q15_t
 */
q15_t DiagnosticEngine::m_specializedCompute(int m_parameterId, float m_speedMultiplier ,uint8_t m_bandValue,q15_t *m_amplitudes){

    //FFT amplitudeCount = 257
    // sampleingRate = 1660Hz
    // SampleCount = 1600
    
    q15_t addition = 0;

    
       
    int lth = lowerCutoff(m_speedMultiplier,m_bandValue,m_freqResolution);   // get lower cutoff in frequency domain
    int hth = higherCutoff(  m_speedMultiplier,m_bandValue,m_freqResolution); // get higher cutoff in frequency domain 
    
    if(lth<= 2 )    // lower Cutoff frequency
    {
      lth = 2;
    }
    if(hth >=257){
      hth =257;   //higher bin index i.e 800Hz
    }
    
  /*  Serial.print("SpeedXMult :");Serial.println(m_speedMultiplier);
    Serial.print("BandValue :"); Serial.println( m_bandValue);
    Serial.print("Frequency Resolution:");Serial.println(m_freqResolution);
    Serial.print("Lower Threshold :");Serial.println(lth);
    Serial.print("Higher Threshold :");Serial.println(hth);
  */
    m_speedMultiplier = m_speedMultiplier/m_freqResolution;

  //  Serial.print("Frquency :");Serial.println(m_speedMultiplier);
    q15_t sum=0;
    
  if(m_parameterId == 0 || m_parameterId == 1 || m_parameterId == 2 )  {       //X-axis
    
   //  Serial.print("[");
     
     for(int i= lth; i<= hth; i++){     // sum all the amplitudes between lower to higher thresholds
     
      addition = m_amplitudes[i];
      sum = sum + addition;  
    //  Serial.print(m_amplitudes[i]);Serial.print(","); 
    }
    
   // Serial.println("]");
  //   Serial.print("Band Sumation:");Serial.println(addition);
  //   Serial.print("SUM :");Serial.println(sum);
  //   Serial.print("Value Count :");Serial.println(hth-lth);
  }
  
   return sum;
}

#include "Conductor.h"
extern Conductor conductor;

bool sync_fingerprint_lock = false; // global variable for locking the fingerprintData buffer
const char* DiagnosticEngine::m_specializedCompute (int m_direction, float *m_amplitudes,float m_resolution)
{
    // acquire the sync_fingerprint_lock
    sync_fingerprint_lock = false;
    Serial.println("Acquired the lock for computing fingerprints");

    // read all the available keys and there values
     const char* fingerprintsIdBuffer_X[13]; 
     const char* fingerprintsIdBuffer_Y[13]; 
     const char* fingerprintsIdBuffer_Z[13]; 
     const char* TempFingerprintData ;
     
     static int idCount = 0; 
     static const char* value_X;
     static const char* value_Y;
     static const char* value_Z;
     //char iuFingerprintOutput[500]; 
       
     static float sxm = 0 ;   
     static int bandValue = 0;
     int i = 0;     
     bool dirFlag_X = false,dirFlag_Y = false,dirFlag_Z = false;
     float sum = 0;
     float addition = 0;
     
     //q15_t sum=0;
     //q15_t addition = 0;
     static float finngerPrint_result[13]; 
     int XCount = 0; int YCount=0; int ZCount = 0; int KeyCount = 0 ;
      
    JsonObject& config = configureFingerPrintsFromFlash("finterprints.conf",true);
    
    JsonObject& root2 = config["fingerprints"];
    
    //String messageId = config["messageId"];
   // Serial.println();
    
    //Serial.print("messageId:");Serial.println(messageId);
    
    for (auto fingerprintsKeyValues : root2) {              // main for loop
        
        
     JsonObject &root3 = root2[fingerprintsKeyValues.key];       // get the nested Keys
    
     //fingerprintsIdBuffer[i] = fingerprintsKeyValues.key;
         
     if(m_direction == 0 && root3["dir"] =="VX" || root3["dir"] == "AX"){                             // seperate all the directions 
          // VX
          Serial.println("CondX");
         float  speedX = root3["speed"]; 
         float multiplierX = root3["mult"];
         float bandX = root3["band"];
         float frequencyX = root3["freq"];

       //Serial.println(fingerprintsKeyValues.key); 
         fingerprintsIdBuffer_X[i] = fingerprintsKeyValues.key;
        
         
        // Serial.print(speedX);Serial.print(",");Serial.print(multiplierX);Serial.print(";");Serial.print(bandX);Serial.print(",");Serial.print(frequencyX);Serial.println();
        // check frequency is present in the json or not 
        if(root3.containsKey("freq") && root3["freq"] != 0) {
                                              // use frequency and band for computation , do not use speed X multiplier
        
         sxm =  frequencyX ;
         bandValue = bandX;
        }
        else {

          sxm = speedX * multiplierX;
          bandValue = bandX;
        }
      //  Serial.println(" Direction X-Axis ......");
      //  Serial.println("Only X .......");
        XCount++;
        KeyCount++;
        dirFlag_X = true;        
       }
       if(m_direction == 1 && root3["dir"] =="VY" || root3["dir"] == "AY"){                             // seperate all the directions 
          // VY
          Serial.println("CondY");
         float  speedY = root3["speed"]; 
         float multiplierY = root3["mult"];
         float bandY = root3["band"];
         float frequencyY = root3["freq"];

      //   Serial.println(fingerprintsKeyValues.key); 
         fingerprintsIdBuffer_Y[i] = fingerprintsKeyValues.key;

         
         
       // Serial.print(speedY);Serial.print(",");Serial.print(multiplierY);Serial.print(";");Serial.print(bandY);Serial.print(",");Serial.print(frequencyY);Serial.println();
        
        if(root3.containsKey("freq") && root3["freq"] != 0) {
                                              // use frequency and band for computation , do not use speed X multiplier
         sxm =  frequencyY ;
         bandValue = bandY;
        }
        else {

          sxm = speedY * multiplierY;
          bandValue = bandY;
        }
       // Serial.println(" Direction Y-Axis ......");
       // Serial.println("Only Y .......");
        YCount++;
        KeyCount++;
        dirFlag_Y = true;
       }
       if(m_direction == 2 && root3["dir"] =="VZ"  || root3["dir"] == "AZ"){                             // seperate all the directions 
          // VZ
            Serial.println("CondZ");
         float  speedZ     =  root3["speed"]; 
         float multiplierZ =  root3["mult"];
         float bandZ       =  root3["band"];
         float frequencyZ  =  root3["freq"];
          
      //   Serial.println(fingerprintsKeyValues.key); 
         fingerprintsIdBuffer_Z[i] = fingerprintsKeyValues.key;
         
       // Serial.print(speedZ);Serial.print(",");Serial.print(multiplierZ);Serial.print(";");Serial.print(bandZ);Serial.print(",");Serial.print(frequencyZ);Serial.println();
        
        if(root3.containsKey("freq") && root3["freq"] != 0) {
                                              // use frequency and band for computation , do not use speed X multiplier
        
         sxm =  frequencyZ ;
         bandValue = bandZ;
         
        }
        else {

          sxm = speedZ * multiplierZ;
          bandValue = bandZ;
        }
       // Serial.println(" Direction Z-Axis ......");
       //  Serial.println("Only Z .......");
        ZCount++;
        KeyCount++;
        dirFlag_Z = true;
       }

      // computation stage

      int lth = lowerCutoff(sxm,bandValue,m_freqResolution);   // get lower cutoff in frequency domain
      int hth = higherCutoff(  sxm,bandValue,m_freqResolution); // get higher cutoff in frequency domain 
      //Serial.print("Lower Threshold :");Serial.println(lth);
      //Serial.print("Higher Threshold :");Serial.println(hth);
      
      m_speedMultiplier = sxm/m_freqResolution;

     // Serial.print("Frequency Index :");Serial.println(m_speedMultiplier);
      
      if(lth<= 1 )    // lower Cutoff frequency
      {
        lth = 1;
      }
      if(hth >=257){      // maximum spectral lines
        hth =257;         //higher bin index i.e 800Hz
      }
      
      if(( (root3["dir"] == "VX" || root3["dir"] == "AX") && dirFlag_X == true )|| ( (root3["dir"] == "VY" || root3["dir"] == "AY" ) && dirFlag_Y == true ) ||
            ( (root3["dir"] == "VZ" || root3["dir"] == "AZ") && dirFlag_Z == true) ){
      //   Serial.println("Inside Computation Loop ......");
         
       //  Serial.print("[");
     
         for(int i= lth; i<= hth; i++){     // sum all the amplitudes between lower to higher thresholds
         
          addition = m_amplitudes[i];
          sum += addition;  
       //   Serial.print(m_amplitudes[i]);Serial.print(","); 
        }
       // Serial.println("]");
        
      }

      
     //Serial.print("speedXMultiplier or Frequency Value :");Serial.print(sxm);Serial.print(";");Serial.print(bandValue);Serial.print(",");Serial.println();
      
     
     finngerPrint_result[i] =  sum ;//((float(sum)/ 128)*0.001197*256/1.414);// /m_resolution;//*m_resolution = 0.001197; //m_fftLength*m_scalingFactor;    // *0.244; // sensor resolution 
    // Serial.print("SUM :");Serial.println(sum);     
      dirFlag_X = false;
      dirFlag_Y = false;
      dirFlag_Z = false; 
      i++;    // next fId 
      
       
      sum = 0;   
    }
  
   static const char* tempData;
   
   //Serial.print("m_resolution :");Serial.println(m_resolution);
   tempData = generateJSONPayload(root2,finngerPrint_result);
   const int messageSize = sizeof(tempData) + 1024;
   
   static char tempX[messageSize];
   static char tempY[messageSize];
   static char tempZ[messageSize];  // need to set dynamically
  
  //  Serial.print("direction :");Serial.println(m_direction);
     
   if(m_direction == 0){
    value_X = tempData;//fingerprintData;
    Serial.print("value_X: "); Serial.println(tempData);
    
    memmove(tempX,tempData,strlen(tempData));
   }
   if(m_direction == 1){
    value_Y = tempData ;//fingerprintDm_directionata;
    Serial.print("value_Y: "); Serial.println(tempData);
  
    memmove(tempY,tempData,strlen(tempData));
   }
   if(m_direction == 2){  
    value_Z = tempData; //fingerprintData;
    Serial.print("value_Z: "); Serial.println(tempData);

    memmove(tempZ,tempData,strlen(tempData));

    // merg the 3 json strings to json objects
   DynamicJsonBuffer jBuffer;
   JsonObject& object1 =jBuffer.parseObject(tempX);
   JsonObject& object2 =jBuffer.parseObject(tempY);
   JsonObject& object3 =jBuffer.parseObject(tempZ);

   mergeJOSN( object1,object2);
   iuFingerprintOutput = mergeJOSN( object1,object3);   
   Serial.print("Output JSON :");Serial.println(iuFingerprintOutput);
  
   sync_fingerprint_lock = true; 
   Serial.println("Released lock after computing fingerprints");

   memset(tempX, 0, sizeof(tempX)); // flush the buffers
   memset(tempY, 0, sizeof(tempY));
   memset(tempZ, 0, sizeof(tempZ));

   }

     // TODO : put this inside the if condition for direction == 2 ?
  fingerprintData = iuFingerprintOutput;     // fingerprints result
  Serial.print("fingerprintData"); Serial.println(iuFingerprintOutput);
  conductor.send_diagnostic_fingerprints(); // function checks if all computation is done with the sync_fingerprint_lock, and sends only if time diff is > 512


   //Serial.print("Temp X Flush :");Serial.println(tempX);
  
   //Serial.print("DATA :");
   //Serial.println(fingerprintData);
   // Serial.println("****************************DONE ****************");

   return fingerprintData;  //TempFingerprintData 
  
}


/*
 * get the keys from diagnostic configuration json
 * 
 */

const char* DiagnosticEngine::getFingerprintKeys(JsonObject &config){

  const char* keys;
  int i = 0;
  for (auto fingerprintsKeyValues : config) {
    debugPrint(fingerprintsKeyValues.key);
    //Serial.println(fingerprintsKeyValues.value.as<float>());
    keys = fingerprintsKeyValues.key; //.c_str();
    keys++;
    i++;
  }

 
return keys;
}


/*
 *  send diagnostic computation resilts to pub-sub
 *   
 */
void DiagnosticEngine::publishFingerprintComputation(float* X_fingerprintResult,float* Y_fingerprintResult,float* Z_fingerprintResult){


 //sendMSPCommand(MSPCommand::SEND_DIAGNOSTIC_RESULTS, X_fingerprintResult);     // send mspCommand to publish data

}


/*
 * generate the JSON object 
 * 
 */

const char* DiagnosticEngine::generateJSONPayload(JsonObject &config, float* values){

   DynamicJsonBuffer  jsonBuffer(2000);
    
   JsonObject& root = jsonBuffer.createObject();
   JsonObject& fingerprintJson = jsonBuffer.createObject();
     
   for (auto fingerprintsKeyValues : config) {
    //Serial.println(fingerprintsKeyValues.key);
    if(*values == 0){
      //pass;
    }else {
      root[fingerprintsKeyValues.key] = *values;
    }
    values++;
  }
   // char jsonChar[500];
    //root.prettyPrintTo(Serial);
  //  root.printTo(jsonChar);
  return mergeJOSN(fingerprintJson,root);

//return jsonChar;
}

/*
 * merg JSON
 * 
 */
const char* DiagnosticEngine::mergeJOSN(JsonObject& dest, JsonObject& src) {
   for (auto kvp : src) {
     dest[kvp.key] = kvp.value;
     
   }
static char json[512];
dest.printTo(json);
  
   return json;
}

//JsonObject& nestedObject = jsonObjectRoot.createObject("SetOfObjects");
//merge(nestedObject, );
//merge(nestedObject, jsonObjectTwo);
//}
