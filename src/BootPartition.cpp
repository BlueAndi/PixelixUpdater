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
 * @file   BootPartition.cpp
 * @brief  Boot partition handling
 * @author Andreas Merkle <web@blue-andi.de>
 */

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "BootPartition.h"

#include <esp_log.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <LittleFS.h>

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

/******************************************************************************
 * Local Variables
 *****************************************************************************/

/**
 * Tag for logging purposes.
 */
static const char LOG_TAG[] = "BootPartition";

/******************************************************************************
 * Public Methods
 *****************************************************************************/

/******************************************************************************
 * Protected Methods
 *****************************************************************************/

/******************************************************************************
 * Private Methods
 *****************************************************************************/

static bool isFsMountable();

/******************************************************************************
 * External Functions
 *****************************************************************************/

BootPartition::BootPartitionResult BootPartition::setApp0()
{
    BootPartitionResult result = BOOT_UNKNOWN_ERROR;

    if (false == isFsMountable())
    {
        result = BOOT_FS_NOT_MOUNTABLE;
    }
    else
    {
        const esp_partition_t* partition;

        partition = esp_partition_find_first(
            esp_partition_type_t::ESP_PARTITION_TYPE_APP,
            esp_partition_subtype_t::ESP_PARTITION_SUBTYPE_APP_OTA_0,
            nullptr);

        if (nullptr != partition)
        {
            esp_err_t err = esp_ota_set_boot_partition(partition);
            ESP_LOGI(LOG_TAG, "Setting app0 partition '%s' as boot partition", partition->label);

            if (ESP_OK != err)
            {
                ESP_LOGE(LOG_TAG, "Failed to set app0 partition '%s' as boot partition: %d", partition->label, err);
                result = BOOT_SET_FAILED;
            }
            else
            {
                result = BOOT_SUCCESS;
            }
        }
        else
        {
            ESP_LOGE(LOG_TAG, "App0 partition not found!");
            result = BOOT_PARTITION_NOT_FOUND;
        }
    }

    return result;
}

/******************************************************************************
 * Local Functions
 *****************************************************************************/

/**
 * Check if the filesystem partition is mountable, which means it exists and is not encrypted.
 *
 * @return If the filesystem partition is mountable, it returns true, otherwise false.
 */
static bool isFsMountable()
{
    bool                   isMountable = false;
    const esp_partition_t* partition;
    const bool             FORMAT_ON_FAIL          = false;
    const char*            BASE_PATH               = "/littlefs";
    const uint8_t          MAX_OPEN_FILES          = 10U;

    /* Try to find LittleFS partition first, fall back to SPIFFS for Arduino 2.x compatibility. */
    partition                                      = esp_partition_find_first(
        esp_partition_type_t::ESP_PARTITION_TYPE_DATA,
        esp_partition_subtype_t::ESP_PARTITION_SUBTYPE_DATA_LITTLEFS,
        nullptr);

    if (nullptr == partition)
    {
        ESP_LOGW(LOG_TAG, "LittleFS partition not found, trying to find SPIFFS partition for backward compatibility.");

        /* Fallback: search for SPIFFS partition for backward compatibility. */
        partition = esp_partition_find_first(
            esp_partition_type_t::ESP_PARTITION_TYPE_DATA,
            esp_partition_subtype_t::ESP_PARTITION_SUBTYPE_DATA_SPIFFS,
            nullptr);
    }

    if (nullptr == partition)
    {
        ESP_LOGE(LOG_TAG, "No filesystem partition (LittleFS or SPIFFS) found!");
    }
    else
    {
        if (false == LittleFS.begin(FORMAT_ON_FAIL, BASE_PATH, MAX_OPEN_FILES, partition->label))
        {
            ESP_LOGE(LOG_TAG, "Failed to mount filesystem partition '%s'!", partition->label);
        }
        else
        {
            isMountable = true;
            LittleFS.end();
        }
    }

    return isMountable;
}
