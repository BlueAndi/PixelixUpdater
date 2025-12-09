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
 * @file   main.cpp
 * @brief  Main entry point
 * @author Andreas Merkle <web@blue-andi.de>
 */

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <esp_log.h>
#include <Settings.h>

#include "MyWebServer.h"
#include "MiniTerminal.h"

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
    STATE_INIT,           /**< Init state */
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

static void appendDeviceUniqueId(String& deviceUniqueId);
static void getChipId(String& chipId);
static void stateMachine();
static void stateInit();
static void stateStaSetup();
static void stateStaConnecting();
static void stateStaConnected();
static void stateApSetup();
static void stateApUp();
static void stateError();

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
static const char LOG_TAG[]      = "main";

/**
 * OTA password.
 */
static const char OTA_PASSWORD[] = "maytheforcebewithyou";

/**
 * Current state of the application.
 */
static State gState              = STATE_INIT;

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

/**
 * Mini terminal instance for command line interface.
 */
static MiniTerminal gMiniTerminal(Serial);

/******************************************************************************
 * External functions
 *****************************************************************************/

/**
 * Setup the system.
 */
void setup()
{
    Settings& settings = Settings::getInstance();
    String    hostname;

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

    /* Load hostname from settings. */
    if (false == settings.open(true))
    {
        hostname = "PixelixUpdater";
    }
    else
    {
        hostname = settings.getHostname().getValue();

        settings.close();
    }

    appendDeviceUniqueId(hostname);

    ESP_LOGI(LOG_TAG, "Target: %s", PIO_ENV);
    ESP_LOGI(LOG_TAG, "Version: %s", VERSION);
    ESP_LOGI(LOG_TAG, "Hostname: %s", hostname.c_str());
    ESP_LOGI(LOG_TAG, "Partition: Factory");

    /* Start wifi */
    (void)WiFi.mode(WIFI_STA);

    MyWebServer::begin();
}

/**
 * Main loop, which is called periodically.
 */
void loop()
{
    stateMachine();
    MyWebServer::handleClient();
    gMiniTerminal.process();

    if (true == gMiniTerminal.isRestartRequested())
    {
        /* Give some time to send the response before restarting. */
        delay(100U);

        /* Disconnect WiFi graceful before restart. */
        if (WIFI_MODE_AP == WiFi.getMode())
        {
            /* In AP mode, stop the access point. */
            (void)WiFi.softAPdisconnect();
        }
        else
        {
            /* In STA mode, disconnect from the access point. */
            (void)WiFi.disconnect();
        }

        ESP.restart();
    }

    /* Schedule other tasks with same or lower priority. */
    delay(LOOP_TASK_PERIOD);
}

/******************************************************************************
 * Local functions
 *****************************************************************************/

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
 * State machine function to handle the current state of the application.
 * This function is called periodically in the loop() function.
 */
static void stateMachine()
{
    switch (gState)
    {
    case STATE_INIT:
        stateInit();
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
 * State machine function for the init state.
 * This is the initial state of the application.
 */
static void stateInit()
{
    Settings& settings = Settings::getInstance();
    String    wifiSSID;

    /* Load settings. */
    if (false == settings.open(true))
    {
        wifiSSID = settings.getWifiSSID().getDefault();
    }
    else
    {
        wifiSSID = settings.getWifiSSID().getValue();

        settings.close();
    }

    if (true == wifiSSID.isEmpty())
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
        Settings&      settings                    = Settings::getInstance();
        const uint32_t CONNECT_TIMEOUT_MS          = 10000U;
        const uint32_t CHECK_CONNECTION_TIMEOUT_MS = 100U;
        uint32_t       startTime                   = millis();
        String         wifiSSID;
        String         wifiPassphrase;

        /* Load settings. */
        if (false == settings.open(true))
        {
            wifiSSID       = settings.getWifiSSID().getDefault();
            wifiPassphrase = settings.getWifiPassphrase().getDefault();
        }
        else
        {
            wifiSSID       = settings.getWifiSSID().getValue();
            wifiPassphrase = settings.getWifiPassphrase().getValue();

            settings.close();
        }

        /* Try to connect to the wifi network. */
        (void)WiFi.begin(wifiSSID.c_str(), wifiPassphrase.c_str());

        ESP_LOGI(LOG_TAG, "Connecting to WiFi '%s'...", wifiSSID.c_str());

        /* Wait until connected. */
        while ((WL_CONNECTED != WiFi.status()) && ((millis() - startTime) < CONNECT_TIMEOUT_MS))
        {
            delay(CHECK_CONNECTION_TIMEOUT_MS);
        }

        if (WL_CONNECTED != WiFi.status())
        {
            ESP_LOGE(LOG_TAG, "Failed to connect to WiFi '%s'", wifiSSID.c_str());
            ESP_LOGI(LOG_TAG, "Setup WiFi Access Point mode.");
            gState = STATE_AP_SETUP;
        }
        else
        {
            ESP_LOGI(LOG_TAG, "Connected to WiFi '%s'", wifiSSID.c_str());
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
    Settings& settings = Settings::getInstance();
    String    hostname;
    String    wifiApSSID;
    String    wifiApPassphrase;

    /* Load settings. */
    if (false == settings.open(true))
    {
        hostname         = settings.getHostname().getDefault();
        wifiApSSID       = settings.getWifiApSSID().getDefault();
        wifiApPassphrase = settings.getWifiApPassphrase().getDefault();
    }
    else
    {
        hostname         = settings.getHostname().getValue();
        wifiApSSID       = settings.getWifiApSSID().getValue();
        wifiApPassphrase = settings.getWifiApPassphrase().getValue();

        settings.close();
    }

    appendDeviceUniqueId(hostname);

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
    else if (false == WiFi.softAPsetHostname(hostname.c_str()))
    {
        ESP_LOGE(LOG_TAG, "Failed to set Access Point hostname.");
        gState = STATE_ERROR;
    }
    /* Setup wifi access point. */
    else if (false == WiFi.softAP(wifiApSSID.c_str(), wifiApPassphrase.c_str()))
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

        ESP_LOGI(LOG_TAG, "Access Point '%s' is up and running.", hostname.c_str());
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
