/*
    Copyright 2015-2026 Clément Gallet <clement.gallet@ens-lyon.org>

    This file is part of libTAS.

    libTAS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    libTAS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with libTAS.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "utils.h"
#include "Context.h"

#include <sys/stat.h>
#include <cerrno> // errno
#include <cstring> // strerror
#include <iostream>
#include <unistd.h> // unlink
#include <sstream>
#include <map>
#include <filesystem>
#include <vector>

void remove_savestates(Context* context)
{
    std::filesystem::path savestateprefix = context->config.savestatedir / context->gamename;
    for (int i=0; i<=10; i++) {
        std::filesystem::path savestatepmpath = savestateprefix;
        savestatepmpath += ".state" + std::to_string(i) + ".pm";
        std::filesystem::remove(savestatepmpath);
        std::filesystem::path savestatepspath = savestateprefix;
        savestatepspath += ".state" + std::to_string(i) + ".p";
        std::filesystem::remove(savestatepspath);
    }
}

int extractBinaryType(std::filesystem::path path)
{
    int extra_flags = 0;
    
    /* Check for MacOS app file, and extract the actual executable if so. */
    std::filesystem::path executable_path = extractMacOSExecutable(path);
    if (!executable_path.empty()) {
        path = executable_path;
        extra_flags = BT_MACOSAPP;
    }
    
    std::ostringstream oss;
    oss << "file --brief --dereference ";
    oss << path;

    std::string outputstr = queryCmd(oss.str());

    if (outputstr.find("pie executable") != std::string::npos) {
        extra_flags |= BT_PIEAPP;
    }

    if (outputstr.find("ELF 32-bit") != std::string::npos) {
        return BT_ELF32 | extra_flags;
    }

    if (outputstr.find("ELF 64-bit") != std::string::npos) {
        return BT_ELF64 | extra_flags;
    }

    if (outputstr.find("PE32 executable") != std::string::npos) {
        return BT_PE32 | extra_flags;
    }

    if (outputstr.find("PE32+ executable") != std::string::npos) {
        return BT_PE32P | extra_flags;
    }

    if (outputstr.find("MS-DOS executable, NE") != std::string::npos) {
        return BT_NE | extra_flags;
    }

    if (outputstr.find("Bourne-Again shell script") != std::string::npos) {
        return BT_SH | extra_flags;
    }

    if (outputstr.find("Mach-O universal binary") != std::string::npos) {
        return BT_MACOSUNI | extra_flags;
    }

    if (outputstr.find("Mach-O executable i386") != std::string::npos) {
        return BT_MACOS32 | extra_flags;
    }

    if (outputstr.find("Mach-O 64-bit") != std::string::npos) {
        return BT_MACOS64 | extra_flags;
    }

    return BT_UNKNOWN | extra_flags;
}

bool isWindowsBinary(int binaryType)
{
    binaryType &= BT_TYPEMASK;
    return (binaryType == BT_PE32) || (binaryType == BT_PE32P) || (binaryType == BT_NE);
}

std::filesystem::path protonDistPath(const std::filesystem::path& proton_path)
{
    /* Proton >= 5.13 uses `files`, older versions use `dist` */
    for (const char* dir : {"files", "dist"}) {
        std::filesystem::path distpath = proton_path / dir;
        if (std::filesystem::exists(distpath / "bin" / "wine"))
            return distpath;
    }
    return "";
}

std::filesystem::path findWinePath(const Context* context, int gameArch)
{
    /* WoW64 builds of wine (default since wine 10) only provide `wine`, which
     * can run both 32-bit and 64-bit games, so fall back to it. */
    std::vector<std::string> winenames;
    if ((gameArch & BT_TYPEMASK) == BT_PE32P)
        winenames.push_back("wine64");
    winenames.push_back("wine");

    if (context->config.use_proton && !context->config.proton_path.empty()) {
        std::filesystem::path distpath = protonDistPath(context->config.proton_path);
        if (distpath.empty())
            return "";

        for (const auto& winename : winenames) {
            std::filesystem::path winepath = distpath / "bin" / winename;
            if (std::filesystem::exists(winepath))
                return winepath;
        }
        return "";
    }

    for (const auto& winename : winenames) {
        std::string winepath = queryCmd("which " + winename);
        if (!winepath.empty())
            return winepath;
    }
    return "";
}

int wineProcessArch(const std::filesystem::path& winepath, int gameArch)
{
    /* The wine loader has the same arch as the process it creates. */
    int wineArch = extractBinaryType(winepath) & BT_TYPEMASK;
    if ((wineArch == BT_ELF32) || (wineArch == BT_ELF64))
        return wineArch;

    /* The loader may be a wrapper script (e.g. on Debian), so guess from the
     * game arch. */
    return ((gameArch & BT_TYPEMASK) == BT_PE32P) ? BT_ELF64 : BT_ELF32;
}

std::filesystem::path extractMacOSExecutable(std::filesystem::path path)
{
    if (std::filesystem::is_directory(path)) {
        /* Extract file name and check for '.app' extension */
        if (path.extension().compare("app") != 00)
            return "";
        
        /* Get executable name from Info.plist CFBundleExecutable field */
        std::filesystem::path plist_path = path / "Contents" / "Info.plist";
        std::ostringstream oss;
        oss << "defaults read ";
        oss << plist_path;
        oss << " CFBundleExecutable";
        
        std::string outputstr = queryCmd(oss.str());

        /* Build path to executable */
        std::filesystem::path executable_path = path / "Contents" / "MacOS" / outputstr;

        /* Check that the file exists */
        if (!std::filesystem::exists(executable_path))
            return "";

        return executable_path;
    }

    return "";
}

std::string queryCmd(std::string_view cmd, int* status)
{
    std::string outputstr;
    std::string cmd_str(cmd);  // Convert string_view to std::string for popen
    FILE *output = popen(cmd_str.c_str(), "r");
    if (output != NULL) {
        char buf[256];
        if (fgets(buf, 256, output) != 0) {
            outputstr = buf;
        }
        int s = pclose(output);
        if (status) *status = s;
    }

    /* Trim the value */
    size_t end = outputstr.find_last_not_of(" \n\r\t\f\v");
    outputstr = (end == std::string::npos) ? "" : outputstr.substr(0, end + 1);
    return outputstr;
}

std::string queryCmdPid(const char **command, pid_t* popen_pid)
{
    std::string outputstr;
    int p[2];
    FILE *output;
    pid_t pid;

    if (pipe(p) != 0)
        return "";

    pid = fork();

    if (pid < 0)
        return "";
    else if (pid == 0) {
        close(p[STDIN_FILENO]);
        dup2(p[STDOUT_FILENO], STDOUT_FILENO);

        execvp(*command, const_cast<char* const*>(command));
        perror("execvp");
        _exit(1);
    }

    *popen_pid = pid;

    close(p[STDOUT_FILENO]);
    output = fdopen(p[STDIN_FILENO], "r"); 
    
    if (output != NULL) {
        char buf[256];
        if (fgets(buf, 256, output) != 0) {
            outputstr = buf;
        }
    }

    /* Trim the value */
    size_t end = outputstr.find_last_not_of(" \n\r\t\f\v");
    outputstr = (end == std::string::npos) ? "" : outputstr.substr(0, end + 1);

    /* Close pipe */
    fclose(output);

    return outputstr;
}

uint64_t getSymbolAddress(const char* symbol, const char* file)
{
    static std::string symbol_file;
    static std::map<std::string, uint64_t> symbol_addresses;
    
    if (symbol_file.compare(file) != 0) {
        symbol_file = file;
        symbol_addresses.clear();
        
        std::ostringstream cmd;
        cmd << "readelf -Ws \"" << file << "\" | awk '{print $2 " " $8}'";

        std::string outputstr;
        FILE *output = popen(cmd.str().c_str(), "r");
        if (output != NULL) {
            char buf[256];
            while (fgets(buf, 256, output) != nullptr) {
                std::istringstream iss(buf);
                uint64_t addr;
                std::string sym;
                iss >> std::hex >> addr >> sym;
                symbol_addresses[sym] = addr;
            }
            pclose(output);
        }
    }
    
    auto search = symbol_addresses.find(std::string(symbol));
    if (search != symbol_addresses.end())
        return search->second;

    return 0;
}
