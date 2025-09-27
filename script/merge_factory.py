"""
This script merges factory.bin and firmware.bin into merged-firmware.bin before upload.
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
import sys
Import("env")  # pylint: disable=undefined-variable

################################################################################
# Variables
################################################################################

FACTORY_PROGNAME = "factory"
MERGE_PROGNAME = "merged-firmware"
PLATFORM = env.PioPlatform() # pylint: disable=undefined-variable

# Get project and build directories from the environment.
PROJECT_DIR = env.subst("$PROJECT_DIR") # pylint: disable=undefined-variable
BUILD_DIR = env.subst("$BUILD_DIR") # pylint: disable=undefined-variable

################################################################################
# Classes
################################################################################

################################################################################
# Functions
################################################################################

def get_partition_table(env):
    """
    Get the partition table from the environment.
    
    Args:
        env: The environment object containing project and build directories.
    
    Returns:
        List[Dict[str, str]]: A list of partitions defined in the partition table CSV file.
    """
    partition_table = []
    partition_table_file_name = env.get("PARTITIONS_TABLE_CSV")

    if partition_table_file_name and os.path.isfile(partition_table_file_name):
        with open(partition_table_file_name, "r", encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                # Skip comments and empty lines
                if not line or line.startswith("#"):
                    continue
                # Partition table CSV format: name, type, subtype, offset, size, flags
                parts = [x.strip() for x in line.split(",")]
                if len(parts) < 5:
                    continue  # Not a valid partition line
                partition = {
                    "name": parts[0],
                    "type": parts[1],
                    "subtype": parts[2],
                    "offset": parts[3],
                    "size": parts[4],
                    "flags": parts[5] if len(parts) > 5 else ""
                }
                partition_table.append(partition)
    else:
        print("No partition table found or PARTITIONS_TABLE_CSV not set.", file=sys.stderr)

    return partition_table

def show_section_info(sections):
    """
    Show the section information in a formatted way.
    
    Args:
        sections: A list of sections to display.
    """
    # Determine the maximum width for offset and file columns
    offsets = []
    files = []
    for section in sections:
        sect_adr, sect_file = section.split(" ", 1)
        offsets.append(sect_adr)
        files.append(sect_file)
    offset_width = max(len("Offset"), max((len(o) for o in offsets), default=0))
    file_width = max(len("File"), max((len(f) for f in files), default=0))

    # Print header
    print(f"{'Offset'.ljust(offset_width)} | {'File'.ljust(file_width)}")
    print(f"{'-' * offset_width}-+-{'-' * file_width}")

    # Print each section
    for sect_adr, sect_file in zip(offsets, files):
        print(f"{sect_adr.ljust(offset_width)} | {sect_file.ljust(file_width)}")

def merge_factory(target, source, env): # pylint: disable=unused-argument
    """
    Merge factor and application binaries into a single binary file.
    
    Args:
        target: The target files (not used in this function).
        source: The source files (not used in this function).
        env: The environment object containing project and build directories.
    """
    print("\nMerging factory and firmware binaries ...")

    # Get program name from the environment
    program_name = env.subst("$PROGNAME")

    # Define paths for factory, application, and merged images.
    factory_image = os.path.join(PROJECT_DIR, f"{FACTORY_PROGNAME}.bin")
    app_image = os.path.join(BUILD_DIR, f"{program_name}.bin")
    merged_image = os.path.join(BUILD_DIR, f"{MERGE_PROGNAME}.bin")

    # Get offset for factory and application images from partition table.
    partition_table = get_partition_table(env)

    factory_offset = 0
    app_offset = 0
    for partition in partition_table:
        if partition["name"] == FACTORY_PROGNAME:
            factory_offset = partition["offset"]
        elif partition["name"] == "app":
            app_offset = partition["offset"]

    # Extend the flash sections with factory and application images.
    sections = env.subst(env.get("FLASH_EXTRA_IMAGES"))
    sections.append(f"{factory_offset} {factory_image}")
    sections.append(f"{app_offset} {app_image}")

    # Get merge relevant information from the environment.
    chip = env.get("BOARD_MCU")

    # Show the sections to be merged.
    print("\n")
    show_section_info(sections)
    print("\n")

    # Get path to esptool.py
    tool_dir = PLATFORM.get_package_dir("tool-esptoolpy")
    esptool_path = os.path.join(tool_dir, "esptool.py")

    if tool_dir is None:
        print("Package tool-esptoolpy could not be found!")
    else:
        # Find the esptool command description here:
        # https://docs.espressif.com/projects/esptool/en/latest/esp32/esptool/basic-commands.html#merge-bin
        cmd = (
            f"python {esptool_path} "
            f"--chip {chip} "
            f"merge_bin "
            f"-o {merged_image} "
            f"{' '.join(sections)}"
        )

        print(f"Executing command: {cmd}")
        env.Execute(cmd)

def change_progname(target, source, env): # pylint: disable=unused-argument
    """
    Change the program name to merged image name for upload.
    
    Args:
        target: The target files (not used in this function).
        source: The source files (not used in this function).
        env: The environment object containing project and build directories.
    """
    env.Replace(PROGNAME=MERGE_PROGNAME)  # pylint: disable=undefined-variable
    print(f"Changed program name to {MERGE_PROGNAME} for upload.")

def replace_firmware(target, source, env): # pylint: disable=unused-argument
    """
    Replace the firmware.bin of the app partition with the merged-firmware.bin
    created in merge_factory.
    
    Args:
        target: The target files (not used in this function).
        source: The source files (not used in this function).
        env: The environment object containing project and build directories
             (not used in this function).
    """

    # Define paths for firmware.bin and the binary it will be replaced by
    old_firmware_image = os.path.join(BUILD_DIR, "firmware.bin")
    new_firmware_image = os.path.join(BUILD_DIR, f"{MERGE_PROGNAME}.bin")

    if not os.path.exists(new_firmware_image):
        print(f"New firmware not found: {new_firmware_image}")
        return
    
    if os.path.exists(old_firmware_image):
        os.remove(old_firmware_image)
        print(f"Removed old firmware: {old_firmware_image}")
    else:
        print("No old firmware found to remove.")

    os.rename(new_firmware_image, old_firmware_image)
    print("Replaced firmware successfully!")

################################################################################
# Main
################################################################################

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", merge_factory) # pylint: disable=undefined-variable
env.AddPreAction("upload", change_progname)  # pylint: disable=undefined-variable
#env.AddPreAction("upload", replace_firmware)
