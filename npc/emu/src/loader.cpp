#include "loader.h"
#include "minirvEMU.h"

#include <exception>
#include <fstream>
#include <string>
#include <vector>

bool emu_load_binary (const std::string &path, uint32_t base,
                      std::size_t &loaded_size, std::string &err) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        err = "cannot open '" + path + "'";
        return false;
    }

    file.seekg(0, std::ios::end);
    const std::streamoff end = file.tellg();
    if (end < 0) {
        err = "cannot determine the size of '" + path + "'";
        return false;
    }
    if (end == 0) {
        err = "'" + path + "' is empty";
        return false;
    }
    if (static_cast<uint64_t>(end) > ref_ram_size()) {
        err = "'" + path + "' is " + std::to_string(end) +
              " bytes, which does not fit in the " +
              std::to_string(ref_ram_size()) + "-byte RAM";
        return false;
    }

    std::vector<uint8_t> image(static_cast<std::size_t>(end));
    file.seekg(0, std::ios::beg);
    if (!file.read(reinterpret_cast<char *>(image.data()), end)) {
        err = "cannot read '" + path + "'";
        return false;
    }

    try {
        ref_load_ram(image.data(), image.size(), base);
    } catch (const std::exception &e) {
        err = e.what();
        return false;
    }

    loaded_size = image.size();
    return true;
}
