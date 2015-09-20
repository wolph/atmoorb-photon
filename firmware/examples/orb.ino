/*
 * This is the basic example of the Atmo Orb client, everything can be
 * configured using the define statements at the top
 */

/*
 * BEGIN SETTINGS
 */

// Port and IP settings, note that the default is 239.15.18.2:49692
#define MULTICAST_PORT 49692
// Note! The IP should be COMMA separated
#define MULTICAST_IP 239, 15, 18, 2

// Orb ID, configurable from AtmoLight
#define ORB_ID 1

// Neopixel led settings
#define PIXEL_PIN D6
#define PIXEL_COUNT 24
#define PIXEL_TYPE WS2812B

// Led smoothing settings so you don't get epilepsy
#define SMOOTH_STEPS 50 // Steps to take for smoothing colors
#define SMOOTH_DELAY 4 // Delay between smoothing steps
#define SMOOTH_BLOCK 0 // Block incoming colors while smoothing

// Define your white color
#define RED_CORRECTION 255.
#define GREEN_CORRECTION 255.
#define BLUE_CORRECTION 255.

// Buffers
#define BUFFER_SIZE  3 + 3 * PIXEL_COUNT
#define TIMEOUT_MS   500

/*
 * END SETTINGS
 */

#include "atmoorb/atmoorb.h"
#define RED 0
#define GREEN 1
#define BLUE 2

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);
UDP client;
IPAddress multicastIP(239, 15, 18, 2);

unsigned long smoothMillis;
uint8_t buffer[BUFFER_SIZE];
byte nextColor[3];
byte prevColor[3];
byte currentColor[3];
byte smoothStep = SMOOTH_STEPS;

void setup(){
    // Init UDP
    client.begin(MULTICAST_PORT);
    
    // Join Orb multicast group
    client.joinMulticast(MULTICAST_IP);
    
    // Init leds
    strip.begin();
    strip.show();
}

void loop(){
    if(client.parsePacket() == BUFFER_SIZE){
        // Fill up the buffer from the UDP client
        client.read(buffer, BUFFER_SIZE);
        unsigned int i = 0;
        
        // Look for 0xC0FFEE
        if(buffer[i++] == 0xC0 && buffer[i++] == 0xFF && buffer[i++] == 0xEE){
            byte commandOptions = buffer[i++];
            byte orbId = buffer[i++];
            
            // Command option: 1 = force off | 2 = validate command by Orb ID
            if(commandOptions == 1){
                // Orb ID 0 = turn off all lights
                // Otherwise turn off selectively
                if(orbId == 0){
                    forceLedsOFF();
                }else if(orbId == ORB_ID){
                    forceLedsOFF();
                }
                return;
            }else if(commandOptions == 2 && rcvOrbID != orbID){
                return;
            }
            
            byte red =  buffer[i++];
            byte green =  buffer[i++];
            byte blue =  buffer[i++];
            setSmoothColor(red, green, blue);
        }
    }
    if (smoothStep < SMOOTH_STEPS && millis() >= (smoothMillis + (SMOOTH_DELAY * (smoothStep + 1)))){
        smoothColor();
    }
}

// Set color
void setColor(byte red, byte green, byte blue){
    for (byte i = 0; i < PIXEL_COUNT; i++){
        strip.setPixelColor(i, red, green, blue);
    }
    strip.show();
}

// Set a new color to smooth to
void setSmoothColor(byte red, byte green, byte blue){
    if (smoothStep == SMOOTH_STEPS || SMOOTH_BLOCK == 0){
        red = (red * RED_CORRECTION) / 255;
        green = (green * GREEN_CORRECTION) / 255;
        blue = (blue * BLUE_CORRECTION) / 255;
        
        if (nextColor[RED] == red && nextColor[GREEN] == green
                && nextColor[BLUE] == blue){
          return;
        }
        
        while(uint8_t i=0; i<3; i++)
            prevColor[i] = currentColor[i];
        
        nextColor[RED] = red;
        nextColor[GREEN] = green;
        nextColor[BLUE] = blue;
        
        smoothMillis = millis();
        smoothStep = 0;
    }
}

// Display one step to the next color
void smoothColor(){
    smoothStep++;
    while(uint8_t i=0; i<3; i++)
        currentColor[i] = prevColor[i] + (((nextColor[i] - prevColor[i]) * smoothStep) / SMOOTH_STEPS);
    
    setColor(currentColor[RED], currentColor[GREEN], currentColor[BLUE]);
}

// Force all leds OFF
void forceLedsOFF(){
    setColor(0, 0, 0);
    clearSmoothColors();
}

// Clear smooth color byte arrays
void clearSmoothColors(){
    memset(prevColor, 0, sizeof(prevColor));
    memset(currentColor, 0, sizeof(nextColor));
    memset(nextColor, 0, sizeof(nextColor));
}

