// ===========================================================
// Quantum-Inspired Green Hydrogen Forecasting (ESP32 + RGB LED)
// Full Version with 3-Bar Serial Graph + HYBRID SENSOR MODE
// With Quantum Memory Weight (Stabilization) + OLED Bar Graph
// ===========================================================

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- SCREEN CONFIG ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

int graphX = 0;
float previousValue = -1;

// --- TWO DISPLAY BUSSES ---
TwoWire I2Cone = TwoWire(0);
TwoWire I2Ctwo = TwoWire(1);

// --- DISPLAY DEFINITIONS ---
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &I2Cone, -1);
Adafruit_SSD1306 display2(SCREEN_WIDTH, SCREEN_HEIGHT, &I2Ctwo, -1);

// --- Pin Definitions ---
const int LDR_AO_PIN = 35;
const int RGB_RED_PIN = 25;
const int RGB_GREEN_PIN = 26;
const int RGB_BLUE_PIN = 27;

int ldrValue = 0;
int potValue = 0;

// --- ADDED FOR CSV + PLOTTING ---
int readingCount = 0;

// --- HYBRID CONFIG ---
float SIM_STRENGTH_SOLAR = 0.30;

// --- MEMORY WEIGHT ---
String lastForecast = "remain";
float memoryStrength = 0.25;

// === OLED2 graph buffer (H2 % over time) ===
#define GRAPH_WIDTH 128
float graphBuffer[GRAPH_WIDTH] = {0};
int graphIndex = 0;

// Function prototypes
void drawHydrogenGraphOnOLED2(float hydrogenPercent);
void printBarGraph3(float solar, float water, float energyPotential, String forecast);
void drawBarGraphOLED(float solar, float water, float energyPotential);
void fadeToColor(int r, int g, int b, int duration);

// ===========================================================
void setup() {

  Serial.begin(115200);

  // --- OLED 1 SETUP ---
  I2Cone.begin(21, 22, 400000);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED 1 FAILED. Check wiring."));
    for(;;);
  }

  // --- OLED 2 SETUP ---
  I2Ctwo.begin(19, 23, 400000);
  if (!display2.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED 2 FAILED. Check wiring."));
    for(;;);
  }

  delay(500);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  display.println("Booting System...");
  display.display();

  display2.clearDisplay();
  display2.setTextSize(1);
  display2.setTextColor(WHITE);
  display2.setCursor(0, 10);
  display2.println("Booting System...");
  display2.display();
  delay(1000);

  Serial.println();
  Serial.println("Quantum-Inspired Green Hydrogen Forecasting");
  Serial.println("------------------------------------------------");
  Serial.println("System ready.\n");

  pinMode(RGB_RED_PIN, OUTPUT);
  pinMode(RGB_GREEN_PIN, OUTPUT);
  pinMode(RGB_BLUE_PIN, OUTPUT);

  digitalWrite(RGB_RED_PIN, HIGH);
  digitalWrite(RGB_GREEN_PIN, HIGH);
  digitalWrite(RGB_BLUE_PIN, HIGH);

  randomSeed(analogRead(36));

  Serial.println("CSV:solar,water,forecast");
}

// ===========================================================
void loop() {

  int realLDR = analogRead(LDR_AO_PIN);

  static float t2 = 0;
  static float t = 0;

  int simLDR = (sin(t2) * 0.5 + 0.5) * 4095;
  int simPOT = (sin(t) * 0.5 + 0.5) * 4095;

  t2 += 0.008;
  t += 0.01;

  ldrValue = (1.0 - SIM_STRENGTH_SOLAR) * realLDR + SIM_STRENGTH_SOLAR * simLDR;
  potValue = simPOT;

  float invertedLdrValue = 4095.0 - ldrValue;

  float solar = invertedLdrValue / 4095.0;
  float water = potValue / 4095.0;

  float energyPotential = (0.6 * solar + 0.4 * water);

  float p_increase = energyPotential;
  float p_same = (1 - fabs(solar - water)) * 0.3;
  float p_decrease = 1.0 - (p_increase + p_same);

  if (p_decrease < 0) p_decrease = 0;

  float total = p_increase + p_same + p_decrease;
  p_increase /= total;
  p_same /= total;
  p_decrease /= total;

  if (lastForecast == "increase") p_increase += memoryStrength;
  else if (lastForecast == "remain") p_same += memoryStrength;
  else if (lastForecast == "decrease") p_decrease += memoryStrength;

  total = p_increase + p_same + p_decrease;
  p_increase /= total;
  p_same /= total;
  p_decrease /= total;

  float r = random(0, 1000) / 1000.0;

  String forecast;
  int targetR = 0, targetG = 0, targetB = 0;

  if (r < p_increase) {
    forecast = "increase";
    targetG = 255;
  } else if (r < (p_increase + p_same)) {
    forecast = "remain";
    targetB = 255;
  } else {
    forecast = "decrease";
    targetR = 255;
  }

  lastForecast = forecast;

  Serial.print("CSV:");
  Serial.print(solar, 3);
  Serial.print(",");
  Serial.print(water, 3);
  Serial.print(",");
  Serial.println(forecast);

  readingCount++;
  if (readingCount >= 10) {
    Serial.print("solar:");
    Serial.println(solar * 100);
    Serial.print("water:");
    Serial.println(water * 100);
    readingCount = 0;
  }

  Serial.println("------------------------------------------------");
  printBarGraph3(solar, water, energyPotential, forecast);
  Serial.println("------------------------------------------------\n");

  auto updateOLED = [&](Adafruit_SSD1306 &scr) {
    scr.clearDisplay();
    scr.setTextSize(1);
    scr.setTextColor(WHITE);

    scr.setCursor(0, 0);
    scr.println("H2 FORECAST SYSTEM");
    scr.drawLine(0, 9, 128, 9, WHITE);

    scr.setCursor(0, 12);
    scr.print("Solar: ");
    scr.print(solar * 100, 0);
    scr.println("%");

    scr.setCursor(0, 22);
    scr.print("Water: ");
    scr.print(water * 100, 0);
    scr.println("%");

    drawBarGraphOLED(solar, water, energyPotential);

    scr.setCursor(0, 56);
    scr.print("FCST: ");
    if (forecast == "increase") scr.print("INCREASE (+)");
    else if (forecast == "remain") scr.print("STABLE (=)");
    else scr.print("DECREASE (-)");

    scr.display();
  };

  updateOLED(display);
  updateOLED(display2);

  drawHydrogenGraphOnOLED2(energyPotential * 100);

  fadeToColor(targetR, targetG, targetB, 800);

  delay(1500);
}

// ===========================================================
void printBarGraph3(float solar, float water, float energyPotential, String forecast) {

  int solarBar = solar * 50;
  int waterBar = water * 50;
  int hydrogenBar = energyPotential * 50;

  Serial.println();
  Serial.println("Graph (each '#' about 2 percent)");

  Serial.print("Solar : [");
  for (int i = 0; i < solarBar; i++) Serial.print("#");
  for (int i = solarBar; i < 50; i++) Serial.print(" ");
  Serial.print("] ");
  Serial.print(solar * 100, 1);
  Serial.println("%");

  Serial.print("Water : [");
  for (int i = 0; i < waterBar; i++) Serial.print("#");
  for (int i = waterBar; i < 50; i++) Serial.print(" ");
  Serial.print("] ");
  Serial.print(water * 100, 1);
  Serial.println("%");

  Serial.print("H2 : [");
  for (int i = 0; i < hydrogenBar; i++) Serial.print("#");
  for (int i = hydrogenBar; i < 50; i++) Serial.print(" ");
  Serial.print("] ");
  Serial.print(energyPotential * 100, 1);
  Serial.print("% -> ");
  Serial.println(forecast);
}

// ===========================================================
void drawBarGraphOLED(float solar, float water, float energyPotential) {

  const int maxChars = 18;
  int solarChars = (int)(solar * maxChars);
  int waterChars = (int)(water * maxChars);
  int h2Chars = (int)(energyPotential * maxChars);

  if (solarChars > maxChars) solarChars = maxChars;
  if (waterChars > maxChars) waterChars = maxChars;
  if (h2Chars > maxChars) h2Chars = maxChars;

  int startY = 32;
  int lineH = 7;

  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(0, startY);
  display.print("S:");
  for (int i = 0; i < maxChars; i++) display.print(i < solarChars ? "#" : " ");

  display.setCursor(0, startY + lineH);
  display.print("W:");
  for (int i = 0; i < maxChars; i++) display.print(i < waterChars ? "#" : " ");

  display.setCursor(0, startY + 2 * lineH);
  display.print("H:");
  for (int i = 0; i < maxChars; i++) display.print(i < h2Chars ? "#" : " ");
}

// ===========================================================
void fadeToColor(int r, int g, int b, int duration) {

  static int currR = 0, currG = 0, currB = 0;

  int steps = 50;
  int stepDelay = duration / steps;

  for (int i = 0; i <= steps; i++) {

    int newR = currR + (r - currR) * i / steps;
    int newG = currG + (g - currG) * i / steps;
    int newB = currB + (b - currB) * i / steps;

    analogWrite(RGB_RED_PIN, 255 - newR);
    analogWrite(RGB_GREEN_PIN, 255 - newG);
    analogWrite(RGB_BLUE_PIN, 255 - newB);

    delay(stepDelay);
  }

  currR = r;
  currG = g;
  currB = b;
}

// ===========================================================
// OLED2 LIVE GRAPH FUNCTION
// ===========================================================
void drawHydrogenGraphOnOLED2(float hydrogenPercent) {

  graphBuffer[graphIndex] = hydrogenPercent;
  graphIndex = (graphIndex + 1) % GRAPH_WIDTH;

  display2.clearDisplay();

  display2.drawLine(0, 63, 127, 63, WHITE);
  display2.drawLine(0, 0, 0, 63, WHITE);

  for(int x = 0; x < GRAPH_WIDTH - 1; x++) {

    int currentIndex = (graphIndex + x) % GRAPH_WIDTH;
    int nextIndex = (graphIndex + x + 1) % GRAPH_WIDTH;

    int y1 = 63 - map(graphBuffer[currentIndex], 0, 100, 0, 63);
    int y2 = 63 - map(graphBuffer[nextIndex], 0, 100, 0, 63);

    display2.drawLine(x, y1, x+1, y2, WHITE);
  }

  display2.setTextSize(1);
  display2.setCursor(2, 2);
  display2.println("H2 Production (%)");

  display2.display();
}
// ===========================================================
