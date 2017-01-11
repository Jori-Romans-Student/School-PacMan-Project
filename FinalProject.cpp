#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>
#include <SD.h>
#include "lcd_image.h"

#define SD_CS 5
#define TFT_CS 6
#define TFT_DC 7
#define TFT_RST 8
#define JOY_SEL 9

#define JOY_VERT_ANALOG 0
#define JOY_HORIZ_ANALOG 1
#define JOY_CENTRE 512

#define JOY_DEADZONE 500

#define MILLIS_PER_FRAME 32 // 100fps

#define BUFFPIXEL 20

using namespace std;

// Structure for Sprites (Ghosts and PacMan)
struct sprite {
    int16_t joyX; //Before cursor update position X
    int16_t joyY; //Before cursor update position Y
    int16_t cursorX; //Cursor location X
    int16_t cursorY; //Cursor location Y
    int delta; //Direction (-2 for negative lcd direction, 2 for positive)
    bool modeX; // True when it has reached a y Coordinate containing a row
    bool modeY; // True when it has reached an x Coordinate containing a collum
    bool moveX; // True when moving in X Direction
    bool moveY; // True when moving in Y Direction (moveX = !moveY)
    int16_t lowerXConstraint; // Lower X limit of movement (tunnel wall)
    int16_t lowerYConstraint; // Lower Y limit of movement (tunnel wall)
    int16_t upperXConstraint; // Upper X limit of movement (tunnel wall)
    int16_t upperYConstraint; // Upper Y limit of movement (tunnel wall)
    uint8_t prevRow; // The row number of the next left or current row intersection
    uint8_t prevCollum; // The collum number of the next upward or current collum intersection
    uint8_t nextRow; // The row number of the next right or current row intersection
    uint8_t nextCollum; // The collum number of the next left or current collum intersection
    int color; // Color of the sprite
};

// Structure containing information for the custom menu
struct custom {
    int color; // Determines which color to make PacMan (1-5)
    uint8_t numOfGhosts; // Determines number of ghosts in maze (1-4)
    uint8_t difficulty; // Determines the difficulty (1-4)
    uint8_t lives; // Determines the number of starting lives (1-9)
    uint8_t map; // Determines which map to play (only one for now)
};

// Contains all relevant info for the map
struct mapData {
    lcd_image_t* image; // lcd map image
    char* name; // Map name
    uint8_t numOfRows; // Number of rows
    uint8_t numOfCollums; //Number of Collums
    int16_t* rows; // Array containing the y coordinates of all rows
    int16_t* collums; // Array containing the x coordinates of all collums
    uint16_t numOfXDots; // The number of dots in a row across the entire maze
    uint16_t numOfYDots; // The number of dots in a collum across the entire maze
    uint8_t numOfXDotsPerRow; // The number of dots in a row
    uint8_t numOfYDotsPerCollum; // The number of dots in a collum
    uint16_t* xDots; // Array containing dot information across all allowable row spaces. Contains 1 for full and 0 for empty
    uint16_t* yDots; // Array containing dot information across all allowable collum spaces. Contains 1 for full and 0 for empty
    uint16_t* locationOfXDots; // Array containing x coordinates for each of the allowable dot spaces in a row
    uint16_t* locationOfYDots; // Array containing y coordinates for each of the allowable dot spaces in a collum
    uint8_t* locationOfCollumXDots; // Contains the location of collum dots in a collum-row intersection. Contains information in dot index
    uint8_t* locationOfRowYDots; // Contains the location of row dots in a collum-row intersection. Contains information in dot index
    uint16_t* specialXDots; // Contains amount of special (big) dots followed by location of special dots
    uint16_t* specialYDots; // Contains amount of special (big) dots followed by location of special dots
    uint16_t* noDotsX; // Contains location of rows that exist without dots
    uint16_t* noDotsY; // Contains location of collums that exist without dots
    uint8_t* xMovement; // Contains information regarding allowable movement in rows, with amount of info being numOfRow * numOfCollums
                        // First element represents the left most intersection of the first row, the 1 + numOfCollums element representing the first intersection of the second row, etc...
    uint8_t* yMovement; // Contains information regarding allowable movement in collums, with amount of info being numOfRow * numOfCollums
                        // First element represents the left most intersection of the first collum, the 1 + numOfRows element representing the first intersection of the second collum, etc...
    int16_t xPacManStart; // Starting X Coordinate for PacMan
    int16_t yPacManStart; // Starting Y Coordinate for Pacman
    uint8_t PacManStartingRowPrev; // Next most upward Pacman Row intersection, since Pacman starts on a row this equals the current row number
    uint8_t PacManStartingCollumPrev; // Next left Collum-Row intersection (in collum number)
    uint8_t PacManStartingRowNext; // Next most downward Pacman row intersection, since PacMan starts on a row this equals the current row number
    uint8_t PacManStartingCollumNext; // Next right Collum-Row intersection (in collum number)
    uint8_t GhostOneStartingRowPrev; // Next most upward ghost Row, also current row for ghosts (in row number)
    uint8_t GhostOneStartingCollumPrev; // Next left Collum-Row intersection (in collum number)
    uint8_t GhostOneStartingRowNext; // Next most downward ghost Row, also current row for ghosts (in row number)
    uint8_t GhostOneStartingCollumNext; // Next right Collum-Row interseciton (in collum number)
    int16_t xGhostStart; // Starting X coordinate for ghosts
    int16_t yGhostStart; // Starting Y Coordinate for ghosts
};

// Map Arrays for Map 1 (containing info for Map Structure)

lcd_image_t mapOneImage = {"Pac-man.lcd", 128, 142};
char mapOneName[10] = "PacMan";
int16_t mapOneRow[10] = {31, 67, 95, 123, 149, 177, 205, 233, 259, 287};
int16_t mapOneCollum[10] = {13, 31, 59, 85, 113, 141, 169, 195, 223, 241};
uint16_t mapOneXDots[260];
uint16_t mapOneYDots[290];
uint16_t mapOneXDotSpaces[26] = {13, 23, 31, 41, 49, 59, 67, 77, 85, 95, 103, 113, 123, 131, 141, 151, 159, 169, 177, 187, 195, 205, 213, 223, 231, 241};
uint16_t mapOneYDotSpaces[29] = {31, 41, 49, 59, 67, 77, 85, 95, 105, 113, 123, 131, 141, 149, 159, 167, 177, 187, 195, 205, 215, 223, 233, 241, 251, 259, 269, 277, 287};
uint8_t mapOneXCollumDots[10] = {0, 2, 5, 8, 11, 14, 17, 20, 23, 25};
uint8_t mapOneYRowDots[10] = {0, 4, 7, 10, 13, 16, 19, 22, 25, 28};
uint16_t specialXDots[3] = {2 ,182, 207};
uint16_t specialYDots[3] = {2, 2, 263};
uint16_t noDotsXMapOne[17] = {8, 78, 82, 86, 95, 99, 108, 110, 123, 125, 134, 136, 149, 151, 155, 194, 195};
uint16_t noDotsYMapOne[17] = {8, 98, 99, 101, 102, 104, 105, 124, 125, 153, 154, 185, 186, 188, 189, 191, 192};
uint8_t mapOneXMovement[100] = {2,3,3,3,1,2,3,3,3,1,2,3,3,3,3,3,3,3,3,1,2,3,1,2,1,2,1,2,3,1,0,0,0,2,3,3,1,0,0,0,3,3,3,1,0,0,2,3,3,3,0,0,0,2,3,3,1,0,0,0,2,3,3,3,1,2,3,3,3,1,2,1,2,3,3,3,3,1,2,1,2,3,1,2,1,2,1,2,3,1,2,3,3,3,3,3,3,3,3,1};
uint8_t mapOneYMovement[100] = {2,3,1,0,0,0,2,1,2,1,0,0,0,0,0,0,0,2,1,0,2,3,3,3,3,3,3,3,1,0,0,2,1,2,3,3,1,2,1,0,2,1,2,1,0,0,2,1,2,1,2,1,2,1,0,0,2,1,2,1,0,2,1,2,3,3,1,2,1,0,2,3,3,3,3,3,3,3,1,0,0,0,0,0,0,0,0,2,1,0,2,3,1,0,0,0,2,1,2,1};

// Map Struct for Map One:

mapData mapOne = {&mapOneImage, &mapOneName[0], 10, 10, &mapOneRow[0], &mapOneCollum[0],
                  260, 290, 26, 29, &mapOneXDots[0], &mapOneYDots[0], &mapOneXDotSpaces[0],
                  &mapOneYDotSpaces[0], &mapOneXCollumDots[0], &mapOneYRowDots[0], &specialXDots[0], &specialYDots[0], &noDotsXMapOne[0],
                  &noDotsYMapOne[0], &mapOneXMovement[0], &mapOneYMovement[0], 127, 233, 7, 4, 7, 5, 3, 4, 3, 5, 127, 123
                };

mapData maps[1] = {mapOne}; // Map Array containg info for different maps. Only one for now

mapData Map; // Map array used in function. Assigned a value from maps[] based on custom menu selection.

// Struct for PacMan sprite

sprite PacMan; // Will Contain info for Pacman sprite

// Struct for ghostOne sprite

sprite Ghosts[4]; // Will Contain info for Ghost Sprites

sprite* GhostPointer = &Ghosts[0];

// Custom Menu Struct

custom menu = {1, 4, 1, 3, 1};

// Custom Menu Array (used to make custom menu selection and modification easier)

int customMenuArray[5] = {menu.color, menu.numOfGhosts, menu.difficulty,
                          menu.lives, menu.map};
// Color Array (Called in custom menu, contains 0 at index 0 since colors range from 1-5

int colorArray[6] = {0, ST7735_YELLOW, ST7735_RED, ST7735_GREEN, ST7735_BLUE,
                     ST7735_WHITE};

// Scores

int score = 0; // PacMan Score
int prevScore = 0; // Prev PacMan Score, used to update
int ghostScore = 0; // Ghosts Score
int totalScore = 0; // Combined total of PacMan + Ghost score
int oneUpScore = 3000; // Score at which first 1up is reached
int resetScore = 0; // Score at which total score resets the Level

// Counting Variables

int i; int j; int k; int l; int m; int n;

// Move Array

int move[8] = {-2000, -2000, -2000, -2000, -2000, -2000, -2000, -2000}; // Info on starting ghost movement (-2000 means left)

// Other

uint8_t movement = 0; // Measures how much PacMan has moved. Resets to 0 when hitting 8, opens/closes his mouth every 4.
uint8_t mode = 1; // Main Function mode, determines what is seen on the screen
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// Menu Cursors

uint8_t mainCursorX = 0;
uint8_t mainCursorY = 0;
uint8_t mainJoyX = 0;
uint8_t mainJoyY = 0;

/* Function Delclarations (Note: All declarations were put up here to better
                            organize definitions below due to the number of
                            functions)
*/

/////////

void drawCircle(int16_t, int16_t, int);

void drawGhost(int16_t, int16_t, sprite*);

void drawGhostBlack(int16_t, int16_t);

void drawPacMan(int16_t, int16_t, int);

uint8_t readDotsX(sprite*);

uint8_t readDotsY(sprite*);

void updateConstraintsX(sprite*);

void updateConstraintsY(sprite*);

uint8_t readDotsXY(sprite*);

void moveX(int, sprite*);

void moveY(int, sprite*);

void changeProbabilities(int*, int*, int*, int*, sprite*);

void evaluateDirections(int*, int*, int*, int*, sprite*);

int randomGenerator(int, int, int, int, int);

int randGhost(sprite*);

void scanScore();

void scanPacMan();

void scanGhosts();

void updateDrawnPacMan();

void updateDrawnGhosts();

void updateCursor(sprite*);

void updatePrevNextX(sprite*);

void updatePrevNextY(sprite*);

void updateOther(sprite*);

void updateSprite(sprite*);

void updateScore();

void updateLives();

void updateGame();

void drawMain();

void drawCustom();

void updateMenuStruct();

void generateNoDots();

void generateSpecialDots();

void generateDots();

void createMap();

void createConstraintX(sprite*);

void createConstraintY(sprite*);

void createPacMan();

void createGhosts();

void loadMap();

void loadPacMan();

void loadGhosts();

void getConstraint(sprite*);

void scanMain();

void updateMain();

void scanCustom();

void updateCustom();

void scan();

void update();

int frameDelay(int, int);

void reset();

void setup() {
  init();

  Serial.begin(9600);

  tft.initR(INITR_BLACKTAB);

  Serial.print("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("failed!");
    return;
  }

  pinMode(JOY_SEL, INPUT);
  digitalWrite(JOY_SEL, HIGH); // enables pull-up resistor
  Serial.println("Joystick initialized!");

  Serial.println("OK!");
  tft.fillScreen(ST7735_BLACK);

}

/* Main works by increasing or decreasing the variable mode to change
    what is seen on screen */
int main() {
    setup(); // Only happens once
    // mode = 1; Resets to the main menu, reset increases mode by 1
    while (mode > 0) {
        reset();
        // mode = 2; Displays menu and scans/updates
        while (mode > 1) {
            scanMain(); //Scans joystick
            updateMain(); // Updates menu screen
            // Mode = 3; Called when custom is selected, resets to the custom menu
            while (mode > 2) {
                reset();
                // Mode = 4; Called automatically when mode = 3, use of custom menu page
                while (mode > 3) {
                    scanCustom(); // Scan joystick
                    updateCustom(); // Updates cursor and custom menu struct
                    // Mode = 5; Called when done with menu or level, creates or resets map and sprites
                    while (mode > 4) {
                        reset(); // Finish Level;
                        // Mode = 6; Called when dead, resets sprites
                        while (mode > 5) {
                            reset(); //Dies;
                            // Mode = 7; Main game
                            while (mode > 6) {
                                int Time = millis();
                                scan();
                                update();
                                frameDelay(MILLIS_PER_FRAME, Time); // Ensures framerate runs at MILLIS_PER_FRAME
                            }
                        }
                    }
                }
            }
        }
    }
    Serial.end();
    return 0;
}

/* Functions:
    Functions are arranged in alphabetical order in order to make finding them easier
*/

// Increases and decreases the probabilities of ghost moving in certain directions based on psition and difficulty
void changeProbabilities(int* up, int* down, int* left, int* right, sprite* Object) {
    int16_t startingChange; // Starts changing probabilities at this distance
    int16_t changeIncrement; // Changes probabilities further every changeIncrement
    uint8_t numOfXChanges; // Number of changes x
    uint8_t numOfYChanges; // Number of changes y
    uint8_t numOfAllowedChanges; // Number of changes allowed based on difficulty
    // Determines how function works based on difficulty
    switch (menu.difficulty) {
        case 1:
            startingChange = 100;
            changeIncrement = 40;
            numOfAllowedChanges = 1;
            break;
        case 2:
            startingChange = 120;
            changeIncrement = 40;
            numOfAllowedChanges = 2;
            break;
        case 3:
            startingChange = 180;
            changeIncrement = 30;
            numOfAllowedChanges = 4;
            break;
        case 4:
            startingChange = 200;
            changeIncrement = 20;
            numOfAllowedChanges = 5;
            break;
    }
    // Increases the number of changes in probability for up down based on difficulty and distance
    for (i = startingChange; i > startingChange - (changeIncrement * numOfAllowedChanges); i -= changeIncrement) {
        if (abs(PacMan.joyY - (*Object).joyY) > i) {
            break;
        }
        numOfYChanges++;
    }
    // Increases the number of changes in probability for left right based on difficulty and distance
    for (i = startingChange; i > startingChange - (changeIncrement * numOfAllowedChanges); i -= changeIncrement) {
        if (abs(PacMan.joyX - (*Object).joyX) > i) {
            break;
        }
        numOfXChanges++;
    }

    // If ghost is underneath PacMan
    if ((*Object).joyY - PacMan.joyY > 0) {
        // If he can move down (LCD up)
        if (*down != 0) {
            *down += numOfYChanges; // changes how much he can move up
            *up = constrain(*up - numOfYChanges, 0, *up); // decreases how much he moves down
        }
    }
    // If ghost is above PacMan
    else {
        // If he can move up (LCD down)
        if (*up != 0) {
            *up += numOfYChanges; // increases change of moving down
            *down = constrain(*down - numOfYChanges, 0, *down); // decreases chance of moving up
        }
    }
    // If ghost is to the right of pacMan
    if ((*Object).joyX - PacMan.joyX > 0) {
        // If ghost can move left
        if (*left != 0) {
            *left += numOfXChanges; // increases left prob
            *right = constrain(*right - numOfXChanges, 0, *right); // decrease right prob
        }
    }
    // If ghost is to the left of PacMan
    else {
        // If ghost can move right
        if (*right != 0) {
            *right += numOfXChanges; // increases right prob
            *left = constrain(*left - numOfXChanges, 0, *left); // decreases left prob
        }
    }
}

// Initializes the minimum and maximum X values (wall boundaries)
void createConstraintsX(sprite* Object) {
    // If the sprite is in a row...
    if ((*Object).modeX) {
          /* For loop:
                i starts at the intersection right of sprite within the row
                i loops through each potential intersection until the last collum
          */
          for (i = ((*Object).nextCollum + ((*Object).nextRow * Map.numOfRows)); i < Map.numOfCollums + ((*Object).nextRow * Map.numOfRows); ++i) {
              // If the value is 1 (1 meaning a right wall)
              if (*(Map.xMovement + i) == 1) {
                  // Evaluates the x coordinate of the wall based on the value of i
                  (*Object).upperXConstraint = *(Map.collums + (i - ((*Object).nextRow * Map.numOfRows)));
                  break;
              }
          }
          /* For loop:
                i starts at the intersection left of sprite within the row
                i loops through each potential intersection until the first collum
          */
          for (i = ((*Object).prevCollum + ((*Object).nextRow * Map.numOfRows)); i >= ((*Object).nextRow * Map.numOfRows); --i) {
              // If the value is 2 (2 meaning a left wall)
              if (*(Map.xMovement + i) == 2) {
                  // Evaluates the x coordinate of the wall based on the value of i
                  (*Object).lowerXConstraint = *(Map.collums + (i - ((*Object).nextRow * Map.numOfRows)));
                  break;
              }
          }
    }
    // If the sprite is not in a row...
    else {
        // Constrant object within collum walls by taking the collum number and finding it's X coordinate in Map.collums
        (*Object).lowerXConstraint = *(Map.collums + (*Object).nextCollum);
        (*Object).upperXConstraint = *(Map.collums + (*Object).nextCollum);
    }
}

// Initializes the minimum and maximum X values (wall boundaries)
void createConstraintsY(sprite* Object) {
  // If the sprite is in a collum...
  if ((*Object).modeY) {
        /* For loop:
            i starts at the intersection below sprite within the row
            i loops through each potential intersection until the last row
        */
        for (i = ((*Object).nextRow + ((*Object).nextCollum * (Map.numOfCollums))); i < (Map.numOfRows + ((*Object).nextCollum * Map.numOfCollums)); ++i) {
            // If the value is a 1 (1 meaning bottom wall)
            if (*(Map.yMovement + i) == 1) {
                // Updates upper Y constraint based on i's value in Map.rows
                (*Object).upperYConstraint = *(Map.rows + (i - ((*Object).nextCollum * Map.numOfRows)));
                break;
            }
        }
        /* For loop:
            i starts at the intersection above sprite within the row
            i loops through each potential intersection until the last row
        */
        for (i = ((*Object).prevRow + ((*Object).nextCollum * Map.numOfCollums)); i >= ((*Object).nextCollum * Map.numOfCollums); --i) {
            // If the value is a 2 (2 meaning top wall)
            if (*(Map.yMovement + i) == 2) {
                // Updates lower Y constraint based on i's value in Map.rows (which is modulated in this case by 10)
                (*Object).lowerYConstraint = *(Map.rows + (i - ((*Object).nextCollum * Map.numOfCollums)));
                break;
            }
        }
  }
  // If the sprite is not in a collum...
  else {
      // Constrain object within row walls by taking the row number and finding it's Y coordinate in Map.rows
      (*Object).lowerYConstraint = *(Map.rows + (*Object).nextRow);
      (*Object).upperYConstraint = *(Map.rows + (*Object).nextRow);
  }
}

// Creates ghost structures information
void createGhosts() {
    // Loop for specified number of ghosts
    for (j = 0; j < menu.numOfGhosts; j++) {
        // Initializes a local ghost sprite based on Map Values
        sprite Ghost_Template = {Map.xGhostStart, Map.yGhostStart, Map.xGhostStart, Map.yGhostStart,
                        -2, 1, 0, 1, 0, 0, 0, 0, 0, Map.GhostOneStartingRowPrev, Map.GhostOneStartingCollumPrev,
                        Map.GhostOneStartingRowNext, Map.GhostOneStartingCollumNext, colorArray[((j + 1) % 5) + ((j + 1)/5)]};
        // Creates ghost struct in struct array based on above struct
        *(GhostPointer + j) = Ghost_Template;
        createConstraintsX(GhostPointer + j);
        createConstraintsY(GhostPointer + j);
    }
}

// Creates data for Map Structure to be used in program
void createMap() {
    Map = maps[menu.map - 1]; // Equals specified map in custom menu
    generateDots(); // Generate the full and empty dots in map
}

// Creates data for PacMan Structure to be used in program
void createPacMan() {
    // Initializes a local PacMan sprite based on Map Values
    sprite Pac_Man = { Map.xPacManStart, Map.yPacManStart, Map.xPacManStart, Map.yPacManStart,
                      -2, 1, 0, 1, 0, 0, 0, 0, 0, Map.PacManStartingRowPrev, Map.PacManStartingCollumPrev,
                      Map.PacManStartingRowNext, Map.PacManStartingCollumNext, colorArray[menu.color]
                     };
    // Creates PacMan struct based on above struct
    PacMan = Pac_Man;
    createConstraintsX(&PacMan);
    createConstraintsY(&PacMan);
}

// Draws a circle with diameter 6 pixels with specified color centered at xCoordinate/2 and yCoordinate/2
void drawCircle(int16_t xCoordinate, int16_t yCoordinate, int color) {
    tft.drawLine(xCoordinate/2 - 2, yCoordinate/2, xCoordinate/2 - 2, yCoordinate/2 + 1, color);
    tft.drawLine(xCoordinate/2 - 1, yCoordinate/2 - 1, xCoordinate/2 - 1, yCoordinate/2 + 2, color);
    tft.drawLine(xCoordinate/2, yCoordinate/2 - 2, xCoordinate/2, yCoordinate/2 + 3, color);
    tft.drawLine(xCoordinate/2 + 1, yCoordinate/2 - 2, xCoordinate/2 + 1, yCoordinate/2 + 3, color);
    tft.drawLine(xCoordinate/2 + 2, yCoordinate/2 - 1, xCoordinate/2 + 2, yCoordinate/2 + 2, color);
    tft.drawLine(xCoordinate/2 + 3, yCoordinate/2, xCoordinate/2 + 3, yCoordinate/2 + 1, color);
}

// Draws the custom menu
void drawCustom() {
    tft.fillScreen(ST7735_BLACK); // init black

    // Print Custom Menu at Top
    tft.setCursor(28, 4);
    tft.setTextSize(2);
    tft.setTextColor(0xFFFF, 0x0000);
    tft.print("Custom");
    tft.setCursor(40, 20);
    tft.print("Menu");

    // Print Sel to Begin Message
    tft.setTextSize(1);
    tft.setCursor(4,40);
    tft.print("(Press Sel To Begin)");

    // Print Color and it's number
    tft.setCursor(48, 52);
    tft.print("Color");
    tft.drawLine(47, 60, 77, 60, ST7735_WHITE);
    tft.setTextColor(0x0000,0xFFFF);
    tft.setCursor(60, 64);
    tft.print(menu.color);

    // Print Number of Ghosts and it's number
    tft.setCursor(16, 74);
    tft.setTextColor(0xFFFF, 0x0000);
    tft.print("Number of Ghosts");
    tft.drawLine(15, 82, 111, 82, ST7735_WHITE);
    tft.setCursor(60, 86);
    tft.print(menu.numOfGhosts);

    // Print Difficulty and its number
    tft.setCursor(33, 96);
    tft.print("Difficulty");
    tft.drawLine(32, 104, 92, 104, ST7735_WHITE);
    tft.setCursor(60, 108);
    tft.print(menu.difficulty);

    // Print Lives and its number
    tft.setCursor(48, 118);
    tft.print("Lives");
    tft.drawLine(47, 126, 77, 126, ST7735_WHITE);
    tft.setCursor(60, 130);
    tft.print(menu.lives);

    // Print Map and its number
    tft.setCursor(54, 140);
    tft.print("Map");
    tft.drawLine(53, 148, 71, 148, ST7735_WHITE);
    tft.setCursor(60, 152);
    tft.print(menu.map);

}

// Draws Ghost centered at xCoordinate/2 and yCoordinate/2
void drawGhost(int16_t xCoordinate, int16_t yCoordinate, sprite* Object) {
    // Draws Ghost Body
    tft.fillRect(xCoordinate/2 - 2, yCoordinate/2 - 1, 6, 5, (*Object).color);
    tft.fillRect(xCoordinate/2 - 1, yCoordinate/2 - 2, 4, 1, (*Object).color);
    tft.fillRect(xCoordinate/2, yCoordinate/2 + 3, 2, 1, ST7735_BLACK);

    if ((*Object).moveX) {
        if ((*Object).delta > 0) {
            // Draws eyes looking right if moving right
            tft.fillRect(xCoordinate/2 + 1, yCoordinate/2 - 1, 1, 2, ST7735_WHITE);
            tft.fillRect(xCoordinate/2 + 3, yCoordinate/2 - 1, 1, 2, ST7735_WHITE);
            tft.fillRect(xCoordinate/2 + 1, yCoordinate/2, 1, 1, ST7735_BLACK);
            tft.fillRect(xCoordinate/2 + 3, yCoordinate/2, 1, 1, ST7735_BLACK);
          }
        else {
            // Draws eyes looking left if moving left
            tft.fillRect(xCoordinate/2 - 1, yCoordinate/2 - 1, 1, 2, ST7735_WHITE);
            tft.fillRect(xCoordinate/2 + 1, yCoordinate/2 - 1, 1, 2, ST7735_WHITE);
            tft.fillRect(xCoordinate/2 - 1, yCoordinate/2, 1, 1, ST7735_BLACK);
            tft.fillRect(xCoordinate/2 + 1, yCoordinate/2, 1, 1, ST7735_BLACK);
        }
    }
    else {
            // Draws eyes regularally if not
            tft.fillRect(xCoordinate/2, yCoordinate/2 - 1, 1, 2, ST7735_WHITE);
            tft.fillRect(xCoordinate/2 + 2, yCoordinate/2 - 1, 1, 2, ST7735_WHITE);
            tft.fillRect(xCoordinate/2, yCoordinate/2, 1, 1, ST7735_BLACK);
            tft.fillRect(xCoordinate/2 + 2, yCoordinate/2, 1, 1, ST7735_BLACK);
      }
    }

// Draws an all black ghost
void drawGhostBlack(int16_t xCoordinate, int16_t yCoordinate) {
    tft.fillRect(xCoordinate/2 - 2, yCoordinate/2 - 1, 6, 5, ST7735_BLACK);
    tft.fillRect(xCoordinate/2 - 1, yCoordinate/2 - 2, 4, 1, ST7735_BLACK);
    tft.fillRect(xCoordinate/2, yCoordinate/2 + 3, 2, 1, ST7735_BLACK);
}

// Draws the main menu
void drawMain() {
    tft.fillScreen(ST7735_BLACK); // init black

    // Print PacMan
    tft.setCursor(28, 30);
    tft.setTextSize(2);
    tft.print("PacMan");

    // Print One Player
    tft.setTextColor(0x0000, 0xFFFF);
    tft.setCursor(34, 90);
    tft.setTextSize(1);
    tft.print("One Player");

    // Print Custom
    tft.setCursor(46, 102);
    tft.setTextColor(0xFFFF, 0x0000);
    tft.print("Custom");
}

// Draws PacMan Sprite Centered at xCoordinate/2, yCoordinate/2 with specified color
void drawPacMan(int16_t xCoordinate, int16_t yCoordinate, int color) {
    drawCircle(xCoordinate, yCoordinate, color); // Draw Circle
    int16_t squareX, squareY;
    // If moving in y direction
    if (PacMan.moveY) {
        squareX = xCoordinate/2;
        // If moving up
        if (PacMan.delta < 0) {
            squareY = yCoordinate/2 - 2;
        }
        // If moving down
        else {
            squareY = yCoordinate/2 + 1;
        }
        tft.fillRect(squareX, squareY, 2, 3, ST7735_BLACK); // Draws Mouth
    }
    // If moving in X direction
    else {
        squareY = yCoordinate/2;
        // If moving right
        if (PacMan.delta > 0) {
            squareX = xCoordinate/2 + 1;
        }
        // If moving left
        else {
            squareX = xCoordinate/2 - 2;
        }
        tft.fillRect(squareX, squareY, 3, 2, ST7735_BLACK); // Draws Mouth
    }
}

// Evaluates which directions a random sprite (ghost) can move
void evaluateDirections(int* up, int* down, int* left, int* right, sprite* Object) {
  // Moving in X direciton
  if ((*Object).modeX) {
      // If it is not at a left wall
      if ((*Object).lowerXConstraint != (*Object).joyX) {
          // If it isn't moving in the opposite direction
          if (!((*Object).moveX && (*Object).delta > 0)) {
              *left = 1; // Can move this way
          }
      }
      // If it is not at a right wall
      if ((*Object).upperXConstraint != (*Object).joyX) {
          // If it isn't moving in the opposite direction
          if (!((*Object).moveX && (*Object).delta < 0)) {
              *right = 1; // Can move this way
          }
      }
  }
  // Moving in Y Direction
  if ((*Object).modeY) {
      // If it is not at a top wall
      if ((*Object).lowerYConstraint != (*Object).joyY) {
          // If it isn't moving in the opposite direction
          if (!((*Object).moveY && (*Object).delta > 0)) {
              *down = 1; // Can move this way
          }
      }
      // If it is not at a bottom wall
      if ((*Object).upperYConstraint != (*Object).joyY) {
          // If it isn't moving in the opposite direction
          if (!((*Object).moveY && (*Object).delta < 0)) {
              *up = 1; // Can move this way
          }
      }
  }
}

// Delays refreshing of screen based on framerate
int frameDelay(int frameRate, int Time) {
    int t = millis(); // Current time
    if (t - Time < frameRate) {
        delay(frameRate - (t - Time));
    }
    return millis(); // return time for next run of frameDelay
}

// Generates 1s and 0s in dot arrays, with 1s indicating points/full and 0s indication empty
void generateDots() {
    uint16_t index = 0; // measures row or collum number
    uint8_t dotValue = 0; // dot value indicating empty or full
    // For loop that loops through each element of X array
    for (i = 0; i < Map.numOfXDots; i++) {
        // If (xCoordinate of dot i in row == xCoordinate of potential collum intersection)
        if (*(Map.locationOfXDots + (i % Map.numOfXDotsPerRow)) == *(Map.collums + (index % Map.numOfCollums))) {
            *(Map.xDots + i) = 1; // initialize at 1
            // Determines if dotValue changes based on allowable movement
            switch (*(Map.xMovement + index)) {
                case 0: case 1:
                    dotValue = 0;
                    break;
                case 2: case 3:
                    dotValue = 1;
                    break;
            }
            ++index;
        }
        // If dot value is not at intersection...
        else {
            *(Map.xDots + i) = dotValue; // previously determined from switch statement
        }
    }

    index = 0;
    dotValue = 0;
    //For loop that loops through each element of y Array (same as above but with collums instead of rows)
    for (i = 0; i < Map.numOfYDots; i++) {
        if (*(Map.locationOfYDots + (i % Map.numOfYDotsPerCollum)) == *(Map.rows + (index % Map.numOfRows))) {
            *(Map.yDots + i) = 0; // Initializes at 0, this is done because collum dots at intersection are read in row dots, so these evaluate to 0
            switch (*(Map.yMovement + index)) {
                case 0: case 1:
                    dotValue = 0;
                    break;
                case 2: case 3:
                    dotValue = 1;
                    break;
            }
            ++index;
        }
        else {
            *(Map.yDots + i) = dotValue;
        }
    }
    generateNoDots(); // generate tunnels with no dots
    generateSpecialDots(); // generate big dots
}

// Generates areas with no dots that should have dots (tunnels) based on map specifications
void generateNoDots() {
    // Loop runs through each range of dots with amount specified as first element in array
    for (i = 0; i < *Map.noDotsX; ++i) {
        // Loop runs through every dot element in given range and equates to 0
        for (j = *(Map.noDotsX + (2 * i) + 1); j <= *(Map.noDotsX + (2 * i) + 2); ++j) {
            *(Map.xDots + j) = 0;
        }
    }
        // Loop runs through the number of ranges without dots specified as first element in array
    for (i = 0; i < *Map.noDotsY; ++i) {
        // Loop runs through every dot element in given range and equates to 0
        for (j = *(Map.noDotsY + (2 * i) + 1); j <= *(Map.noDotsY + (2 * i) + 2); ++j) {
            *(Map.yDots + j) = 0;
        }
    }
}

// Generates special dots (with value 5) at map specified locations
void generateSpecialDots() {
    // Loops run through all elements of special dots array, equating the value to 5
    for (i = 1; i <= *Map.specialXDots; ++i) {
        *(Map.xDots + *(Map.specialXDots + i)) = 5;
    }
    for (i = 1; i <= *Map.specialYDots; ++i) {
        *(Map.yDots + *(Map.specialYDots + i)) = 5;
    }
}

// Main function to create all constraints depending on if the sprite is entering an intersection
void getConstraints(sprite* Object) {
    // If sprite wasn't in a collum before...
    if (!((*Object).modeY)) {
        // If it's in a collum now (the previous collum)
        if ((*Object).joyX == *(Map.collums + (*Object).prevCollum)) {
            (*Object).nextCollum = (*Object).prevCollum;
            (*Object).modeY = 1; // in a collum
            createConstraintsY(Object); // create upper and lower y bounds
        }
        // If it's in a collum now (the next collum)
        else if ((*Object).joyX == *(Map.collums + (*Object).nextCollum)) {
            (*Object).prevCollum = (*Object).nextCollum;
            (*Object).modeY = 1; // in a collum
            createConstraintsY(Object); // create upper and lower y bounds
        }
    }
    // If sprite wasn't in a row before...
    if (!((*Object).modeX)) {
        // If it's in a row now (the previous row)
        if ((*Object).joyY == *(Map.rows + (*Object).prevRow)) {
            (*Object).nextRow = (*Object).prevRow;
            (*Object).modeX = 1; // in a row
            createConstraintsX(Object); // create upper and lower x bounds
        }
        // If it's in a collum now (the next row)
        else if ((*Object).joyY == *(Map.rows + (*Object).nextRow)) {
            (*Object).prevRow = (*Object).nextRow;
            (*Object).modeX = 1; // in a row
            createConstraintsX(Object); // create upper and lower x bounds
        }
    }
}

// Draws all ghosts to the screen at the beggining of level
void loadGhosts() {
    // Loops through all ghosts
    for (j = 0; j < menu.numOfGhosts; j++) {
        drawGhost((*(GhostPointer + j)).joyX, (*(GhostPointer + j)).joyY, GhostPointer + j);
    }
}

// Draws the specified created map to the screen
void loadMap() {
    tft.fillScreen(ST7735_BLACK); // init black
    lcd_image_draw(Map.image, &tft, 0, 0, 0, 9, 128, 142); // draw map

    // Print current score
    tft.setCursor(12, 0);
    tft.setTextSize(1);
    tft.setTextColor(0xFFFF, 0x0000);
    tft.print("Score:");
    tft.fillRect(50, 0, 60, 8, ST7735_BLACK);
    tft.setCursor(50, 0);
    tft.print(score);

    // Draws amount of pacman lives to the bottom of the screen
    for (i = 0; i < menu.lives; i++) {
        drawCircle(29 + (16 * i), 309, PacMan.color);
        tft.fillRect(15 + (8 * i), 154, 3, 2, ST7735_BLACK);
    }

}

// Draws PacMan to the screen at the beggining of the level
void loadPacMan() {
    drawPacMan(PacMan.joyX, PacMan.joyY, PacMan.color);
}

// Function that updates the joyX of the sprite
void moveX(int horiz, sprite* Object) {
  // If (There is a request to move in the x direction and sprite is in a row OR it's already moving in the x direction)
  if ((abs(horiz - JOY_CENTRE) > JOY_DEADZONE && (*Object).modeX == 1) || (*Object).moveX == 1) {
     // If it's been requested to move
     if (abs(horiz - JOY_CENTRE) > JOY_DEADZONE) {
        if ((horiz - JOY_CENTRE) > 0) {
            (*Object).delta = 2; // moving to the right
        }
        else {
            (*Object).delta = -2; // moving to the left
        }
        (*Object).moveX = 1; // moving in x direction
        (*Object).moveY = 0; // not moving in y direction
    }
    // Updates based on previously defined constraints (walls)
    (*Object).joyX = constrain((*Object).joyX + (*Object).delta, (*Object).lowerXConstraint, (*Object).upperXConstraint);
  }
  // If the object is going through a left tunnel
  if ((*Object).joyX == *Map.collums - 26) {
      (*Object).joyX = *(Map.collums + Map.numOfCollums - 1) + 26; // move to the right
      (*Object).prevCollum = 7; // location of next left collum intersection
      (*Object).nextCollum = 7;
      updateConstraintsX(Object); // update constraints
  }
  // If the object is going through a right tunnel
  else if ((*Object).joyX == *(Map.collums + Map.numOfCollums - 1) + 26) {
      (*Object).joyX = *Map.collums - 26; // move to the left tunnel
      (*Object).prevCollum = 2; // location of next right collum intersection
      (*Object).nextCollum = 2;
      updateConstraintsX(Object); // update constraints
  }
}

// Fucntion that updates the joyY of the sprite
void moveY(int vert, sprite* Object) {
    // If (A request to move in the y direction AND it's in a collum OR moving in the y direction)
    if ((abs(vert - JOY_CENTRE) > JOY_DEADZONE && (*Object).modeY == 1) || (*Object).moveY) {
        // If (there was a request to move in y direction)
        if (abs(vert - JOY_CENTRE) > JOY_DEADZONE) {
            if ((vert - JOY_CENTRE) > 0) {
                (*Object).delta = 2; // moving down
            }
            else {
                (*Object).delta = -2; // moving up
            }
            (*Object).moveY = 1;
            (*Object).moveX = 0;
        }
        // Updates y within walls
        (*Object).joyY = constrain((*Object).joyY + (*Object).delta, (*Object).lowerYConstraint, (*Object).upperYConstraint);
    }
}

// Function that returns a random number pertaining to one of the 4 directions
// The higher the value entered, the more likely it returns the pertaining number
int randomGenerator(int upNumber, int downNumber, int leftNumber, int rightNumber, int randomIndex) {
    int randomArray[randomIndex]; // Stores random numbers
    // Stores value 2 upNumber times in randomArray
    for (i = 0; i < upNumber; ++i) {
        randomSeed(analogRead(2));
        randomArray[i] = random(2,3);
    }
    // Stores value 1 rightNumber times in randomArray
    for (i = 0; i < rightNumber; ++i) {
        randomSeed(analogRead(2));
        randomArray[i + upNumber] = random(1,2);
    }
    // Stores value -2 downNumber times in randomArray
    for (i = 0; i < downNumber; ++i) {
        randomSeed(analogRead(2));
        randomArray[i + upNumber + rightNumber] = random(-2,-1);
    }
    // Stores value -1 leftNumber times in randomArray
    for (i = 0; i < leftNumber; ++i) {
        randomSeed(analogRead(2));
        randomArray[i + upNumber + rightNumber + downNumber] = random(-1,0);
    }
    randomSeed(analogRead(2));
    // Returns one of the values randomly chosen from the array
    return randomArray[random(0,randomIndex)];
}

// Computes a direction number (1, 2, -1, or -2) to be used in updating joyX of the ghost
int randGhost(sprite* Object) {
    // Probabilities of moving in these directions
    int up = 0;
    int down = 0;
    int left = 0;
    int right = 0;
    // Sum of probabilities
    int sum = 0;

    // Evaluates which of the directions the ghost can move in (returns 1 for true)
    evaluateDirections(&up, &down, &left, &right, Object);
    sum = up + down + left + right; // sum of probabilities

    // Multiplies probability values so that they sum to 12
    uint8_t probValue = 12 / sum;
    up *= probValue;
    down*= probValue;
    right *= probValue;
    left *= probValue;

    // Increases certain probabilities depending on ghost position relative to Pacman
    changeProbabilities(&up, &down, &left, &right, Object);
    sum = up + down + left + right;

    // returns which direction to go
    return randomGenerator(up, down, left, right, sum);
}

// Reads dots between prev and next collum intersections. If on a dot, it will compute
// if the dot is read or not and return its value
uint8_t readDotsX(sprite* Object) {
    // x positions of prev and next collum intersections
    uint8_t prevIndex = *(Map.locationOfCollumXDots + (*Object).prevCollum);
    uint8_t nextIndex = *(Map.locationOfCollumXDots + (*Object).nextCollum);

    uint8_t value = 0; // dot value
    // Loops through each dot position between intersections
    for (i = prevIndex; i <= nextIndex; i++) {
        // If at a dot positon...
        if ((*Object).joyX == *(Map.locationOfXDots + i)) {
            // Value equals the equivalent dot position in the xDots array
            value = *(Map.xDots + i + ((*Object).prevRow * Map.numOfXDotsPerRow));
            if (value != 0) {
                // Updates that the dot has been read and makes it 0
                *(Map.xDots + i + ((*Object).prevRow * Map.numOfXDotsPerRow)) = 0;
            }
            break;
        }
    }
    return value;
}

// Reads the dots in the intersections of collums and rows
uint8_t readDotsXY (sprite* Object) {
    uint8_t indexOne;
    uint8_t indexTwo;
    uint8_t value = 0; // dot value
    // For loop that determines which row the dot is on
    for (i = 0; i < Map.numOfRows; i++) {
        if ((*Object).joyY == *(Map.rows + i)) {
            indexOne = i;
            break;
        }
    }
    // For loop that determines which collum the dot is on
    for (i = 0; i < Map.numOfXDotsPerRow; i++) {
        if ((*Object).joyX == *(Map.locationOfXDots + i)) {
            indexTwo = i;
            break;
        }
    }
    // Computes the value (stored in xDots) at the given intersection
    value = *(Map.xDots + indexTwo + (indexOne * Map.numOfXDotsPerRow));
    if (value != 0) {
        // Updates dot value to 0
        *(Map.xDots + indexTwo + (indexOne * Map.numOfXDotsPerRow)) = 0;
    }
    return value;
}

// Reads dots between prev and next row intersections. If on a dot, it will compute
// if the dot is read or not and return its value
uint8_t readDotsY(sprite* Object) {
    // y positions of prev and next row intersections
    uint8_t prevIndex = *(Map.locationOfRowYDots + (*Object).prevRow);
    uint8_t nextIndex = *(Map.locationOfRowYDots + (*Object).nextRow);

    uint8_t value = 0; // dot value
    // Loops through each dot position between intersectinos
    for (i = prevIndex; i <= nextIndex; i++) {
        // if at a dot position...
        if ((*Object).joyY == *(Map.locationOfYDots + i)) {
            // Value eqals the equivalent dot position in the yDots array
            value = *(Map.yDots + i + ((*Object).prevCollum * Map.numOfYDotsPerCollum));
            if (value != 0) {
                // Update that the dot has been read and make it 0
                *(Map.yDots + i + ((*Object).prevCollum * Map.numOfYDotsPerCollum)) = 0;
            }
            break;
        }
    }
    return value;
}

// Fucntions used to reset certain values when changing menus, dying, finishing the level or leaving a game
void reset() {
    switch (mode) {
        case 1:
            drawMain(); // draw main menu

            // Reset the joystick and cursor
            mainJoyY = 0;
            mainCursorY = 0;

            // Reset the custom menu struct and array values
            menu.color = 1;
            menu.numOfGhosts = 4;
            menu.difficulty = 1;
            menu.lives = 3;
            menu.map = 1;
            customMenuArray[0] = menu.color;
            customMenuArray[1] = menu.numOfGhosts;
            customMenuArray[2] = menu.difficulty;
            customMenuArray[3] = menu.lives;
            customMenuArray[4] = menu.map;

            // Reset each score
            score = 0;
            ghostScore = 0;
            totalScore = 0;
            oneUpScore = 3000;
            resetScore = 0;

            mode++;
            break;
        case 3:
            drawCustom(); // draw custom menu

            // update joy and cursors
            mainJoyY = 0; mainJoyX = 0;
            mainCursorY = 0; mainCursorX = 0;

            mode++;
            break;
        case 5:
            updateMenuStruct(); // Update struct based on custom menu input
            createMap(); // create map struct
            createPacMan(); // create pacman struct
            createGhosts(); // create ghost struct
            loadMap(); // Draw and load map elements
            loadPacMan(); // Draw and load PacMan elements
            loadGhosts(); // Draw and load Ghost elements
            movement = 0; // Update movement (PacMan open and close mouth variable)
            resetScore += 2560; // Resets level at this score
            // Resets the movement values for potential ghosts
            move[0] = -2000;
            move[1] = -2000;
            move[2] = -2000;
            move[3] = -2000;
            move[4] = -2000;
            move[5] = -2000;
            move[6] = -2000;
            move[7] = -2000;

            mode += 2;
            delay(2000);
            break;
        case 6:
            createPacMan(); // create pacman struct
            createGhosts(); // create ghosts structs
            loadPacMan(); // load and draw pacman
            loadGhosts(); // load and draw ghosts
            movement = 0; // Update movement (PacMan open and close mouth variable)

            // Resets the movement values for potential ghosts
            move[0] = -2000;
            move[1] = -2000;
            move[2] = -2000;
            move[3] = -2000;
            move[4] = -2000;
            move[5] = -2000;
            move[6] = -2000;
            move[7] = -2000;

            // Redraw score
            tft.setCursor(12, 0);
            tft.setTextSize(1);
            tft.setTextColor(0xFFFF, 0x0000);
            tft.print("Score:");
            tft.fillRect(50, 0, 60, 8, ST7735_BLACK);
            tft.setCursor(50, 0);
            tft.print(score);
            tft.fillRect(12, 152, 116, 8, ST7735_BLACK);

            // Redraw PacMan lives
            for (i = 0; i < menu.lives; i++) {
                drawCircle(29 + (16 * i), 309, PacMan.color);
                tft.fillRect(15 + (8 * i), 154, 3, 2, ST7735_BLACK);
            }
            mode += 1;
            delay(2000);
            break;
    }
}

// Scans everything in main game
void scan() {
    scanScore();
    scanPacMan();
    scanGhosts();
}

// Scans everything in custom menu
void scanCustom() {
    int vert = analogRead(JOY_VERT_ANALOG);
    int horiz = analogRead(JOY_HORIZ_ANALOG);
    int select = digitalRead(JOY_SEL);
    int joyDeadZone = 500;
    int customDelta = 0;
    uint8_t upperConstraint; // How high you can make the custom values

    // (If request to move in y direction)
    if (abs(vert - JOY_CENTRE) > joyDeadZone) {
        if ((vert - JOY_CENTRE) > 0)
            customDelta = 1; // move down
        else {
            customDelta = -1; // move up
        }
        mainJoyY = constrain(mainJoyY + customDelta, 0, 4); // updates main joy
        mainJoyX = customMenuArray[mainJoyY]; // update the joyX based on which custom number you are on
        mainCursorX = mainJoyX; // updates the x cursor
    }
    // (If request to move in x direction)
    else if (abs(horiz - JOY_CENTRE) > joyDeadZone) {
        if ((horiz - JOY_CENTRE) > 0) {
            customDelta = 1; // move right
        }
        else {
            customDelta = -1; // move left
        }
        // Constraints (maximum number) depending on which number you are changing
        switch (mainJoyY) {
            case 0:
                upperConstraint = 5;
                break;
            case 1:
                upperConstraint = 4;
                break;
            case 2:
                upperConstraint = 4;
                break;
            case 3:
                upperConstraint = 9;
                break;
            case 4:
                upperConstraint = 1;
                break;
        }
        mainJoyX = constrain(mainJoyX + customDelta, 1, upperConstraint);
    }
    // Changes mode when joy is pushed down
    if (select == 0)
        mode = 5;
}

// Scans everything for ghosts
void scanGhosts() {
    // Loops through all ghosts
    for (j = 0; j < menu.numOfGhosts; j++) {
        // If ghost is at an intersection
        if (((*(GhostPointer + j)).moveX && (*(GhostPointer + j)).modeY) || ((*(GhostPointer + j)).moveY && (*(GhostPointer + j)).modeX)) {
            *(move + j) = randGhost((GhostPointer + j)); // determine which way to move
        }
        // If not, use previous move direction
        else {
            *(move + j) /= 2000;
        }
        // If moving horizontally
        if (abs(*(move + j)) == 1) {
            *(move + j) *= 2000;
            moveX(*(move + j), GhostPointer + j); // updates joyX
        }
        // If moving vertically
        else {
            *(move + j) *= 2000;
            moveY(*(move + j), GhostPointer + j); // updates joyY
        }
    }
}

// Scans everything in main menu
void scanMain() {

    int vert = analogRead(JOY_VERT_ANALOG);
    int select = digitalRead(JOY_SEL);
    int joyDeadZone = 300;
    int mainDelta = 0;

    // If request to move in y direction
    if (abs(vert - JOY_CENTRE) > joyDeadZone) {
        if ((vert - JOY_CENTRE) > 0)
            mainDelta = 1; // move down
        else {
            mainDelta = -1; // move up
        }
            mainJoyY = constrain(mainJoyY + mainDelta, 0, 1);
    }
    // if pushed down
    if (select == 0) {
        if (mainJoyY == 0)
            mode = 5;
        else
            mode = 3;
        }
}

// scan everything for pacMan
void scanPacMan() {
    int vert = analogRead(JOY_VERT_ANALOG);
    int horiz = analogRead(JOY_HORIZ_ANALOG);
    // If moving in y direciton
    if (PacMan.moveY) {
        moveX(horiz, &PacMan); // check and update x direction first
        // If x wasn't updated
        if (PacMan.moveX == 0) {
            moveY(vert, &PacMan); // check and update y direction
        }
    }
    // If moving in x direction
    else {
        moveY(vert, &PacMan); // check and update y direction first
        // If y wasn't updated
        if (PacMan.moveY == 0) {
          moveX(horiz, &PacMan); // check and update x direction
        }
    }
}

// scan everything for the score
void scanScore() {
    // If in a row and not in an intersection
    if (PacMan.modeX == 1 && PacMan.modeY == 0) {
        score += (10 * readDotsX(&PacMan));
    }
    // If in a collum and not in an intersection
    if (PacMan.modeY == 1 && PacMan.modeX == 0) {
        score += (10 * readDotsY(&PacMan));
    }
    // If in an intersection
    if (PacMan.modeX == 1 && PacMan.modeY == 1) {
        score += (10 * readDotsXY(&PacMan));
    }
    // Loops for every ghost
    for (l = 0; l < menu.numOfGhosts; l++) {
        // If in a row and not in an intersection
        if ((*(GhostPointer + l)).modeX == 1 && (*(GhostPointer + l)).modeY == 0) {
            ghostScore += (10 * readDotsX(GhostPointer + l));
        }
        // If in a collum and not in an intersection
        if ((*(GhostPointer + l)).modeY == 1 && (*(GhostPointer + l)).modeX == 0) {
            ghostScore += (10 * readDotsY(GhostPointer + l));
        }
        // If in an intersection
        if ((*(GhostPointer + l)).modeX == 1 && (*(GhostPointer + l)).modeY == 1) {
            ghostScore += (10 * readDotsXY(GhostPointer + l));
        }
    }
}

// Update everything
void update() {
    updateSprite(&PacMan);
    updateSprite(GhostPointer);
    updateScore();
    updateLives();
    updateGame();
}

// Update constraint for x (walls)
void updateConstraintsX(sprite* Object) {
  // Checks every potential intersection in row starting at the next right collum intersection
  for (i = ((*Object).nextCollum + ((*Object).nextRow * Map.numOfRows)); i < Map.numOfCollums + ((*Object).nextRow * Map.numOfRows); ++i) {
      // If i marks a right wall
      if (*(Map.xMovement + i) == 1) {
          // Upper x constraints equals the x coordinate of that wall
          (*Object).upperXConstraint = *(Map.collums + (i - ((*Object).nextRow * Map.numOfRows)));
          break;
      }
      // If there is free movement and it's reached the last collum intersection (marking a out of map tunnel)
      else if (*(Map.xMovement + i) == 3 && i == Map.numOfCollums + ((*Object).nextRow * (Map.numOfRows)) - 1) {
          // Wall is placed outside of map
          (*Object).upperXConstraint = *(Map.collums + Map.numOfCollums - 1) + 26;
          break;
      }
  }
  // Checks every potential intersection in row starting at the next left collum intersection
  for (i = ((*Object).prevCollum + ((*Object).nextRow * Map.numOfRows)); i >= ((*Object).nextRow * Map.numOfRows); --i) {
      // If i marks a left wall
      if (*(Map.xMovement + i) == 2) {
          // Lower x constraint equals the x coordinate of that wall
          (*Object).lowerXConstraint = *(Map.collums + (i - ((*Object).nextRow * Map.numOfRows)));
          break;
      }
      // If there is free movement and the first collum intersection is reached (marking a out of map tunnel)
      else if (*(Map.xMovement + i) == 3 && i == ((*Object).nextRow * Map.numOfRows)) {
          // Wall is placed outside of map
          (*Object).lowerXConstraint = *Map.collums - 26;
          break;
      }
  }
}

// Update constraint for y (walls)
void updateConstraintsY(sprite* Object) {
  // Checks every potential intersection in collum starting at the next right row intersection
  for (i = ((*Object).nextRow + ((*Object).nextCollum * (Map.numOfCollums))); i < (Map.numOfRows + ((*Object).nextCollum * Map.numOfCollums)); ++i) {
      // If i marks a top wall
      if (*(Map.yMovement + i) == 1) {
          // Upper y constraint equals the y coordinate of that wall
          (*Object).upperYConstraint = *(Map.rows + (i - ((*Object).nextCollum * Map.numOfRows)));
          break;
      }
      // If there is free movement and the first row intersection is reached (marking a out of map tunnel)
      else if (*(Map.yMovement + i) == 3 && i == Map.numOfRows + ((*Object).nextCollum * (Map.numOfCollums)) - 1) {
          // Wall is placed outside of map
          (*Object).upperYConstraint = *(Map.rows + Map.numOfRows - 1) + 26;
          break;
      }
  }
  // Checks every potential intersection in collum starting at the next left row intersection
  for (i = ((*Object).prevRow + ((*Object).nextCollum * Map.numOfCollums)); i >= ((*Object).nextCollum * Map.numOfCollums); --i) {
      // If i marks a bottom wall
      if (*(Map.yMovement + i) == 2) {
          // Upper y constraint equals the y coordinate of that wall
          (*Object).lowerYConstraint = *(Map.rows + (i - ((*Object).nextCollum * Map.numOfCollums)));
          break;
      }
      // If there is free movement and the last tow intersection is reached (amrking a out of map tunnel)
      else if (*(Map.yMovement + i) == 3 && i == ((*Object).nextCollum * Map.numOfCollums)) {
          // Upper y constraint equals the y coordinate of that wall
          (*Object).lowerYConstraint = *Map.rows - 26;
          break;
      }
  }
}

// Update the drawn postions of both sprites
void updateCursor(sprite* Object) {
    // If joyY or joyX does not equal cursor positions
    if ((*Object).joyX != (*Object).cursorX || (*Object).joyY != (*Object).cursorY) {
        if (Object == &PacMan) {
            updateDrawnPacMan();
            movement += 1;
            // Loops movement back to 0
            if (movement == 8) {
                movement = 0;
            }
        }
        else {
            updateDrawnGhosts();
        }
        (*Object).cursorX = (*Object).joyX;
        (*Object).cursorY = (*Object).joyY;
    }
    // Happens when PacMan hits a wall, ensuring his mouth is always open
    else {
        if (Object == &PacMan) {
            drawPacMan(PacMan.joyX, PacMan.joyY, PacMan.color);
        }
    }
}

// Updates everything about the custom menu
void updateCustom() {
    // (If joy has been moved in Y direction)
    if (mainJoyY != mainCursorY) {

        // Makes the old text number white text with black background
        tft.setCursor(60, 64 + (22 * mainCursorY));
        tft.setTextColor(0xFFFF, 0x0000);
        tft.print(customMenuArray[mainCursorY]);

        // Makes the new text number black text with white background
        tft.setCursor(60, 64 + (22 * mainJoyY));
        tft.setTextColor(0x0000, 0xFFFF);
        tft.print(customMenuArray[mainJoyY]);

        mainCursorY = mainJoyY;
        mainCursorX = customMenuArray[mainJoyY]; // syncs xCursor with number
        delay(150);
    }
    // If joy has been moved in x direciton
    else if (mainJoyX != mainCursorX) {

        // Increases text number on screen
        tft.setCursor(60, 64 + (22 * mainCursorY));
        tft.setTextColor(0x0000,0xFFFF);
        tft.print(mainJoyX);
        mainCursorX = mainJoyX;
        customMenuArray[mainCursorY] = mainJoyX;
        delay(150);
    }
}

// Update ghosts on screen depending on joy movement
void updateDrawnGhosts() {
    // Lopps for all ghosts
    for (j = 0; j < menu.numOfGhosts; j++) {
        // Draws the screen black for previous ghost position
        drawGhostBlack((*(GhostPointer + j)).cursorX, (*(GhostPointer + j)).cursorY);
        // Draws the ghost in new position
        drawGhost((*(GhostPointer + j)).joyX, (*(GhostPointer + j)).joyY, GhostPointer + j);
      }
}

// Update PacMan on screen depending on joy movement
void updateDrawnPacMan() {
    // Draws black in previous position
    drawCircle(PacMan.cursorX, PacMan.cursorY, ST7735_BLACK);
    // If the mouth should be open
    if (movement > 3) {
        drawPacMan(PacMan.joyX, PacMan.joyY, PacMan.color);
    }
    // If the mouth should be closed
    else {
        drawCircle(PacMan.joyX, PacMan.joyY, PacMan.color);
    }
}

// Updates the status of the game (level completion or death)
void updateGame() {
    totalScore = score + ghostScore;
    // If score has reached its max (ie finishing a level)
    if (totalScore == resetScore) {
        delay(1500);
        mode = 5; // reset level mode
    }
    // Loops for all ghosts
    for (m = 0; m < menu.numOfGhosts; m++) {
        // If a ghost is 2 away from PacMan in the x and y direction
        if ((abs(PacMan.cursorX - (*(GhostPointer + m)).cursorX) <= 2 && abs(PacMan.cursorY - (*(GhostPointer + m)).cursorY) <= 2)) {
            delay(1500); // freeze screen
            menu.lives--; // reduce lives
            customMenuArray[3]--; // reduce lives in array
            drawCircle(PacMan.cursorX, PacMan.cursorY, ST7735_BLACK); // Make PacMan dissapear
            // Make every ghost dissapear
            for (n = 0; n < menu.numOfGhosts; n++) {
                drawGhostBlack((*(GhostPointer + n)).cursorX, (*(GhostPointer + n)).cursorY);
            }
            mode = 6; // reset death mode
            // If game over...
            if (menu.lives == 0) {
                mode = 1; // reset game
            }
            break;
        }
    }
}

// Updates PacMan one ups
void updateLives() {
    if (score == oneUpScore) {
        drawCircle(29 + (16 * menu.lives), 309, PacMan.color); // draws circle in lives row
        tft.fillRect(15 + (8 * menu.lives), 154, 3, 2, ST7735_BLACK); // draws mouth
        menu.lives += 1; // increases lives
        customMenuArray[3] += 1; // increases lives in array
        oneUpScore += 5000; // increases one up score
    }
}

// Updates the main menu
void updateMain() {
    // If request to move in y direction
    if (mainJoyY != mainCursorY) {
        // If currently on One Player...
        if (mainJoyY == 0) {
            // Set One Player on black background, custom on white
            tft.setTextColor(0x0000, 0xFFFF);
            tft.setCursor(34, 90);
            tft.setTextSize(1);
            tft.print("One Player");
            tft.setCursor(46, 102);
            tft.setTextColor(0xFFFF, 0x0000);
            tft.print("Custom");
        }
        // If currently on custom...
        else {
            // Set One Player on white background, custom on black
            tft.setTextColor(0xFFFF, 0x0000);
            tft.setCursor(34, 90);
            tft.setTextSize(1);
            tft.print("One Player");
            tft.setCursor(46, 102);
            tft.setTextColor(0x0000, 0xFFFF);
            tft.print("Custom");
        }
        mainCursorY = mainJoyY;
    }
}

// Updates the menu struct based on custom array values
void updateMenuStruct() {
    menu.color = customMenuArray[0];
    menu.numOfGhosts = customMenuArray[1];
    menu.difficulty = customMenuArray[2];
    menu.lives = customMenuArray[3];
    menu.map = customMenuArray[4];
}

// Updates other features for PacMan and ghosts (such as modes, prev and next collum/row, and constraints)
void updateOther(sprite* Object) {
    // Turn Modes On:
    // If in a row and not collum
    if (!(*Object).modeY) {
        // If sprite is at the location of the next left or next right collum intersection...
        if (*(Map.collums + (*Object).prevCollum) == (*Object).cursorX || *(Map.collums + (*Object).nextCollum) == (*Object).cursorX) {
            (*Object).modeY = 1;
            updatePrevNextY(Object);
            updateConstraintsY(Object);
        }
    }
    // If in a collum and not row
    if (!(*Object).modeX) {
        // If sprite is at the location of the next left of next right row intersection...
        if (*(Map.rows + (*Object).prevRow) == (*Object).cursorY || *(Map.rows + (*Object).nextRow) == (*Object).cursorY) {
            (*Object).modeX = 1;
            updatePrevNextX(Object);
            updateConstraintsX(Object);
        }
    }
    // Turn Modes Off:
    // If in an intersection
    if ((*Object).modeY == 1 && (*Object).modeX == 1) {
        // If sprite moves in x direction and it moves to a location outside the intersection...
        if ((*Object).moveX == 1 && ((*Object).cursorX != *(Map.collums + (*Object).prevCollum) || (*Object).cursorX != *(Map.collums + (*Object).nextCollum))) {
            (*Object).modeY = 0;
            updatePrevNextY(Object);
        }
        // If sprite moves in y direction outside of the intersection...
        if ((*Object).moveY == 1 && ((*Object).cursorY != *(Map.rows + (*Object).prevRow) || (*Object).cursorY != *(Map.rows + (*Object).nextRow))) {
            (*Object).modeX = 0;
            updatePrevNextX(Object);
        }
    }
}

// Updates the prev and next row intersections for sprite
void updatePrevNextX(sprite* Object) {
  // If in a row
  if ((*Object).modeX == 1) {
      // If moving to the right
      if ((*Object).delta > 0) {
          (*Object).prevRow = (*Object).nextRow;
      }
      // If moving to the left
      else {
          (*Object).nextRow = (*Object).prevRow;
      }
  }
  // If not in a row
  else {
    // If moving down
    if ((*Object).delta > 0) {
        // Loops through every possible intersection in current collum
        for (i = (*Object).prevCollum + (((*Object).prevRow + 1) * Map.numOfCollums); i < (*Object).prevCollum + ((Map.numOfRows) * Map.numOfCollums); i += Map.numOfCollums) {
            // If there is an intersection at y row coordinate...
            if (*(Map.xMovement + i) != 0) {
                (*Object).nextRow = i / Map.numOfCollums;
                break;
            }
        }
    }
    // If moving up
    else {
      // Loops through every possible intersection in current collum starting at the previous prevRow
      for (i = (*Object).nextCollum + (((*Object).prevRow - 1) * Map.numOfCollums); i >= (*Object).prevCollum; i -= Map.numOfCollums) {
          if (*(Map.xMovement + i) != 0) {
              (*Object).prevRow = i / Map.numOfCollums;
              break;
          }
      }
    }
  }
}

// Updates the prev and next row intersecitons for sprite
void updatePrevNextY(sprite* Object) {
    // If in a collum
    if ((*Object).modeY == 1) {
        // If moving down
        if ((*Object).delta > 0) {
            (*Object).prevCollum = (*Object).nextCollum;
        }
        // If moving up
        else {
            (*Object).nextCollum = (*Object).prevCollum;
        }
    }
    // If not in a collum
    else {
      // If moving to the right
      if ((*Object).delta > 0) {
          // Loops through every possible intersection in current row starting at the previous nextCollum
          for (i = (*Object).prevRow + (((*Object).prevCollum + 1) * Map.numOfRows); i < (*Object).prevRow + ((Map.numOfCollums) * Map.numOfRows); i += Map.numOfRows) {
              // If possible intersection is an intersection
              if (*(Map.yMovement + i) != 0) {
                  (*Object).nextCollum = i / Map.numOfRows;
                  break;
              }
          }
      }
      // If moving to the left
      else {
        // Loops through every possible interseciton in current row starting at the previous prevCollum
        for (i = (*Object).nextRow + (((*Object).prevCollum - 1) * Map.numOfRows) ; i >= (*Object).prevRow; i -= Map.numOfRows) {
            // If possible intersection is an intersection...
            if (*(Map.yMovement + i) != 0) {
                (*Object).prevCollum = i / Map.numOfRows;
                break;
            }
        }
      }
    }
}

// Updates the score
void updateScore() {
  // If score has changed
  if (prevScore != score) {
      // Reprints the score and updates it
      tft.setTextSize(1);
      tft.setTextColor(0xFFFF, 0x0000);
      tft.setCursor(50, 0);
      tft.print(score);
      prevScore = score;
  }
}

// Main function that updates everything for the sprite
void updateSprite(sprite* Object) {
    // If a ghost
    if (Object != &PacMan) {
        // Loops for all ghosts
        for (k = 0; k < menu.numOfGhosts; k++) {
            updateCursor(Object + k);
            updateOther(Object + k);
      }
    }
    // If PacMan
    else {
        updateCursor(Object);
        updateOther(Object);
    }
}
