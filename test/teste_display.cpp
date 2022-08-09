#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
int a;
int b;
int c;
void setup() {
 a=1;
  b=6;
  c=2;
  Serial.begin(9600);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  
  }
}

void loop() {
  a=a+1;
  b=b+2;
  c=c+3;
  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
 display.println("Teste");
 display.println("");
 display.println("  ");
 display.print("");
 display.println("");
 display.println("  ");
  display.print(" ");
 display.print("");
 display.print(" ");
 display.display(); 
}