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
 * @file   BootPartition.h
 * @brief  Boot partition handling
 * @author Andreas Merkle <web@blue-andi.de>
 *
 * @addtogroup UTILITIES
 *
 * @{
 */

#ifndef BOOT_PARTITION_H
#define BOOT_PARTITION_H

/******************************************************************************
 * Compile Switches
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>

/**
 * Boot partition related functions.
 */
namespace BootPartition
{

/******************************************************************************
 * Macros
 *****************************************************************************/

/******************************************************************************
 * Types and Classes
 *****************************************************************************/

/**
 * Result of setting boot partition.
 */
typedef enum
{
    BOOT_SUCCESS,             /**< App0 partition was set as boot partition successfully. */
    BOOT_PARTITION_NOT_FOUND, /**< App0 partition was not found. */
    BOOT_SET_FAILED,          /**< Failed to set App0 partition as boot partition. */
    BOOT_UNKNOWN_ERROR        /**< An unknown error occurred. */

} BootPartitionResult;

/******************************************************************************
 * Functions
 *****************************************************************************/

/**
 * Set the application partition 0 active to be the next boot partition.
 *
 * @return BootPartitionResult indicating wether application partition 0 was set as boot partition successfully or not.
 */
BootPartitionResult setApp0();

/**
 * Check if the filesystem partition is mountable.
 * 
 * @return true if the filesystem partition is mountable, false otherwise.
 */
bool isFsMountable();

} /* namespace BootPartition */

#endif /* BOOT_PARTITION_H */

/** @} */