#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <vector>

constexpr uint8_t PROXIMITY_PIN = A0;
constexpr uint8_t SPEAKER_PIN = 10;
constexpr uint8_t SD_CS_PIN = 17;

constexpr uint16_t TRIGGER_THRESHOLD = 3100;
constexpr uint32_t SAMPLE_RATE = 16000;
constexpr uint32_t SAMPLE_PERIOD_US = 1000000UL / SAMPLE_RATE;
constexpr uint32_t COOLDOWN_MS = 1500;

std::vector<String> clips;
bool sdReady = false;
unsigned long lastTrigger = 0;

bool loadClips() {
  if (!sdReady) {
    return false;
  }

  clips.clear();
  File dir = SD.open("/audio");
  if (!dir || !dir.isDirectory()) {
    Serial.println(F("Put clips inside /audio on the SD card."));
    return false;
  }

  while (File entry = dir.openNextFile()) {
    if (!entry.isDirectory()) {
      String name = entry.name();
      name.toLowerCase();
      if (name.endsWith(".wav") || name.endsWith(".raw") || name.endsWith(".pcm")) {
        clips.push_back(String("/audio/") + entry.name());
      }
    }
    entry.close();
  }
  dir.close();

  Serial.print(F("Clips found: "));
  Serial.println(clips.size());
  return !clips.empty();
}

void playRandomClip() {
  if (clips.empty()) {
    if (!loadClips()) {
      return;
    }
  }

  const String path = clips[random(clips.size())];
  File clip = SD.open(path.c_str(), FILE_READ);
  if (!clip) {
    Serial.println(F("Could not open clip."));
    return;
  }

  Serial.print(F("Playing: "));
  Serial.println(path);

  while (clip.available()) {
    analogWrite(SPEAKER_PIN, clip.read());
    delayMicroseconds(SAMPLE_PERIOD_US);
  }
  clip.close();
  analogWrite(SPEAKER_PIN, 0);
}

void setup() {
  Serial.begin(115200);
  pinMode(PROXIMITY_PIN, INPUT);
  pinMode(SPEAKER_PIN, OUTPUT);

  if (SD.begin(SD_CS_PIN)) {
    sdReady = true;
    loadClips();
  } else {
    Serial.println(F("SD mount failed."));
  }

  randomSeed(analogRead(PROXIMITY_PIN) ^ micros());
}

void loop() {
  const uint16_t reading = analogRead(PROXIMITY_PIN);
  const unsigned long now = millis();

  if (reading >= TRIGGER_THRESHOLD && (now - lastTrigger) > COOLDOWN_MS) {
    lastTrigger = now;
    if (sdReady) {
      playRandomClip();
    } else if (SD.begin(SD_CS_PIN)) {
      sdReady = true;
      playRandomClip();
    }
  }

  delay(10);
}
