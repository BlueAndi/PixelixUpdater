/* MIT License
 *
 * Copyright (c) 2025 Andreas Merkle <web@blue-andi.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*******************************************************************************
 *  DESCRIPTION
 ******************************************************************************/
/**
 * @brief  Main entry point
 * @author Andreas Merkle <web@blue-andi.de>
 */

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <DNSServer.h>

#include <esp_log.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>

#include "EmbeddedFiles.h"

/******************************************************************************
 * Macros
 *****************************************************************************/

#ifndef CONFIG_ESP_LOG_SEVERITY
#define CONFIG_ESP_LOG_SEVERITY (ESP_LOG_INFO)
#endif /* CONFIG_ESP_LOG_SEVERITY */

/******************************************************************************
 * Types and Classes
 *****************************************************************************/

/**
 * State of the application.
 */
typedef enum
{
    STATE_IDLE,           /**< Idle state */
    STATE_STA_SETUP,      /**< Setup WiFi station */
    STATE_STA_CONNECTING, /**< Connecting to WiFi */
    STATE_STA_CONNECTED,  /**< Connected to WiFi */
    STATE_AP_SETUP,       /**< Setup Access Point */
    STATE_AP_UP,          /**< Access Point is up and running */
    STATE_ERROR           /**< Error state */

} State;

/******************************************************************************
 * Prototypes
 *****************************************************************************/

static void loadSettings();
static void appendDeviceUniqueId(String& deviceUniqueId);
static void getChipId(String& chipId);
static void setAppPartition0Active();
static void setupOta();
static void setupWebServer();
static void stateMachine();
static void stateIdle();
static void stateStaSetup();
static void stateStaConnecting();
static void stateStaConnected();
static void stateApSetup();
static void stateApUp();
static void stateError();
static void handleUpload();
static void handleFileUpload();

/******************************************************************************
 * Variables
 *****************************************************************************/

/** Serial interface baudrate. */
static const uint32_t SERIAL_BAUDRATE  = 115200U;

/** Task period in ms of the loop() task. */
static const uint32_t LOOP_TASK_PERIOD = 10U;

#if ARDUINO_USB_MODE
#if ARDUINO_USB_CDC_ON_BOOT /* Serial used for USB CDC */

/**
 * Minimize the USB tx timeout (ms) to avoid too long blocking behaviour during
 * writing e.g. log messages to it. If the value is too high, it will influence
 * the display refresh bad.
 */
static const uint32_t HWCDC_TX_TIMEOUT = 4U;

#endif /* ARDUINO_USB_CDC_ON_BOOT */
#endif /* ARDUINO_USB_MODE */

/**
 * Tag for logging purposes.
 */
static const char* LOG_TAG                    = "main";

/**
 * OTA password.
 */
static const char* OTA_PASSWORD               = "maytheforcebewithyou";

/**
 * SettingsService namespace used for preferences.
 */
static const char* PREF_NAMESPACE             = "settings";

/** Hostname key */
static const char* KEY_HOSTNAME               = "hostname";

/** Wifi network key */
static const char* KEY_WIFI_SSID              = "sta_ssid";

/** Wifi network passphrase key */
static const char* KEY_WIFI_PASSPHRASE        = "sta_passphrase";

/** Wifi Access Point SSID key */
static const char* KEY_WIFI_AP_SSID           = "ap_ssid";

/** Wifi Access Point passphrase key */
static const char* KEY_WIFI_AP_PASSPHRASE     = "ap_passphrase";

/** Hostname default value */
static const char* DEFAULT_HOSTNAME           = "pixelix";

/** Wifi network default value */
static const char* DEFAULT_WIFI_SSID          = "";

/** Wifi network passphrase default value */
static const char* DEFAULT_WIFI_PASSPHRASE    = "";

/** Wifi Access Point SSID default value */
static const char* DEFAULT_WIFI_AP_SSID       = "pixelix";

/** Wifi Access Point passphrase default value */
static const char* DEFAULT_WIFI_AP_PASSPHRASE = "Luke, I am your father.";

/**
 * The hostname of the device.
 */
static String gSettingsHostname = DEFAULT_HOSTNAME;

/**
 * WiFi SSID.
 */
static String gSettingsWifiSSID = DEFAULT_WIFI_SSID;

/**
 * WiFi passphrase.
 */
static String gSettingsWifiPassphrase = DEFAULT_WIFI_PASSPHRASE;

/**
 * WiFi Access Point SSID.
 */
static String gSettingsWifiApSSID = DEFAULT_WIFI_AP_SSID;

/**
 * WiFi Access Point passphrase.
 */
static String gSettingsWifiApPassphrase = DEFAULT_WIFI_AP_PASSPHRASE;

/**
 * Web server instance.
 */
static WebServer gWebServer(80U);

/**
 * Current state of the application.
 */
static State gState = STATE_IDLE;

/**
 * Set access point local address.
 *
 * The ip-address shall be from a public ip-address space and not from a private one,
 * like 192.168.0.0/16 or 172.16.0.0/12. This is required to get a pop-up notification
 * on Samsung mobile devices (Android OS) after wifi connection, which routes the
 * user to the captive portal.
 */
static const IPAddress LOCAL_IP(192U, 169U, 4U, 1U);

/* Set access point gateway address. */
static const IPAddress GATEWAY(192U, 169U, 4U, 1U);

/* Set access point subnet mask. */
static const IPAddress SUBNET(255U, 255U, 255U, 0U);

/* Set DNS port */
static const uint16_t DNS_PORT = 53U;

/**
 * DNS server instance.
 *
 * The DNS server is used to resolve the hostname to the access point local address.
 * This is required to get a pop-up notification on Samsung mobile devices (Android OS)
 * after wifi connection, which routes the user to the captive portal.
 */
static DNSServer gDnsServer;

/** Firmware binary filename, used for update. */
static const char* FIRMWARE_FILENAME   = "firmware.bin";

/** Filesystem binary filename, used for update. */
static const char* FILESYSTEM_FILENAME = "littlefs.bin";

/** Firmware binary size HTTP request header. */
static const char* FIRMWARE_SIZE_HEADER = "X-File-Size-Firmware";

/** Filesystem binary size HTTP request header.  */
static const char* FILESYSTEM_SIZE_HEADER = "X-File-Size-Filesystem";

/******************************************************************************
 * External functions
 *****************************************************************************/

/**
 * Setup the system.
 */
void setup()
{
    /* Setup serial interface */
    Serial.begin(SERIAL_BAUDRATE);

#if ARDUINO_USB_MODE
#if ARDUINO_USB_CDC_ON_BOOT
    Serial.setTxTimeoutMs(HWCDC_TX_TIMEOUT);
#endif /* ARDUINO_USB_CDC_ON_BOOT */
#endif /* ARDUINO_USB_MODE */

    /* Ensure a distance between the boot mode message and the first log message.
     * Otherwise the first log message appears in the same line than the last
     * boot mode message.
     */
    Serial.println("\n");

    /* Set severity for esp logging system. */
    esp_log_level_set("*", CONFIG_ESP_LOG_SEVERITY);

    /* Load settings from the persistent storage. */
    loadSettings();

    /* To avoid name clashes, add a unqiue id to the hostname. */
    appendDeviceUniqueId(gSettingsHostname);

    ESP_LOGI(LOG_TAG, "Target: %s", PIO_ENV);
    ESP_LOGI(LOG_TAG, "Version: %s", VERSION);
    ESP_LOGI(LOG_TAG, "Hostname: %s", gSettingsHostname.c_str());
    ESP_LOGI(LOG_TAG, "Partition: Factory");

    /* Start wifi */
    (void)WiFi.mode(WIFI_STA);

    setupOta();
    setupWebServer();
    setAppPartition0Active();
}

/**
 * Main loop, which is called periodically.
 */
void loop()
{
    stateMachine();
    ArduinoOTA.handle();
    gWebServer.handleClient();

    /* Schedule other tasks with same or lower priority. */
    delay(LOOP_TASK_PERIOD);
}

/******************************************************************************
 * Local functions
 *****************************************************************************/

/**
 * Load settings from preferences to be used by the application.
 * If no settings are found, default values will be used.
 *
 * The settings are stored in the preferences storage, which is a key-value
 * storage. The keys are defined by Pixelix!
 */
static void loadSettings()
{
    Preferences preferences;

    /* Open Preferences with namespace. Each application module, library, etc
     * has to use a namespace name to prevent key name collisions. We will open storage in
     * RW-mode (second parameter has to be false).
     * Note: Namespace name is limited to 15 chars.
     */
    bool status = preferences.begin(PREF_NAMESPACE, true);

    /* Settings not found? */
    if (false == status)
    {
        ESP_LOGW(LOG_TAG, "No settings found, using default values.");
        gSettingsHostname         = DEFAULT_HOSTNAME;
        gSettingsWifiSSID         = DEFAULT_WIFI_SSID;
        gSettingsWifiPassphrase   = DEFAULT_WIFI_PASSPHRASE;
        gSettingsWifiApSSID       = DEFAULT_WIFI_AP_SSID;
        gSettingsWifiApPassphrase = DEFAULT_WIFI_AP_PASSPHRASE;
    }
    /* Settings found. */
    else
    {
        gSettingsHostname         = preferences.getString(KEY_HOSTNAME, DEFAULT_HOSTNAME);
        gSettingsWifiSSID         = preferences.getString(KEY_WIFI_SSID, DEFAULT_WIFI_SSID);
        gSettingsWifiPassphrase   = preferences.getString(KEY_WIFI_PASSPHRASE, DEFAULT_WIFI_PASSPHRASE);
        gSettingsWifiApSSID       = preferences.getString(KEY_WIFI_AP_SSID, DEFAULT_WIFI_AP_SSID);
        gSettingsWifiApPassphrase = preferences.getString(KEY_WIFI_AP_PASSPHRASE, DEFAULT_WIFI_AP_PASSPHRASE);
    }

    preferences.end();
}

/**
 * Append device unique ID to string.
 * The device unique ID is derived from factory programmed wifi MAC address.
 *
 * @param[in,out] dst   Destination string to append the device unique id to.
 */
static void appendDeviceUniqueId(String& dst)
{
    /* Use the last 4 bytes of the factory programmed wifi MAC address to generate a unique id. */
    String chipId;

    getChipId(chipId);

    dst += "-";
    dst += chipId.substring(4U);
}

/**
 * Get the unique chip id.
 *
 * @param[out] chipId   Chip id
 */
static void getChipId(String& chipId)
{
    uint64_t efuseMAC    = ESP.getEfuseMac();
    int32_t  highPart    = (efuseMAC >> 8U) & 0x0000ffffU;
    int32_t  lowPart     = (efuseMAC >> 0U) & 0xffffffffU;
    size_t   BUFFER_SIZE = 13U;
    char     buffer[BUFFER_SIZE];

    (void)snprintf(buffer, sizeof(buffer), "%04X%08X", highPart, lowPart);

    chipId = buffer;
}

/**
 * Set the application partition 0 active to be considered as the boot partition.
 */
static void setAppPartition0Active()
{
    const esp_partition_t* partition = esp_partition_find_first(esp_partition_type_t::ESP_PARTITION_TYPE_APP, esp_partition_subtype_t::ESP_PARTITION_SUBTYPE_APP_OTA_0, nullptr);

    if (nullptr != partition)
    {
        ESP_LOGI(LOG_TAG, "Setting app0 partition '%s' as boot partition", partition->label);
        esp_ota_set_boot_partition(partition);
    }
}

/**
 * Setup OTA (Over-The-Air) updates.
 */
static void setupOta()
{
    (void)ArduinoOTA.setPassword(OTA_PASSWORD);
    (void)ArduinoOTA.setRebootOnSuccess(true);
    (void)ArduinoOTA.setMdnsEnabled(true);
    (void)ArduinoOTA.setHostname(gSettingsHostname.c_str());
    ArduinoOTA.begin();
}

/**
 * Setup the web server.
 */
static void setupWebServer()
{
    const char *headerKeys[] = {FIRMWARE_SIZE_HEADER, FILESYSTEM_SIZE_HEADER};
    size_t keyCount = sizeof(headerKeys) / sizeof(headerKeys[0]);

    /* Start the web server, before configuration! */
    gWebServer.begin();

    /* Webserver only keeps headers that are specified through collectHeaders(). */
    gWebServer.collectHeaders(headerKeys, keyCount);

    /* Configure web server */
    gWebServer.onNotFound(
        []() {
            gWebServer.sendHeader("Location", "/");
            gWebServer.send(302, "text/plain", "");
        });

    gWebServer.on("/", HTTP_GET, []() {
        gWebServer.sendHeader("Location", "/index.html");
        gWebServer.send(302, "text/plain", "");
    });

    gWebServer.on("/upload.html", HTTP_POST, handleUpload, handleFileUpload);

    EmbeddedFiles_setup(gWebServer);
}

/**
 * State machine function to handle the current state of the application.
 * This function is called periodically in the loop() function.
 */
static void stateMachine()
{
    switch (gState)
    {
    case STATE_IDLE:
        stateIdle();
        break;

    case STATE_STA_SETUP:
        stateStaSetup();
        break;

    case STATE_STA_CONNECTING:
        stateStaConnecting();
        break;

    case STATE_STA_CONNECTED:
        stateStaConnected();
        break;

    case STATE_AP_SETUP:
        stateApSetup();
        break;

    case STATE_AP_UP:
        stateApUp();
        break;

    case STATE_ERROR:
        stateError();
        break;

    default:
        ESP_LOGE(LOG_TAG, "Unknown state: %d", gState);
        break;
    }
}

/**
 * State machine function for the idle state.
 * This is the initial state of the application.
 */
static void stateIdle()
{
    if (true == gSettingsWifiSSID.isEmpty())
    {
        ESP_LOGI(LOG_TAG, "No WiFi SSID configured, starting in Access Point mode.");
        gState = STATE_AP_SETUP;
    }
    else
    {
        ESP_LOGI(LOG_TAG, "Setup WiFi station.");
        gState = STATE_STA_SETUP;
    }
}

/**
 * State machine function for the setup of the WiFi station.
 * This state is entered when the device is not connected to a WiFi network
 * and needs to setup the WiFi station.
 */
static void stateStaSetup()
{
    /* Setup WiFi station */
    if (false == WiFi.mode(WIFI_STA))
    {
        ESP_LOGE(LOG_TAG, "Failed to setup WiFi station mode.");
        gState = STATE_AP_SETUP;
    }
    else
    {
        gState = STATE_STA_CONNECTING;
    }
}

/**
 * State machine function for the connecting state.
 * This state is entered when the wifi station was setup successfully.
 */
static void stateStaConnecting()
{
    if (WL_CONNECTED != WiFi.status())
    {
        const uint32_t CONNECT_TIMEOUT_MS          = 10000U;
        const uint32_t CHECK_CONNECTION_TIMEOUT_MS = 100U;
        uint32_t       startTime                   = millis();

        /* Try to connect to the wifi network. */
        (void)WiFi.begin(gSettingsWifiSSID.c_str(), gSettingsWifiPassphrase.c_str());

        ESP_LOGI(LOG_TAG, "Connecting to WiFi '%s'...", gSettingsWifiSSID.c_str());

        /* Wait until connected. */
        while ((WL_CONNECTED != WiFi.status()) && ((millis() - startTime) < CONNECT_TIMEOUT_MS))
        {
            delay(CHECK_CONNECTION_TIMEOUT_MS);
        }

        if (WL_CONNECTED != WiFi.status())
        {
            ESP_LOGE(LOG_TAG, "Failed to connect to WiFi '%s'", gSettingsWifiSSID.c_str());
            ESP_LOGI(LOG_TAG, "Setup WiFi Access Point mode.");
            gState = STATE_AP_SETUP;
        }
        else
        {
            ESP_LOGI(LOG_TAG, "Connected to WiFi '%s'", gSettingsWifiSSID.c_str());
            ESP_LOGI(LOG_TAG, "IP address: %s", WiFi.localIP().toString().c_str());
        }
    }
}

/**
 * State machine function for the connected state.
 * This state is entered when the device is connected to the WiFi network.
 */
static void stateStaConnected()
{
    if (WL_CONNECTED != WiFi.status())
    {
        ESP_LOGE(LOG_TAG, "WiFi connection lost, switching to connecting state.");
        gState = STATE_STA_CONNECTING;
    }
}

/**
 * State machine function for the setup of the Access Point.
 * This state is entered when the device is not connected to a WiFi network
 * and needs to setup the Access Point.
 */
static void stateApSetup()
{
    /* Configure access point.
     * The DHCP server will automatically be started and uses the range x.x.x.1 - x.x.x.11
     */
    if (false == WiFi.softAPConfig(LOCAL_IP, GATEWAY, SUBNET))
    {
        ESP_LOGE(LOG_TAG, "Failed to configure Access Point.");
        gState = STATE_ERROR;
    }
    /* Set hostname. Note, wifi must be started, which is done
     * by setting the mode before.
     */
    else if (false == WiFi.softAPsetHostname(gSettingsHostname.c_str()))
    {
        ESP_LOGE(LOG_TAG, "Failed to set Access Point hostname.");
        gState = STATE_ERROR;
    }
    /* Setup wifi access point. */
    else if (false == WiFi.softAP(gSettingsWifiApSSID.c_str(), gSettingsWifiApPassphrase.c_str()))
    {
        ESP_LOGE(LOG_TAG, "Failed to setup Access Point.");
        gState = STATE_ERROR;
    }
    else
    {
        /* Start DNS and redirect to webserver. */
        if (false == gDnsServer.start(DNS_PORT, "*", WiFi.softAPIP()))
        {
            ESP_LOGE(LOG_TAG, "Failed to start DNS server.");
            gState = STATE_ERROR;
        }
        else
        {
            /* If any other hostname than our is requested, it shall not send a error back,
             * otherwise the client stops instead of continue to the captive portal.
             */
            gDnsServer.setErrorReplyCode(DNSReplyCode::NoError);
        }

        ESP_LOGI(LOG_TAG, "Access Point '%s' is up and running.", gSettingsHostname.c_str());
        gState = STATE_AP_UP;
    }
}

/**
 * State machine function for the Access Point up state.
 * This state is entered when the Access Point is up and running.
 */
static void stateApUp()
{
    /* Nothing to do. */
}

/**
 * State machine function for the error state.
 * This state is entered when an error occurs, e.g. WiFi connection failed.
 */
static void stateError()
{
    /* Nothing to do. */
}

/**
 * Handle upload requests.
 * This function is called when a file is uploaded to the web server.
 * It sends a response back to the client indicating that the upload was successful.
 */
static void handleUpload()
{
    gWebServer.send(200, "text/plain", "File upload successful.");
}

/**
 * Handle file upload requests.
 * This function is called when a file is uploaded to the web server.
 * It logs the upload progress and sends a response back to the client.
 */
static void handleFileUpload()
{
    HTTPUpload& upload = gWebServer.upload();

    if (UPLOAD_FILE_START == upload.status)
    {
        int    cmd      = U_FLASH;
        size_t fileSize = UPDATE_SIZE_UNKNOWN;
        String headerXFileSize;

        /* If there is a pending upload, abort it. */
        if (true == Update.isRunning())
        {
            Update.abort();
            ESP_LOGW(LOG_TAG, "Aborted pending upload.");
        }

        /* Upload firmware or filesystem? */
        if (upload.filename == FIRMWARE_FILENAME)
        {
            headerXFileSize = gWebServer.header("X-File-Size-Firmware");
            cmd             = U_FLASH;
        }
        else if (upload.filename == FILESYSTEM_FILENAME)
        {
            headerXFileSize = gWebServer.header("X-File-Size-Filesystem");
            cmd             = U_SPIFFS;
        }
        else
        {
            /* Unknown. */
            ;
        }

        /* File size available? */
        if (false == headerXFileSize.isEmpty())
        {
            int32_t headerXFileSizeValue = headerXFileSize.toInt();

            if (0 < headerXFileSizeValue)
            {
                fileSize = static_cast<size_t>(headerXFileSizeValue);

                ESP_LOGI(LOG_TAG, "File size from header: %u bytes", fileSize);
            }
        }

        if (false == Update.begin(fileSize, cmd))
        {
            ESP_LOGE(LOG_TAG, "Failed to begin file upload: %s", upload.filename.c_str());
            gWebServer.send(500, "text/plain", "Failed to begin file upload.");
        }
        else
        {
            ESP_LOGI(LOG_TAG, "File upload started: %s", upload.filename.c_str());
        }
    }
    else if (UPLOAD_FILE_WRITE == upload.status)
    {
        if (upload.currentSize != Update.write(upload.buf, upload.currentSize))
        {
            ESP_LOGE(LOG_TAG, "Failed to write file upload: %s", upload.filename.c_str());
            ESP_LOGE(LOG_TAG, "Upload error: %s", Update.errorString());
            Update.abort();
            gWebServer.send(500, "text/plain", "Failed to write file upload.");
        }
        else
        {
            ESP_LOGI(LOG_TAG, "File upload progress: %u bytes", upload.currentSize);
        }
    }
    else if (UPLOAD_FILE_END == upload.status)
    {
        if (false == Update.end())
        {
            ESP_LOGE(LOG_TAG, "Failed to end file upload: %s", upload.filename.c_str());
            ESP_LOGE(LOG_TAG, "Upload error: %s", Update.errorString());
            Update.abort();
            gWebServer.send(500, "text/plain", "Failed to end file upload.");
        }
        else
        {
            ESP_LOGI(LOG_TAG, "File upload finished: %s (%u bytes)", upload.filename.c_str(), upload.totalSize);
        }
    }
    else
    {
        ESP_LOGI(LOG_TAG, "File upload aborted: %s", upload.filename.c_str());
        Update.abort();
        gWebServer.send(500, "text/plain", "File upload aborted.");
    }
}
