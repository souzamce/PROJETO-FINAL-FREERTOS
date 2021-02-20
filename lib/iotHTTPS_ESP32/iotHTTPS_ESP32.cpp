#include "iotHTTPS_ESP32.h"
/*
 * Construtor
 */
iot::iot(char* token_iot) {
   
    _token_iot = token_iot;
	
    maxValues = 5; 
    currentValue = 0;
    val = (Value *)malloc(maxValues*sizeof(Value));

}


void iot::add(char *variable_id, float value) {
    return add(variable_id, value, NULL, NULL);
}

void iot::add(char *variable_id, float value, char *ctext) {
    return add(variable_id, value, ctext, NULL);   
}

void iot::add(char *variable_id, float value, unsigned long timestamp) {
    return add(variable_id, value, NULL, timestamp);
}

void iot::add(char *variable_id, float value, char *ctext, unsigned long timestamp) {
    (val+currentValue)->id = variable_id;
    (val+currentValue)->value_id = value;
    (val+currentValue)->context = ctext;
    (val+currentValue)->timestamp = timestamp;
    currentValue++;
    if (currentValue>maxValues) {
        Serial.println(F("Max 5 consecutives variables."));
        currentValue = maxValues;
    }
}


bool iot::send() {
	
  bool retorno = false; 
  String buffer; 
  buffer.reserve(300);

  String str;
	str.reserve(50);
	
	if(currentValue == 0) {
	  return false; 
	}
	//[{"variable": "S00", "value":1240},{"variable": "S01", "value":45.67, "timestamp":1481929105,"description":"Maquina Ligada"}]
  buffer = "[";
  for (int i = 0; i < currentValue; ) {
      str = String(((val+i)->value_id), 4);
      buffer += "{\"variable\":\"";
      buffer += String((val + i)->id);
      buffer += "\",\"value\":";
      buffer += str;
		
			String time = "";
			if(String((val+i)->timestamp).length() >= 10)
			{
			  buffer += ",\"timestamp\"";
			  buffer += ":";
			  buffer += String((val+i)->timestamp);
			} 

      if(String((val+i)->context).length() >= 1)
      {
        buffer += ",\"description\"";
        buffer += ":\"";
        buffer += String((val+i)->context);
        buffer += "\"";
      } 

      buffer += "}";
      i++;
      if (i < currentValue) {
          buffer += ","; 
      }
    }
    buffer += "]";

	
	  currentValue = 0;
	
    if( (wifiMulti.run() == WL_CONNECTED) ) {

      String bdata = ""; 
      bdata += HTTPSERVER;
      bdata += "/things/services/api/v1/collections/value/?token=";
      bdata += _token_iot; 
 
      HTTPClient http;
      http.begin( bdata, ca ); 
      //http.begin( bdata );   //Sem o certificado SSL
                               //Retirar o 'S' do HTTP(S) da URL do domínio.
      http.addHeader( "Content-Type", "application/json" );
      http.addHeader( "Connection", "close" ); 

      int httpCode = http.POST( buffer );

      if( httpCode > 0 ) {

          if( DEBUG )
            vPrintStringAndInteger("HTTP_CODE_RETURN:", httpCode);

          if( httpCode == HTTP_CODE_OK ) {  //Code 200
                String payload = http.getString();

                if( DEBUG )
                   vPrintString( payload.c_str() );

                retorno = true; 
            }

      } else {

          if( DEBUG )
              vPrintTwoStrings( "[HTTP] POST.. falha, error: %s\n", http.errorToString(httpCode).c_str() );
      }

    http.end();
  }

	return retorno;	
}


bool iot::read( char * variable_id ) {
    bool retorno = false; 
    if( (wifiMulti.run() == WL_CONNECTED) ) {

      String bdata = ""; 
      bdata += HTTPSERVER;
      bdata += "/things/services/api/v1/variables/";
      bdata += variable_id; 
      bdata += "/value/?token=";
      bdata += _token_iot;
 
      if( DEBUG )
        vPrintString(bdata.c_str());

      HTTPClient http;
      http.begin( bdata, ca ); 
      //http.begin( bdata);

      http.addHeader( "Content-Type", "application/json" );
      http.addHeader( "Connection", "close" ); 

      int httpCode = http.GET(  );

      if( httpCode > 0 ) {

          if( DEBUG )
            vPrintStringAndInteger("HTTP_CODE_RETURN:", httpCode);

          if( httpCode == HTTP_CODE_OK ) { 
                String payload = http.getString();

                if( DEBUG )
                   vPrintString( payload.c_str() );

                  cJSON *resolution = NULL;
                  cJSON *resolutions = NULL;

                  cJSON *json = cJSON_Parse(payload.c_str());
                  if( json != NULL )
                  {
                   
                   resolutions = cJSON_GetObjectItem(json, "results");
                    #define cJSON_ArrayForEach(element, array) for(element = (array != NULL) ? (array)->child : NULL; element != NULL; element = element->next)
                    cJSON_ArrayForEach(resolution, resolutions)
                    {
                       cJSON *value = cJSON_GetObjectItem((cJSON*)resolution, "value");
                       cJSON *description = cJSON_GetObjectItem((cJSON*)resolution, "description");

                        if((description->valuestring != NULL)){

                          if( DEBUG )
                            vPrintString( (char*)description->valuestring );

                          sprintf( iotText, "%s",description->valuestring );
                        }

                          if( DEBUG )
                            vPrintStringAndFloat( (char*) "Value: ", value->valuedouble );

                          iotValue = value->valuedouble; 
                          retorno = true;

                    }
                   
                    
                  }

                  cJSON_Delete(json);


            }

      } else {

          if( DEBUG )
              vPrintTwoStrings( "[HTTP] GET.. falha, error: %s\n", http.errorToString(httpCode).c_str() );
      }

    http.end();
  }

  return retorno; 
}




bool iot::wifiConnection(char* ssid, char* pass) {
    wifiMulti.addAP(ssid, pass);

    if (wifiMulti.run() == WL_CONNECTED) {
       return true;
    } else {
        return false;
    }
}



void iot::vPrintString( const char *pcString ){
  taskENTER_CRITICAL( &myMutex );
  {
    Serial.println( (char*)pcString );
  }
  taskEXIT_CRITICAL( &myMutex );
}

void iot::vPrintStringAndFloat( const char *pcString, float ulValue ){
  taskENTER_CRITICAL( &myMutex );
  {
    char buffer [50]; 
    sprintf( buffer, "%s %.2f", pcString, ulValue ); 
    Serial.println( (char*)buffer );
  }
  taskEXIT_CRITICAL( &myMutex );
}

void iot::vPrintStringAndInteger( const char *pcString, uint32_t ulValue ){
  taskENTER_CRITICAL( &myMutex );
  {
    char buffer [50]; 
    sprintf( buffer, "%s %lu", pcString, ulValue );
    Serial.println( (char*)buffer );
  }
  taskEXIT_CRITICAL( &myMutex );
}

void iot::vPrintTwoStrings(const char *pcString1, const char *pcString2){
  taskENTER_CRITICAL( &myMutex  );
  {
    char buffer [50]; 
    sprintf(buffer, "%s %s", pcString1, pcString2);
    Serial.println( (char*)buffer );
  }
  taskEXIT_CRITICAL( &myMutex );    
}