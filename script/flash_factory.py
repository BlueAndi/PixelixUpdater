"""
This script is used to flash a factory binary to an ESP32 device after the upload action.
"""

# MIT License
#
# Copyright (c) 2019 - 2025 Andreas Merkle (web@blue-andi.de)
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# pyright: reportUndefinedVariable=false

################################################################################
# Imports
################################################################################
import os

Import("env") # pylint: disable=undefined-variable

################################################################################
# Variables
################################################################################

FACTORY_PROGNAME = "factory"
FACTORY_OFFSET = "0x10000"

################################################################################
# Classes
################################################################################

################################################################################
# Functions
################################################################################

# pylint: disable=unused-argument
def flash_factory_binary(target, source, env):
    """
    Flash the factory binary to the ESP32 device.
    This function is called after the upload action to write the factory binary
    to the specified offset in the flash memory.

    Args:
        target: The target files (not used in this function).
        source: The source files (not used in this function).
        env: The environment object containing project and build directories.
    """

    # Get project and build directories from the environment.
    project_dir = env.subst("$PROJECT_DIR")

    # Define paths for factory, application, and merged images.
    factory_image = os.path.join(project_dir, f"{FACTORY_PROGNAME}.bin")

    # Get write flash relevant parameters from the environment.
    chip = env.get("BOARD_MCU")
    upload_speed = env.BoardConfig().get("upload.speed")

    env.Execute(
        f"esptool.py "
        f"--baud {upload_speed} "
        f"--chip {chip} "
        f"--port {env.get('UPLOAD_PORT')} "
        f"write_flash "
        f"{FACTORY_OFFSET} "
        f"{factory_image}"
    )

################################################################################
# Main
################################################################################

env.AddPostAction("upload", flash_factory_binary) # pylint: disable=undefined-variable
