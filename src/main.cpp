/*
  ============================================================
   NANO RAIDER v2 - PERFORMANCE RENDERER REWRITE
  ============================================================
*/

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

// ---- Display ----
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// ---- Pins ----
#define PIN_BTN_LEFT    2
#define PIN_BTN_RIGHT   3
#define PIN_BTN_FORWARD 4
#define PIN_BTN_FIRE    5
#define PIN_BUZZER      9

// ============================================================
// MAP
// ============================================================
#define MAP_SIZE 16
const uint16_t PROGMEM mapData[MAP_SIZE] = {
  0b1111111111111111,
  0b1000100000000011,
  0b1110111110101011,
  0b1010100010101011,
  0b1010101011101111,
  0b1010001000100011,
  0b1011111110101011,
  0b1000100010101011,
  0b1011101010111011,
  0b1000001010000011,
  0b1011111011111011,
  0b1000001010001011,
  0b1111101010101011,
  0b1000001000100011,
  0b1111111111111111,
  0b1111111111111111,
};

#define START_X 1
#define START_Y 1
#define EXIT_X  13
#define EXIT_Y  13

#define NUM_ENEMIES 4
const int16_t enemyStartX[NUM_ENEMIES] = {13, 1, 9, 8};
const int16_t enemyStartY[NUM_ENEMIES] = { 5, 9, 2, 5};

// ============================================================
// TRIG TABLES 
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
// GAME CONSTANTS
// ============================================================
#define FP_ONE          256
#define MOVE_SPEED      12
#define TURN_SPEED      10
#define COLLIDE_R       50
#define MAX_RAY_DIST    2560
#define WALL_HEIGHT_K   70
#define NEAR_SHADE_DIST 768
#define FAR_SHADE_DIST  1536
#define SPRITE_PROJ_K   109

#define PLAYER_MAX_HP     100
#define ENEMY_MAX_HP      100
#define SHOT_DAMAGE       34
#define ENEMY_DAMAGE      8
#define FIRE_COOLDOWN_FR  6
#define HURT_COOLDOWN_FR  20
#define MAX_SHOT_RANGE    2560
#define SHOT_TOLERANCE    70
#define CHASE_RANGE       1536
#define MELEE_RANGE       320
#define CHASE_STEP        40

// ============================================================
// GAME STATE & ARCHITECTURE STRUCTS
// ============================================================
enum GameState { ST_TITLE, ST_PLAYING, ST_WIN, ST_LOSE };
GameState gameState = ST_TITLE;

struct Player {
  int16_t posX, posY;
  int16_t angle;
  int8_t  hp;
  uint8_t hurtCooldown;
  int8_t  recoilPitch; 
};
Player player;

struct Enemy {
  int16_t x, y;
  int8_t  hp;
  bool    alive;
  uint8_t hitFlash;
  uint8_t animTimer;
};
Enemy enemies[NUM_ENEMIES];
uint8_t aiTurn = 0;

struct RayHit {
  int16_t dist;
  uint8_t side;
  uint8_t texX;
  bool hit;
};

uint8_t zbuffer[128];
uint8_t fireCooldown = 0;
uint8_t muzzleFlashFrames = 0;
bool btnLeft, btnRight, btnForward, btnFire, prevFireMenu = false;
uint8_t stateEnterGuard = 0;
uint32_t frameCount = 0;

// ============================================================
// SETUP
// ============================================================
void setup() {
  pinMode(PIN_BTN_LEFT, INPUT_PULLUP);
  pinMode(PIN_BTN_RIGHT, INPUT_PULLUP);
  pinMode(PIN_BTN_FORWARD, INPUT_PULLUP);
  pinMode(PIN_BTN_FIRE, INPUT_PULLUP);

  u8g2.setBusClock(400000); 
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x10_tr);

  gameState = ST_TITLE;
  stateEnterGuard = 15;
}

// ============================================================
// CORE LOGIC & AI
// ============================================================
bool isWall(int16_t x, int16_t y) {
  int16_t cx = x >> 8;
  int16_t cy = y >> 8;
  if (cx < 0 || cx >= MAP_SIZE || cy < 0 || cy >= MAP_SIZE) return true;
  uint16_t row = pgm_read_word(&mapData[cy]);
  return (row >> (15 - cx)) & 1;
}

void resetGame() {
  player.posX = START_X * FP_ONE + FP_ONE / 2;
  player.posY = START_Y * FP_ONE + FP_ONE / 2;
  player.angle = 0;
  player.hp = PLAYER_MAX_HP;
  player.hurtCooldown = 0;
  player.recoilPitch = 0;
  fireCooldown = 0;
  muzzleFlashFrames = 0;
  for (uint8_t i = 0; i < NUM_ENEMIES; i++) {
    enemies[i].x = enemyStartX[i] * FP_ONE + FP_ONE / 2;
    enemies[i].y = enemyStartY[i] * FP_ONE + FP_ONE / 2;
    enemies[i].hp = ENEMY_MAX_HP;
    enemies[i].alive = true;
    enemies[i].hitFlash = 0;
    enemies[i].animTimer = 0;
  }
}

void readButtons() {
  btnLeft    = !digitalRead(PIN_BTN_LEFT);
  btnRight   = !digitalRead(PIN_BTN_RIGHT);
  btnForward = !digitalRead(PIN_BTN_FORWARD);
  btnFire    = !digitalRead(PIN_BTN_FIRE);
}

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
// RENDERING PIPELINE
// ============================================================
RayHit castRay(int16_t rx, int16_t ry, int16_t rayAngle) {
  RayHit result = {MAX_RAY_DIST, 0, 0, false};
  
  int16_t dx = pgm_read_word(&cosTable[rayAngle]);
  int16_t dy = pgm_read_word(&sinTable[rayAngle]);
  if (dx == 0) dx = 1; 
  if (dy == 0) dy = 1;

  int16_t mapX = rx >> 8;
  int16_t mapY = ry >> 8;
  
  int32_t deltaDistX = 65536L / abs(dx);
  int32_t deltaDistY = 65536L / abs(dy);
  
  int32_t sideDistX, sideDistY;
  int8_t stepX, stepY;
  
  if (dx < 0) {
    stepX = -1;
    sideDistX = ((rx - (mapX << 8)) * deltaDistX) >> 8;
  } else {
    stepX = 1;
    sideDistX = ((((mapX + 1) << 8) - rx) * deltaDistX) >> 8;
  }
  
  if (dy < 0) {
    stepY = -1;
    sideDistY = ((ry - (mapY << 8)) * deltaDistY) >> 8;
  } else {
    stepY = 1;
    sideDistY = ((((mapY + 1) << 8) - ry) * deltaDistY) >> 8;
  }
  
  uint8_t side = 0;
  
  // FIX: Allow loop to progress until boundary hit or explicit map break
  while (!result.hit) {
    if (sideDistX < sideDistY) {
      sideDistX += deltaDistX;
      mapX += stepX;
      side = 0;
    } else {
      sideDistY += deltaDistY;
      mapY += stepY;
      side = 1;
    }
    
    if (mapX < 0 || mapX >= MAP_SIZE || mapY < 0 || mapY >= MAP_SIZE) {
        break; // Out of bounds safety
    }
    
    if ((pgm_read_word(&mapData[mapY]) >> (15 - mapX)) & 1) {
        result.hit = true;
    }
  }
  
  if (result.hit) {
    result.side = side;
    if (side == 0) {
      int16_t edgeX = (stepX < 0) ? ((mapX + 1) << 8) : (mapX << 8);
      result.dist = labs((int32_t)(edgeX - rx) * 256L / dx);
      int16_t hitY = ry + ((int32_t)dy * result.dist) / 256;
      result.texX = hitY & 255;
    } else {
      int16_t edgeY = (stepY < 0) ? ((mapY + 1) << 8) : (mapY << 8);
      result.dist = labs((int32_t)(edgeY - ry) * 256L / dy);
      int16_t hitX = rx + ((int32_t)dx * result.dist) / 256;
      result.texX = hitX & 255;
    }
  }
  return result;
}

void drawWallColumn(uint8_t col, RayHit hit, int16_t correctedDist) {
  if (correctedDist < 16) correctedDist = 16;
  zbuffer[col] = (correctedDist >> 3 > 255) ? 255 : (uint8_t)(correctedDist >> 3);

  int16_t wallH = (int16_t)(((int32_t)WALL_HEIGHT_K << 8) / correctedDist);
  if (wallH > 160) wallH = 160;
  int16_t center = 32 + player.recoilPitch;
  int16_t top = center - wallH / 2;
  int16_t bottom = center + wallH / 2;
  
  if (top < 0) top = 0;
  if (bottom > 63) bottom = 63;
  if (top > 63) return;

  u8g2.setDrawColor(1);
  
  if (correctedDist > FAR_SHADE_DIST) {
    for (int16_t y = top; y <= bottom; y += 4) u8g2.drawPixel(col, y);
  } else if (correctedDist > NEAR_SHADE_DIST) {
    for (int16_t y = top + (col % 2); y <= bottom; y += 2) u8g2.drawPixel(col, y);
  } else {
    // FIX: Using fast bitwise evaluation over modulo to prevent stuttering
    if (hit.side == 1 && ((hit.texX & 31) < 4)) {
       u8g2.drawVLine(col, top, bottom - top + 1);
       u8g2.setDrawColor(0);
       for(int16_t y = top; y <= bottom; y += 8) u8g2.drawPixel(col, y); 
    } else {
       u8g2.drawVLine(col, top, bottom - top + 1);
    }
  }

  u8g2.setDrawColor(0);
  if (top > 0) u8g2.drawPixel(col, top);
  if (bottom < 63) u8g2.drawPixel(col, bottom);
}

void drawFloorCeiling() {
  u8g2.setDrawColor(1);
  if (frameCount % 2 == 0) {
     u8g2.drawPixel(random(0, 128), random(0, 16));
     u8g2.drawPixel(random(0, 128), random(48, 64));
  }
}

void drawSprites() {
  int16_t cosP = pgm_read_word(&cosTable[player.angle]);
  int16_t sinP = pgm_read_word(&sinTable[player.angle]);

  for (uint8_t i = 0; i < NUM_ENEMIES; i++) {
    if (!enemies[i].alive) continue;
    
    enemies[i].animTimer++;
    
    int32_t ddx = enemies[i].x - player.posX;
    int32_t ddy = enemies[i].y - player.posY;
    int32_t forward = (ddx * cosP + ddy * sinP) >> 8;
    if (forward < 32 || forward > MAX_RAY_DIST) continue;
    int32_t right = (ddx * sinP - ddy * cosP) >> 8;

    int16_t screenX = 64 + (int16_t)((right * SPRITE_PROJ_K) / forward);
    int16_t size = (int16_t)(((int32_t)WALL_HEIGHT_K << 8) / forward);
    if (size > 56) size = 56;
    if (size < 6) size = 6;
    
    int16_t halfW = size / 3 + 1;
    int16_t center = 32 + player.recoilPitch;
    int16_t top = center - size / 2;
    int16_t bottom = center + size / 2;
    if (bottom > 63) bottom = 63;

    int16_t left = screenX - halfW;
    int16_t rightCol = screenX + halfW;
    
    // FIX: Replaced macros with explicit constraints to bypass compiler type matching errors
    if (rightCol < 0 || left > 127) continue;
    if (left < 0) left = 0;
    if (rightCol > 127) rightCol = 127;
    
    uint8_t fdist8 = (forward >> 3 > 255) ? 255 : (uint8_t)(forward >> 3);
    
    for (int16_t col = left; col <= rightCol; col++) {
      if (zbuffer[col] > fdist8) {
        if (enemies[i].hitFlash > 0) {
            u8g2.setDrawColor(1);
            u8g2.drawVLine(col, top, bottom - top);
        } else {
            u8g2.setDrawColor(1);
            int16_t hSize = size / 4;
            u8g2.drawVLine(col, top, hSize);
            u8g2.drawVLine(col, top + hSize + 1, size/2);
            
            bool legToggle = (enemies[i].animTimer / 8) % 2;
            if ((col < screenX && legToggle) || (col >= screenX && !legToggle)) {
               u8g2.drawVLine(col, bottom - hSize, hSize);
            }
            u8g2.setDrawColor(0);
            u8g2.drawPixel(col, top);
            u8g2.drawPixel(col, bottom);
        }
      }
    }
    if (enemies[i].hitFlash > 0) enemies[i].hitFlash--;
  }
}

void drawWeapon() {
  int16_t gunTop = 45;
  int16_t gunLeft = 60;
  
  if (fireCooldown > 0) {
    gunTop += 4;
  }
  
  u8g2.setDrawColor(1);
  u8g2.drawBox(gunLeft, gunTop, 12, 64 - gunTop);
  u8g2.drawBox(gunLeft - 4, gunTop + 8, 20, 10);
  
  if (muzzleFlashFrames > 0) {
    u8g2.drawDisc(gunLeft + 6, gunTop - 4, 6 + (muzzleFlashFrames % 2)*2);
    u8g2.setDrawColor(0);
    u8g2.drawDisc(gunLeft + 6, gunTop - 4, 3);
    muzzleFlashFrames--;
  }
}

void drawHUD() {
  u8g2.setDrawColor(0);
  u8g2.drawBox(0, 0, 46, 9);
  u8g2.setDrawColor(1);
  u8g2.drawFrame(2, 2, 42, 6);
  int16_t w = (player.hp > 0) ? ((int16_t)player.hp * 40) / PLAYER_MAX_HP : 0;
  if (w > 0) u8g2.drawBox(3, 3, w, 4);

  u8g2.setDrawColor(1);
  int16_t cx = 64;
  int16_t cy = 32 + player.recoilPitch;
  u8g2.drawPixel(cx, cy);
  u8g2.drawHLine(cx - 4, cy, 3);
  u8g2.drawHLine(cx + 2, cy, 3);
  u8g2.drawVLine(cx, cy - 4, 3);
  u8g2.drawVLine(cx, cy + 2, 3);
}

// ============================================================
// COMBAT & GAMELOOP
// ============================================================
void handleFire() {
  if (!btnFire || fireCooldown > 0) return;
  fireCooldown = FIRE_COOLDOWN_FR;
  muzzleFlashFrames = 3;
  player.recoilPitch = -3;
  
  tone(PIN_BUZZER, 1200, 50);

  uint16_t wallDist = (uint16_t)zbuffer[64] << 3; 
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
    if (abs(right) < (forward * SHOT_TOLERANCE) / 256) {
      if (forward < bestForward) { bestForward = forward; bestIdx = i; }
    }
  }

  if (bestIdx >= 0) {
    enemies[bestIdx].hp -= SHOT_DAMAGE;
    enemies[bestIdx].hitFlash = 3;
    tone(PIN_BUZZER, 600, 40);
    if (enemies[bestIdx].hp <= 0) enemies[bestIdx].alive = false;
  }
}

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

void loop() {
  readButtons();
  if (stateEnterGuard > 0) stateEnterGuard--;
  bool fireJustPressed = btnFire && !prevFireMenu;
  prevFireMenu = btnFire;
  
  frameCount++;

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
      updatePlayer();
      updateEnemyAI();
      
      if (player.recoilPitch < 0) player.recoilPitch++;
      if (fireCooldown > 0) fireCooldown--;
      if (player.hurtCooldown > 0) player.hurtCooldown--;

      u8g2.clearBuffer();
      
      if (player.hurtCooldown > 15) {
         u8g2.setDrawColor(1);
         u8g2.drawBox(0,0,128,64);
         u8g2.setDrawColor(0);
      }

      drawFloorCeiling();

      for (uint8_t col = 0; col < 128; col++) {
        int16_t rayAngle = (player.angle + (int16_t)pgm_read_word(&colAngleOffset[col])) & 1023;
        RayHit hit = castRay(player.posX, player.posY, rayAngle);
        
        if (!hit.hit) { zbuffer[col] = 255; continue; }
        
        int16_t relAngle = (rayAngle - player.angle) & 1023;
        int16_t cosRel = pgm_read_word(&cosTable[relAngle]);
        if (cosRel < 16) cosRel = 16; 
        
        int32_t correctedDist = ((int32_t)hit.dist * cosRel) >> 8;
        drawWallColumn(col, hit, correctedDist);
      }

      handleFire(); 
      drawSprites();
      drawWeapon();
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