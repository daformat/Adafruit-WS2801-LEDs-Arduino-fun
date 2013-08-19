/***************************************************
  Arduino SNAKE
  -------------
  forked from:
  https://github.com/TKJElectronics/LEDStringMatrix
 ****************************************************/

// WS2801
uint8_t dataPin  = 2;    
uint8_t clockPin = 3;

// push buttons
uint8_t upBtnPin = 4;
uint8_t dwnBtnPin = 5;
uint8_t lftBtnPin = 6;
uint8_t rgtBtnPin = 7;

#include <SPI.h>
#include <Adafruit_WS2801.h>

#define BaneGridXmax 7 // Width of the LED matrix
#define BaneGridYmax 14 // Height of the LED matrix

Adafruit_WS2801 strip = Adafruit_WS2801((uint16_t)BaneGridXmax, (uint16_t)BaneGridYmax, dataPin, clockPin);

byte oldSnakeItems = 0;
byte oldSnakeItemPosX[BaneGridXmax*BaneGridYmax];
byte oldSnakeItemPosY[BaneGridXmax*BaneGridYmax];
byte SnakeItems = 0;
byte SnakeItemPosX[BaneGridXmax*BaneGridYmax];
byte SnakeItemPosY[BaneGridXmax*BaneGridYmax];
byte ApplePosX;
byte ApplePosY;
byte AppleMoveCountDown;
#define APPLE_COUNTDOWN_STEPS_MIN 8 // Minimum moves before an apple disappear
#define APPLE_COUNTDOWN_STEPS_MAX 15 // Max moves before an apple disappear

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
byte movingDirection = SNAKE_RIGHT; // Actual moving direction
byte snakeDirection = SNAKE_RIGHT; // Next moving direction (determined by the controller state)

byte AddSnakeItem = 0; // Do we need to grow the snake on next move ?
byte SnakeItemsToAddAtApple = 1; // How many blocks will be added when the snake eats an apple

int delayBetweenMoves = 100; // How much time between each move (milliseconds, less == speedier game)

byte Score = 0;
boolean GameRunning = false;
boolean SnakeDiesOnEdges = false; // Sets the behavior when touching an edge 

long timeBefore = millis();

void setup(void) {
  Serial.begin(9600);
  Serial.print("Setup ");
  Serial.println("using LEDs");
   setupLED();
}


void setupLED(){
  strip.begin();
  // Update LED contents, to start they are all 'off'
  strip.show();  
  //NewGame();
}

void loop() {
    loopLED();
}

void loopLED() {
    if ((millis() >= (timeBefore + delayBetweenMoves)) && (GameRunning == true)) {
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

// Convert a color from its 0-255 RGB code
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
  movingDirection = snakeDirection; // set moving direction to the direction given by controller

  if (AddSnakeItem == 0) { // Move the rear Snake Object to the front and set SnakeHeadID to this object's ID - Flyt det bagerste Snake Objekt til fronten, og sæt SnakeHeadID til dette objekts ID
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
  } else { // Should we add a Snake block (AddSnakeItem> 0), we must add it to the front, WITHOUT moving the rest
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

    // Are we within the game board						
    if (SnakeItemPosX[SnakeHeadID] > 0 && SnakeItemPosX[SnakeHeadID] <= BaneGridXmax && SnakeItemPosY[SnakeHeadID] > 0 && SnakeItemPosY[SnakeHeadID] <= BaneGridYmax) {
      if (Playfield[SnakeItemPosX[SnakeHeadID]][SnakeItemPosY[SnakeHeadID]] != SNAKE) { // Is the head position on a blank or apple block?
	if (Playfield[SnakeItemPosX[SnakeHeadID]][SnakeItemPosY[SnakeHeadID]] == APPLE) { // Is the head position on an apple box
	  Score++;
	  AddSnakeItem += SnakeItemsToAddAtApple; // Add x-number of snake items (being added to the front of the snake continuously)
	  AppleCount--; //Remove an apple from the RAM
	}			
	if (AppleCount == 0) { // If there's no more apple
	  placeRandomApple(); // place an apple a random place
	}
	render(); // Render Snake objects
      } else { // Game over when we hit ourselves (snake field)
	GameOver();				
      }
    } else { // Game over when we hit an edge (Should be changed to wrap at the opposite edge)
      if ( SnakeDiesOnEdges )
        GameOver();
      else if(SnakeItemPosX[SnakeHeadID] > BaneGridXmax)
        SnakeItemPosX[SnakeHeadID] = 1;
      else if (SnakeItemPosX[SnakeHeadID] <= 0 )
        SnakeItemPosX[SnakeHeadID] = BaneGridXmax;
      if(SnakeItemPosY[SnakeHeadID] > BaneGridYmax)
        SnakeItemPosY[SnakeHeadID] = 1;
      else if (SnakeItemPosY[SnakeHeadID] <= 0 )
        SnakeItemPosY[SnakeHeadID] = BaneGridYmax;
      render();
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
  strip.setPixelColor(x, y, R, G, B);
}

void render(void) { // Render snake items
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
