#include "DiagnosticFingerPrint.h"
#include "IUESP8285.h"
#include "Conductor.h"
#include "FFTConfiguration.h"

extern Conductor conductor;
extern const char* fingerprintData ;
const char* iuFingerprintOutput;//[500];

/*
 * Checks weather diagnostic configuration data present in file/memory 
 * return True - available else False
 *      
 * 
 */

int DiagnosticEngine::m_SampleingFrequency = FFTConfiguration::DEFAULT_SAMPLING_RATE;
int DiagnosticEngine::m_smapleSize = FFTConfiguration::DEFAULT_BLOCK_SIZE;
int DiagnosticEngine::m_fftLength = FFTConfiguration::DEFAULT_BLOCK_SIZE / 2;

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
  
  //DynamicJsonBuffer jsonBuffer(bufferSize);
   StaticJsonBuffer<bufferSize> jsonBuffer;
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
    
  return  round(m_speedMultiplier - m_speedMultiplier*m_bandValue/100.0);
  
 }

 /*
 * get Higher Cutoff
 */

float DiagnosticEngine::higherCutoff(float m_speedMultiplier,int m_bandValue,float m_freqResolution){
  
  m_speedMultiplier = m_speedMultiplier/m_freqResolution;                         // divid by frequency resolution
    
  return  round(m_speedMultiplier + m_speedMultiplier*m_bandValue/100.0);
  
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



char* DiagnosticEngine::m_specializedCompute (int m_direction, const q15_t *m_amplitudes,int amplitudeCount, float m_resolution, float scaling)
{
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
     //float sum = 0;
     //float addition = 0;
     float rmsFingerprints = 0;

     //q15_t sum=0;
     //q15_t addition = 0;
     static float finngerPrint_result[13]; 
     int XCount = 0; int YCount=0; int ZCount = 0; int KeyCount = 0 ;

    DynamicJsonBuffer jsonBuffer;  
    //JsonObject& config = configureFingerPrintsFromFlash("finterprints.conf",true);
    //Serial.print("DEBUG INFO : Availabel Fingerprints : ");Serial.println(conductor.availableFingerprints);

    JsonObject& config = jsonBuffer.parseObject(conductor.availableFingerprints);
    JsonObject& root2 = config["fingerprints"];
    

    //String messageId = config["messageId"];
   // Serial.println();
    
    //Serial.print("messageId:");Serial.println(messageId);
    
    for (auto fingerprintsKeyValues : root2) {              // main for loop
        
        
     JsonObject &root3 = root2[fingerprintsKeyValues.key];       // get the nested Keys
    
     //fingerprintsIdBuffer[i] = fingerprintsKeyValues.key;
         
     if(m_direction == 0 && root3["dir"] =="VX" || root3["dir"] == "AX"){                             // seperate all the directions 
          // VX
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
      if(hth >=(FFTConfiguration::currentBlockSize/2 + 1)){      // maximum spectral lines
        hth = (FFTConfiguration::currentBlockSize/2 + 1);         //higher bin index i.e 800Hz
      }
      
      if(( (root3["dir"] == "VX" || root3["dir"] == "AX") && dirFlag_X == true )|| ( (root3["dir"] == "VY" || root3["dir"] == "AY" ) && dirFlag_Y == true ) ||
            ( (root3["dir"] == "VZ" || root3["dir"] == "AZ") && dirFlag_Z == true) ){
      //   Serial.println("Inside Computation Loop ......");
         
        // Serial.print("[");
        // lower_bound and upper_bound are frequency bounds as per the entered fingeprint configuration
        float lower_bound = sxm - sxm * bandValue/100.0;  // lower frequency bound
        float upper_bound = sxm + sxm * bandValue/100.0;  // upper frequency bound
        if(lower_bound <= FFTConfiguration::currentLowCutOffFrequency )    
        {
          lower_bound = FFTConfiguration::currentLowCutOffFrequency;
        }
        if(upper_bound >= FFTConfiguration::currentHighCutOffFrequency) {      
          upper_bound = FFTConfiguration::currentHighCutOffFrequency;        
        }
     
        float df = (float)FFTConfiguration::currentSamplingRate / (float)FFTConfiguration::currentBlockSize;
        int lower_index = (int)(lower_bound/df + 1); // ceiling(lower_bound/df)
        int upper_index = (int)(upper_bound/df); // floor(upper_bound/df)
        // ideally we need to start from lower_index = ceiling(lower_bound/df) to upper_index = floor(upper_bound/df)
        // to ensure we are not losing out any sample in edge case (if lower_bound is a multiple of df),
        // we consider one extra index at lower and bound and select the value only if : lower_bound < df*i < upper_bound
        //float factor = (1000*1.414*m_resolution/scaling);
        // 1000 -> convert m/s to mm/s
        // 1.414 -> sqrt(2) for converting to rms
        // for (int i = lower_index-1; i <= upper_index+1; i++) {
        //     if((lower_bound <= df*i) && (df*i <= upper_bound)) {
        //         addition = (float)m_amplitudes[i] * factor;
        //         sum += addition;
        //     }
        //}
        rmsFingerprints = getRMS(m_amplitudes,lower_index,upper_index);
        //NOTE : Below Ternary conditional operator is used to avoid the empty fingerprint computation, when rms value is zero 
        //then JSON does not append the zero values in the final JSON merging due to having the similar keys.values with similar Keys and Zero value are ignored.
        // if computated rms is Zero then we are sending the factor value instead of Zero value. 
        rmsFingerprints = (rmsFingerprints!=0) ? rmsFingerprints: 1;  
      }

      
     //Serial.print("speedXMultiplier or Frequency Value :");Serial.print(sxm);Serial.print(";");Serial.print(bandValue);Serial.print(",");Serial.println();
      
     float factor = (1000*m_resolution/scaling);
     float result =  rmsFingerprints*factor ;
     finngerPrint_result[i] =  result ;//((float(sum)/ 128)*0.001197*256/1.414);// /m_resolution;//*m_resolution = 0.001197; //m_fftLength*m_scalingFactor;    // *0.244; // sensor resolution 
    
    // Serial.print("SUM :");Serial.println(sum);     
      dirFlag_X = false;
      dirFlag_Y = false;
      dirFlag_Z = false; 
      i++;    // next fId 
      
       
      //sum = 0;
      rmsFingerprints = 0;   
    }
  
   const char* tempData;
   
   //Serial.print("m_resolution :");Serial.println(m_resolution);
   tempData = generateJSONPayload(root2,finngerPrint_result);
   const int messageSize = sizeof(tempData) + 1024;
   
  char return_fingerprints[messageSize];
  memmove(return_fingerprints, tempData, strlen(tempData));
  DynamicJsonBuffer tempjBuffer;
  JsonObject& json = tempjBuffer.parseObject(return_fingerprints);
  char fingerprints_json[512];
  json.printTo(fingerprints_json);
   
   static char tempX[messageSize];
   static char tempY[messageSize];
   static char tempZ[messageSize];  // need to set dynamically
  
     
   if(m_direction == 0){
    value_X = tempData;//fingerprintData;
    memmove(tempX,tempData,strlen(tempData));
   // Serial.print("X Fingerprints :");Serial.println(tempX);
   }

   if(m_direction == 1){
    value_Y = tempData ;//fingerprintData;
    memmove(tempY,tempData,strlen(tempData));

   // Serial.print("Y Fingerprints :");Serial.println(tempY);
   }

   if(m_direction == 2){  
    value_Z = tempData; //fingerprintData;
    memmove(tempZ,tempData,strlen(tempData));
    
   // Serial.print("Z Fingerprints :");Serial.println(tempZ);
    // merg the 3 json strings to json objects
   DynamicJsonBuffer jBuffer;
   JsonObject& object1 =jBuffer.parseObject(tempX);
   JsonObject& object2 =jBuffer.parseObject(tempY);
   JsonObject& object3 =jBuffer.parseObject(tempZ);

   mergeJOSN( object1,object2);
   iuFingerprintOutput = mergeJOSN( object1,object3);  
 
   memset(tempX, 0, sizeof(tempX)); // flush the buffers
   memset(tempY, 0, sizeof(tempY));
   memset(tempZ, 0, sizeof(tempZ));

    fingerprintData = iuFingerprintOutput;     // fingerprints result
    //debugPrint(F("fingerprintData"), false); debugPrint(F(iuFingerprintOutput), true); 

    conductor.sendDiagnosticFingerPrints(); // function sends only if time diff is > 512
   }  

   
   //Serial.print("Final JSON :");Serial.println(fingerprintData);
  
   //Serial.print("DATA :");
   //Serial.println(fingerprintData);
   // Serial.println("****************************DONE ****************");

  // fingerprints_json holds the json string for all the fingerprints in X, Y or Z direction corresponding to direction passed : 0,1 or 2
  return fingerprints_json;
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
static char json[500];
dest.printTo(json);
  
   return json;
}

//JsonObject& nestedObject = jsonObjectRoot.createObject("SetOfObjects");
//merge(nestedObject, );
//merge(nestedObject, jsonObjectTwo);
//}

/**
 * Return the RMS from RFFT amplitudes
 */
float DiagnosticEngine::getRMS(const q15_t *amplitudes, uint16_t lower_index,
                             uint16_t upper_index)
{
    uint32_t rms = 0;
    // if (!removeDC)
    // {
    //     rms += (uint32_t) sq((int32_t) (amplitudes[0]));
    // }
    for (uint16_t i = lower_index; i <= upper_index ; ++i)
    {
        /* factor 2 because RFFT and we use only half (positive part) of freq
        spectrum */
        rms += 2 * (uint32_t) sq((int32_t) (amplitudes[i]));
    }
    return (float) sqrt(rms);
}