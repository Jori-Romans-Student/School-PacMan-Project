README


NAME AND INFORMATION


Jori Romans, 1424535

Section LBL EA2

Final Project "PacMan"


ACCESSORIES


* Arduino Mega Board (1)

* Conductive Breadboard Wires (25)

* LCD Screen (1)

* Joystick (1)

* Serial Port Cable (1)

* Desktop Computer (1)

* Pull-up Resistor (5)

* Potentiometer (1)


WIRING

Positive Breadboard  <—> Arduino 5V Power Supply

Negative Breadboard <—> Arduino GND

TFT Display GND <--> Negative Breadboard

TFT Display VCC <--> Positive Breadboard

TFT Display RESET <--> Arduino Pin 8

TFT Display D/C <--> Arduino Pin 7

TFT Display CARD_CS <--> Arduino Pin 5

TFT Display TFT_CS <--> Arduino Pin 6

TFT Display MOSI <--> Arduino Pin 51

TFT Display SCK <--> Arduino Pin 52

TFT Display MISO <--> Arduino Pin 50

TFT Display Lite <--> Positive Breadboard

Joystick GND <--> Negative Breadboard

Joystick SEL <--> Arduino Pin 9

Joystick HORZ <--> Arduino Pin A1

Joystick VERT <--> Arduino Pin A0

Joystick VCC <--> Positive Breadboard	

Negative Breadboard <--> Potentiometer Neg Pin

Positive Breadboard <--> Potentiometer Pos Pin

Potentiometer Analog Pin <--> Arduino Pin A2

Note: The next 5 must be set up in order on the breadboard...

Negative Breadboard <--> Resistor <--> LED Negative Terminal <--> LED Positive Terminal <--> Arduino Pin 2

Negative Breadboard <--> Resistor <--> LED Negative Terminal <--> LED Positive Terminal <--> Arduino Pin 3

Negative Breadboard <--> Resistor <--> LED Negative Terminal <--> LED Positive Terminal <--> Arduino Pin 4

Negative Breadboard <--> Resistor <--> LED Negative Terminal <--> LED Positive Terminal <--> Arduino Pin 10

Negative Breadboard <--> Resistor <--> LED Negative Terminal <--> LED Positive Terminal <--> Arduino Pin 11


UPLOAD 


Program is uploaded using the command line code found in VmWare Software operated by Linux. First the file containing the program (Assignment-01a) must be made the directory. To do this, type cd followed by the location of the program folder into the command line tool. From there, ensuring the Arduino is connected to the computer, and the second Arduino connected to the first Arduino via conductive breadboard wires, type “make upload” into the command line. This will initiate the make file found in the program folder, and will upload the program to the Arduino for use.  


USE

Program allows the user to play the classic arcade game PacMan on the LCD screen. The LCD screen opens to a menu with choice of one player or custom. The menu can be scrolled through and each option selected by pressing down on the joystick. If custom is selected, a custom menu will be opened allowing the user to change certain values (PacMan color, number of ghosts, difficulty, amount of starting lives and map) and will start the custom game when select is pushed (NOTE: Due to time constraints, multiple maps were not able to be loaded, however looking through the code it can be seen that creating other maps would have been very easy to install). Once in the main game, the user can move pacman around the maze within the walls. Tunnel walls going outside the map can be used to transport pacman from one side to the other. PacMan will collect points by eating dots found in the maze, increasing his score per dot. At a score of 3000, PacMan will earn another life, displayed at the bottom of the screen. He will earn additonal one ups every 5000 points after this. The level is completed when all of the dots have been eaten, in which case a new level will be loaded. There are ghosts (4 in one player game) that chase pacman to varying extents depending on difficulty. If a ghost catched pacman, he loses a life, and the player/ghosts start in their starting positions. One notable change from this version vs the arcade version is that the ghosts eat the dots, competing with PacMan for points. If a ghost eats a dot, PacMan will no longer be able to eat or get points from that dot. Another notable change is that PacMan cannot eat the ghosts by eating the big dots. The big dots do still give extra points (50 to be exact) however this was not added in due to time constraints. When all of PacMans lives have been expended, the game quits and restarts at the main menu

ACKNOWLEDGMENTS

This program was developed by Jori Romans. Specific code functions regarding the use of the joystick were developed using ideas found in class. The Pacman maze is the one used in the actual PacMan game. All other ideas came from Jori Romans

