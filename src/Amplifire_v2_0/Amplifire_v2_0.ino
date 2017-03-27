// Adafruit HUZZAH ESP8266

#include <Streaming.h>
#include <Metro.h>
#include <Bounce.h>
#include <EEPROM.h>
#define FASTLED_ESP8266_RAW_PIN_ORDER
#include <FastLED.h>
#include "Solenoid.h"
#include "Sensor.h"

Solenoid solenoid;
#define SOLENOID_PIN 15
#define SOLENOID_OFF LOW

Sensor sensor;
#define ANALOG_PIN A0

unsigned long simulationDelay = 5000UL;

#define NUM_LEDS 14
#define PIN_CLK 12 // yellow wire on LED strip
#define PIN_DATA 13 // green wire on LED strip
CRGB leds[NUM_LEDS];
#define MASTER_BRIGHTNESS 255

// saved to EEPROM
struct Settings {
  unsigned long onDuration;
  unsigned long offDuration;
  byte nCycles;
  boolean armed;

  byte thresholdPercent;
  unsigned long retriggerDelay;
};
Settings s;

Metro simulationInterval(1UL);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);


  // http://esp8266.github.io/Arduino/versions/2.0.0/doc/libraries.html#eeprom
  EEPROM.begin(512);

  s.onDuration = 300UL;
  s.offDuration = 100UL;
  s.nCycles = 10;
  s.armed = false;
  s.retriggerDelay = 5000UL;
  s.thresholdPercent = 80;

  saveEEPROM();
  loadEEPROM();

  s.armed = true;

  // set up the sensor
  sensor.begin(ANALOG_PIN);
  setSensorAction();

  // set up the solenoid
  solenoid.begin(SOLENOID_PIN, SOLENOID_OFF);
  setSolenoidAction();

  // set the simulation up
  setSimulationAction();

  Serial << endl;

  // set up the LEDs
  FastLED.addLeds<WS2801, PIN_DATA, PIN_CLK, RGB>(leds, NUM_LEDS);

  // set master brightness control
  FastLED.setBrightness(MASTER_BRIGHTNESS);

}

void saveEEPROM() {
  // NEVER save armed=true
  boolean armedState = s.armed;
  s.armed = false;

  // push settings to EEPROM for power-up recovery
  EEPROM.put(0, s);
  EEPROM.commit();

  s.armed = armedState;
}

void loadEEPROM() {
  EEPROM.get(0, s);
  // just in case
  s.armed = false;
}

void loop() {

  // see if arming state has changed
  if ( ! s.armed ) {

    // see if we need to run another trial loop
    if ( !solenoid.running() ) {
      if ( simulationInterval.check() ) {
        solenoid.start();
      }
    } else {
      simulationInterval.reset();
    }

  } else {
    // check for sensor activity
    if ( !solenoid.running() ) {
      if ( sensor.analogTrue() )
        solenoid.start();
    }
  }

  // update the lights
  showSettings(s.armed, solenoid.isFiring());

  static Metro printInterval(1000UL);
  if ( printInterval.check() ) {
    sensor.show();
    solenoid.show();
    Serial << endl;
  }

}

void showSettings(boolean armed, boolean on) {
  // used colors
  const CRGB armedOn = CRGB(255, 0, 0);
  const CRGB armedOff = CRGB(64, 0, 0);
  const CRGB disarmedOn = CRGB(0, 0, 255);
  const CRGB disarmedOff = CRGB(0, 0, 64);

  // track color
  static CRGB color = disarmedOff;

  if (armed && on) color = blend(color, armedOn, 50);
  else if (armed && !on) color = blend(color, armedOff, 50);
  else if (!armed && on) color = blend(color, disarmedOn, 5);
  else if (!armed && !on) color = blend(color, disarmedOff, 5);

  fill_solid(leds, NUM_LEDS, color);

  FastLED.show();
}

void setSimulationAction() {
  simulationInterval.interval( s.retriggerDelay );
}

void setSensorAction() {
  sensor.setRetriggerDelay(s.retriggerDelay + (s.onDuration + s.offDuration)*s.nCycles);
  sensor.setThreshold(s.thresholdPercent);

  sensor.show();
}

void setSolenoidAction() {
  solenoid.set(s.onDuration, s.offDuration, s.nCycles);
  solenoid.stop();
  if ( s.armed ) solenoid.arm();
  else solenoid.disarm();

  solenoid.show();
}
