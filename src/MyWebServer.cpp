/* MIT License
 *
 * Copyright (c) 2019 - 2026 Andreas Merkle <web@blue-andi.de>
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
    DESCRIPTION
*******************************************************************************/
/**
 * @file   MyWebServer.cpp
 * @brief  The web server with its pages and handlers.
 * @author Andreas Merkle <web@blue-andi.de>
 */

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "MyWebServer.h"
#include <WebServer.h>
#include <Update.h>
#include <WiFi.h>
#include <Settings.h>

#include <esp_log.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>

#include "EmbeddedFiles.h"
#include "BootPartition.h"
#include "HttpStatus.h"

/******************************************************************************
 * Compiler Switches
 *****************************************************************************/

/******************************************************************************
 * Macros
 *****************************************************************************/

/******************************************************************************
 * Types and classes
 *****************************************************************************/

/******************************************************************************
 * Prototypes
 *****************************************************************************/

static bool requireAuthentication();
static void sendJsonResponse(HttpStatus::StatusCode status, const String& json);
static void sendSuccessResponse(const String& message);
static void sendSuccessPayloadResponse(const String& message, const String& jsonPayload);
static void sendErrorResponse(HttpStatus::StatusCode status, const String& code, const String& message);
static void handleUpload();
static void handleFileUpload();
static void handleFileStart(HTTPUpload& upload);
static void handleFileWrite(HTTPUpload& upload);
static void handleFileEnd(HTTPUpload& upload);
static void handleActivateAppPartition();
static void handlePartitionSize();
static void handleHostname();

/******************************************************************************
 * Local Variables
 *****************************************************************************/

/**
 * Tag for logging purposes.
 */
static const char LOG_TAG[] = "MyWebServer";

/**
 * Web server instance.
 */
static WebServer gWebServer(80U);

/** Basic authentication username. */
static String gBasicAuthUser;

/** Basic authentication password. */
static String gBasicAuthPassword;

/** Indicates if the filesystem has been updated. */
static bool gIsFsUpdated                   = false;

/** Firmware binary size HTTP request header. */
static const char FIRMWARE_SIZE_HEADER[]   = "X-File-Size-Firmware";

/** Filesystem binary size HTTP request header.  */
static const char FILESYSTEM_SIZE_HEADER[] = "X-File-Size-Filesystem";

/******************************************************************************
 * Public Methods
 *****************************************************************************/

/******************************************************************************
 * Protected Methods
 *****************************************************************************/

/******************************************************************************
 * Private Methods
 *****************************************************************************/

/******************************************************************************
 * External Functions
 *****************************************************************************/

void MyWebServer::begin()
{
    const char* headerKeys[] = { FIRMWARE_SIZE_HEADER, FILESYSTEM_SIZE_HEADER };
    size_t      keyCount     = sizeof(headerKeys) / sizeof(headerKeys[0]);
    Settings&   settings     = Settings::getInstance();

    /* Prepare basic authentication credentials from settings. */
    if (false == settings.open(true))
    {
        gBasicAuthUser     = settings.getWebLoginUser().getDefault();
        gBasicAuthPassword = settings.getWebLoginPassword().getDefault();
    }
    else
    {
        gBasicAuthUser     = settings.getWebLoginUser().getValue();
        gBasicAuthPassword = settings.getWebLoginPassword().getValue();

        settings.close();
    }

    /* Start the web server, before configuration! */
    gWebServer.begin();

    /* Webserver only keeps headers that are specified through collectHeaders(). */
    gWebServer.collectHeaders(headerKeys, keyCount);

    /* Configure web server */
    gWebServer.onNotFound(
        []() {
            gWebServer.sendHeader("Location", "/");
            gWebServer.send(HttpStatus::STATUS_CODE_FOUND, "text/plain", "");
        });

    gWebServer.on("/", HTTP_GET, []() {
        gWebServer.sendHeader("Location", "/index.html");
        gWebServer.send(HttpStatus::STATUS_CODE_MOVED_PERMANENTLY, "text/plain", "");
    });

    gWebServer.on("/activateAppPartition", HTTP_GET, handleActivateAppPartition);
    gWebServer.on("/upload.html", HTTP_POST, handleUpload, handleFileUpload);
    gWebServer.on("/partitionSize", HTTP_GET, handlePartitionSize);
    gWebServer.on("/hostname", HTTP_GET, handleHostname);

    EmbeddedFiles_setup(gWebServer);
}

void MyWebServer::handleClient()
{
    gWebServer.handleClient();
}

/******************************************************************************
 * Local Functions
 *****************************************************************************/

/**
 * Check if the current request is authenticated.
 * If not authenticated, sends a 401 response with authentication challenge.
 *
 * @return true if authenticated, false otherwise.
 */
static bool requireAuthentication()
{
    bool isAuthenticated = true;

    if (false == gWebServer.authenticate(gBasicAuthUser.c_str(), gBasicAuthPassword.c_str()))
    {
        gWebServer.requestAuthentication();
        isAuthenticated = false;
    }

    return isAuthenticated;
}

/**
 * Send a JSON response to the client.
 *
 * @param[in] status    HTTP status code to send.
 * @param[in] json      JSON string to send as the response body.
 */
static void sendJsonResponse(HttpStatus::StatusCode status, const String& json)
{
    gWebServer.send(status, "application/json", json);
}


/**
 * Send a success response to the client.
 *
 * @param[in] message  Success message to send.
 */
static void sendSuccessResponse(const String& message)
{
    String json = "{ \"data\": { \"message\": \"" + message + "\" } }";

    sendJsonResponse(HttpStatus::STATUS_CODE_OK, json);
}

/**
 * Send a success response with a JSON payload to the client.
 *
 * @param[in] message      Success message to send.
 * @param[in] jsonPayload  JSON payload to include in the response.
 */
static void sendSuccessPayloadResponse(const String& message, const String& jsonPayload)
{
    String json = "{ \"data\": { \"message\": \"" + message + "\", " + jsonPayload + " } }";

    sendJsonResponse(HttpStatus::STATUS_CODE_OK, json);
}

/**
 * Send an error response to the client.
 *
 * @param[in] status    HTTP status code to send.
 * @param[in] code      Error code to send.
 * @param[in] message   Error message to send.
 */
static void sendErrorResponse(HttpStatus::StatusCode status, const String& code, const String& message)
{
    String json = "{ \"error\": { \"code\": \"" + code + "\", \"message\": \"" + message + "\" } }";

    sendJsonResponse(status, json);
}

/**
 * Handle upload requests.
 * This function is called when a file is uploaded to the web server.
 * It sends a response back to the client indicating that the upload was successful.
 */
static void handleUpload()
{
    if (true == requireAuthentication())
    {
        sendSuccessResponse("File upload successful.");
    }
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
        handleFileStart(upload);
    }
    else if (UPLOAD_FILE_WRITE == upload.status)
    {
        handleFileWrite(upload);
    }
    else if (UPLOAD_FILE_END == upload.status)
    {
        handleFileEnd(upload);
    }
    else
    {
        ESP_LOGI(LOG_TAG, "File upload aborted: %s", upload.filename.c_str());
        Update.abort();

        sendErrorResponse(HttpStatus::STATUS_CODE_INTERNAL_SERVER_ERROR, "UPLOAD_ABORTED", "File upload aborted.");
    }
}

/**
 * Handle start of a new file during file upload.
 *
 * @param[in] upload Reference to the HTTPUpload object containing upload metadata.
 */
static void handleFileStart(HTTPUpload& upload)
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
    if (false == gWebServer.header(FIRMWARE_SIZE_HEADER).isEmpty())
    {
        headerXFileSize = gWebServer.header(FIRMWARE_SIZE_HEADER);
        cmd             = U_FLASH;
    }
    else if (false == gWebServer.header(FILESYSTEM_SIZE_HEADER).isEmpty())
    {
        headerXFileSize = gWebServer.header(FILESYSTEM_SIZE_HEADER);
        cmd             = U_SPIFFS;
        gIsFsUpdated    = true;
    }
    else
    {
        ESP_LOGE(LOG_TAG, "Could not find %s or %s header. Cannot upload file!", FIRMWARE_SIZE_HEADER, FILESYSTEM_SIZE_HEADER);
        sendErrorResponse(HttpStatus::STATUS_CODE_BAD_REQUEST, "MISSING_SIZE_HEADER", "Missing size header in request!");
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
        sendErrorResponse(HttpStatus::STATUS_CODE_INTERNAL_SERVER_ERROR, "UPLOAD_BEGIN_FAILED", "Failed to begin file upload.");
    }
    else
    {
        ESP_LOGI(LOG_TAG, "File upload started: %s", upload.filename.c_str());
    }
}

/**
 * Handle writing new file data during file upload.
 *
 * @param[in] upload Reference to the HTTPUpload object containing upload metadata.
 */
static void handleFileWrite(HTTPUpload& upload)
{
    if (upload.currentSize != Update.write(upload.buf, upload.currentSize))
    {
        ESP_LOGE(LOG_TAG, "Failed to write file upload: %s", upload.filename.c_str());
        ESP_LOGE(LOG_TAG, "Upload error: %s", Update.errorString());
        Update.abort();
        sendErrorResponse(HttpStatus::STATUS_CODE_INTERNAL_SERVER_ERROR, "UPLOAD_WRITE_FAILED", "Failed to write file upload.");
    }
    else
    {
        ESP_LOGI(LOG_TAG, "File upload progress: %u bytes", upload.currentSize);
    }
}

/**
 * Handle end of a file during file upload.
 *
 * @param[in] upload Reference to the HTTPUpload object containing upload metadata.
 */
static void handleFileEnd(HTTPUpload& upload)
{
    if (false == Update.end())
    {
        ESP_LOGE(LOG_TAG, "Failed to end file upload: %s", upload.filename.c_str());
        ESP_LOGE(LOG_TAG, "Upload error: %s", Update.errorString());
        Update.abort();
        sendErrorResponse(HttpStatus::STATUS_CODE_INTERNAL_SERVER_ERROR, "UPLOAD_END_FAILED", "Failed to end file upload.");
    }
    else
    {
        ESP_LOGI(LOG_TAG, "File upload finished: %s (%u bytes)", upload.filename.c_str(), upload.totalSize);
    }
}

/**
 * Handle activation of the app partition.
 */
static void handleActivateAppPartition()
{
    if (true == requireAuthentication())
    {
        bool isSuccessful = true;

        /* Check whether the filesystem is mountable only, if it has been updated.
         *
         * Because between Arduino 2.x and Tasmota Arduino 3.x the LittleFS implementation has changed,
         * it can happen that after the filesystem is updated by the application, it is not mountable anymore
         * by the factory LittleFS implementation.
         */
        if (true == gIsFsUpdated)
        {
            if (false == BootPartition::isFsMountable())
            {
                sendErrorResponse(HttpStatus::STATUS_CODE_INTERNAL_SERVER_ERROR, "FS_NOT_MOUNTABLE", "Filesystem partition is not mountable. Cannot switch to app0 partition!");
                isSuccessful = false;
            }
        }

        if (true == isSuccessful)
        {
            switch (BootPartition::setApp0())
            {
            case BootPartition::BOOT_SUCCESS: {
                const uint32_t RESTART_DELAY = 100U; /* ms */

                sendSuccessResponse("Partition switched. Restarting...");

                /* To ensure that a positive response will be sent before the device restarts,
                 * a short delay is necessary.
                 */
                delay(RESTART_DELAY);

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
                break;
            }

            case BootPartition::BOOT_PARTITION_NOT_FOUND:
                sendErrorResponse(HttpStatus::STATUS_CODE_INTERNAL_SERVER_ERROR, "APP0_PARTITION_NOT_FOUND", "App0 partition not found!");
                break;

            case BootPartition::BOOT_SET_FAILED:
                sendErrorResponse(HttpStatus::STATUS_CODE_INTERNAL_SERVER_ERROR, "BOOT_SET_FAILED", "Failed to set app0 partition as boot partition!");
                break;

            case BootPartition::BOOT_UNKNOWN_ERROR:
                sendErrorResponse(HttpStatus::STATUS_CODE_INTERNAL_SERVER_ERROR, "BOOT_UNKNOWN_ERROR", "Cannot switch to app0 partition. Error unknown!");
                break;
            }
        }
    }
}

/**
 * Handle request for partition size.
 */
static void handlePartitionSize()
{
    if (true == requireAuthentication())
    {
        uint32_t size = 0U;

        /* Firmware or filesystem? */
        if (false == gWebServer.header(FIRMWARE_SIZE_HEADER).isEmpty())
        {
            const esp_partition_t* partition = esp_partition_find_first(
                esp_partition_type_t::ESP_PARTITION_TYPE_APP,
                esp_partition_subtype_t::ESP_PARTITION_SUBTYPE_APP_OTA_0,
                nullptr);

            if (nullptr != partition)
            {
                size = partition->size;
            }
        }
        else if (false == gWebServer.header(FILESYSTEM_SIZE_HEADER).isEmpty())
        {
            const esp_partition_t* partition = esp_partition_find_first(
                esp_partition_type_t::ESP_PARTITION_TYPE_DATA,
                esp_partition_subtype_t::ESP_PARTITION_SUBTYPE_DATA_SPIFFS,
                nullptr);

            if (nullptr != partition)
            {
                size = partition->size;
            }
        }

        if (0U != size)
        {
            String payload = "\"size\": " + String(size);

            sendSuccessPayloadResponse("Partition size retrieved successfully", payload);
        }
        else
        {
            sendErrorResponse(HttpStatus::STATUS_CODE_INTERNAL_SERVER_ERROR, "PARTITION_NOT_FOUND", "Partition not found!");
        }
    }
}

/**
 * Handle request for hostname.
 */
static void handleHostname()
{
    if (true == requireAuthentication())
    {
        String hostname = WiFi.getHostname();

        if (hostname.isEmpty())
        {
            sendErrorResponse(HttpStatus::STATUS_CODE_INTERNAL_SERVER_ERROR, "HOSTNAME_NOT_FOUND", "Hostname not found!");
        }
        else
        {
            String payload = "\"hostname\": \"" + hostname + "\"";

            sendSuccessPayloadResponse("Hostname retrieved successfully", payload);
        }
    }
}
