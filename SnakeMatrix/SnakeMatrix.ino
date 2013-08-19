/***************************************************
  Arduino SNAKE
  -------------
  forked from:
  https://github.com/TKJElectronics/LEDStringMatrix
 ****************************************************/

// You can use any (4 or) 5 pins
#define sclk 13
#define mosi 11
#define cs   10
#define dc   8
#define rst  0  // you can also connect this to the Arduino reset

// WS2801
uint8_t dataPin  = 2;    
uint8_t clockPin = 3;

// push buttons
uint8_t upBtnPin = 4;
uint8_t dwnBtnPin = 5;
uint8_t lftBtnPin = 6;
uint8_t rgtBtnPin = 7;


#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library

#include <SPI.h>
#include <Adafruit_WS2801.h>

// Option 1: use any pins but a little slower
//Adafruit_ST7735 tft = Adafruit_ST7735(cs, dc, mosi, sclk, rst);

// Option 2: must use the hardware SPI pins
// (for UNO thats sclk = 13 and sid = 11) and pin 10 must be
// an output. This is much faster - also required if you want
// to use the microSD card (see the image drawing example)
Adafruit_ST7735 tft = Adafruit_ST7735(cs, dc, rst);

Adafruit_WS2801 strip = Adafruit_WS2801((uint16_t)7, (uint16_t)14, dataPin, clockPin);


#define BUTTON_NONE 0
#define BUTTON_DOWN 1
#define BUTTON_RIGHT 2
#define BUTTON_SELECT 3
#define BUTTON_UP 4
#define BUTTON_LEFT 5

#define USE_ST7735 false


byte oldSnakeItems = 0;
byte oldSnakeItemPosX[98];
byte oldSnakeItemPosY[98];
byte SnakeItems = 0;
byte SnakeItemPosX[98];
byte SnakeItemPosY[98];
byte ApplePosX;
byte ApplePosY;
byte AppleMoveCountDown;
#define APPLE_COUNTDOWN_STEPS_MIN 8
#define APPLE_COUNTDOWN_STEPS_MAX 15

#define BaneGridXmax 7 // 
#define BaneGridYmax 14 // 
#define BLANK 0
#define SNAKE 1
#define APPLE 2
byte Playfield[BaneGridXmax+1][BaneGridYmax+1];


byte SnakeHeadID = 0; // Array ID of the head
byte SnakeBackID = 0; // Array ID of the tail

byte AppleCount = 0;

#define SNAKE_LEFT 0
#define SNAKE_RIGHT 1
#define SNAKE_UP 2
#define SNAKE_DOWN 3
byte movingDirection = SNAKE_RIGHT; // Vores nuværende retning
byte snakeDirection = SNAKE_RIGHT; // Piletasternes/vores næste retning

byte AddSnakeItem = 0; // Tilføj et snake item næste gang vi flytter os
byte SnakeItemsToAddAtApple = 1; // Hvor mange Snake objekter der skal tilføjes når der spises et æble

byte Score = 0;
boolean GameRunning = false;

long timeBefore = millis();

void setup(void) {
  Serial.begin(9600);
  Serial.print("Setup ");

  if (USE_ST7735) {
    Serial.println("using Adafruit ST7735 TFT screen");

    setupST7735();
  }
  else {
    Serial.println("using LEDs");

     setupLED();
  }
}

void setupST7735(void){
  
  tft.initR(INITR_REDTAB);   // initialize a ST7735R chip, red tab
  tft.fillScreen(ST7735_BLACK);

}

void setupLED(){

  strip.begin();
  // Update LED contents, to start they are all 'off'
  strip.show();  
  //NewGame();
}

void loop() {
  if (USE_ST7735) {
    loopST7735();
  }
  else {
    loopLED();
  }
}

void loopST7735() {
  
  tft.invertDisplay(true);
  delay(1000);
  tft.invertDisplay(false);
  delay(1000);
}

void loopLED() {
    if ((millis() >= (timeBefore+300)) && (GameRunning == true)) {
      moveSnake();
      timeBefore = millis();
    }
    
    if(digitalRead(upBtnPin) == LOW){
      Serial.println("UP");
      snakeDirection = SNAKE_UP;
      if(!GameRunning) NewGame();
    }
    if(digitalRead(dwnBtnPin) == LOW){
      Serial.println("DOWN");
      snakeDirection = SNAKE_DOWN;
      if(!GameRunning) NewGame();
    }
    if(digitalRead(lftBtnPin) == LOW){
      Serial.println("LEFT");
      snakeDirection = SNAKE_LEFT;
      if(!GameRunning) NewGame();
    }
    if(digitalRead(rgtBtnPin) == LOW){
      Serial.println("RIGHT");
      snakeDirection = SNAKE_RIGHT;
      if(!GameRunning) NewGame();
    }
    
    if(!GameRunning)
      rainbowCycle(1);    
    
}

void rainbowCycle(uint8_t wait) {
  int i, j;
  boolean play = true;

  for (j=0; j < 256 * 5; j++) {     // 5 cycles of all 25 colors in the wheel
    for (i=0; i < strip.numPixels(); i++) {
      // tricky math! we use each pixel as a fraction of the full 96-color wheel
      // (thats the i / strip.numPixels() part)
      // Then add in j which makes the colors go around per pixel
      // the % 96 is to make the wheel cycle around
      strip.setPixelColor(i, Wheel( ((i * 256 / strip.numPixels()) + j) % 256) );
      if(digitalRead(upBtnPin) == LOW || digitalRead(dwnBtnPin) == LOW || digitalRead(lftBtnPin) == LOW ||digitalRead(rgtBtnPin) == LOW){
        play = false;
        break;
      }
    }  
    if(!play) break;
    strip.show();   // write all the pixels out
    delay(wait);
  }
}

uint32_t Color(byte r, byte g, byte b)
{
  uint32_t c;
  c = r;
  c <<= 8;
  c |= g;
  c <<= 8;
  c |= b;
  return c;
}

//Input a value 0 to 255 to get a color value.
//The colours are a transition r - g -b - back to r
uint32_t Wheel(byte WheelPos)
{
  if (WheelPos < 85) {
   return Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if (WheelPos < 170) {
   WheelPos -= 85;
   return Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170; 
   return Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

uint8_t readButtonST7735(void) {
  float a = analogRead(3);
  
  a *= 5.0;
  a /= 1024.0;
  
  Serial.print("Button read analog = ");
  Serial.println(a);
  if (a < 0.2) return BUTTON_DOWN;
  if (a < 1.0) return BUTTON_RIGHT;
  if (a < 1.5) return BUTTON_SELECT;
  if (a < 2.0) return BUTTON_UP;
  if (a < 3.2) return BUTTON_LEFT;
  else return BUTTON_NONE;
}

void NewGame(void)
{
  byte x, y;
  
  SnakeItems = 1;
  SnakeHeadID = 1;
  SnakeItemPosX[1] = 1;
  SnakeItemPosY[1] = 1;
  movingDirection = SNAKE_DOWN;
  snakeDirection = SNAKE_DOWN;

  for (y = 1; y <= BaneGridYmax; y++)
  {
    for (x = 1; x <= BaneGridXmax; x++)
    {
      Playfield[x][y] = BLANK;
    }
  }

  AddSnakeItem = 0;
  AppleCount = 0;
  placeRandomApple();
  
  GameRunning = true; 
  render(); 
  timeBefore = millis();
}

void moveSnake(void) {
  byte i;
  movingDirection = snakeDirection; // Sæt movingDirection til den retning vi har valgt med piletasterne

  if (AddSnakeItem == 0) { // Flyt det bagerste Snake Objekt til fronten, og sæt SnakeHeadID til dette objekts ID
    SnakeBackID = SnakeHeadID - 1;
    if (SnakeBackID == 0) SnakeBackID = SnakeItems;
    SnakeItemPosX[SnakeBackID] = SnakeItemPosX[SnakeHeadID];
    SnakeItemPosY[SnakeBackID] = SnakeItemPosY[SnakeHeadID];
    switch (movingDirection) {
      case SNAKE_RIGHT:
        SnakeItemPosX[SnakeBackID] += 1;
	break;
      case SNAKE_LEFT:
	SnakeItemPosX[SnakeBackID] -= 1;
	break;
      case SNAKE_DOWN:
	SnakeItemPosY[SnakeBackID] += 1;
	break;
      case SNAKE_UP:
	SnakeItemPosY[SnakeBackID] -= 1;
	break;
    }			
    SnakeHeadID = SnakeBackID;
  } else { // Skal vi tilføje et Snake objekt (AddSnakeItem > 0), da skal vi tilføje én foran, UDEN at fjerne den bagved
    for (i = SnakeItems; i >= SnakeHeadID; i--) {
      SnakeItemPosX[i+1] = SnakeItemPosX[i];
      SnakeItemPosY[i+1] = SnakeItemPosY[i];
    }
    SnakeItemPosX[SnakeHeadID] = SnakeItemPosX[SnakeHeadID+1];
    SnakeItemPosY[SnakeHeadID] = SnakeItemPosY[SnakeHeadID+1];
    switch (movingDirection) {
      case SNAKE_RIGHT:
        SnakeItemPosX[SnakeHeadID] += 1;
        break;
      case SNAKE_LEFT:
	SnakeItemPosX[SnakeHeadID] -= 1;
	break;
      case SNAKE_DOWN:
	SnakeItemPosY[SnakeHeadID] += 1;
	break;
      case SNAKE_UP:
	SnakeItemPosY[SnakeHeadID] -= 1;
	break;
    }	

    SnakeItems++;			
    AddSnakeItem--;
  }

    // Befinder vi os inden for banen?						
    if (SnakeItemPosX[SnakeHeadID] > 0 && SnakeItemPosX[SnakeHeadID] <= BaneGridXmax && SnakeItemPosY[SnakeHeadID] > 0 && SnakeItemPosY[SnakeHeadID] <= BaneGridYmax) {
      if (Playfield[SnakeItemPosX[SnakeHeadID]][SnakeItemPosY[SnakeHeadID]] != SNAKE) { // Er hovedets position på et blankt eller æble felt?
	if (Playfield[SnakeItemPosX[SnakeHeadID]][SnakeItemPosY[SnakeHeadID]] == APPLE) { // Er hovedets position på et æble felt
	  Score++;
	  AddSnakeItem += SnakeItemsToAddAtApple; // Tilføj x-antal snake items (bliver tilføjet til fronten af snake løbende)
	  AppleCount--; // Fjern et æble fra RAM'en
	}			
	if (AppleCount == 0) { // Hvis der ikke er flere æbler på banen
	  placeRandomApple(); // placer da et æble et tilfældigt sted
	}
	render(); // Render Snake objekterne i de rigtige felter
      } else { // Game over da vi ramte ind i os selv (snake felt)
	GameOver();				
      }
    } else { // Game over da vi ramte ind i kanten
      GameOver();
    }		
  }
  
void placeRandomApple(void) {
  byte x, y;
  x = random(1, BaneGridXmax);
  y = random(1, BaneGridYmax);
  while (Playfield[x][y] != BLANK) {
    x = random(1, BaneGridXmax);
    y = random(1, BaneGridYmax);  
  }
  placeApple(x, y);
}


void placeApple(byte x, byte y) {
  if (x > 0 && y > 0 && x <= BaneGridXmax && y <= BaneGridYmax) {
    Playfield[x][y] = APPLE;
    ApplePosX = x;
    ApplePosY = y;
    AppleMoveCountDown = random(APPLE_COUNTDOWN_STEPS_MIN, APPLE_COUNTDOWN_STEPS_MAX);

    AppleCount++;
  }
}

void removeApple(void) {
  Playfield[ApplePosX][ApplePosY] = BLANK;
  AppleCount = 0; 
}

void setXYpixel(char x, char y, unsigned char R, unsigned char G, unsigned char B)
{ 
  /*
  if ((y % 2) > 0)
    strip.setPixelColor(x, y, R, G, B);
  else
  */
    strip.setPixelColor(x, y, R, G, B);
}

void render(void) { // Render de forskellige snake Items
  byte i, x, y;
  
  for (i=1; i <= oldSnakeItems; i++) {
    if (oldSnakeItemPosX[i] > 0 && oldSnakeItemPosY[i] > 0 && oldSnakeItemPosX[i] <= BaneGridXmax && oldSnakeItemPosY[i] <= BaneGridYmax) {
      Playfield[oldSnakeItemPosX[i]][oldSnakeItemPosY[i]] = BLANK;
    }
  }

  for (i=1; i <= SnakeItems; i++) {
    if (SnakeItemPosX[i] > 0 && SnakeItemPosY[i] > 0 && SnakeItemPosX[i] <= BaneGridXmax && SnakeItemPosY[i] <= BaneGridYmax) {			
      Playfield[SnakeItemPosX[i]][SnakeItemPosY[i]] = SNAKE;
      oldSnakeItemPosX[i] = SnakeItemPosX[i];
      oldSnakeItemPosY[i] = SnakeItemPosY[i];			
    }
  }
  oldSnakeItems = SnakeItems;
  
  if (AppleCount > 0 && AppleMoveCountDown == 0) {
    removeApple();
    placeRandomApple();
  } else if (AppleCount > 0) {
    AppleMoveCountDown--;
  }
  
  for (y = 1; y <= BaneGridYmax; y++)
  {
    for (x = 1; x <= BaneGridXmax; x++)
    {
      switch (Playfield[x][y]) {
        case BLANK:       
          setXYpixel((x-1), (y-1), 0, 0, 0);
          break;       
        case SNAKE:
          if (SnakeItemPosX[SnakeHeadID] == x && SnakeItemPosY[SnakeHeadID] == y)
            setXYpixel((x-1), (y-1), 255, 255, 0); // Yellow snake head
          else
            setXYpixel((x-1), (y-1), 0, 255, 0); // Green snake body
          break;  
        case APPLE:
          setXYpixel((x-1), (y-1), 255, 0, 0);
          break;   
        default:   
          setXYpixel((x-1), (y-1), 0, 0, 0);
          break; 
      }          
    }
  }
  strip.show();
}


void GameOver(void) {
  if (SnakeItemPosX[SnakeHeadID] == 0) SnakeItemPosX[SnakeHeadID]++;
  else if (SnakeItemPosX[SnakeHeadID] > BaneGridXmax) SnakeItemPosX[SnakeHeadID]--;
  if (SnakeItemPosY[SnakeHeadID] == 0) SnakeItemPosY[SnakeHeadID]++;
  else if (SnakeItemPosY[SnakeHeadID] > BaneGridYmax) SnakeItemPosY[SnakeHeadID]--;	
  
  if (SnakeHeadID < SnakeItems)	
    setXYpixel(SnakeItemPosX[SnakeHeadID+1]-1, SnakeItemPosY[SnakeHeadID+1]-1, 0, 255, 0); // Set second snake object to green (from yellow head color)
  else
    setXYpixel(SnakeItemPosX[1]-1, SnakeItemPosY[1]-1, 0, 255, 0); // Set second snake object to green (from yellow head color)
    
  setXYpixel(SnakeItemPosX[SnakeHeadID]-1, SnakeItemPosY[SnakeHeadID]-1, 255, 100, 0); // Dark orange if dead snake head
  strip.show();
  
  GameRunning = false;
}
