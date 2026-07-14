/*
  ============================================================
   NANO RAIDER
   A raycasted (Wolfenstein-style) FPS for Arduino Nano + tiny
   128x64 I2C OLED (SSD1306) + 4 push buttons + passive piezo.
  ============================================================

  HARDWARE / WIRING
  ------------------------------------------------------------
  OLED (SSD1306, 128x64, I2C):
    VCC -> 5V (or 3.3V - check your module's silkscreen)
    GND -> GND
    SCL -> A5   (Nano hardware I2C clock)
    SDA -> A4   (Nano hardware I2C data)

  IMPORTANT: some cheap "0.96 OLED" modules use the SH1106
  driver instead of SSD1306 - they look identical but need a
  different U8g2 constructor. If the picture is shifted/garbled,
  swap the constructor line below for the SH1106 one (see
  comment next to it).

  Buttons (normally-open, one leg to GND, other leg to pin,
  using the Nano's internal pull-ups - no resistors needed):
    D2 -> Turn Left switch   -> other leg to GND
    D3 -> Turn Right switch  -> other leg to GND
    D4 -> Move Forward switch-> other leg to GND
    D5 -> Fire switch        -> other leg to GND

  Buzzer (passive piezo, 2-pin GND/SIG - confirmed your part):
    SIG -> D9
    GND -> GND

  Bluetooth module (HC-05/HC-06 style, Key/TX/RX):
    Not used in this version. If you add it later, do NOT wire
    it to D0/D1 (that's the USB programming serial port - wiring
    BT there will fight with uploads). Reserve D10 (SoftwareSerial
    RX), D11 (SoftwareSerial TX), D12 (Key) instead - those are
    left free by this sketch.

  Free pins for future extras (vibration motor, sensors, etc.):
    D6, D7, D8, D13, A0-A3, A6, A7

  LIBRARIES REQUIRED (Arduino IDE -> Library Manager):
    "U8g2" by oliver (olikraus)

  ------------------------------------------------------------
  DESIGN NOTES (why things are done this way)
  ------------------------------------------------------------
  The Nano has only 2KB of RAM. The OLED's own framebuffer
  (128*64/8 = 1024 bytes) eats HALF of that before the game
  even starts. Every choice below is driven by that fact:
    - The map is 16x16, packed as one uint16_t per row (1 bit
      per cell) and stored in PROGMEM (flash), not RAM.
    - All trig is done via 1024-entry sin/cos lookup tables in
      PROGMEM - no floating point math anywhere in the main loop.
    - Positions use Q8 fixed-point integers (1 map cell = 256).
    - Enemies are drawn as flat-shaded silhouette boxes, not
      bitmap sprites, to avoid needing sprite scaling code/tables.
    - Wall "shading" by distance is done with a dithered pixel
      pattern (no grayscale on a 1-bit display).

  These constants are starting points - real timing depends on
  your exact hardware, so expect to tune MOVE_SPEED, TURN_SPEED,
  etc. after your first flash.
*/

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

// ---- Display ----
// Standard SSD1306 128x64, no reset pin, hardware I2C:
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
// If your OLED uses the SH1106 driver instead, comment the line
// above and uncomment this one:
// U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// ---- Pins ----
#define PIN_BTN_LEFT    2
#define PIN_BTN_RIGHT   3
#define PIN_BTN_FORWARD 4
#define PIN_BTN_FIRE    5
#define PIN_BUZZER      9

// ============================================================
// MAP  (16x16, 1 = wall, 0 = floor. Verified solvable/connected
// by a maze-generator script - not hand-typed, so it can't be
// a broken/unsolvable maze.)
// ============================================================
#define MAP_SIZE 16
const uint16_t PROGMEM mapData[MAP_SIZE] = {
  0b1111111111111111, // row 0
  0b1000100000000011, // row 1
  0b1110111110101011, // row 2
  0b1010100010101011, // row 3
  0b1010101011101111, // row 4
  0b1010001000100011, // row 5
  0b1011111110101011, // row 6
  0b1000100010101011, // row 7
  0b1011101010111011, // row 8
  0b1000001010000011, // row 9
  0b1011111011111011, // row 10
  0b1000001010001011, // row 11
  0b1111101010101011, // row 12
  0b1000001000100011, // row 13
  0b1111111111111111, // row 14
  0b1111111111111111, // row 15
};

#define START_X 1
#define START_Y 1
#define EXIT_X  13
#define EXIT_Y  13

#define NUM_ENEMIES 4
const int16_t enemyStartX[NUM_ENEMIES] = {13, 1, 9, 8};
const int16_t enemyStartY[NUM_ENEMIES] = { 5, 9, 2, 5};

// ============================================================
// TRIG TABLES - Q8 fixed point (value = table/256), 1024 steps
// per full circle. Angle wraps with "& 1023" (fast, power of 2).
// ============================================================
const int16_t PROGMEM cosTable[1024] = {
  256,256,256,256,256,256,256,256,256,256,256,255,255,255,255,255,
  255,255,254,254,254,254,254,253,253,253,253,252,252,252,252,251,
  251,251,250,250,250,249,249,249,248,248,248,247,247,246,246,245,
  245,245,244,244,243,243,242,242,241,241,240,239,239,238,238,237,
  237,236,235,235,234,233,233,232,231,231,230,229,229,228,227,227,
  226,225,224,224,223,222,221,220,220,219,218,217,216,215,215,214,
  213,212,211,210,209,208,207,207,206,205,204,203,202,201,200,199,
  198,197,196,195,194,193,192,191,190,189,188,186,185,184,183,182,
  181,180,179,178,177,175,174,173,172,171,170,168,167,166,165,164,
  162,161,160,159,157,156,155,154,152,151,150,149,147,146,145,144,
  142,141,140,138,137,136,134,133,132,130,129,128,126,125,123,122,
  121,119,118,117,115,114,112,111,109,108,107,105,104,102,101,99,
  98,97,95,94,92,91,89,88,86,85,83,82,80,79,77,76,
  74,73,71,70,68,67,65,64,62,61,59,58,56,55,53,51,
  50,48,47,45,44,42,41,39,38,36,34,33,31,30,28,27,
  25,24,22,20,19,17,16,14,13,11,9,8,6,5,3,2,
  0,-2,-3,-5,-6,-8,-9,-11,-13,-14,-16,-17,-19,-20,-22,-24,
  -25,-27,-28,-30,-31,-33,-34,-36,-38,-39,-41,-42,-44,-45,-47,-48,
  -50,-51,-53,-55,-56,-58,-59,-61,-62,-64,-65,-67,-68,-70,-71,-73,
  -74,-76,-77,-79,-80,-82,-83,-85,-86,-88,-89,-91,-92,-94,-95,-97,
  -98,-99,-101,-102,-104,-105,-107,-108,-109,-111,-112,-114,-115,-117,-118,-119,
  -121,-122,-123,-125,-126,-128,-129,-130,-132,-133,-134,-136,-137,-138,-140,-141,
  -142,-144,-145,-146,-147,-149,-150,-151,-152,-154,-155,-156,-157,-159,-160,-161,
  -162,-164,-165,-166,-167,-168,-170,-171,-172,-173,-174,-175,-177,-178,-179,-180,
  -181,-182,-183,-184,-185,-186,-188,-189,-190,-191,-192,-193,-194,-195,-196,-197,
  -198,-199,-200,-201,-202,-203,-204,-205,-206,-207,-207,-208,-209,-210,-211,-212,
  -213,-214,-215,-215,-216,-217,-218,-219,-220,-220,-221,-222,-223,-224,-224,-225,
  -226,-227,-227,-228,-229,-229,-230,-231,-231,-232,-233,-233,-234,-235,-235,-236,
  -237,-237,-238,-238,-239,-239,-240,-241,-241,-242,-242,-243,-243,-244,-244,-245,
  -245,-245,-246,-246,-247,-247,-248,-248,-248,-249,-249,-249,-250,-250,-250,-251,
  -251,-251,-252,-252,-252,-252,-253,-253,-253,-253,-254,-254,-254,-254,-254,-255,
  -255,-255,-255,-255,-255,-255,-256,-256,-256,-256,-256,-256,-256,-256,-256,-256,
  -256,-256,-256,-256,-256,-256,-256,-256,-256,-256,-256,-255,-255,-255,-255,-255,
  -255,-255,-254,-254,-254,-254,-254,-253,-253,-253,-253,-252,-252,-252,-252,-251,
  -251,-251,-250,-250,-250,-249,-249,-249,-248,-248,-248,-247,-247,-246,-246,-245,
  -245,-245,-244,-244,-243,-243,-242,-242,-241,-241,-240,-239,-239,-238,-238,-237,
  -237,-236,-235,-235,-234,-233,-233,-232,-231,-231,-230,-229,-229,-228,-227,-227,
  -226,-225,-224,-224,-223,-222,-221,-220,-220,-219,-218,-217,-216,-215,-215,-214,
  -213,-212,-211,-210,-209,-208,-207,-207,-206,-205,-204,-203,-202,-201,-200,-199,
  -198,-197,-196,-195,-194,-193,-192,-191,-190,-189,-188,-186,-185,-184,-183,-182,
  -181,-180,-179,-178,-177,-175,-174,-173,-172,-171,-170,-168,-167,-166,-165,-164,
  -162,-161,-160,-159,-157,-156,-155,-154,-152,-151,-150,-149,-147,-146,-145,-144,
  -142,-141,-140,-138,-137,-136,-134,-133,-132,-130,-129,-128,-126,-125,-123,-122,
  -121,-119,-118,-117,-115,-114,-112,-111,-109,-108,-107,-105,-104,-102,-101,-99,
  -98,-97,-95,-94,-92,-91,-89,-88,-86,-85,-83,-82,-80,-79,-77,-76,
  -74,-73,-71,-70,-68,-67,-65,-64,-62,-61,-59,-58,-56,-55,-53,-51,
  -50,-48,-47,-45,-44,-42,-41,-39,-38,-36,-34,-33,-31,-30,-28,-27,
  -25,-24,-22,-20,-19,-17,-16,-14,-13,-11,-9,-8,-6,-5,-3,-2,
  0,2,3,5,6,8,9,11,13,14,16,17,19,20,22,24,
  25,27,28,30,31,33,34,36,38,39,41,42,44,45,47,48,
  50,51,53,55,56,58,59,61,62,64,65,67,68,70,71,73,
  74,76,77,79,80,82,83,85,86,88,89,91,92,94,95,97,
  98,99,101,102,104,105,107,108,109,111,112,114,115,117,118,119,
  121,122,123,125,126,128,129,130,132,133,134,136,137,138,140,141,
  142,144,145,146,147,149,150,151,152,154,155,156,157,159,160,161,
  162,164,165,166,167,168,170,171,172,173,174,175,177,178,179,180,
  181,182,183,184,185,186,188,189,190,191,192,193,194,195,196,197,
  198,199,200,201,202,203,204,205,206,207,207,208,209,210,211,212,
  213,214,215,215,216,217,218,219,220,220,221,222,223,224,224,225,
  226,227,227,228,229,229,230,231,231,232,233,233,234,235,235,236,
  237,237,238,238,239,239,240,241,241,242,242,243,243,244,244,245,
  245,245,246,246,247,247,248,248,248,249,249,249,250,250,250,251,
  251,251,252,252,252,252,253,253,253,253,254,254,254,254,254,255,
  255,255,255,255,255,255,256,256,256,256,256,256,256,256,256,256,
};

const int16_t PROGMEM sinTable[1024] = {
  0,2,3,5,6,8,9,11,13,14,16,17,19,20,22,24,
  25,27,28,30,31,33,34,36,38,39,41,42,44,45,47,48,
  50,51,53,55,56,58,59,61,62,64,65,67,68,70,71,73,
  74,76,77,79,80,82,83,85,86,88,89,91,92,94,95,97,
  98,99,101,102,104,105,107,108,109,111,112,114,115,117,118,119,
  121,122,123,125,126,128,129,130,132,133,134,136,137,138,140,141,
  142,144,145,146,147,149,150,151,152,154,155,156,157,159,160,161,
  162,164,165,166,167,168,170,171,172,173,174,175,177,178,179,180,
  181,182,183,184,185,186,188,189,190,191,192,193,194,195,196,197,
  198,199,200,201,202,203,204,205,206,207,207,208,209,210,211,212,
  213,214,215,215,216,217,218,219,220,220,221,222,223,224,224,225,
  226,227,227,228,229,229,230,231,231,232,233,233,234,235,235,236,
  237,237,238,238,239,239,240,241,241,242,242,243,243,244,244,245,
  245,245,246,246,247,247,248,248,248,249,249,249,250,250,250,251,
  251,251,252,252,252,252,253,253,253,253,254,254,254,254,254,255,
  255,255,255,255,255,255,256,256,256,256,256,256,256,256,256,256,
  256,256,256,256,256,256,256,256,256,256,256,255,255,255,255,255,
  255,255,254,254,254,254,254,253,253,253,253,252,252,252,252,251,
  251,251,250,250,250,249,249,249,248,248,248,247,247,246,246,245,
  245,245,244,244,243,243,242,242,241,241,240,239,239,238,238,237,
  237,236,235,235,234,233,233,232,231,231,230,229,229,228,227,227,
  226,225,224,224,223,222,221,220,220,219,218,217,216,215,215,214,
  213,212,211,210,209,208,207,207,206,205,204,203,202,201,200,199,
  198,197,196,195,194,193,192,191,190,189,188,186,185,184,183,182,
  181,180,179,178,177,175,174,173,172,171,170,168,167,166,165,164,
  162,161,160,159,157,156,155,154,152,151,150,149,147,146,145,144,
  142,141,140,138,137,136,134,133,132,130,129,128,126,125,123,122,
  121,119,118,117,115,114,112,111,109,108,107,105,104,102,101,99,
  98,97,95,94,92,91,89,88,86,85,83,82,80,79,77,76,
  74,73,71,70,68,67,65,64,62,61,59,58,56,55,53,51,
  50,48,47,45,44,42,41,39,38,36,34,33,31,30,28,27,
  25,24,22,20,19,17,16,14,13,11,9,8,6,5,3,2,
  0,-2,-3,-5,-6,-8,-9,-11,-13,-14,-16,-17,-19,-20,-22,-24,
  -25,-27,-28,-30,-31,-33,-34,-36,-38,-39,-41,-42,-44,-45,-47,-48,
  -50,-51,-53,-55,-56,-58,-59,-61,-62,-64,-65,-67,-68,-70,-71,-73,
  -74,-76,-77,-79,-80,-82,-83,-85,-86,-88,-89,-91,-92,-94,-95,-97,
  -98,-99,-101,-102,-104,-105,-107,-108,-109,-111,-112,-114,-115,-117,-118,-119,
  -121,-122,-123,-125,-126,-128,-129,-130,-132,-133,-134,-136,-137,-138,-140,-141,
  -142,-144,-145,-146,-147,-149,-150,-151,-152,-154,-155,-156,-157,-159,-160,-161,
  -162,-164,-165,-166,-167,-168,-170,-171,-172,-173,-174,-175,-177,-178,-179,-180,
  -181,-182,-183,-184,-185,-186,-188,-189,-190,-191,-192,-193,-194,-195,-196,-197,
  -198,-199,-200,-201,-202,-203,-204,-205,-206,-207,-207,-208,-209,-210,-211,-212,
  -213,-214,-215,-215,-216,-217,-218,-219,-220,-220,-221,-222,-223,-224,-224,-225,
  -226,-227,-227,-228,-229,-229,-230,-231,-231,-232,-233,-233,-234,-235,-235,-236,
  -237,-237,-238,-238,-239,-239,-240,-241,-241,-242,-242,-243,-243,-244,-244,-245,
  -245,-245,-246,-246,-247,-247,-248,-248,-248,-249,-249,-249,-250,-250,-250,-251,
  -251,-251,-252,-252,-252,-252,-253,-253,-253,-253,-254,-254,-254,-254,-254,-255,
  -255,-255,-255,-255,-255,-255,-256,-256,-256,-256,-256,-256,-256,-256,-256,-256,
  -256,-256,-256,-256,-256,-256,-256,-256,-256,-256,-256,-255,-255,-255,-255,-255,
  -255,-255,-254,-254,-254,-254,-254,-253,-253,-253,-253,-252,-252,-252,-252,-251,
  -251,-251,-250,-250,-250,-249,-249,-249,-248,-248,-248,-247,-247,-246,-246,-245,
  -245,-245,-244,-244,-243,-243,-242,-242,-241,-241,-240,-239,-239,-238,-238,-237,
  -237,-236,-235,-235,-234,-233,-233,-232,-231,-231,-230,-229,-229,-228,-227,-227,
  -226,-225,-224,-224,-223,-222,-221,-220,-220,-219,-218,-217,-216,-215,-215,-214,
  -213,-212,-211,-210,-209,-208,-207,-207,-206,-205,-204,-203,-202,-201,-200,-199,
  -198,-197,-196,-195,-194,-193,-192,-191,-190,-189,-188,-186,-185,-184,-183,-182,
  -181,-180,-179,-178,-177,-175,-174,-173,-172,-171,-170,-168,-167,-166,-165,-164,
  -162,-161,-160,-159,-157,-156,-155,-154,-152,-151,-150,-149,-147,-146,-145,-144,
  -142,-141,-140,-138,-137,-136,-134,-133,-132,-130,-129,-128,-126,-125,-123,-122,
  -121,-119,-118,-117,-115,-114,-112,-111,-109,-108,-107,-105,-104,-102,-101,-99,
  -98,-97,-95,-94,-92,-91,-89,-88,-86,-85,-83,-82,-80,-79,-77,-76,
  -74,-73,-71,-70,-68,-67,-65,-64,-62,-61,-59,-58,-56,-55,-53,-51,
  -50,-48,-47,-45,-44,-42,-41,-39,-38,-36,-34,-33,-31,-30,-28,-27,
  -25,-24,-22,-20,-19,-17,-16,-14,-13,-11,-9,-8,-6,-5,-3,-2,
};

// Per-column ray angle offsets for a 60 degree FOV across 128 columns.
const int16_t PROGMEM colAngleOffset[128] = {
  -86,-84,-83,-81,-80,-79,-77,-76,-75,-73,-72,-71,-69,-68,-67,-65,
  -64,-63,-61,-60,-59,-57,-56,-55,-53,-52,-50,-49,-48,-46,-45,-44,
  -42,-41,-40,-38,-37,-36,-34,-33,-32,-30,-29,-28,-26,-25,-24,-22,
  -21,-20,-18,-17,-15,-14,-13,-11,-10,-9,-7,-6,-5,-3,-2,-1,
  1,2,3,5,6,7,9,10,11,13,14,15,17,18,20,21,
  22,24,25,26,28,29,30,32,33,34,36,37,38,40,41,42,
  44,45,46,48,49,50,52,53,55,56,57,59,60,61,63,64,
  65,67,68,69,71,72,73,75,76,77,79,80,81,83,84,86,
};

// ============================================================
// GAME CONSTANTS (tune these after your first test flash)
// ============================================================
#define FP_ONE          256      // Q8 fixed point: 1 map cell = 256
#define MOVE_SPEED      12       // player move speed, Q8 units/frame
#define TURN_SPEED      10       // player turn speed, angle units/frame (of 1024)
#define COLLIDE_R       50       // player collision radius, Q8 units
#define RAY_STEP        28       // raycast march step, Q8 units
#define MAX_RAY_DIST    2560     // max ray length, Q8 units (10 cells)
#define WALL_HEIGHT_K   70       // projection constant for wall height
#define NEAR_SHADE_DIST 1024     // start light dithering beyond 4 cells
#define FAR_SHADE_DIST  1792     // heavy dithering beyond 7 cells
#define SPRITE_PROJ_K   109      // projection constant for sprite screen-X

#define PLAYER_MAX_HP     100
#define ENEMY_MAX_HP      100
#define SHOT_DAMAGE       34
#define ENEMY_DAMAGE      8
#define FIRE_COOLDOWN_FR  6
#define HURT_COOLDOWN_FR  20
#define MAX_SHOT_RANGE    2560
#define SHOT_TOLERANCE    70     // aiming cone tightness
#define CHASE_RANGE       1536   // 6 cells
#define MELEE_RANGE       320    // ~1.25 cells
#define CHASE_STEP        40

// ============================================================
// GAME STATE
// ============================================================
enum GameState { ST_TITLE, ST_PLAYING, ST_WIN, ST_LOSE };
GameState gameState = ST_TITLE;

struct Player {
  int16_t posX, posY;   // Q8 world position
  int16_t angle;        // 0-1023
  int8_t  hp;
  uint8_t hurtCooldown;
};
Player player;

struct Enemy {
  int16_t x, y;    // Q8 world position
  int8_t  hp;
  bool    alive;
  uint8_t hitFlash; // frames remaining of "just got shot" hatch flash
};
Enemy enemies[NUM_ENEMIES];
uint8_t aiTurn = 0;

uint8_t zbuffer[128];        // per-column wall distance (>>3), for sprite occlusion
uint8_t fireCooldown = 0;
uint8_t gunFlashTimer = 0;   // frames remaining of muzzle-flash/recoil animation
uint8_t frameCount = 0;      // free-running counter, used for flash/hatch patterns
bool btnLeft, btnRight, btnForward, btnFire, prevFireMenu = false;
uint8_t stateEnterGuard = 0; // ignore fire briefly after entering a menu state

static inline int16_t clampi(int16_t v, int16_t lo, int16_t hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

// ============================================================
// MAP HELPERS
// ============================================================
bool isWall(int16_t x, int16_t y) {
  int16_t cx = x >> 8;
  int16_t cy = y >> 8;
  if (cx < 0 || cx >= MAP_SIZE || cy < 0 || cy >= MAP_SIZE) return true;
  uint16_t row = pgm_read_word(&mapData[cy]);
  return (row >> (15 - cx)) & 1;
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  pinMode(PIN_BTN_LEFT, INPUT_PULLUP);
  pinMode(PIN_BTN_RIGHT, INPUT_PULLUP);
  pinMode(PIN_BTN_FORWARD, INPUT_PULLUP);
  pinMode(PIN_BTN_FIRE, INPUT_PULLUP);

  u8g2.setBusClock(400000); // 400kHz I2C - much faster frame pushes than default 100kHz
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x10_tr);

  gameState = ST_TITLE;
  stateEnterGuard = 15;
}

void resetGame() {
  player.posX = START_X * FP_ONE + FP_ONE / 2;
  player.posY = START_Y * FP_ONE + FP_ONE / 2;
  player.angle = 0;
  player.hp = PLAYER_MAX_HP;
  player.hurtCooldown = 0;
  fireCooldown = 0;
  for (uint8_t i = 0; i < NUM_ENEMIES; i++) {
    enemies[i].x = enemyStartX[i] * FP_ONE + FP_ONE / 2;
    enemies[i].y = enemyStartY[i] * FP_ONE + FP_ONE / 2;
    enemies[i].hp = ENEMY_MAX_HP;
    enemies[i].alive = true;
    enemies[i].hitFlash = 0;
  }
  gunFlashTimer = 0;
  frameCount = 0;
}

// ============================================================
// INPUT
// ============================================================
void readButtons() {
  btnLeft    = !digitalRead(PIN_BTN_LEFT);
  btnRight   = !digitalRead(PIN_BTN_RIGHT);
  btnForward = !digitalRead(PIN_BTN_FORWARD);
  btnFire    = !digitalRead(PIN_BTN_FIRE);
}

// ============================================================
// PLAYER MOVEMENT
// ============================================================
void updatePlayer() {
  if (btnLeft)  player.angle = (player.angle - TURN_SPEED) & 1023;
  if (btnRight) player.angle = (player.angle + TURN_SPEED) & 1023;

  if (btnForward) {
    int16_t dx = pgm_read_word(&cosTable[player.angle]);
    int16_t dy = pgm_read_word(&sinTable[player.angle]);
    int16_t stepX = ((int32_t)dx * MOVE_SPEED) >> 8;
    int16_t stepY = ((int32_t)dy * MOVE_SPEED) >> 8;

    int16_t newX = player.posX + stepX;
    int16_t edgeX = (stepX >= 0) ? COLLIDE_R : -COLLIDE_R;
    if (!isWall(newX + edgeX, player.posY - COLLIDE_R) &&
        !isWall(newX + edgeX, player.posY + COLLIDE_R)) {
      player.posX = newX;
    }

    int16_t newY = player.posY + stepY;
    int16_t edgeY = (stepY >= 0) ? COLLIDE_R : -COLLIDE_R;
    if (!isWall(player.posX - COLLIDE_R, newY + edgeY) &&
        !isWall(player.posX + COLLIDE_R, newY + edgeY)) {
      player.posY = newY;
    }
  }
}

// ============================================================
// ENEMY AI - one enemy updated per frame (round robin) to keep
// per-frame CPU cost tiny and constant.
// ============================================================
void updateEnemyAI() {
  Enemy &e = enemies[aiTurn];
  aiTurn = (aiTurn + 1) % NUM_ENEMIES;
  if (!e.alive) return;

  int32_t ddx = (int32_t)e.x - player.posX;
  int32_t ddy = (int32_t)e.y - player.posY;
  int32_t distSq = ddx * ddx + ddy * ddy;

  if (distSq > (int32_t)CHASE_RANGE * CHASE_RANGE) return;

  if (distSq > (int32_t)MELEE_RANGE * MELEE_RANGE) {
    int16_t stepx = (ddx > 0) ? -CHASE_STEP : (ddx < 0 ? CHASE_STEP : 0);
    int16_t stepy = (ddy > 0) ? -CHASE_STEP : (ddy < 0 ? CHASE_STEP : 0);
    int16_t nx = e.x + stepx;
    int16_t ny = e.y + stepy;
    if (!isWall(nx, e.y)) e.x = nx;
    if (!isWall(e.x, ny)) e.y = ny;
  } else if (player.hurtCooldown == 0) {
    player.hp -= ENEMY_DAMAGE;
    player.hurtCooldown = HURT_COOLDOWN_FR;
    tone(PIN_BUZZER, 150, 100);
  }
}

// ============================================================
// COMBAT
// ============================================================
void handleFire() {
  if (!btnFire || fireCooldown > 0) return;
  fireCooldown = FIRE_COOLDOWN_FR;
  gunFlashTimer = 5;
  tone(PIN_BUZZER, 1200, 50);

  uint16_t wallDist = (uint16_t)zbuffer[64] << 3; // approx wall distance dead ahead
  int16_t cosP = pgm_read_word(&cosTable[player.angle]);
  int16_t sinP = pgm_read_word(&sinTable[player.angle]);

  int8_t bestIdx = -1;
  int32_t bestForward = 999999;
  for (uint8_t i = 0; i < NUM_ENEMIES; i++) {
    if (!enemies[i].alive) continue;
    int32_t ddx = enemies[i].x - player.posX;
    int32_t ddy = enemies[i].y - player.posY;
    int32_t forward = (ddx * cosP + ddy * sinP) >> 8;
    if (forward <= 0 || forward > MAX_SHOT_RANGE || forward > wallDist) continue;
    int32_t right = (ddx * sinP - ddy * cosP) >> 8;
    if (labs(right) < (forward * SHOT_TOLERANCE) / 256) {
      if (forward < bestForward) { bestForward = forward; bestIdx = i; }
    }
  }

  if (bestIdx >= 0) {
    enemies[bestIdx].hp -= SHOT_DAMAGE;
    enemies[bestIdx].hitFlash = 8;
    tone(PIN_BUZZER, 600, 40);
    if (enemies[bestIdx].hp <= 0) enemies[bestIdx].alive = false;
  }
}

// ============================================================
// RENDERING - walls
// ============================================================
void castAndDrawWalls() {
  for (uint8_t col = 0; col < 128; col++) {
    int16_t rayAngle = (player.angle + (int16_t)pgm_read_word(&colAngleOffset[col])) & 1023;
    int16_t dx = pgm_read_word(&cosTable[rayAngle]);
    int16_t dy = pgm_read_word(&sinTable[rayAngle]);

    int16_t rx = player.posX, ry = player.posY;
    int16_t dist = 0;
    bool hit = false;

    while (dist < MAX_RAY_DIST) {
      rx += ((int32_t)dx * RAY_STEP) >> 8;
      ry += ((int32_t)dy * RAY_STEP) >> 8;
      dist += RAY_STEP;
      if (isWall(rx, ry)) { hit = true; break; }
    }

    if (!hit) { zbuffer[col] = 255; continue; } // nothing there, leave column blank

    int16_t relAngle = (rayAngle - player.angle) & 1023;
    int16_t cosRel = pgm_read_word(&cosTable[relAngle]);
    if (cosRel < 16) cosRel = 16; // safety floor, shouldn't trigger within our FOV

    int32_t correctedDist = ((int32_t)dist * cosRel) >> 8;
    if (correctedDist < 16) correctedDist = 16;
    zbuffer[col] = (correctedDist >> 3 > 255) ? 255 : (uint8_t)(correctedDist >> 3);

    int16_t wallH = (int16_t)(((int32_t)WALL_HEIGHT_K << 8) / correctedDist);
    if (wallH > 160) wallH = 160;
    int16_t top = 32 - wallH / 2;
    int16_t bottom = 32 + wallH / 2;
    if (top < 0) top = 0;
    if (bottom > 63) bottom = 63;

    if (correctedDist > FAR_SHADE_DIST) {
      for (int16_t y = top; y <= bottom; y += 3) u8g2.drawPixel(col, y);
    } else if (correctedDist > NEAR_SHADE_DIST) {
      for (int16_t y = top; y <= bottom; y += 2) u8g2.drawPixel(col, y);
    } else {
      u8g2.drawVLine(col, top, bottom - top + 1);
    }
  }
}

// ============================================================
// RENDERING - enemies
//
// On a 1-bit screen a plain silhouette box can visually merge
// into a solid wall column right behind it. Two fixes are used
// here: (1) a black "keyline" halo is erased around the sprite
// first, so there's always a gap between the enemy and whatever
// is behind it, and (2) the shape is a head+body silhouette
// instead of a flat box, so it doesn't read as "just another
// wall segment". A hatch pattern flashes briefly on a landed hit.
// ============================================================
void drawEnemies() {
  int16_t cosP = pgm_read_word(&cosTable[player.angle]);
  int16_t sinP = pgm_read_word(&sinTable[player.angle]);

  for (uint8_t i = 0; i < NUM_ENEMIES; i++) {
    if (!enemies[i].alive) continue;
    int32_t ddx = enemies[i].x - player.posX;
    int32_t ddy = enemies[i].y - player.posY;
    int32_t forward = (ddx * cosP + ddy * sinP) >> 8;
    if (forward < 32 || forward > MAX_RAY_DIST) continue;
    int32_t right = (ddx * sinP - ddy * cosP) >> 8;

    int16_t screenX = 64 + (int16_t)((right * SPRITE_PROJ_K) / forward);
    int16_t size = (int16_t)(((int32_t)WALL_HEIGHT_K << 8) / forward);
    if (size > 56) size = 56;
    if (size < 4) size = 4;
    int16_t halfW      = size / 3 + 2;
    int16_t headR      = size / 5 + 1;
    int16_t bodyTop    = 32 - size / 2 + headR;
    int16_t bodyBottom = 32 + size / 2;
    int16_t headCY     = bodyTop - headR;
    int16_t spriteTop  = headCY - headR;

    int16_t left  = screenX - halfW;
    int16_t rightCol = screenX + halfW;
    if (rightCol < 0 || left > 127) continue;

    int16_t clampedLeft  = clampi(left, 0, 127);
    int16_t clampedRight = clampi(rightCol, 0, 127);

    uint8_t fdist8 = (forward >> 3 > 255) ? 255 : (uint8_t)(forward >> 3);

    // skip entirely if every column is hidden behind a nearer wall
    bool anyVisible = false;
    for (int16_t col = clampedLeft; col <= clampedRight; col++) {
      if (zbuffer[col] > fdist8) { anyVisible = true; break; }
    }
    if (!anyVisible) continue;

    // halo: erase a 1px black border around the sprite's bounding box so
    // it always has contrast against whatever wall pixels sit behind it
    int16_t hx = clampi(left - 1, 0, 127);
    int16_t hy = clampi(spriteTop - 1, 0, 63);
    int16_t hw = clampi(rightCol + 1, 0, 127) - hx + 1;
    int16_t hh = clampi(bodyBottom + 1, 0, 63) - hy + 1;
    if (hw > 0 && hh > 0) {
      u8g2.setDrawColor(0);
      u8g2.drawBox(hx, hy, hw, hh);
      u8g2.setDrawColor(1);
    }

    bool flash = enemies[i].hitFlash > 0;

    // body
    for (int16_t col = clampedLeft; col <= clampedRight; col++) {
      if (zbuffer[col] <= fdist8) continue; // occluded by a nearer wall
      if (flash && ((col + frameCount) & 1)) continue; // hatched while flashing
      u8g2.drawVLine(col, bodyTop, bodyBottom - bodyTop + 1);
    }
    // head - a circle reads as a creature, not a wall segment
    if (!flash || !(frameCount & 1)) {
      if (screenX >= clampedLeft - headR && screenX <= clampedRight + headR) {
        u8g2.drawDisc(screenX, headCY, headR);
      }
    }
  }
}

// ============================================================
// HUD
//
// Note: this is a 1-bit display, so "color" isn't something
// software can set - pixels are only on/off, and the color you
// see is fixed by the physical panel (often blue, or yellow on
// the top ~16 rows / blue below on the common two-tone SSD1306
// modules). The HP meter below sits in that top strip, so on a
// two-tone panel it'll already appear yellow for free. What we
// CAN control is legibility, so it's drawn as a segmented block
// meter with a label rather than a plain bar.
// ============================================================
void drawHUD() {
  u8g2.setDrawColor(0);
  u8g2.drawBox(0, 0, 64, 12);
  u8g2.setDrawColor(1);
  u8g2.drawStr(0, 9, "HP");
  u8g2.drawFrame(15, 1, 47, 9);
  uint8_t segs = ((int16_t)(player.hp > 0 ? player.hp : 0) * 9) / PLAYER_MAX_HP;
  for (uint8_t s = 0; s < segs; s++) {
    u8g2.drawBox(17 + s * 5, 3, 4, 5);
  }

  u8g2.drawLine(60, 32, 68, 32);
  u8g2.drawLine(64, 28, 64, 36);
}

// ============================================================
// GUN SPRITE - static pistol silhouette bottom-center, with a
// muzzle-flash starburst and a small upward recoil kick on fire.
// ============================================================
// void drawWeapon() {
//   int16_t gunTop = 45;
//   int16_t gunLeft = 60;
  
//   if (fireCooldown > 0) {
//     gunTop += 4;
//   }
  
//   u8g2.setDrawColor(1);
//   u8g2.drawBox(gunLeft, gunTop, 12, 64 - gunTop);
//   u8g2.drawBox(gunLeft - 4, gunTop + 8, 20, 10);
  
//   if (muzzleFlashFrames > 0) {
//     u8g2.drawDisc(gunLeft + 6, gunTop - 4, 6 + (muzzleFlashFrames % 2)*2);
//     u8g2.setDrawColor(0);
//     u8g2.drawDisc(gunLeft + 6, gunTop - 4, 3);
//     muzzleFlashFrames--;
//   }
// }

void drawGun(uint8_t flashTimer) {
  int16_t kick = (flashTimer > 2) ? 3 : 0; // recoil for the first couple of flash frames
  int16_t gx = 46, gy = 44 - kick;         // bottom-center anchor, kept inside the screen

  u8g2.setDrawColor(1);
  u8g2.drawBox(gx, gy, 12, 18);
  u8g2.drawBox(gx - 4, gy + 8, 20, 10);

  if (flashTimer > 0) {
    int16_t fx = gx + 11, fy = gy - 10;
    u8g2.drawLine(fx, fy, fx - 4, fy - 5);
    u8g2.drawLine(fx, fy, fx + 4, fy - 5);
    u8g2.drawLine(fx, fy, fx, fy - 8);
    u8g2.drawLine(fx, fy, fx - 3, fy - 2);
    u8g2.drawLine(fx, fy, fx + 3, fy - 2);
    u8g2.drawDisc(gx + 6, gy - 4, 6 + (flashTimer % 2) * 2);
    u8g2.setDrawColor(0);
    u8g2.drawDisc(gx + 6, gy - 4, 3);
  }
}

// ============================================================
// WIN / LOSE CHECK
// ============================================================
void checkWinLose() {
  if (player.hp <= 0) {
    gameState = ST_LOSE;
    stateEnterGuard = 20;
    tone(PIN_BUZZER, 100, 500);
    return;
  }
  int16_t ex = EXIT_X * FP_ONE + FP_ONE / 2;
  int16_t ey = EXIT_Y * FP_ONE + FP_ONE / 2;
  int32_t ddx = player.posX - ex;
  int32_t ddy = player.posY - ey;
  if (ddx * ddx + ddy * ddy < (int32_t)200 * 200) {
    gameState = ST_WIN;
    stateEnterGuard = 20;
    tone(PIN_BUZZER, 1500, 300);
  }
}

// ============================================================
// MAIN LOOP
// ============================================================
void loop() {
  readButtons();
  if (stateEnterGuard > 0) stateEnterGuard--;
  bool fireJustPressed = btnFire && !prevFireMenu;
  prevFireMenu = btnFire;

  switch (gameState) {
    case ST_TITLE: {
      u8g2.clearBuffer();
      u8g2.drawStr(22, 22, "NANO RAIDER");
      u8g2.drawStr(10, 40, "FIRE to start");
      u8g2.drawStr(4, 55, "L/R turn  FWD move");
      u8g2.sendBuffer();
      if (fireJustPressed && stateEnterGuard == 0) {
        resetGame();
        gameState = ST_PLAYING;
        tone(PIN_BUZZER, 800, 80);
      }
      break;
    }

    case ST_PLAYING: {
      frameCount++;
      updatePlayer();
      updateEnemyAI();
      if (fireCooldown > 0) fireCooldown--;
      if (gunFlashTimer > 0) gunFlashTimer--;
      if (player.hurtCooldown > 0) player.hurtCooldown--;
      for (uint8_t i = 0; i < NUM_ENEMIES; i++) {
        if (enemies[i].hitFlash > 0) enemies[i].hitFlash--;
      }

      u8g2.clearBuffer();
      castAndDrawWalls();
      handleFire();       // uses zbuffer from castAndDrawWalls, run after
      drawEnemies();
      drawGun(gunFlashTimer);
      drawHUD();
      u8g2.sendBuffer();

      checkWinLose();
      break;
    }

    case ST_WIN: {
      u8g2.clearBuffer();
      u8g2.drawStr(24, 28, "YOU ESCAPED!");
      u8g2.drawStr(14, 44, "FIRE for title");
      u8g2.sendBuffer();
      if (fireJustPressed && stateEnterGuard == 0) {
        gameState = ST_TITLE;
        stateEnterGuard = 15;
      }
      break;
    }

    case ST_LOSE: {
      u8g2.clearBuffer();
      u8g2.drawStr(30, 28, "GAME OVER");
      u8g2.drawStr(14, 44, "FIRE for title");
      u8g2.sendBuffer();
      if (fireJustPressed && stateEnterGuard == 0) {
        gameState = ST_TITLE;
        stateEnterGuard = 15;
      }
      break;
    }
  }
}