// ===========================================================
// Quantum-Inspired Green Hydrogen Forecasting (ESP32 + RGB LED)
// Full Version with 3-Bar Serial Graph + HYBRID SENSOR MODE
// With Quantum Memory Weight (Stabilization) + OLED Bar Graph + Live Line Graph
// ===========================================================

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- SCREEN CONFIG ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define GRAPH_WIDTH 128
#define GRAPH_HEIGHT 50

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

// --- Variables ---
int ldrValue = 0;
int potValue = 0;
int readingCount = 0;

float SIM_STRENGTH_SOLAR = 0.30;
String lastForecast = "remain";
float memoryStrength = 0.25;

// OLED2 graph variables
int graphX = 0;
float previousValue = -1;
int markerCounter = 0;

// Previous hydrogen production tracking
float previousHydrogenProduction = -1;
float previousHydrogenPercentage = -1;

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
  
  // --- QUANTUM NOISE  ---
float quantumNoise = (random(-300, 300) / 1000.0);

p_increase += quantumNoise * 0.4;
p_same     += quantumNoise * -0.2; // inverse coupling
p_decrease += quantumNoise * 0.4;

// Prevent negatives
if (p_increase < 0) p_increase = 0;
if (p_same     < 0) p_same     = 0;
if (p_decrease < 0) p_decrease = 0;

// Renormalize again
total = p_increase + p_same + p_decrease;
p_increase /= total;
p_same     /= total;
p_decrease /= total;


  float r = random(0, 1000) / 1000.0;

  String forecast;
  int targetR = 0, targetG = 0, targetB = 0;

  // Determine forecast based on hydrogen production change
  float currentHydrogenPercentage = energyPotential * 100;
  
  if (previousHydrogenPercentage < 0) {
    // First reading, default to remain
    forecast = "remain";
    targetB = 255;
  } else {
    if (currentHydrogenPercentage > previousHydrogenPercentage) {
      forecast = "increase";
      targetG = 255;
    } else if (currentHydrogenPercentage < previousHydrogenPercentage) {
      forecast = "decrease";
      targetR = 255;
    } else {
      forecast = "remain";
      targetB = 255;
    }
  }

  // Update previous hydrogen percentage
  previousHydrogenPercentage = currentHydrogenPercentage;

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

  // --- OLED1 and OLED2 Bar Graphs ---
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

updateOLED(display); // OLED1 bar + forecast


  // --- OLED2 live line graph ---
  drawHydrogenGraphOnOLED2(energyPotential * 100);

  fadeToColor(targetR, targetG, targetB, 100);

  delay(100);
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
// OLED2 LEFT-TO-RIGHT LIVE LINE GRAPH
// ===========================================================
void drawHydrogenGraphOnOLED2(float hydrogenPercent) {

  int y = map(hydrogenPercent, 0, 100, GRAPH_HEIGHT, 0); // invert Y axis

  // Only clear at the start of the graph
  if (graphX == 0) {
    display2.clearDisplay();
    display2.drawLine(0, 0, 0, GRAPH_HEIGHT, WHITE);        // Y-axis
    display2.drawLine(0, GRAPH_HEIGHT, GRAPH_WIDTH, GRAPH_HEIGHT, WHITE); // X-axis
    display2.setTextSize(1);
    display2.setCursor(10, 0);
    display2.print("H2 Production (%)");
    previousValue = y;
  }

  // Draw connecting line
  if (previousValue >= 0) {
    display2.drawLine(graphX - 1, previousValue, graphX, y, WHITE);
  }

  // Draw marker every few loops
  markerCounter++;
  if (markerCounter >= 2) {
    display2.drawPixel(graphX, y, WHITE);
    markerCounter = 0;
  }

  previousValue = y;
  graphX++;

  // Reset when reaching end
  if (graphX >= GRAPH_WIDTH) {
    graphX = 0;
    previousValue = -1;
  }

  // **Do not clear display here!** Only draw new points
  display2.display();
}