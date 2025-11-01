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
 * @file   Settings.h
 * @brief  Settings service
 * @author Andreas Merkle <web@blue-andi.de>
 *
 * @addtogroup SETTINGS
 *
 * @{
 */

#ifndef SETTINGS_H
#define SETTINGS_H

/******************************************************************************
 * Compile Switches
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <Preferences.h>
#include <vector>

#include "KeyValue.h"
#include "KeyValueString.h"

/******************************************************************************
 * Macros
 *****************************************************************************/

/******************************************************************************
 * Types and Classes
 *****************************************************************************/

/**
 * Persistent storage of key value pairs.
 */
class Settings
{
public:

    /**
     * Get settings service.
     *
     * @return Settings service
     */
    static Settings& getInstance()
    {
        static Settings instance; /* idiom */

        return instance;
    }

    /**
     * Open settings.
     * If the settings storage doesn't exist, it will be created.
     *
     * @param[in] readOnly  Open read only or read/write
     *
     * @return Status
     * @retval false    Failed to open
     * @retval true     Successful opened
     */
    bool open(bool readOnly);

    /**
     * Close settings.
     */
    void close();

    /**
     * Get remote wifi network SSID.
     *
     * @return Key value pair
     */
    KeyValueString& getWifiSSID()
    {
        return m_wifiSSID;
    }

    /**
     * Get remote wifi network passphrase.
     *
     * @return Key value pair
     */
    KeyValueString& getWifiPassphrase()
    {
        return m_wifiPassphrase;
    }

    /**
     * Get wifi access point network SSID.
     *
     * @return Key value pair
     */
    KeyValueString& getWifiApSSID()
    {
        return m_apSSID;
    }

    /**
     * Get wifi access point network passphrase.
     *
     * @return Key value pair
     */
    KeyValueString& getWifiApPassphrase()
    {
        return m_apPassphrase;
    }

    /**
     * Get website login user account.
     *
     * @return Website login user account
     */
    KeyValueString& getWebLoginUser()
    {
        return m_webLoginUser;
    }

    /**
     * Get website login user password.
     *
     * @return Website login user password
     */
    KeyValueString& getWebLoginPassword()
    {
        return m_webLoginPassword;
    }

    /**
     * Get hostname.
     *
     * @return Key value pair
     */
    KeyValueString& getHostname()
    {
        return m_hostname;
    }

private:

    Preferences    m_preferences;      /**< Persistent storage */
    KeyValueString m_wifiSSID;         /**< Remote wifi network SSID */
    KeyValueString m_wifiPassphrase;   /**< Remote wifi network passphrase */
    KeyValueString m_apSSID;           /**< Access point SSID */
    KeyValueString m_apPassphrase;     /**< Access point passphrase */
    KeyValueString m_webLoginUser;     /**< Website login user account */
    KeyValueString m_webLoginPassword; /**< Website login user password */
    KeyValueString m_hostname;         /**< Hostname */

    /**
     * Constructs the settings service instance.
     */
    Settings();

    /**
     * Destroys the settings service instance.
     */
    ~Settings();

    /* An instance shall not be copied. */
    Settings(const Settings& service);
    Settings& operator=(const Settings& service);
};

/******************************************************************************
 * Functions
 *****************************************************************************/

#endif /* SETTINGS_H */

/** @} */