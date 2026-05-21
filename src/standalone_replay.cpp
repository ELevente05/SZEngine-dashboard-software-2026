#include "standalone_replay.h"

#ifdef STANDALONE_REPLAY

#include <Arduino.h>
#include <SPIFFS.h>
#include <math.h>

extern volatile int activeScreen;

extern volatile float oilTemp;
extern volatile float oilPress;
extern volatile float engineWaterTemp;
extern volatile float icWaterTemp;
extern volatile float lambdaVal;
extern volatile float intakeTemp;
extern volatile float egt;
extern volatile float batteryVolts;
extern volatile float boostPressure;
extern volatile float hybridTemp;
extern volatile float hybridVolts;
extern volatile int currentGear;
extern volatile int stateOfCharge;
extern volatile int rpm;

extern volatile float T_1;
extern volatile float T_2;
extern volatile float T_3;
extern volatile float T_4;
extern volatile float T_5;
extern volatile float T_6;
extern volatile float T_7;
extern volatile float T_8;
extern volatile float T_9;
extern volatile float T_10;
extern volatile float T_11;
extern volatile float T_12;
extern volatile float T_13;
extern volatile float T_14;
extern volatile float T_15;
extern volatile float T_16;

extern volatile float V_1;
extern volatile float V_2;
extern volatile float V_3;
extern volatile float V_4;
extern volatile float V_5;
extern volatile float V_6;
extern volatile float V_7;
extern volatile float V_8;
extern volatile float V_9;
extern volatile float V_10;
extern volatile float V_out;
extern volatile float I_out;

namespace {

constexpr char LOG_PATH[] = "/race.csv";
constexpr uint32_t REPLAY_INTERVAL_MS = 50;
constexpr uint32_t SCREEN_ROTATE_MS = 6000;
constexpr size_t LINE_BUFFER_SIZE = 256;
constexpr size_t MAX_FIELDS = 16;

struct ColumnMap {
  int battery = -1;
  int coolantTemp = -1;
  int intakeTemp = -1;
  int lambda = -1;
  int map = -1;
  int rpm = -1;
  int oilPress = -1;
  int oilTemp = -1;
  int egt = -1;
  int gear = -1;
};

File logFile;
ColumnMap columnMap;
char lineBuffer[LINE_BUFFER_SIZE];
char replayStatusBuffer[64] = "Replay idle";
bool replayReady = false;
uint32_t lastReplayUpdate = 0;
uint32_t lastScreenRotate = 0;
uint32_t replayRow = 0;

const float temperatureOffsets[16] = {
  -0.42f, -0.30f, -0.18f, -0.08f,
   0.02f,  0.10f,  0.18f,  0.28f,
  -0.24f, -0.14f, -0.04f,  0.08f,
   0.16f,  0.24f,  0.34f,  0.44f,
};

const float voltageOffsets[10] = {
  -0.03f, -0.02f, -0.01f, 0.00f, 0.01f,
   0.02f,  0.03f, -0.02f, 0.02f, 0.03f,
};

void setReplayStatus(const char* message) {
  snprintf(replayStatusBuffer, sizeof(replayStatusBuffer), "%s", message);
}

void trimLine(char* line) {
  size_t length = strlen(line);
  while (length > 0 && (line[length - 1] == '\r' || line[length - 1] == '\n')) {
    line[--length] = '\0';
  }
}

int splitCsvLine(char* line, char* fields[], const int maxFields) {
  int count = 0;
  fields[count++] = line;

  for (char* cursor = line; *cursor != '\0' && count < maxFields; ++cursor) {
    if (*cursor == ',') {
      *cursor = '\0';
      fields[count++] = cursor + 1;
    }
  }

  return count;
}

bool readCsvLine(char* destination, const size_t destinationSize) {
  while (logFile && logFile.available()) {
    const size_t bytesRead = logFile.readBytesUntil('\n', destination, destinationSize - 1);
    destination[bytesRead] = '\0';
    trimLine(destination);
    if (destination[0] != '\0') {
      return true;
    }
  }

  return false;
}

bool mapColumnsFromHeader(char* headerLine) {
  char* fields[MAX_FIELDS];
  const int fieldCount = splitCsvLine(headerLine, fields, MAX_FIELDS);

  columnMap = ColumnMap{};
  for (int index = 0; index < fieldCount; ++index) {
    if (strcmp(fields[index], "Battery voltage [21]") == 0) {
      columnMap.battery = index;
    } else if (strcmp(fields[index], "Coolant temp [18]") == 0) {
      columnMap.coolantTemp = index;
    } else if (strcmp(fields[index], "Intake air temp [17]") == 0) {
      columnMap.intakeTemp = index;
    } else if (strcmp(fields[index], "Lambda [5]") == 0) {
      columnMap.lambda = index;
    } else if (strcmp(fields[index], "MAP [20]") == 0) {
      columnMap.map = index;
    } else if (strcmp(fields[index], "RPM [61]") == 0) {
      columnMap.rpm = index;
    } else if (strcmp(fields[index], "Engine Oil Pressure [804]") == 0) {
      columnMap.oilPress = index;
    } else if (strcmp(fields[index], "Engine Oil Temp [805]") == 0) {
      columnMap.oilTemp = index;
    } else if (strcmp(fields[index], "EGT 1  [128]") == 0) {
      columnMap.egt = index;
    } else if (strcmp(fields[index], "VSS Gear [90]") == 0) {
      columnMap.gear = index;
    }
  }

  return columnMap.battery >= 0 && columnMap.coolantTemp >= 0 && columnMap.intakeTemp >= 0 &&
         columnMap.lambda >= 0 && columnMap.map >= 0 && columnMap.rpm >= 0 &&
         columnMap.oilPress >= 0 && columnMap.oilTemp >= 0 && columnMap.egt >= 0 &&
         columnMap.gear >= 0;
}

bool openReplayFile() {
  if (logFile) {
    logFile.close();
  }

  logFile = SPIFFS.open(LOG_PATH, FILE_READ);
  if (!logFile) {
    setReplayStatus("Log file missing");
    return false;
  }

  if (!readCsvLine(lineBuffer, sizeof(lineBuffer))) {
    setReplayStatus("Log file empty");
    logFile.close();
    return false;
  }

  if (!mapColumnsFromHeader(lineBuffer)) {
    setReplayStatus("CSV header mismatch");
    logFile.close();
    return false;
  }

  setReplayStatus("Replaying onboard log");
  return true;
}

float readFloatField(char* fields[], const int fieldCount, const int index, const float fallbackValue) {
  if (index < 0 || index >= fieldCount || fields[index][0] == '\0') {
    return fallbackValue;
  }

  return atof(fields[index]);
}

int readIntField(char* fields[], const int fieldCount, const int index, const int fallbackValue) {
  return static_cast<int>(roundf(readFloatField(fields, fieldCount, index, static_cast<float>(fallbackValue))));
}

void synthesizePackTelemetry() {
  const float phase = replayRow * 0.07f;
  const float packTempBase = constrain(hybridTemp, 20.0f, 80.0f);
  volatile float* packTemperatures[16] = {
    &T_1, &T_2, &T_3, &T_4,
    &T_5, &T_6, &T_7, &T_8,
    &T_9, &T_10, &T_11, &T_12,
    &T_13, &T_14, &T_15, &T_16,
  };

  for (int index = 0; index < 16; ++index) {
    *packTemperatures[index] = packTempBase + temperatureOffsets[index] + 0.25f * sinf(phase + index * 0.21f);
  }

  const float cellVoltageBase = constrain(V_out / 10.0f, 3.7f, 4.2f);
  volatile float* cellVoltages[10] = {
    &V_1, &V_2, &V_3, &V_4, &V_5,
    &V_6, &V_7, &V_8, &V_9, &V_10,
  };

  for (int index = 0; index < 10; ++index) {
    *cellVoltages[index] = cellVoltageBase + voltageOffsets[index] + 0.01f * sinf(phase * 0.7f + index * 0.3f);
  }
}

bool readNextReplayRow() {
  if (readCsvLine(lineBuffer, sizeof(lineBuffer))) {
    return true;
  }

  if (!openReplayFile()) {
    return false;
  }

  return readCsvLine(lineBuffer, sizeof(lineBuffer));
}

void applyReplayRow() {
  char* fields[MAX_FIELDS];
  const int fieldCount = splitCsvLine(lineBuffer, fields, MAX_FIELDS);

  batteryVolts = readFloatField(fields, fieldCount, columnMap.battery, batteryVolts);
  engineWaterTemp = readFloatField(fields, fieldCount, columnMap.coolantTemp, engineWaterTemp);
  intakeTemp = readFloatField(fields, fieldCount, columnMap.intakeTemp, intakeTemp);
  lambdaVal = readFloatField(fields, fieldCount, columnMap.lambda, lambdaVal);
  boostPressure = readFloatField(fields, fieldCount, columnMap.map, boostPressure * 100.0f) / 100.0f;
  rpm = readIntField(fields, fieldCount, columnMap.rpm, rpm);
  oilPress = readFloatField(fields, fieldCount, columnMap.oilPress, oilPress);
  oilTemp = readFloatField(fields, fieldCount, columnMap.oilTemp, oilTemp);
  egt = readFloatField(fields, fieldCount, columnMap.egt, egt);
  currentGear = readIntField(fields, fieldCount, columnMap.gear, currentGear);

  const float phase = replayRow * 0.04f;
  icWaterTemp = constrain(engineWaterTemp - 20.0f, 25.0f, 95.0f);
  hybridTemp = 24.0f + fmaxf(0.0f, oilTemp - 80.0f) * 0.08f + 0.35f * sinf(phase);
  hybridVolts = 39.6f + 0.18f * sinf(phase * 0.8f);
  V_out = hybridVolts + 2.3f + 0.15f * sinf(phase * 0.6f);
  I_out = fmaxf(0.0f, boostPressure * 12.0f + fmaxf(0, rpm - 3500) * 0.0025f);
  stateOfCharge = constrain(100 - static_cast<int>((replayRow / 40) % 31), 70, 100);

  synthesizePackTelemetry();
  ++replayRow;
}

}  // namespace

void standaloneReplaySetup() {
  if (!SPIFFS.begin(true)) {
    setReplayStatus("SPIFFS mount failed");
    replayReady = false;
    return;
  }

  if (!openReplayFile()) {
    replayReady = false;
    return;
  }

  replayReady = true;
  lastReplayUpdate = millis();
  lastScreenRotate = millis();
  replayRow = 0;
  activeScreen = 1;
}

void standaloneReplayTick() {
  if (!replayReady) {
    return;
  }

  const uint32_t now = millis();

  if (now - lastReplayUpdate >= REPLAY_INTERVAL_MS) {
    lastReplayUpdate = now;
    if (readNextReplayRow()) {
      applyReplayRow();
    } else {
      replayReady = false;
      setReplayStatus("Replay stopped");
    }
  }

  if (now - lastScreenRotate >= SCREEN_ROTATE_MS) {
    lastScreenRotate = now;
    activeScreen = (activeScreen >= 4) ? 1 : activeScreen + 1;
  }
}

bool standaloneReplayReady() {
  return replayReady;
}

const char* standaloneReplayStatus() {
  return replayStatusBuffer;
}

#else

void standaloneReplaySetup() {}

void standaloneReplayTick() {}

bool standaloneReplayReady() {
  return false;
}

const char* standaloneReplayStatus() {
  return "disabled";
}

#endif