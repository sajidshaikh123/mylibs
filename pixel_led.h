#include <Adafruit_NeoPixel.h>
#include "pindefinition.h"

Adafruit_NeoPixel rgb_light(1,RGB_LED_PIN,NEO_GRB + NEO_KHZ800);

#define RED    0xFF0000
#define GREEN  0x00FF00
#define BLUE   0x0000FF
#define YELLOW 0xFFFF00
#define PINK   0xFF1088
#define ORANGE 0xE05800
#define WHITE  0xFFFFFF
#define BLACK  0x000000


void setpixel(uint32_t color){
    rgb_light.setPixelColor(0,color);
    rgb_light.show();
}
void pixelInit(){
    // rgb_light.setPin(RGB_LED_PIN);
    // rgb_light.updateLength(1);
    // rgb_light.updateType(NEO_BRG + NEO_KHZ800);
    rgb_light.begin(); 
    rgb_light.setBrightness(50);
    rgb_light.clear();
    // setpixel(GREEN);
    
}