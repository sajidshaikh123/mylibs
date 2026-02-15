#include <Adafruit_NeoPixel.h>
#include "pindefinition.h"
#include "esp_task_wdt.h"

Adafruit_NeoPixel rgb_light(1,RGB_LED_PIN,NEO_GRB + NEO_KHZ800);

#define RED    0xFF0000
#define GREEN  0x00FF00
#define BLUE   0x0000FF
#define YELLOW 0xFFFF00
#define PINK   0xFF1088
#define ORANGE 0xE05800
#define WHITE  0xFFFFFF
#define BLACK  0x000000
#define PURPLE 0x800080

bool toggle = false;


void setpixel(uint32_t color){
    rgb_light.setPixelColor(0,color);
    rgb_light.show();
}
void pixelInit(){
    // rgb_light.setPin(RGB_LED_PIN);
    // rgb_light.updateLength(1);
    // rgb_light.updateType(NEO_BRG + NEO_KHZ800);
    // Watchdog is temporarily disabled during this function call
    // Do not call esp_task_wdt_reset() here
    rgb_light.begin(); 
    rgb_light.setBrightness(50);
    rgb_light.clear();
    // setpixel(GREEN);
    
}


void toggleLED(uint8_t conn_status){
    if(toggle ){
          toggle =!toggle;
          switch(conn_status){
              case 0: 
                  setpixel(RED);
                  break;
              case 1: 
                  setpixel(YELLOW);
                  break;
              case 2: 
                  setpixel(GREEN);
                  break;
          }
      }else{
          toggle =!toggle;
          setpixel(BLACK);
      }
}