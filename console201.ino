///////////////
// Sketch:        Console201      26 feb 2018  
///////////////

// #define TCH_FT6206              1    // new touch, rem -> old
#define P100                  300    // пауза при таче

#include <TFT_ILI9341.h> // Hardware-specific library
#include <SPI.h>
#include <Wire.h>        // Wire header file for I2C and 2 wire
#include <EEPROM.h>

class TFT_ILI9341_Button {
 public:
  TFT_ILI9341_Button(void);
  // New/alt initButton() uses upper-left corner & size
  void initButtonUL(TFT_ILI9341 *gfx, int16_t x1, int16_t y1,
    uint16_t w, uint16_t h, uint16_t outline, uint16_t fill,
    uint16_t textcolor, char *label, uint8_t textsize);
  void drawButton(boolean inverted = false);
  boolean contains(int16_t x, int16_t y);

  void press(boolean p);
  boolean isPressed();
  boolean justPressed();
  boolean justReleased();

 private:
  TFT_ILI9341  *_gfx;
  int16_t       _x1, _y1; // Coordinates of top-left corner
  uint16_t      _w, _h;
  uint8_t       _textsize;
  uint16_t      _outlinecolor, _fillcolor, _textcolor;
  char          _label[10];

  boolean currstate, laststate;
};

#if defined( TCH_FT6206 )  
   #include <Adafruit_FT6206.h>
#else     
   #include "TouchScreen.h"
#endif

//         Отладка 
// #define CON_DEBUG_        1
// #define CON_DEBUG_PRESS   1
// #define CON_DEBUG_TEMP    1
// #define CON_DEBUG_LED     1 

// I used a Sensor PCB from a scrapped White Intel based iMac
// Note there are two sensor boards both with a TMP75 sensor in the iMac.

// TMP75 Address is 0x48 and from its Datasheet = A0,A1,A2 are all grounded.
int TMP75_Address = 0x48; // Apple iMac Temp Sensor Circular PCB
// int TMP75_Address = 0x49; // Apple iMac Temp Sensor Square PCB

// Declare some nice variables
int numOfBytes = 2;

#define YP A3  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!
#define YM A1   // can be a digital pin
#define XP A0   // can be a digital pin

// For better pressure precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// For the one we're using, its 300 ohms across the X plate

#if defined( TCH_FT6206 )  
  // The FT6206 uses hardware I2C (SCL/SDA)
  Adafruit_FT6206 ctp = Adafruit_FT6206();
  
  // This is calibration data for the raw touch data to the screen coordinates

  #define TS_MINX  18
  #define TS_MINY   4
  #define TS_MAXX 232
  #define TS_MAXY 310 
#else  
  TouchScreen tscr = TouchScreen(XP, YP, XM, YM, 300); 
   
  // This is calibration data for the raw touch data to the screen coordinates

  #define TS_MINX 190   
  #define TS_MINY 888   
  #define TS_MAXX 842   
  #define TS_MAXY 119     
#endif

#define MINPRESSURE 10
#define MAXPRESSURE 1000

// For the Adafruit shield, these are the default.
/*
#define TFT_DC 9    //PB1
#define TFT_CS 10   //PB2
#define TFT_MOSI 11 //PB3
#define TFT_CLK  13 //PB5
#define TFT_RST 4   //PD4
#define TFT_MISO 12 //PB4
*/

int LED = 7;        //Подсветка

//   Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
//   Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
//   If using the breakout, change pins as desired
//   Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

TFT_ILI9341 tft = TFT_ILI9341();

int But1    = 3;     //SS  PD3 Кнопка ON
int But2    = 2;     //SS  PD2 Кнопка OFF
int Setout  = 6;     //SS PD6 Выход установки температуры
int Tempout = 5;     //SS PD5 Выход датчика температуры

// My

#define VBXI     19
#define VBYI     280
#define VBXIW    80
#define VBYIH    32
#define VBID     3
char VBIS[] =     "I";
//
#define VBXO     140
#define VBYO     280
#define VBXOW    80
#define VBYOH    32
#define VBOD     3
char  VBOS[] =     "O";
//
#define TRPX1    20
#define TRPX2    98
#define TRMX1    141
#define TRMX2    219
#define TRY1     200
#define TRY2     256
#define TRD      3
char  TRP[] =      "+";
char  TRM[] =      "-";

int   lc    =    0;   //  0 - France,   1 - English 
char  latext[2][4] = { "Fr", "En" };

int   tn = 0;         //  0 - C,  1 - F 
char  ttsp[2][4] = { " C", " F" };
int   tc[2] =      {202, 690};      // current temp                   
int   tp[2] =      {220, 715};      // setpoint temp

int   steps[2] =   { 5, 10 };      // step C = 0.5 (old 1.0),  F = 1.0 (old 1.5)

uint16_t clr_last = ILI9341_BLACK;

unsigned long cc1 = 0L;              // current time

#if defined( CON_DEBUG_TEMP )
   int   tminmax[2][2] = { { 99, 0 },          // Для отладки
                           { 99, 0 } };        // F
   char  spl[2][2][4] = { { "99", "00" },      // Для отладки
                          { "99", "00" } };    // F  max - min   menu
#else     
   int   tminmax[2][2] = { { 30, 15 },         // C
                           { 86, 59 } };       // F    
   char  spl[2][2][4] = { { "26", "18" },      // C  max - min   menu
                          { "79", "64" } };    // F  max - min   menu                                                   
#endif

// Buttons

#define BUTTONS      5
#define BUTTON_PLUS  0
#define BUTTON_MINUS 1
#define BUTTON_ON    2
#define BUTTON_OFF   3
#define BUTTON_MENU  4

TFT_ILI9341_Button buttons[BUTTONS+1];

//-- Setup

void setup(void) {

  pinMode(But1, OUTPUT);    //SS Назначаем на вывод
  pinMode(But2, OUTPUT);    //SS Назначаем на вывод
  digitalWrite(But1,LOW);   //SS На выходе 0
  digitalWrite(But2,LOW);   //SS На выходе 0  
  
  ttsp[0][0] = char(247);
  ttsp[1][0] = char(247);

  setup_Temp();
  loop_Temp();           // current temp
  
  #if defined( CON_DEBUG_ )
    Serial.begin(9600);
    Serial.println("ILI9341 Test!"); 
  #endif
  
  //  pinMode(LED, OUTPUT);
  //  digitalWrite(LED, HIGH); //Подсветка включена
  led_setup();
  
  // Inicialize the controller     
  
  tft.begin();

  tft.setRotation(2); // Need for the Mega, please changed for your choice or rotation initial

#if defined( TCH_FT6206 )  
  if (! ctp.begin(40)) {             // pass in 'sensitivity' coefficient
     #if defined( CON_DEBUG_PRESS ) 
        Serial.println("Couldn't start FT6206 touchscreen controller");
     #endif
     // while (1);
  }
#else    
  // empty
#endif

  initializeButtons();
  
  // Initial screen

  read_memory();
  init_Screen();

  cc1 = millis();    
}


// -- Loop

int  ctempo   = 1001;   // Last cur. temp
int  cnt_Menu =    0;

//  Управление подсветкой
unsigned long maxPause = 30000L;
unsigned long ledTime;
bool ledFlag;

void led_setup() {
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH); //Подсветка включена
  ledTime = millis();
  ledFlag = true;
}

void led_loop(int i){
   unsigned long t = millis();
   if( i == 1 ) {
      digitalWrite(LED, HIGH); //Подсветка включена
      ledTime = t;
      cc1 = t;
      ledFlag = true;
   } else if( ledFlag ) {
      if( t < ledTime || t < cc1 )
         cc1 = ledTime = t;
      else {
         if( t > (cc1 + maxPause) ) {
           digitalWrite(LED, LOW); //Подсветка выкл
           ledTime = cc1 = t;
           ledFlag = false;   
           save_memory();          // возможно были изменения setpoint !...
         }
      }
   }   
}  

void loop()
{  
  #if defined( TCH_FT6206 )  
    TS_Point p;
  #else    
    TSPoint p;  
  #endif
  
  int     Set_norm  = tp[tn];
  int     Temp_norm = tc[0];    //SS
  char    lines[10];
  
  loop_Temp();           // current temp
  if ( (ctempo - tc[tn]) != 0 ) {
      Circles1(1, 120, 95, 90); 
      ctempo = tc[tn];
  }    
  
  #if defined( CON_DEBUG_TEMP )
      Serial.print("Start Temp_norm = ");  Serial.print(tc[tn]);
      Serial.print(" - Set_norm  = ");     Serial.println(tp[tn]);
  #endif
  if( tn == 1 ) Set_norm = (int)(((Set_norm - 320) * 5.0 / 9.0)); 
  Temp_norm = map (Temp_norm, 0, 1000, 0, 255);  //SS Нормирование значения температуры  
  Set_norm  = map (Set_norm, 0, 1000, 0, 255); 
  #if defined( CON_DEBUG_TEMP )
      Serial.print("MAP.  Temp_norm = "); Serial.print(Temp_norm);
      Serial.print(" - Set_norm  = ");    Serial.println(Set_norm);
  #endif
  analogWrite(Setout, Set_norm);             //SS
  analogWrite(Tempout, Temp_norm);           //SS
    
  // Wait a touch
  
  p = waitOneTouch();

  if ( !ledFlag && buttons[5].contains(p.x, p.y)) {     // экран выкл
     led_loop(1);                                       // и нажатие есть --> включаем ...
     p.x = p.y = 0;     
  }  
   
  // Buttons
  // Go thru all the buttons, checking if they were pressed
  
  for (uint8_t b=0; b<BUTTONS; b++) {

    if (buttons[b].contains(p.x, p.y)) {

        // Action

        cc1 = millis();    // время последнего нажатия

        switch  (b) {

          case BUTTON_PLUS:

              // PLUS

              cnt_Menu = 0;
              // Serial.println("\tPressure = PLUS");
              if ( (tp[tn] += steps[tn]) > (atoi(spl[tn][0]) * 10))
                 tp[tn] -= steps[tn];
           
              delay(P100);
              Circles1(2, 120, 95, 90);

              break;
              
          case BUTTON_MINUS:

              // MINUS
              
              cnt_Menu = 0;
              // Serial.println("\tPressure = MINUS");
              if ( (tp[tn] -= steps[tn]) < (atoi(spl[tn][1]) * 10))
                 tp[tn] += steps[tn];
              
              delay(P100);
              Circles1(2, 120, 95, 90);
              
              break;
              
          case BUTTON_ON:

              // ON
              
              cnt_Menu = 0;
              // Serial.println("\tPressure =    ON");
              
              buttons[2].drawButton(1);      // inverted
              digitalWrite(But1,HIGH);       //SS высокий уровень на 1 сек 
              delay(1000);
              
              buttons[2].drawButton(0);
              digitalWrite(But1,LOW);        //SS низкий уровень постоянно
              break;
              
          case BUTTON_OFF:

              // OFF

              cnt_Menu = 0;
              // Serial.println("\tPressure =   OFF");
              
              buttons[3].drawButton(1);      // inverted
              digitalWrite(But2,HIGH);       //SS высокий уровень на 1 сек
              delay(1000);
              
              buttons[3].drawButton(0);
              digitalWrite(But2,LOW);        //SS низкий уровень постоянно
              
              break;

            case BUTTON_MENU:

              // Serial.println("\tPressure = MENU ...");

              if( ++cnt_Menu > 3 ) {
                 mk_Menu();
                 save_memory();     // сохранение всех изменений
                 init_Screen();
                 cc1 = millis();    // время последнего нажатия menu
                 cnt_Menu = 0;
              }   
                          
              break;   
        }
    }
  }
  led_loop(0);         // Проверка- может пора выключать ...
}

void init_Screen() {

  tft.fillScreen(ILI9341_BLACK);

  clr_last = ILI9341_BLACK;
  Circles1(0, 120, 95, 90); 

  Triangles2(TRP, (TRPX1+TRPX2)/2, TRY1, TRPX1, TRY2, TRPX2, TRY2, TRD);
  Triangles2(TRM, (TRMX2+TRMX1)/2, TRY2, TRMX2, TRY1, TRMX1, TRY1, TRD);

  // Rects2(VBIS, VBXI, VBYI, VBXIW, VBYIH, VBID);
  buttons[2].drawButton(0);         
  // Rects2(VBOS, VBXO, VBYO, VBXOW, VBYOH, VBOD);
  buttons[3].drawButton(0);
}              

// wait 1 touch to return the point 

#if defined( TCH_FT6206 )  
TS_Point waitOneTouch() {
 
    // Retrieve a point  
    TS_Point p;
    p.x = p.y = p.z = 0;

    // Wait for a touch
    if (! ctp.touched()) {
      return p;
    }
    p = ctp.getPoint();

  // we have some minimum pressure we consider 'valid'
  // pressure of 0 means no pressing!
  if (p.z > 0) {
     #if defined( CON_DEBUG_PRESS )
       Serial.print("X = ");   Serial.print(p.x);
       Serial.print("\tY = "); Serial.print(p.y);
       Serial.print("\tPressure = "); Serial.println(p.z);  
     #endif
  
    // we have some minimum pressure we consider 'valid'
    // pressure of 0 means no pressing! 
    
    // Scale from ~0->1000 to tft.width using the calibration #'s
    p.x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
    p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());

    #if defined( CON_DEBUG_PRESS ) 
      Serial.print(" - ("); Serial.print(p.x);
      Serial.print(", ");   Serial.print(p.y);
      Serial.println(")");
    #endif
  }
  return p;
}
#else     
TSPoint waitOneTouch() {
  
    // Retrieve a point  
    TSPoint p;
    p.x = p.y = p.z = 0;
    p = tscr.getPoint();
  
   // we have some minimum pressure we consider 'valid'
   // pressure of 0 means no pressing!
   if (p.z > tscr.pressureThreshhold) {
     #if defined( CON_DEBUG_PRESS )
       Serial.print("X = ");   Serial.print(p.x);
       Serial.print("\tY = "); Serial.print(p.y);
       Serial.print("\tPressure = "); Serial.println(p.z);  
     #endif
  
    // we have some minimum pressure we consider 'valid'
    // pressure of 0 means no pressing! 
    
    if (p.z < MINPRESSURE || p.z > MAXPRESSURE) {
      p.x = p.y = 0;
      return p;
    }
  
    // Scale from ~0->1000 to tft.width using the calibration #'s
    p.x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
    p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());

    #if defined( CON_DEBUG_PRESS ) 
      Serial.print(" - ("); Serial.print(p.x);
      Serial.print(", ");   Serial.print(p.y);
      Serial.println(")");
    #endif
  }
  return p;
} 
#endif

// Initialize buttons

void initializeButtons() {

  uint8_t textSize = 3;

  char buttonlabels[4][2] = {"+", "-", "I", "O"};

  buttons[0].initButtonUL(&tft,                                     // +
                  TRPX1,  TRY1,                                     // x, y,
                  TRPX2-TRPX1, TRY2-TRY1, ILI9341_BLACK, ILI9341_BLACK, ILI9341_WHITE,    // w, h, outline, fill, 
                  buttonlabels[0], textSize);                       // +
 
  buttons[1].initButtonUL(&tft,                                     // -
                  TRMX1,  TRY1,                                     // x, y,
                  TRMX2-TRMX1, TRY2-TRY1, ILI9341_BLACK, ILI9341_BLACK, ILI9341_WHITE,    // w, h, outline, fill, 
                  buttonlabels[1], textSize);                       // -
  
  buttons[2].initButtonUL(&tft,                           // ON
                  VBXI,  VBYI,                            // x, y,
                  VBXIW, VBYIH, ILI9341_WHITE, ILI9341_BLACK, ILI9341_WHITE,    // w, h, outline, fill, 
                  buttonlabels[2], textSize);             // ON
              
  buttons[3].initButtonUL(&tft,                           // OFF
                  VBXO,  VBYO,                            // x, y,
                  VBXOW, VBYOH, ILI9341_WHITE, ILI9341_BLACK, ILI9341_WHITE,    // w, h, outline, fill, 
                  buttonlabels[3], textSize);             // OFF

  buttons[4].initButtonUL(&tft,                           // MENU
                  40,  20,                                // x, y,
                  170, 160, ILI9341_WHITE, ILI9341_BLACK, ILI9341_WHITE,    // w, h, outline, fill, 
                  buttonlabels[3], textSize);             // 

   buttons[5].initButtonUL(&tft,                           // MENU
                  1,  1,                                   // x, y,
                  238, 318, ILI9341_WHITE, ILI9341_BLACK, ILI9341_WHITE,    // w, h, outline, fill, 
                  buttonlabels[3], textSize);             // 
}

// my func

void Triangles2(char b[], int cxp, int cyp, int cxl, int cyl, int cxr, int cyr, int cd) {

  int j = (cxp<120)? (+1) : (-1);   
  
  for(int i=0; i<cd; i++) {
    tft.drawTriangle(
      cxp,      cyp+i*j,        // peak
      cxl+i*j,  cyl-i*j,        // bottom left
      cxr-i*j,  cyr-i*j,        // bottom right
      (cxp<120)? ILI9341_RED : ILI9341_GREEN );
  }
  if (cxp < 120) 
     tft.setCursor(50, 221);
  else
     tft.setCursor(170, 206);
  tft.setTextColor((cxp < 120)? ILI9341_RED : ILI9341_GREEN);  
  tft.setTextSize(4);
  tft.print(b);
    
}

void Circles1(int cf, int cx, int cy, uint8_t radius) {

  char  ctext[2][2][18] = {  { " Temperature", " Point de cosigne" }, 
                             { " Temperature", "     Setpoint"    } };
  char lines[8];                    
  uint16_t clr;
  boolean  fscr = false;

  ctext[0][0][5] = char(130);  

  if( tc[tn] < (tp[tn] - steps[tn]) ) 
     clr = ILI9341_BLUE;
  else if( tc[tn] > (tp[tn] + steps[tn]) ) 
     clr = ILI9341_RED;
  else
     clr = ILI9341_GREEN;
    
  if( clr != clr_last ) {       
    if( clr_last != ILI9341_BLACK ) tft.fillRect(0, 0, 239, 190, ILI9341_BLACK);
    fscr = true;
    for(int i=0; i<3; i++) {
      tft.drawCircle(cx, cy, radius-i, clr);
    }
    clr_last = clr;
  }  

  if( (cf == 0) || fscr ) {
    tft.setCursor(80, 35);
    tft.setTextColor(ILI9341_WHITE);  
    tft.setTextSize(1);
    tft.setCursor(80, 35);
    tft.print(ctext[lc][0]);
  }

  if( !fscr) tft.fillRect(46, 50, 144, 28, ILI9341_BLACK);
  tft.setCursor(50, 50);
  tft.setTextColor(clr);  
  tft.setTextSize(4);
  dtostrf( (0.1 * tc[tn]), 3, 1, lines );
  tft.print(lines);
  tft.print(ttsp[tn]);
  
  if( (cf == 0) || fscr ) {
    tft.setCursor(60, 105);
    tft.setTextColor(ILI9341_WHITE);  
    tft.setTextSize(1);
    tft.print(ctext[lc][1]);
  }

  if( (cf == 0) || (cf > 1) || fscr ) {
    if( !fscr ) tft.fillRect(61, 120, 124, 21, ILI9341_BLACK);
    tft.setCursor(65, 120);
    tft.setTextColor(ILI9341_WHITE);  
    tft.setTextSize(3);
    dtostrf( (0.1 * tp[tn]), 3, 1, lines );
    tft.print(lines);
    tft.print(ttsp[tn]);
  }
}

//     option menu

// Buttons menu

#define MBUTTONS      5
#define MBUTTON_UNIT  0
#define MBUTTON_LANG  1
#define MBUTTON_HI    2
#define MBUTTON_LOW   3
#define MBUTTON_OK    4

#define MX1   20
#define MY1   60
#define MW    80
#define MH    80
#define MSW   40
#define MSH   30
#define MX2   80
#define MY2   264
#define MHOK  32

// char    TXTOK[] = "OK";  = 1
// char    TXTCL[] = "CL";  = 2 
char  txtc[12][3] = { "0", "OK", "CL", "1", "2", "3", "4", "5", "6", "7", "8", "9" };

TFT_ILI9341_Button mbuttons[MBUTTONS];

void init_Menu() {

  uint8_t     kx[4] = {0,1,0,1};
  uint8_t     ky[4] = {0,0,1,1};
  uint8_t     textSize = 3;
  const char *mtext[2][4] = { { "  Unit", "   Langue",   "PC haute limite", "PC basse limite" },
                              { "  Unit",  "  Language",  "  SP Hi limit",   " SP low limit" } };
  char        s[16];

  tft.fillScreen(ILI9341_BLACK);

  tft.setCursor( 20, 5 );
  tft.setTextColor(ILI9341_WHITE);  
  tft.setTextSize(2);
  tft.print( "DAP Controls Inc");
  tft.setCursor( 20, 25 );
  tft.print( "   DAP-TSP-2B");
  
  for( int i = 0; i < (MBUTTONS - 1); i++ ) {
      strcpy(s,mtext[lc][i]);
      tft.setCursor(MX1 + (MW+MSW)*kx[i] + 2,  MY1 + (MH+MSH)*ky[i] - 12);
      tft.setTextColor(ILI9341_WHITE);  
      tft.setTextSize(1);
      tft.print(s);
      if( (lc == 0) && (i == 0) ) tft.print( char(130) );

      strcpy(s,mtext[lc][i]);
      mbuttons[i].initButtonUL(&tft,                             
                  MX1 + (MW+MSW)*kx[i],  MY1 + (MH+MSH)*ky[i],               // x, y,
                  MW,   MH,  ILI9341_WHITE, ILI9341_BLACK, ILI9341_WHITE,    // w, h, outline, fill, 
                  (i==0)? ttsp[tn] : (i==1)? latext[lc] : spl[tn][i-2], textSize);                                  
             
  }                
  mbuttons[4].initButtonUL(&tft,                             
                  MX2,  MY2,                                                 // x, y,
                  MW,   MHOK,  ILI9341_WHITE, ILI9341_BLACK, ILI9341_WHITE,  // w, h, outline, fill, 
                  txtc[1], textSize);  
                     
  for( int i = 0; i < MBUTTONS; i++ ) {
      mbuttons[i].drawButton(0);
  }  

  tft.setCursor( 66, 308 );
  tft.setTextColor(ILI9341_WHITE);  
  tft.setTextSize(1);
  tft.print( "APP 1.0     Rev 1.0");
    
}

void mk_Menu()
{
 #if defined( TCH_FT6206 )  
    TS_Point p;
 #else    
    TSPoint p;  
 #endif 

 init_Menu();

 delay( 1000 );
 
 for ( int j = 0; j < 10; ) {
  
  // Wait a touch

  p = waitOneTouch();
  
  // Buttons
  // Go thru all the buttons, checking if they were pressed
  
  for (uint8_t b=0; b<MBUTTONS; b++) {
    
    if (mbuttons[b].contains(p.x, p.y)) {

        // Action

        mbuttons[b].drawButton(1);
        delay(P100);
        mbuttons[b].drawButton(0);

        switch  (b) {

          case MBUTTON_UNIT:

              tn = (tn == 0)? 1 : 0;
              // mbuttons[b].drawButton(0);
              init_Menu();
              
              break;
              
          case MBUTTON_LANG:

              lc = (lc ==0)? 1 : 0;
              // mbuttons[b].drawButton(0);
              init_Menu();
              
              break;
              
          case MBUTTON_HI:

              // HI
              
              strncpy(spl[tn][0], mk_Consol(spl[tn][0]), 2);
              init_Menu();
              
              break;
      
          case MBUTTON_LOW:

              // LOW
              
              strncpy(spl[tn][1], mk_Consol(spl[tn][1]), 2);
              init_Menu();
              
              break;
              

          case MBUTTON_OK:

              return;
              
        }
    }
  }  
}
}

// Buttons control

#define CBUTTONS      12
#define CBUTTON_0     0
#define CBUTTON_OK    1
#define CBUTTON_CL    2
#define CBUTTON_1     3
#define CBUTTON_2     4
#define CBUTTON_3     5
#define CBUTTON_4     6
#define CBUTTON_5     7
#define CBUTTON_6     8
#define CBUTTON_7     9
#define CBUTTON_8     10
#define CBUTTON_9     11

#define CCX1   30
#define CCY1   10
#define CCW    60
#define CCH    60

TFT_ILI9341_Button cbuttons[CBUTTONS];

int   nc = 0;
int   km = 0;
char  mbufs[8];

void Monitor(uint16_t textcolor) {
   // tft.fillRect(CCX1+40, CCY1+11, 120, 47, ILI9341_BLACK);
   tft.fillRect(CCX1+4, CCY1+4, 3*CCW-10, CCH-10, ILI9341_BLACK);
   tft.setCursor( CCX1+41, CCY1+12 );
   tft.setTextColor(textcolor);  
   tft.setTextSize(4);
   tft.print( mbufs );
   if( km == 132 ) tft.print( "-km" );
}

char *mk_Consol(char s[])   //   2 - HI,    3 - LOW
{  
  #if defined( TCH_FT6206 )  
    TS_Point p;
  #else    
    TSPoint p;  
  #endif
  
  int k;
  char ss[4];

  sprintf(mbufs, "%2s %2s", s, ttsp[tn]);
  
  init_Consol();

  Monitor(ILI9341_WHITE);

  km = 0;
  for ( int j = 0; j < 10; ) {
  
   // Wait a touch

   p = waitOneTouch();
  
   // Buttons
   // Go thru all the buttons, checking if they were pressed
  
   for (uint8_t b=0; b<CBUTTONS; b++) {
    
    if (cbuttons[b].contains(p.x, p.y)) {

        // Action

        cbuttons[b].drawButton(1);
        delay(P100);
        cbuttons[b].drawButton(0);
        

        switch  (b) {
  
          case CBUTTON_OK:
              k = atoi( &mbufs[0] );
              if( (tminmax[tn][0] >= k) && (tminmax[tn][1] <= k) )
                  return ( &mbufs[0] ); 
              Monitor(ILI9341_RED);   
              break;    
              
          case CBUTTON_CL:

              mbufs[0] = '0';
              mbufs[1] = '0';
              nc = 0;
              Monitor(ILI9341_WHITE);
              break;
              
          case CBUTTON_0:

              // 0
              
              mbufs[nc] = '0';
              nc = (nc == 0)? 1 : 0;
              Monitor(ILI9341_WHITE);
              break;
      
          case CBUTTON_1:
          case CBUTTON_2:
          case CBUTTON_3:
          case CBUTTON_4:
          case CBUTTON_5:
          case CBUTTON_6:
          case CBUTTON_7:
          case CBUTTON_8:
          case CBUTTON_9:

              sprintf( ss, "%1d", b-2 );
              mbufs[nc] = ss[0];
              km = (b == 11)? (km+b) : 0;
              nc = (nc == 0)? 1 : 0;
              Monitor(ILI9341_WHITE);
              
              break;
                  
        }
    }
  }  
}

}

void init_Consol() {

  uint8_t textSize = 4;

  tft.fillScreen(ILI9341_BLACK);

  int k = 0;

  for(int i=0; i<2; i++) {
      tft.drawRoundRect(CCX1+i, CCY1+i, 3*CCW-i*2-2, CCH-i*2-2, 2, ILI9341_WHITE);
  }
  
  for( int i = 0; i < 4; i++ ) {
    for( int j = 0; j < 3; j++ ) { 
      // led_loop();
      k = i*3+j;
      cbuttons[k].initButtonUL(&tft,                             
                  CCX1 + CCW*j,  CCY1 + CCH*(4-i),                                 // x, y,
                  CCW-2,   CCH-2,  ILI9341_WHITE, ILI9341_BLACK, ILI9341_WHITE,    // w, h, outline, fill, 
                  txtc[k], textSize);                                  
    }  
  }              
                      
  for( int i = 0; i < CBUTTONS; i++ ) {
      cbuttons[i].drawButton(0);
  }  

}

// I used a Sensor PCB from a scrapped White Intel based iMac

// Setup the Arduino LCD (Columns, Rows)

void setup_Temp(){  
  Wire.begin();                       // Join the I2C bus as a master
  
  Wire.beginTransmission(TMP75_Address);       // Address the TMP75 sensor
  Wire.write(1);                               // 
  Wire.write((byte)0);                         //  
  Wire.endTransmission();                      // Stop transmitting
  
  Wire.beginTransmission(TMP75_Address);       // Address the TMP75 sensor
  Wire.write((byte)0);                         //  
  Wire.endTransmission();                      // Stop transmitting 
}

void loop_Temp(){
   Wire.requestFrom(TMP75_Address,numOfBytes);  // Address the TMP75 and set number of bytes to receive
   byte MostSigByte  = Wire.read();             // Read the first byte this is the MSB
   byte LeastSigByte = Wire.read();             // Now Read the second byte this is the LSB

   // Being a 12 bit integer use 2's compliment for negative temperature values
   int TempSum = (((MostSigByte << 8) | LeastSigByte) >> 4); 
   // From Datasheet the TMP75 has a quantisation value of 0.0625 degreesC per bit
   float temp = (TempSum*0.0625);
 
   tc[0] = (int) (temp*10);                   // C
   tc[1] = (int) (((temp * 1.8) + 32)*10);    // F

   #if defined( CON_DEBUG_TEMP )
       // Serial.print("tc[0] = "); Serial.print(tc[0]); //SS
       Serial.print(" Temp = "); Serial.print(temp);
       Serial.print("\tC = ");   Serial.print(tc[0]*0.1);
       Serial.print("\tF = ");   Serial.println(tc[1]*0.1);  
   #endif

   delay(300);
}

// code for the GFX button UI element

TFT_ILI9341_Button::TFT_ILI9341_Button(void) {
  _gfx = 0;
}

// Newer function instead accepts upper-left corner & size
void TFT_ILI9341_Button::initButtonUL(
 TFT_ILI9341 *gfx, int16_t x1, int16_t y1, uint16_t w, uint16_t h,
 uint16_t outline, uint16_t fill, uint16_t textcolor,
 char *label, uint8_t textsize)
{
  _x1           = x1;
  _y1           = y1;
  _w            = w;
  _h            = h;
  _outlinecolor = outline;
  _fillcolor    = fill;
  _textcolor    = textcolor;
  _textsize     = textsize;
  _gfx          = gfx;
  strncpy(_label, label, 9);
}

void TFT_ILI9341_Button::drawButton(boolean inverted) {
  uint16_t fill, outline, text;

  if(!inverted) {
    fill    = _fillcolor;
    outline = _outlinecolor;
    text    = _textcolor;
  } else {
    fill    = _textcolor;
    outline = _outlinecolor;
    text    = _fillcolor;
  }

  uint8_t r = min(_w, _h) / 4; // Corner radius
  _gfx->fillRoundRect(_x1, _y1, _w, _h, r, fill);
  _gfx->drawRoundRect(_x1, _y1, _w, _h, r, outline);
  _gfx->drawRoundRect(_x1+1, _y1+1, _w-2, _h-2, r, outline);  // double line +

  _gfx->setCursor(_x1 + (_w/2) - (strlen(_label) * 3 * _textsize),
    _y1 + (_h/2) - (4 * _textsize));
  _gfx->setTextColor(text);
  _gfx->setTextSize(_textsize);
  _gfx->print(_label);
}

boolean TFT_ILI9341_Button::contains(int16_t x, int16_t y) {
  return ((x >= _x1) && (x < (_x1 + _w)) &&
          (y >= _y1) && (y < (_y1 + _h)));
}

void TFT_ILI9341_Button::press(boolean p) {
  laststate = currstate;
  currstate = p;
}

boolean TFT_ILI9341_Button::isPressed() { return currstate; }
boolean TFT_ILI9341_Button::justPressed() { return (currstate && !laststate); }
boolean TFT_ILI9341_Button::justReleased() { return (!currstate && laststate); }

//  save memory

void read_memory() {
  if( EEPROM.read(0) == 252 ) {   //  (0) = 252  - флаг записи
    tn    = EEPROM.read(1);
    lc    = EEPROM.read(2);
    tp[0] = EEPROM.read(3);
    tp[1] = EEPROM.read(4);
    EEPROM.get(5,spl);
  }
}

void save_memory() {
  char  s[2][2][4];
  boolean f = false;

  if( EEPROM.read(0) != 252 ) {   //  (0) = 252  - записи еще небыло
     f = true;
  }
  
  EEPROM.update(0,252);
  EEPROM.update(1,tn);
  EEPROM.update(2,lc);
  EEPROM.update(3,tp[0]);
  EEPROM.update(4,tp[1]);

  if( !f ) {
    EEPROM.get(5,s);  
    for( int i=0; i < 2; i++ ) {
      for( int j=0; j < 2; i++ ) {
        if( strcmp(s[i][j], spl[i][j]) != 0 ) 
          f = true;
      }
    }
  }
  if( f ) EEPROM.put(5,spl);
}
