// Copyright (c) 2020 matsujirushi
// Released under the MIT license
// https://github.com/matsujirushi/uart-to-azureiot/blob/master/LICENSE.txt

static constexpr int TEMPERATURE_MIN = 10;
static constexpr int TEMPERATURE_MAX = 40;
static constexpr int HUMIDITY_MIN = 0;
static constexpr int HUMIDITY_MAX = 100;

static constexpr char PART_SEPARATOR = '\t';

static constexpr int INTERVAL = 5000; // [msec.]

void setup()
{
  Serial.begin(115200, SERIAL_8N1);
}

void loop()
{
  float temperature;
  float humidity;
  GetTemperatureAndHumidityRandom(&temperature, &humidity);
  
  String temperatureStr{ temperature, 1 };
  temperatureStr.trim();
  String humidityStr{ humidity, 0 };
  humidityStr.trim();
  
  String message;
  message += "temperature:" + temperatureStr;
  message += PART_SEPARATOR;
  message += "humidity:" + humidityStr;
  Serial.println(message);
  
  delay(INTERVAL);
}

void GetTemperatureAndHumidityRandom(float* temperature, float* humidity)
{
  *temperature = (float)random(TEMPERATURE_MIN * 10, TEMPERATURE_MAX * 10 + 1) / 10.0f;
  *humidity = (float)random(HUMIDITY_MIN, HUMIDITY_MAX + 1);
}
