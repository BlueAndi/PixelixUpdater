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
 * @file   Settings.cpp
 * @brief  Settings service
 * @author Andreas Merkle <web@blue-andi.de>
 */

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "Settings.h"
#include "nvs.h"
#include <algorithm>

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

/* clang-format off */

/** Settings namespace used for preferences */
static const char*  PREF_NAMESPACE                  = "settings";

/* ---------- Keys ---------- */

/* Note:
 * Zero-terminated ASCII string containing a key name.
 * Maximum string length is 15 bytes, excluding a zero terminator.
 * https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/storage/nvs_flash.html
 */

/** Wifi network key */
static const char*  KEY_WIFI_SSID                   = "sta_ssid";

/** Wifi network passphrase key */
static const char*  KEY_WIFI_PASSPHRASE             = "sta_passphrase";

/** Wifi access point network key */
static const char*  KEY_WIFI_AP_SSID                = "ap_ssid";

/** Wifi access point network passphrase key */
static const char*  KEY_WIFI_AP_PASSPHRASE          = "ap_passphrase";

/** Website login user account key */
static const char*  KEY_WEB_LOGIN_USER              = "web_login_user";

/** Website login user password key */
static const char*  KEY_WEB_LOGIN_PASSWORD          = "web_login_pass";

/** Hostname key */
static const char*  KEY_HOSTNAME                    = "hostname";

/* ---------- Key value pair names ---------- */

/** Wifi network name of key value pair */
static const char*  NAME_WIFI_SSID                  = "Wifi SSID";

/** Wifi network passphrase name of key value pair */
static const char*  NAME_WIFI_PASSPHRASE            = "Wifi passphrase";

/** Wifi access point network name of key value pair */
static const char*  NAME_WIFI_AP_SSID               = "Wifi AP SSID";

/** Wifi access point network passphrase name of key value pair */
static const char*  NAME_WIFI_AP_PASSPHRASE         = "Wifi AP passphrase";

/** Website login user account name of key value pair */
static const char*  NAME_WEB_LOGIN_USER             = "Website login user";

/** Website login user password name of key value pair */
static const char*  NAME_WEB_LOGIN_PASSWORD         = "Website login password";

/** Hostname name of key value pair */
static const char*  NAME_HOSTNAME                   = "Hostname";

/* ---------- Default values ---------- */

/** Wifi network default value */
static const char*      DEFAULT_WIFI_SSID                   = "";

/** Wifi network passphrase default value */
static const char*      DEFAULT_WIFI_PASSPHRASE             = "";

/** Wifi access point network default value */
static const char*      DEFAULT_WIFI_AP_SSID                = "pixelix";

/** Wifi access point network passphrase default value */
static const char*      DEFAULT_WIFI_AP_PASSPHRASE          = "Luke, I am your father.";

/** Website login user account default value */
static const char*      DEFAULT_WEB_LOGIN_USER              = "luke";

/** Website login user password default value */
static const char*      DEFAULT_WEB_LOGIN_PASSWORD          = "skywalker";

/** Hostname default value */
static const char*      DEFAULT_HOSTNAME                    = "pixelix";

/* ---------- Minimum values ---------- */

/** Wifi network SSID min. length. Section 7.3.2.1 of the 802.11-2007 specification. */
static const size_t     MIN_VALUE_WIFI_SSID                 = 0;

/** Wifi network passphrase min. length */
static const size_t     MIN_VALUE_WIFI_PASSPHRASE           = 8U;

/** Wifi access point network SSID min. length. Section 7.3.2.1 of the 802.11-2007 specification. */
static const size_t     MIN_VALUE_WIFI_AP_SSID              = 0;

/** Wifi access point network passphrase min. length */
static const size_t     MIN_VALUE_WIFI_AP_PASSPHRASE        = 8U;

/** Website login user account min. length */
static const size_t     MIN_VALUE_WEB_LOGIN_USER            = 4U;

/** Website login user password min. length */
static const size_t     MIN_VALUE_WEB_LOGIN_PASSWORD        = 4U;

/** Hostname min. length */
static const size_t     MIN_VALUE_HOSTNAME                  = 1U;

/* ---------- Maximum values ---------- */

/** Wifi network SSID max. length. Section 7.3.2.1 of the 802.11-2007 specification. */
static const size_t     MAX_VALUE_WIFI_SSID                 = 32U;

/** Wifi network passphrase max. length */
static const size_t     MAX_VALUE_WIFI_PASSPHRASE           = 64U;

/** Wifi access point network SSID max. length. Section 7.3.2.1 of the 802.11-2007 specification. */
static const size_t     MAX_VALUE_WIFI_AP_SSID              = 32U;

/** Wifi access point network passphrase max. length */
static const size_t     MAX_VALUE_WIFI_AP_PASSPHRASE        = 64U;

/** Website login user account max. length */
static const size_t     MAX_VALUE_WEB_LOGIN_USER            = 16U;

/** Website login user password max. length */
static const size_t     MAX_VALUE_WEB_LOGIN_PASSWORD        = 32U;

/** Hostname max. length */
static const size_t     MAX_VALUE_HOSTNAME                  = 63U;

/* clang-format on */

/******************************************************************************
 * Public Methods
 *****************************************************************************/

bool Settings::open(bool readOnly)
{
    /* Open Preferences with namespace. Each application module, library, etc
     * has to use a namespace name to prevent key name collisions. We will open storage in
     * RW-mode (second parameter has to be false).
     * Note: Namespace name is limited to 15 chars.
     */
    bool status = m_preferences.begin(PREF_NAMESPACE, readOnly);

    /* If settings storage doesn't exist, it will be created. */
    if ((false == status) &&
        (true == readOnly))
    {
        status = m_preferences.begin(PREF_NAMESPACE, false);

        if (true == status)
        {
            m_preferences.end();
            status = m_preferences.begin(PREF_NAMESPACE, readOnly);
        }
    }

    return status;
}

void Settings::close()
{
    m_preferences.end();
}

/******************************************************************************
 * Protected Methods
 *****************************************************************************/

/******************************************************************************
 * Private Methods
 *****************************************************************************/

/* clang-format off */
Settings::Settings() :
    m_preferences(),
    m_wifiSSID                  (m_preferences, KEY_WIFI_SSID,                  NAME_WIFI_SSID,                 DEFAULT_WIFI_SSID,              MIN_VALUE_WIFI_SSID,                MAX_VALUE_WIFI_SSID),
    m_wifiPassphrase            (m_preferences, KEY_WIFI_PASSPHRASE,            NAME_WIFI_PASSPHRASE,           DEFAULT_WIFI_PASSPHRASE,        MIN_VALUE_WIFI_PASSPHRASE,          MAX_VALUE_WIFI_PASSPHRASE,      true),
    m_apSSID                    (m_preferences, KEY_WIFI_AP_SSID,               NAME_WIFI_AP_SSID,              DEFAULT_WIFI_AP_SSID,           MIN_VALUE_WIFI_AP_SSID,             MAX_VALUE_WIFI_AP_SSID),
    m_apPassphrase              (m_preferences, KEY_WIFI_AP_PASSPHRASE,         NAME_WIFI_AP_PASSPHRASE,        DEFAULT_WIFI_AP_PASSPHRASE,     MIN_VALUE_WIFI_AP_PASSPHRASE,       MAX_VALUE_WIFI_AP_PASSPHRASE,   true),
    m_webLoginUser              (m_preferences, KEY_WEB_LOGIN_USER,             NAME_WEB_LOGIN_USER,            DEFAULT_WEB_LOGIN_USER,         MIN_VALUE_WEB_LOGIN_USER,           MAX_VALUE_WEB_LOGIN_USER),
    m_webLoginPassword          (m_preferences, KEY_WEB_LOGIN_PASSWORD,         NAME_WEB_LOGIN_PASSWORD,        DEFAULT_WEB_LOGIN_PASSWORD,     MIN_VALUE_WEB_LOGIN_PASSWORD,       MAX_VALUE_WEB_LOGIN_PASSWORD,   true),
    m_hostname                  (m_preferences, KEY_HOSTNAME,                   NAME_HOSTNAME,                  DEFAULT_HOSTNAME,               MIN_VALUE_HOSTNAME,                 MAX_VALUE_HOSTNAME)
{
}
/* clang-format on */

Settings::~Settings()
{
}

/******************************************************************************
 * External Functions
 *****************************************************************************/

/******************************************************************************
 * Local Functions
 *****************************************************************************/
