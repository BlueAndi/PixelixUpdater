/* MIT License
 *
 * Copyright (c) 2019 - 2025 Andreas Merkle <web@blue-andi.de>
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

#include <esp_log.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>

#include "EmbeddedFiles.h"
#include "BootPartition.h"

/******************************************************************************
 * Compiler Switches
 *****************************************************************************/

/******************************************************************************
 * Macros
 *****************************************************************************/

/******************************************************************************
 * Types and classes
 *****************************************************************************/

/**
 * This type defines supported HTTP response status codes according to RFC7231.
 */
typedef enum
{
    STATUS_CODE_CONTINUE                        = 100, /**< Continue */
    STATUS_CODE_SWITCHING_PROTOCOLS             = 101, /**< Switching Protocols */
    STATUS_CODE_PROCESSING                      = 102, /**< Processing */
    STATUS_CODE_OK                              = 200, /**< Ok */
    STATUS_CODE_CREATED                         = 201, /**< Created */
    STATUS_CODE_ACCEPTED                        = 202, /**< Accepted */
    STATUS_CODE_NON_AUTHORITATIVE_INFORMATION   = 203, /**< Non-Authoritative Information */
    STATUS_CODE_NO_CONTENT                      = 204, /**< No Content */
    STATUS_CODE_RESET_CONTENT                   = 205, /**< Reset Content */
    STATUS_CODE_PARTIAL_CONTENT                 = 206, /**< Partial Content */
    STATUS_CODE_MULTI_STATUS                    = 207, /**< Multi-Status */
    STATUS_CODE_ALREADY_REPORTED                = 208, /**< Already Reported */
    STATUS_CODE_IM_USED                         = 226, /**< IM Used */
    STATUS_CODE_MULTIPLE_CHOICES                = 300, /**< Multiple Choices */
    STATUS_CODE_MOVED_PERMANENTLY               = 301, /**< Moved Permantently */
    STATUS_CODE_FOUND                           = 302, /**< Found */
    STATUS_CODE_SEE_OTHER                       = 303, /**< See Other */
    STATUS_CODE_NOT_MODIFIED                    = 304, /**< Not Modified */
    STATUS_CODE_USE_PROXY                       = 305, /**< Use Proxy */
    STATUS_CODE_TEMPORARY_REDIRECT              = 307, /**< Temporary Redirect */
    STATUS_CODE_PERMANENT_REDIRECT              = 308, /**< Permanent Redirect */
    STATUS_CODE_BAD_REQUEST                     = 400, /**< Bad Request */
    STATUS_CODE_UNAUTHORIZED                    = 401, /**< Unauthorized */
    STATUS_CODE_PAYMENT_REQUIRED                = 402, /**< Payment Required */
    STATUS_CODE_FORBIDDEN                       = 403, /**< Forbidden */
    STATUS_CODE_NOT_FOUND                       = 404, /**< Not Found */
    STATUS_CODE_METHOD_NOT_ALLOWED              = 405, /**< Method Not Allowed */
    STATUS_CODE_NOT_ACCEPTABLE                  = 406, /**< Not Acceptable */
    STATUS_CODE_PROXY_AUTHENTICATION_REQUIRED   = 407, /**< Proxy Authentication Required */
    STATUS_CODE_REQUEST_TIMEOUT                 = 408, /**< Request Timeout */
    STATUS_CODE_CONFLICT                        = 409, /**< Conflict */
    STATUS_CODE_GONE                            = 410, /**< Gone */
    STATUS_CODE_LENGTH_REQUIRED                 = 411, /**< Length Required */
    STATUS_CODE_PRECONDITION_FAILED             = 412, /**< Precondition Failed */
    STATUS_CODE_PAYLOAD_TOO_LARGE               = 413, /**< Payload Too Large */
    STATUS_CODE_URI_TOO_LONG                    = 414, /**< URI Too Long */
    STATUS_CODE_UNSUPPORTED_MEDIA_TYPE          = 415, /**< Unsupported Media Type */
    STATUS_CODE_RANGE_NOT_SATISFIABLE           = 416, /**< Range Not Satisfiable */
    STATUS_CODE_EXPECTATION_FAILED              = 417, /**< Expectation Failed */
    STATUS_CODE_MISDIRECTED_REQUEST             = 421, /**< Misdirected Request */
    STATUS_CODE_UNPROCESSABLE_ENTITY            = 422, /**< Unprocessable Entity */
    STATUS_CODE_LOCKED                          = 423, /**< Locked */
    STATUS_CODE_FAILED_DEPENDENCY               = 424, /**< Failed Dependency */
    STATUS_CODE_UPGRADE_REQUIRED                = 426, /**< Upgrade Required */
    STATUS_CODE_PRECONDITION_REQUIRED           = 428, /**< Precondition Required */
    STATUS_CODE_TOO_MANY_REQUESTS               = 429, /**< Too Many Requests */
    STATUS_CODE_REQUEST_HEADER_FIELDS_TOO_LARGE = 431, /**< Request Header Fields Too Large */
    STATUS_CODE_INTERNAL_SERVER_ERROR           = 500, /**< Internal Server Error */
    STATUS_CODE_NOT_IMPLEMENTED                 = 501, /**< Not Implemented */
    STATUS_CODE_BAD_GATEWAY                     = 502, /**< Bad Gateway */
    STATUS_CODE_SERVICE_UNAVAILABLE             = 503, /**< Service Unavailable */
    STATUS_CODE_GATEWAY_TIMEOUT                 = 504, /**< Gateway Timeout */
    STATUS_CODE_HTTP_VERSION_NOT_SUPPORTED      = 505, /**< Http Version Not Supported */
    STATUS_CODE_VARIANT_ALSO_NEGOTIATES         = 506, /**< Variant Also Negotiates */
    STATUS_CODE_INSUFFICIENT_STORAGE            = 507, /**< Insufficient Storage */
    STATUS_CODE_LOOP_DETECTED                   = 508, /**< Loop Detected */
    STATUS_CODE_NOT_EXTENDED                    = 510, /**< Not Extended */
    STATUS_CODE_NETWORK_AUTHENTICATION_REQUIRED = 511  /**< Network Authentication Required */

} HTTPStatusCode;

/******************************************************************************
 * Prototypes
 *****************************************************************************/

static void handleUpload();
static void handleFileUpload();
static void handleFileStart(HTTPUpload& upload);
static void handleFileWrite(HTTPUpload& upload);
static void handleFileEnd(HTTPUpload& upload);

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

    /* Start the web server, before configuration! */
    gWebServer.begin();

    /* Webserver only keeps headers that are specified through collectHeaders(). */
    gWebServer.collectHeaders(headerKeys, keyCount);

    /* Configure web server */
    gWebServer.onNotFound(
        []() {
            gWebServer.sendHeader("Location", "/");
            gWebServer.send(STATUS_CODE_FOUND, "text/plain", "");
        });

    gWebServer.on("/", HTTP_GET, []() {
        gWebServer.sendHeader("Location", "/index.html");
        gWebServer.send(STATUS_CODE_FOUND, "text/plain", "");
    });

    gWebServer.on("/change-partition", HTTP_GET, []() {
        switch (BootPartition::setApp0())
        {
        case BootPartition::BOOT_SUCCESS: {
            const uint32_t RESTART_DELAY = 100U; /* ms */

            gWebServer.send(STATUS_CODE_OK, "text/plain", "Partition switched. Restarting...");

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
            gWebServer.send(STATUS_CODE_INTERNAL_SERVER_ERROR, "text/plain", "App0 partition not found!");
            break;

        case BootPartition::BOOT_SET_FAILED:
            gWebServer.send(STATUS_CODE_INTERNAL_SERVER_ERROR, "text/plain", "Failed to set app0 partition as boot partition!");
            break;

        case BootPartition::BOOT_UNKNOWN_ERROR:
            gWebServer.send(STATUS_CODE_INTERNAL_SERVER_ERROR, "text/plain", "Cannot switch to app0 partition. Error unknown!");
            break;
        }
    });

    gWebServer.on("/upload.html", HTTP_POST, handleUpload, handleFileUpload);

    gWebServer.on("/partition-size", HTTP_GET, []() {
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
            gWebServer.send(STATUS_CODE_OK, "text/plain", String(size));
        }
        else
        {
            gWebServer.send(STATUS_CODE_INTERNAL_SERVER_ERROR, "text/plain", "Partition not found!");
        }
    });

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
 * Handle upload requests.
 * This function is called when a file is uploaded to the web server.
 * It sends a response back to the client indicating that the upload was successful.
 */
static void handleUpload()
{
    gWebServer.send(STATUS_CODE_OK, "text/plain", "File upload successful.");
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
        gWebServer.send(STATUS_CODE_INTERNAL_SERVER_ERROR, "text/plain", "File upload aborted.");
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
    }
    else
    {
        ESP_LOGE(LOG_TAG, "Could not find %s or %s header. Cannot upload file!", FIRMWARE_SIZE_HEADER, FILESYSTEM_SIZE_HEADER);
        gWebServer.send(STATUS_CODE_BAD_REQUEST, "text/plain", "Missing size header in request!");
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
        gWebServer.send(STATUS_CODE_INTERNAL_SERVER_ERROR, "text/plain", "Failed to begin file upload.");
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
        gWebServer.send(STATUS_CODE_INTERNAL_SERVER_ERROR, "text/plain", "Failed to write file upload.");
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
        gWebServer.send(STATUS_CODE_INTERNAL_SERVER_ERROR, "text/plain", "Failed to end file upload.");
    }
    else
    {
        ESP_LOGI(LOG_TAG, "File upload finished: %s (%u bytes)", upload.filename.c_str(), upload.totalSize);
    }
}
