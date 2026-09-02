#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <ArduinoJson.h>

#define WIFI_SSID       "EBIW-AP"
#define WIFI_PASSWORD   "1234567890"

#define FW_VERSION      "2.0.3"

// GitHub manifest URL
const char* manifestURL =
    "https://raw.githubusercontent.com/shakir-ebiw/SmartWeld_Firmware/main/manifest.json";

// Check OTA every 1 week
#define OTA_CHECK_INTERVAL 604800000UL

unsigned long lastOTACheck = 0;


// =====================================================
// Connect WiFi
// =====================================================

void connectWiFi()
{
    if (WiFi.status() == WL_CONNECTED)
        return;

    Serial.println();
    Serial.println("Connecting to WiFi...");

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long startTime = millis();

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");

        if (millis() - startTime > 30000)
        {
            Serial.println();
            Serial.println("WiFi connection timeout.");
            return;
        }
    }

    Serial.println();
    Serial.println("WiFi connected.");

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
}


// =====================================================
// Compare versions
// =====================================================

bool isNewVersion(String currentVersion, String newVersion)
{
    int cMajor, cMinor, cPatch;
    int nMajor, nMinor, nPatch;

    sscanf(currentVersion.c_str(),
           "%d.%d.%d",
           &cMajor,
           &cMinor,
           &cPatch);

    sscanf(newVersion.c_str(),
           "%d.%d.%d",
           &nMajor,
           &nMinor,
           &nPatch);

    if (nMajor > cMajor)
        return true;

    if (nMajor == cMajor && nMinor > cMinor)
        return true;

    if (nMajor == cMajor &&
        nMinor == cMinor &&
        nPatch > cPatch)
        return true;

    return false;
}


// =====================================================
// Check GitHub for new firmware
// =====================================================

void checkForOTA()
{
    Serial.println();
    Serial.println("==============================");
    Serial.println("Checking OTA...");
    Serial.println("==============================");

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi not connected.");
        return;
    }

    WiFiClientSecure client;

    // TEST ONLY
    client.setInsecure();

    HTTPClient http;

    if (!http.begin(client, manifestURL))
    {
        Serial.println("Manifest connection failed.");
        return;
    }

    Serial.println("Downloading manifest...");

    int httpCode = http.GET();

    Serial.print("HTTP Code: ");
    Serial.println(httpCode);

    if (httpCode != HTTP_CODE_OK)
    {
        Serial.println("Manifest download failed.");
        http.end();
        return;
    }

    String payload = http.getString();

    http.end();

    Serial.println("Manifest received:");
    Serial.println(payload);


    // =================================================
    // Parse JSON
    // =================================================

    StaticJsonDocument<512> doc;

    DeserializationError error =
        deserializeJson(doc, payload);

    if (error)
    {
        Serial.print("JSON error: ");
        Serial.println(error.c_str());
        return;
    }

    String latestVersion =
        doc["version"].as<String>();

    String firmwareURL =
        doc["firmware"].as<String>();


    // =================================================
    // Display Version Information
    // =================================================

    Serial.println();
    Serial.println("------------------------------");
    Serial.print("Current Firmware : ");
    Serial.println(FW_VERSION);

    Serial.print("Latest Firmware  : ");
    Serial.println(latestVersion);
    Serial.println("------------------------------");


    // =================================================
    // Check Version
    // =================================================

    if (!isNewVersion(FW_VERSION, latestVersion))
    {
        Serial.println();
        Serial.println("Firmware is already up to date.");
        Serial.println("No OTA update required.");

        return;
    }


    // =================================================
    // NEW VERSION AVAILABLE
    // =================================================

    Serial.println();
    Serial.println("********************************");
    Serial.println(" NEW FIRMWARE AVAILABLE!");
    Serial.println("********************************");

    Serial.print("Current Version : ");
    Serial.println(FW_VERSION);

    Serial.print("New Version     : ");
    Serial.println(latestVersion);

    Serial.println();
    Serial.println("Firmware URL:");
    Serial.println(firmwareURL);

    Serial.println();
    Serial.println("Downloading firmware...");
    Serial.println("Download started...");


    // =================================================
    // OTA UPDATE
    // =================================================

    ESPhttpUpdate.rebootOnUpdate(true);

    t_httpUpdate_return result =
        ESPhttpUpdate.update(client, firmwareURL);


    // =================================================
    // OTA RESULT
    // =================================================

    switch (result)
    {
        case HTTP_UPDATE_FAILED:

            Serial.println();
            Serial.println("********************************");
            Serial.println(" OTA UPDATE FAILED");
            Serial.println("********************************");

            Serial.printf(
                "Error Code: %d\n",
                ESPhttpUpdate.getLastError()
            );

            Serial.print("Error Message: ");
            Serial.println(
                ESPhttpUpdate.getLastErrorString()
            );

            break;


        case HTTP_UPDATE_NO_UPDATES:

            Serial.println();
            Serial.println("No update available.");

            break;


        case HTTP_UPDATE_OK:

            Serial.println();
            Serial.println("********************************");
            Serial.println(" OTA UPDATE SUCCESSFUL");
            Serial.println("********************************");

            Serial.println("ESP8266 will restart...");

            break;
    }
}


// =====================================================
// SETUP
// =====================================================

void setup()
{
    Serial.begin(115200);

    delay(1000);

    Serial.println();
    Serial.println("==============================");
    Serial.println("SmartWeld ESP8266");
    Serial.println("==============================");

    Serial.print("Firmware Version: ");
    Serial.println(FW_VERSION);

    connectWiFi();


    // =================================================
    // Check OTA immediately after boot
    // =================================================

    if (WiFi.status() == WL_CONNECTED)
    {
        checkForOTA();
    }

    lastOTACheck = millis();

    Serial.println();
    Serial.println("Device started.");
}


// =====================================================
// LOOP
// =====================================================

void loop()
{
    // Your normal application code

    Serial.println("SmartWeld device running...");

    delay(5000);


    // =================================================
    // Check OTA once every week
    // =================================================

    if (millis() - lastOTACheck >= OTA_CHECK_INTERVAL)
    {
        lastOTACheck = millis();

        connectWiFi();

        if (WiFi.status() == WL_CONNECTED)
        {
            checkForOTA();
        }
    }
}