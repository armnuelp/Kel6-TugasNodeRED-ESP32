#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include "EspMQTTClient.h"
#include <DHT.h>

// ===== KONFIGURASI KELOMPOK, RUANGAN, DAN JARINGAN =====
#define KELOMPOK "klp6"
#define ROOM "ruang4"
#define WIFI_SSID "KOS KEJAWAN 24"
#define WIFI_PASS "studenthousing"
#define BROKER "192.168.100.40"

// ===== TOPIC MQTT =====
#define PREFIX "pens/" KELOMPOK "/" ROOM

const char *T_TEMP = PREFIX "/temperature";
const char *T_HUM = PREFIX "/humidity";
const char *T_L1 = PREFIX "/lamp1";
const char *T_L2 = PREFIX "/lamp2";
const char *T_STATUS = PREFIX "/status";

// ===== KONFIGURASI PIN ESP32 =====
#define DHT_PIN 4
#define DHT_TYPE DHT11
const int L1_PIN = 33;
const int L2_PIN = 32;

DHT dht(DHT_PIN, DHT_TYPE);

// ===== KONEKSI MQTT =====
EspMQTTClient client(
    WIFI_SSID,
    WIFI_PASS,
    BROKER,
    "",
    "",
    "esp32-" KELOMPOK "-" ROOM,
    1883);

// ===== FUNGSI KONTROL LAMPU =====
void setLamp(int pin, const String &payload)
{
    if (payload == "1")
    {
        digitalWrite(pin, HIGH);
        Serial.println("Lampu ON");
    }
    else if (payload == "0")
    {
        digitalWrite(pin, LOW);
        Serial.println("Lampu OFF");
    }
}

// ===== SETUP =====
void setup()
{
    Serial.begin(115200);

    pinMode(L1_PIN, OUTPUT);
    pinMode(L2_PIN, OUTPUT);
    digitalWrite(L1_PIN, LOW);
    digitalWrite(L2_PIN, LOW);

    dht.begin();

    client.enableDebuggingMessages();
    client.setKeepAlive(15);
    client.enableLastWillMessage(T_STATUS, "offline", true);

    Serial.println("ESP32 MQTT Starting...");
}

// ===== CALLBACK KETIKA MQTT TERHUBUNG =====
void onConnectionEstablished()
{
    Serial.println("MQTT Connection established.");

    client.subscribe(T_L1, [](const String &payload)
                     { setLamp(L1_PIN, payload); }, 1);

    client.subscribe(T_L2, [](const String &payload)
                     { setLamp(L2_PIN, payload); }, 1);

    client.publish(T_STATUS, "online", true);
}

// ===== LOOP UTAMA =====
unsigned long lastPublish = 0;

void loop()
{
    client.loop();

    if (!client.isMqttConnected())
    {
        return;
    }

    unsigned long now = millis();

    // Publish setiap 5 detik
    if (now - lastPublish >= 5000)
    {
        lastPublish = now;

        // Membaca DHT11
        float suhu = dht.readTemperature();
        float lembap = dht.readHumidity();

        // Jika gagal baca, jangan publish data sampah (NaN)
        if (isnan(suhu) || isnan(lembap))
        {
            Serial.println("Gagal membaca DHT11, cek wiring/pin data");
            return;
        }

        client.publish(T_TEMP, String(suhu, 1));
        client.publish(T_HUM, String(lembap, 1));

        Serial.print("Suhu: ");
        Serial.print(suhu, 1);
        Serial.print(" C | Kelembapan: ");
        Serial.print(lembap, 1);
        Serial.println(" %");
    }
}