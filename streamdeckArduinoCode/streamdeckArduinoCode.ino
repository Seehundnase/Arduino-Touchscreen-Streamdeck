#include <Elegoo_GFX.h>
#include <Elegoo_TFTLCD.h>
#include <TouchScreen.h>

// TFT Pins
#define LCD_CS A3
#define LCD_CD A2
#define LCD_WR A1
#define LCD_RD A0
#define LCD_RESET A4

// Touch Pins
#define YP A3
#define XM A2
#define YM 9
#define XP 8

//kalibrierung
#define TS_MINX 120
#define TS_MAXX 900
#define TS_MINY 70
#define TS_MAXY 920
#define MINPRESSURE 3
#define MAXPRESSURE 1000

// Farben
#define	BLACK    0x0000
#define	BLUE     0x001F
#define	RED      0xF800
#define	GREEN    0x07E0
#define CYAN     0x07FF
#define MAGENTA  0xF81F
#define YELLOW   0xFFE0
#define WHITE    0xFFFF
#define GREY     0x8410
#define ORANGE   0xd401
#define PURPLE   0x985a
#define BLUEPURPLE 0x615f
#define BEIGE    0xacae
#define DARKBLUE 0x316d
#define BROWN    0x7a65
#define DEEPBLUE 0x3176
#define GREYBLUE 0x7395
#define GREYGREEN 0x7d0f
//initialisieren des tft
Elegoo_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

//Buttons definieren
Elegoo_GFX_Button playButton;
Elegoo_GFX_Button skipButton;
Elegoo_GFX_Button prevButton;
Elegoo_GFX_Button volupButton;
Elegoo_GFX_Button voldownButton;
Elegoo_GFX_Button volumeLabel;
Elegoo_GFX_Button calculator;
Elegoo_GFX_Button appsMenu;
Elegoo_GFX_Button browserButton;
Elegoo_GFX_Button explorerButton;
Elegoo_GFX_Button deezerButton;
Elegoo_GFX_Button discordButton;
Elegoo_GFX_Button youtubeButton;
Elegoo_GFX_Button appsMenuBack;
//variablen für den code
//zeit-anzeige
char buffer[16];
int len;
//menu-mamagement
bool inAppMenu = 0;

char trackBuffer[200];  // Puffer für Trackname
int trackLen = 0;

void setup() {

  Serial.begin(9600);//serielle schnittstelle, um mit dem PC zu kommunizieren
  
  tft.reset();
  //findet den richtigen treiber
  uint16_t identifier = tft.readID();
  if(identifier==0x0101)
  {     
    identifier=0x9341;
  }
  else 
  {
    identifier=0x9341;
  }
  //touchscreen starten
  tft.begin(identifier);
  tft.setRotation(1);
  tft.fillScreen(0x0000); // Schwarz

  //BUTTONS------------------------------------------
  //media buttons
  playButton.initButton(&tft, 195, 155, 80, 40, BROWN, ORANGE, WHITE, "Play", 2);
  skipButton.initButton(&tft, 260, 155, 40, 40, BROWN, ORANGE, WHITE, ">", 2);
  prevButton.initButton(&tft, 130, 155, 40, 40, BROWN, ORANGE, WHITE, "<", 2);
 //volume control buttons
  volupButton.initButton(&tft, 150, 210, 40, 40, DARKBLUE, DEEPBLUE, WHITE, "+", 2);
  voldownButton.initButton(&tft, 105, 210, 40, 40, DARKBLUE, DEEPBLUE, WHITE, "-", 2);
  volumeLabel.initButton(&tft, 195, 210, 40, 40, DARKBLUE, RED, WHITE, "X", 2);
  //APPS buttons
  appsMenu.initButton(&tft, 270, 210, 80, 40, BLACK, GREEN, BLACK, "Apps", 2);
  browserButton.initButton(&tft, 60, 46, 100, 40, BLACK, ORANGE, BLACK, "Browser", 2);
  explorerButton.initButton(&tft, 165, 91, 100, 40, BLACK, YELLOW, BLACK, "Files", 2);
  deezerButton.initButton(&tft, 165, 46, 100, 40, BLACK, PURPLE, WHITE, "Deezer", 2);
  youtubeButton.initButton(&tft, 60, 136, 100, 40, BLACK, RED, WHITE, "YouTube", 2);
  discordButton.initButton(&tft, 165, 136, 100, 40, BLACK, BLUEPURPLE, WHITE, "Discord", 2);
  calculator.initButton(&tft, 60, 91, 100, 40 , BLACK, BLUE, WHITE, "Calc", 2);
  //menü zeichnen
  drawMenu();
  
}
void loop() {

  TSPoint p = ts.getPoint();

  // Pins (wieder) auf OUTPUT für TFT 
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  int x = -1, y = -1;
  if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
  x = tft.width() - map(p.y, TS_MINY, TS_MAXY, 0, tft.width());  // Y des Touchscreens → X des TFT (spiegeln)
  y = tft.height() - map(p.x, TS_MINX, TS_MAXX, 0, tft.height()); // X → Y (gespiegelt)
  }
  if(!inAppMenu){//sichergehen, dass buttons nur dann gedrückt werden wenn das jeweilige menü angezeigt wird
    checkButtonPressed(playButton, x, y, "PLAY");
    checkButtonPressed(skipButton, x, y, "SKIP");
    checkButtonPressed(prevButton, x, y, "PREVIOUS");

  }
  else{
    checkButtonPressed(browserButton, x, y, "BROWSER");
    checkButtonPressed(deezerButton, x, y, "DEEZER");
    checkButtonPressed(explorerButton, x, y, "EXPLORER");
    checkButtonPressed(calculator, x, y, "CALC");
    checkButtonPressed(youtubeButton, x, y, "YOUTUBE");
    checkButtonPressed(discordButton, x, y, "DISCORD");
  }
  checkButtonPressed(volupButton, x, y, "VOLUP");
  checkButtonPressed(voldownButton, x, y, "VOLDOWN");
  checkButtonPressed(volumeLabel, x, y, "MUTE");
  checkAppButtonPressed(appsMenu, x, y);
  //Signale vom PC empfangen
  handleSerial();
}

void drawMenu(){ 
  //media
  tft.fillRect(0, 21, 320, 219, BLACK);
  tft.fillRoundRect(35, 130, 250, 50, 15, BEIGE);
  tft.setTextSize(2);
  tft.setCursor(40, 148);
  tft.setTextColor(YELLOW);
  tft.print("Media");
  playButton.drawButton(); 
  skipButton.drawButton(); 
  prevButton.drawButton(); 

  //Volume
  tft.fillRoundRect(5, 185, 215, 50, 15, GREYBLUE);
  tft.setCursor(10, 200);
  tft.setTextColor(CYAN);
  tft.print("Volume");
  volupButton.drawButton();
  voldownButton.drawButton();
  volumeLabel.drawButton();
  //APPS button
  tft.fillRoundRect(225, 185, 90, 50, 15, GREYGREEN);
  appsMenu.drawButton();
  //trackname
          tft.fillRoundRect(5, 21, 310, 104, 15, GREY);
      drawWrappedText(trackBuffer + 6,
                10,   // abstand Links
                32,   // Start-Y
                310,  // rechtes Limit
                WHITE,
                2);   // Textgröße
}

void drawAppMenu(){
    tft.fillRoundRect(5, 21, 310, 159, 15, GREY);
    browserButton.drawButton();
    deezerButton.drawButton();
    explorerButton.drawButton();
    youtubeButton.drawButton();
    discordButton.drawButton();
  calculator.drawButton();
}
void handleSerial() {
  if (Serial.available()) {
    char line[200];
    int len = Serial.readBytesUntil('\n', line, sizeof(line) - 1);
    line[len] = 0; // null-terminieren

    if (strncmp(line, "TIME:", 5) == 0) {
      // Zeit updaten
      tft.fillRect(0, 0, 100, 21, BLACK);  // alte zeit überschreiben
      tft.setCursor(0, 0);
      tft.setTextSize(2);
      tft.setTextColor(WHITE);
      tft.print(line + 5);  // nur nach "TIME:"
    }   
    else if (strncmp(line, "CONNECTION:", 11) == 0) {
      // Zeit updaten
      tft.fillRect(100, 0, 220, 21, BLACK);   
      tft.setCursor(100, 0);
      tft.setTextSize(2);
      tft.setTextColor(WHITE);
      tft.print(line + 11);  // nur nach "CONNECTION:"
    }
      else if (strncmp(line, "CLIP:", 5) == 0) {
      if(inAppMenu == false){
        tft.fillRoundRect(5, 21, 310, 104, 15, GREY);
        tft.setCursor(10, 24);
        tft.setTextColor(WHITE);
        tft.print("Clipboard:");
        drawWrappedText(line + 5,
                10,   // abstand Links
                35,   // Start-Y
                310,  // rechtes Limit
                WHITE,
                2);   // Textgröße
      }
    }
    else if (strncmp(line, "TRACK:", 6) == 0) {
      // Trackname updaten
      strncpy(trackBuffer, line, sizeof(trackBuffer)); //namen speichern in puffer
    trackBuffer[sizeof(trackBuffer) - 1] = '\0';  // sicherstellen, dass es keinen overflow gibt

      if(inAppMenu == false)
      {
        tft.fillRoundRect(5, 21, 310, 104, 15, GREY);
      drawWrappedText(line + 6,
                10,   // abstand Links
                28,   // Start-Y
                310,  // rechtes Limit
                WHITE,
                2);   // Textgröße
      }
    }
  }
}
void checkButtonPressed(Elegoo_GFX_Button &btn, int x, int y, const char* action) {

    btn.press(btn.contains(x, y));//wenn der druckpunkt im button liegt, ist btn.contains true

    if (btn.justPressed()) {
        btn.drawButton(true);       // invert color
        Serial.println(action);     // perform action
        delay(300);                 // simple debounce
    }

    if (btn.justReleased()) {
        btn.drawButton();           // reset color
    }
}
void checkAppButtonPressed(Elegoo_GFX_Button &btn, int x, int y) {
    // Update button pressed state


    btn.press(btn.contains(x, y));//wenn der druckpunkt im button liegt, ist btn.contains true

    if (btn.justPressed()) {
        btn.drawButton(true);       // invert color
        if(inAppMenu == false){
          drawAppMenu();
          inAppMenu = true;
        }
        else{drawMenu();inAppMenu=false;}
        delay(500);                 // simple debounce
    }

    if (btn.justReleased()) {
        btn.drawButton();           // reset color
    }
}
//Hilfsfunktion, um zeilenumbrüche zu handeln
void drawWrappedText(const char* text, int startX, int startY, int maxX, uint16_t color, int textSize) {
  int cursorX = startX;
  int cursorY = startY;
  int charWidth = 6 * textSize;   // Standardbreite pro Zeichen (Font 5x7 +1 Pixel)
  int charHeight = 8 * textSize;  // Standardhöhe
  int printedChars = 0;
  int maxChars = 120;

  tft.setTextSize(textSize);
  tft.setTextColor(color);
  tft.setCursor(cursorX, cursorY);

  for (int i = 0; text[i] != 0; i++) {
    char c = text[i];

    // Prüfen, ob nächstes Zeichen noch ins Limit passt
    if (cursorX + charWidth > maxX) {
      cursorX = startX;              // links neu anfangen
      cursorY += charHeight + 2;     // neue Zeile
      tft.setCursor(cursorX, cursorY);
    }

    tft.print(c);
    cursorX += charWidth; // nach rechts gehen
    printedChars ++;
    if(printedChars >= maxChars){return;}
  }
}

