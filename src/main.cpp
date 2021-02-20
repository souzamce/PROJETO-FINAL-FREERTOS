
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <Wire.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h" 

#include "Arduino.h"
#include "WiFi.h"
#include "WiFiMulti.h"
#include "HTTPClient.h"

#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_system.h"
#include "esp_adc_cal.h"
#include <iotHTTPS_ESP32.h>

#include "SSD1306.h" 
SSD1306  display(0x3c, 21, 22);



#define N1 30



int value1=0;
int Pa1=0;

int filtered1;
int vals1[N1];

void prvSetupHardware( void );
void vTask1( void *pvParameters );
#define V_REF 1100  // ADC reference voltage

#define CORE_0 0 
#define CORE_1 1  //OU tskNO_AFFINITY 

QueueHandle_t xQueue_adc;

#define WIFISSID "MDSGF"       
#define PASSWORD "58913092"     

#define TOKEN_IOT  "e380c18614fea768c34867433a8f2730"  
#define DP1 "S00"
#define Alarme "S01"



iot client((char*)TOKEN_IOT);

const byte relay = 25;

void prvSetupHardware(){
  Serial.begin( 9600 ); 
  
loop:
  for(uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] Aguarde %d...\n", t);
    Serial.flush();
    delay(500);
  }

 if(!(client.wifiConnection((char*)WIFISSID, (char*)PASSWORD) ) ){
    goto loop;
  }

  pinMode(relay, OUTPUT);

  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_24);

}



void vTask1( void *pvParameters ){

  BaseType_t xStatus;
  int adc_result; 
  client.vPrintString( "Task1 Iniciando..." ); // Mensagem de Inicio
  
  for( ;; ){

//--------------------------------------------------------------------
    xStatus = xQueueReceive( xQueue_adc, &adc_result, 0);
    if( xStatus == pdPASS ){
          //Envia os valores das variáveis para o servidor iot
          client.add((char*)DP1, (float)Pa1);
         
          if(client.send()){
            client.vPrintString("-----------------------------------------------");
            client.vPrintString((char*)"Enviado com sucesso!"); 
          } 
    }
//--------------------------------------------------------------------      
    vTaskDelay( 200 / portTICK_PERIOD_MS );
    if(client.read((char*)DP1))
    {
      client.vPrintString("-----------------------------------------------");
      client.vPrintStringAndFloat((char*)"Press.Alarme: ", client.iotValue);

      if( (int)client.iotValue >= 24){
          digitalWrite(relay, LOW); 
          client.vPrintString("Rele Ligado");  
      } else {
          digitalWrite(relay, HIGH);   
          client.vPrintString("Rele Desligado");  
      }

    }

       vTaskDelay( 200 / portTICK_PERIOD_MS );
     if(client.read((char*)Alarme))
    {
      client.vPrintString("-----------------------------------------------");
      client.vPrintStringAndFloat((char*)"Emergencia: ", client.iotValue);

      if( (int)client.iotValue >= 1){
          digitalWrite(relay, LOW); 
          client.vPrintString("Rele Ligado");  
      }
    }

//--------------------------------------------------------------------
    vTaskDelay( 5000 / portTICK_PERIOD_MS );
  }
}


void vTask2( void *pvParameters ){



  BaseType_t xStatus;

  for( ;; ){

     #define V_REF 1100  // ADC reference voltage
    adc1_config_width(ADC_WIDTH_12Bit);
    
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_11db);

    esp_adc_cal_characteristics_t characteristics;
    esp_adc_cal_get_characteristics(V_REF, ADC_ATTEN_11db, ADC_WIDTH_12Bit, &characteristics);
    
    //Read ADC and obtain result in mV
   
    uint32_t value1 = adc1_to_voltage(ADC1_CHANNEL_3, &characteristics);
 
// ------------------------------------------------------------------------
    //Filtro média móvel SENSOR 1 = ADC1 CH0
    for(int i1 = N1 - 1; i1 > 0; i1--){
      vals1[i1] = vals1[i1-1];
    }
    vals1[0] = value1;

    long sum1 = 0;
    for(int i1=0; i1 < N1; i1++){
        sum1 = sum1 + vals1[i1];
   }

   filtered1 = sum1 / N1;

 Pa1=((1.8856 * filtered1) - 2643.5);

   if ((Pa1 > -20) && (Pa1 < 20))
  {
    Pa1=0; 
  }
  
// -------------------------------------------------------------------------

     
    xStatus = xQueueSend( xQueue_adc, &Pa1, 0);
    if( xStatus != pdPASS ){   
    }
   
// -------------------------------------------------------------------------

      display.clear();
      display.setFont(ArialMT_Plain_16);   
      display.drawString(0,0,"DP Estágio1"); 
      display.setFont(ArialMT_Plain_24);
      display.drawString(0,30, String(Pa1)+" Pa");
      display.display();
      
    vTaskDelay( 100 / portTICK_PERIOD_MS );

  }
  
}

void setup() {
  prvSetupHardware(); 

  xQueue_adc = xQueueCreate( 5, sizeof( int ) );
  if( xQueue_adc != NULL ){
    xTaskCreatePinnedToCore( vTask1, "Task 1", configMINIMAL_STACK_SIZE+10000, NULL, 1, NULL, CORE_0 );  
    xTaskCreatePinnedToCore( vTask2, "Task 2", configMINIMAL_STACK_SIZE+1000, NULL, 1, NULL, CORE_1 );   
  }
}

void loop() {  
  vTaskDelay( 100 / portTICK_PERIOD_MS );
}
