"""
This script compresses files in the 'embed' folder using gzip and
generates C header files with the compressed content as byte arrays.
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

################################################################################
# Imports
################################################################################
import gzip
import os

################################################################################
# Variables
################################################################################

SRC_FOLDER = "embed"
DST_FOLDER = os.path.join("src", "generated")
INDEX_FILE_BASE_NAME = "EmbeddedFiles"

LICENSE = """\
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

"""

MIME_TYPES = {
    "html": "text/html",
    "css": "text/css",
    "js": "application/javascript",
    "json": "application/json",
    "png": "image/png",
    "jpg": "image/jpeg",
    "jpeg": "image/jpeg",
    "gif": "image/gif",
    "svg": "image/svg+xml",
    "ico": "image/x-icon"
}

COMPRESSED_FILE_EXTENSIONS = [
    "html",
    "css",
    "js",
    "json",
    "svg"
]

################################################################################
# Classes
################################################################################

# pylint: disable=too-few-public-methods
class CppFileGenerator:
    """
    A class to generate C++ files with a specific structure.
    This class provides methods to write license, comments, and function declarations
    in a structured format.
    """

    def __init__(self):
        self._line_ending = "\n"

    def _convert_line_endings(self, text):
        """
        Convert line endings in the text to the system's default line ending.

        Args:
            text (str): The text to convert.

        Returns:
            str: The text with converted line endings.
        """
        # Replace all line endings in the text with the system's default line ending.
        return self._line_ending.join(text.splitlines())

    def _write_license(self, file):
        """
        Write the license text to the file.

        Args:
            file (file object): The file to which the license text will be written.
        """
        file.write(self._convert_line_endings(LICENSE) + self._line_ending)

    def _write_block_comment(self, file, text):
        comment = f"""\
/******************************************************************************
* {text}
*****************************************************************************/

"""
        file.write(self._convert_line_endings(comment) + self._line_ending)

    def _write_doxygen_header(self, file, brief, author, group):
        """
            Write the Doxygen header comment to the file.
            Args:
                file (file object): The file to which the Doxygen header will be written.
                brief (str): A brief description of the file.
                author (str): The author's name.
                group (str): The group name for Doxygen documentation.
        """
        doxygen_header = f"""\
/**
 * @file   {os.path.basename(file.name)}
 * @brief  {brief}
 * @author {author}
"""
        doxygen_group = f"""\
 *
 * @addtogroup {group}
 *
 * @{{
"""
        comment_end = """\
 */

"""
        file.write(self._convert_line_endings(doxygen_header) + self._line_ending)
        if group is not None:
            file.write(self._convert_line_endings(doxygen_group) + self._line_ending)
        file.write(self._convert_line_endings(comment_end) + self._line_ending)

    def _write_doxygen_footer(self, file):
        doxygen_footer = """\
/** @} */
"""
        file.write(self._convert_line_endings(doxygen_footer))

    def _write_lines(self, file, lines):
        """
        Write multiple lines to the file.

        Args:
            file (file object): The file to which the lines will be written.
            lines (list of str): The lines to write.
        """
        for line in lines:
            file.write(self._convert_line_endings(line) + self._line_ending)

    def _write_next_line(self, file):
        """
        Write a new line to the file.

        Args:
            file (file object): The file to which the new line will be written.
        """
        file.write(self._line_ending)

# pylint: disable=too-few-public-methods, too-many-instance-attributes
class CppHeaderGenerator(CppFileGenerator):
    """
    A class to generate C++ header files with a specific structure.
    This class provides methods to write license, comments, and function declarations
    in a structured format.
    """

    def __init__(self, file_path, **kwargs):
        super().__init__()
        self._file_path = file_path
        self._brief = kwargs.get("brief")
        self._author = kwargs.get("author")
        self._group = kwargs.get("group")
        self._compile_switch = kwargs.get("compile_switch", [])
        self._includes = kwargs.get("includes", [])
        self._macros = kwargs.get("macros", [])
        self._types_and_classes = kwargs.get("types_and_classes", [])
        self._external_functions = kwargs.get("external_functions", "")

        self._blocks = [{
            "name": "Compile Switches",
            "content": self._compile_switch
        }, {
            "name": "Includes",
            "content": self._includes
        }, {
            "name": "Macros",
            "content": self._macros
        }, {
            "name": "Types and Classes",
            "content": self._types_and_classes
        }, {
            "name": "External Functions",
            "content": self._external_functions
        }]

    def _write(self, file):
        """
            Write the header file with the specified structure.

            Args:
                file (file object): The file to which the header will be written.
        """

        self._write_license(file)
        self._write_block_comment(file, "Description")
        self._write_doxygen_header(file, self._brief, self._author, self._group)

        file.write("#pragma once" + self._line_ending + self._line_ending)

        for block in self._blocks:
            self._write_block_comment(file, block["name"])

            if isinstance(block["content"], list):
                if block["content"]:
                    self._write_lines(file, block["content"])
                    self._write_next_line(file)
            elif isinstance(block["content"], str):
                if block["content"]:
                    file.write(f"{block['content']}")
                    self._write_next_line(file)

        self._write_doxygen_footer(file)

    def generate(self):
        """
        Generate the header file at the specified path.
        If the directory does not exist, it will be created.
        """
        # Ensure the directory exists before writing the file
        os.makedirs(os.path.dirname(self._file_path), exist_ok=True)
        with open(self._file_path, "w", encoding="utf-8") as file:
            self._write(file)

class CppSourceGenerator(CppFileGenerator):
    """
    A class to generate C++ source files with a specific structure.
    This class provides methods to write license, comments, and function definitions
    in a structured format.
    """

    def __init__(self, file_path, **kwargs):
        super().__init__()
        self._file_path = file_path
        self._brief = kwargs.get("brief")
        self._author = kwargs.get("author")
        self._compile_switch = kwargs.get("compile_switch", [])
        self._includes = kwargs.get("includes", [])
        self._macros = kwargs.get("macros", [])
        self._types_and_classes = kwargs.get("types_and_classes", [])
        self._prototypes = kwargs.get("prototypes", [])
        self._local_variables = kwargs.get("local_variables", [])
        self._public_methods = kwargs.get("public_methods", [])
        self._protected_methods = kwargs.get("protected_methods", [])
        self._private_methods = kwargs.get("private_methods", [])
        self._external_functions = kwargs.get("external_functions", [])
        self._local_functions = kwargs.get("local_functions", [])

        self._blocks = [{
            "name": "Compile Switches",
            "content": self._compile_switch
        }, {
            "name": "Includes",
            "content": self._includes
        }, {
            "name": "Macros",
            "content": self._macros
        }, {
            "name": "Types and Classes",
            "content": self._types_and_classes
        }, {
            "name": "Prototypes",
            "content": self._prototypes
        }, {
            "name": "Local Variables",
            "content": self._local_variables
        }, {
            "name": "Public Methods",
            "content": self._public_methods
        }, {
            "name": "Protected Methods",
            "content": self._protected_methods
        }, {
            "name": "Private Methods",
            "content": self._private_methods
        }, {
            "name": "External Functions",
            "content": self._external_functions
        }, {
            "name": "Local Functions",
            "content": self._local_functions
        }]

    def _write(self, file):
        """
            Write the source file with the specified structure.

            Args:
                file (file object): The file to which the source will be written.
        """

        self._write_license(file)
        self._write_block_comment(file, "Description")
        self._write_doxygen_header(file, self._brief, self._author, None)

        for block in self._blocks:
            self._write_block_comment(file, block["name"])

            if isinstance(block["content"], list):
                if block["content"]:
                    self._write_lines(file, block["content"])
                    self._write_next_line(file)
            elif isinstance(block["content"], str):
                if block["content"]:
                    file.write(f"{block['content']}")
                    self._write_next_line(file)

    def generate(self):
        """
        Generate the source file at the specified path.
        If the directory does not exist, it will be created.
        """
        # Ensure the directory exists before writing the file
        os.makedirs(os.path.dirname(self._file_path), exist_ok=True)
        with open(self._file_path, "w", encoding="utf-8") as file:
            self._write(file)

################################################################################
# Functions
################################################################################

def embed_file_name(file_name):
    """
    Generate a C++ module with embedded file content.
    
    Args:
        file_name (str): The path of the file.
        
    Returns:
        (str, str): The original file name and the generated C++ module base name.
    """
    file_path = os.path.join(SRC_FOLDER, file_name)
    file_extension = file_name.split('.')[-1].lower()

    # Convert file name to CamelCase base name without extension
    name_without_ext = os.path.splitext(file_name)[0]
    parts = [p for p in name_without_ext.replace('-', '_').replace('.', '_').split('_') if p]
    base_name = ''.join(part.capitalize() for part in parts)
    header_filename = base_name + ".h"
    source_filename = base_name + ".cpp"
    header_path = os.path.join(DST_FOLDER, header_filename)
    source_path = os.path.join(DST_FOLDER, source_filename)

    # Skip file if its content is already embedded and didn't change.
    if os.path.exists(header_path) and os.path.exists(source_path):
        # Check by commparing the timestamps of the source files.
        if os.path.getmtime(file_path) <= os.path.getmtime(source_path):
            return file_name, base_name

    mime_type = MIME_TYPES.get(file_extension, "application/octet-stream")

    with open(file_path, "rb") as f:
        html_content = f.read()

    # Compress only specific file types.
    content = html_content
    is_compressed = "false"
    if file_extension in COMPRESSED_FILE_EXTENSIONS:
        content = gzip.compress(html_content)
        is_compressed = "true"

    header_includes = [
        "#include <stdint.h>",
        "#include <stddef.h>"]
    header_external_functions = []
    header_external_functions.append(f"""\
/**
 * @brief  Get the content of {file_name}.
 *
 * @param[out] size Pointer to store the size of the content.
 *
 * @return Pointer to the content.
 */
extern const uint8_t* {base_name}_getFile(size_t* size);

/**
 * @brief Get MIME type for the compressed files.
 * 
 * @return MIME type as a string.
 */
extern const char* {base_name}_getMimeType();

/**
 * @brief Is the file compressed?
 * 
 * @return true if the file is compressed, otherwise false.
 */
extern bool {base_name}_isCompressed();\
""")

    header_generator = CppHeaderGenerator(
        file_path=header_path,
        brief=f"Content of {file_name}",
        author="Andreas Merkle <web@blue-andi.de>",
        group="GENERATED",
        includes=header_includes,
        external_functions=header_external_functions)

    source_includes = [f'#include "{header_filename}"']
    source_local_variables = []
    source_local_variables.append(f"""\
/**
 * @brief Content of {file_name}.
 */
static const uint8_t {base_name}_data[] = {{
""")

    # Write hex data, wrap every 80 characters (about 16 bytes per line)
    hex_bytes = [f"0x{byte:02X}U" for byte in content]
    line = "    "
    for idx, byte in enumerate(hex_bytes):
        if 0 < idx:
            line += ", "

            if (idx % 16) == 0:
                line += "\n    "

        line += byte

    line += "\n};\n"
    source_local_variables.append(line)

    source_external_functions = []
    source_external_functions.append(f"""\
extern const uint8_t* {base_name}_getFile(size_t* size)
{{
if (nullptr != size)
{{
    *size = sizeof({base_name}_data);
}}

return {base_name}_data;
}}

extern const char* {base_name}_getMimeType()
{{
return "{mime_type}";
}}

extern bool {base_name}_isCompressed()
{{
return {is_compressed};
}}\
""")

    source_generator = CppSourceGenerator(
        file_path=source_path,
        brief=f"Compressed content of {file_name}",
        author="Andreas Merkle <web@blue-andi.de>",
        includes=source_includes,
        local_variables=source_local_variables,
        external_functions=source_external_functions)

    # Generate header and source files.
    header_generator.generate()
    source_generator.generate()

    return (file_name, base_name)

def generate():
    """Generate C header and source files from files in the embed folder.
    """
    embed_data = []

    for file_name in os.listdir(SRC_FOLDER):
        embed_data.append(embed_file_name(file_name))

    # Genreate module with all compressed files.
    module_header_path = os.path.join(DST_FOLDER, f"{INDEX_FILE_BASE_NAME}.h")
    module_source_path = os.path.join(DST_FOLDER, f"{INDEX_FILE_BASE_NAME}.cpp")

    header_includes = [
        "#include <stdint.h>",
        "#include <stddef.h>",
        "#include <WebServer.h>",]
    header_external_functions = []
    header_external_functions.append(f"""\
/**
 * @brief Setup embedded files for the web server.
 * 
 * @param[in] server    Web server instance to register the embedded files with.
 */
extern void {INDEX_FILE_BASE_NAME}_setup(WebServer& server);\
""")

    header_generator = CppHeaderGenerator(
        file_path=module_header_path,
        brief="Embedded files for the web server",
        author="Andreas Merkle <web@blue-andi.de>",
        group="GENERATED",
        includes=header_includes,
        external_functions=header_external_functions)

    source_includes = [f'#include "{INDEX_FILE_BASE_NAME}.h"']

    for data in embed_data:
        _file_name, base_name = data
        source_includes.append(f'#include "{base_name}.h"')

    source_external_functions = []
    source_external_functions.append(f"""\
extern void {INDEX_FILE_BASE_NAME}_setup(WebServer& server)
{{
""")

    for data in embed_data:
        file_name, base_name = data
        source_external_functions.append(f"""\
    server.on("/{file_name}", HTTP_GET, [&server]() {{
        size_t      size     = 0U;
        const char* content  = reinterpret_cast<const char*>({base_name}_getFile(&size));
        const char* mimeType = {base_name}_getMimeType();

        if (true == {base_name}_isCompressed())
        {{
            server.sendHeader("Content-Encoding", "gzip");
        }}

        server.send_P(200, mimeType, content, size);
    }});
""")

    source_external_functions.append("""\
}
""")

    source_generator = CppSourceGenerator(
        file_path=module_source_path,
        brief="Embedded files for the web server",
        author="Andreas Merkle <web@blue-andi.de>",
        includes=source_includes,
        external_functions=source_external_functions)

    # Generate module header and source files.
    header_generator.generate()
    source_generator.generate()

################################################################################
# Main
################################################################################

generate()
