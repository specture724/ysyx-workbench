#ifndef MINIRV_LOADER_H
#define MINIRV_LOADER_H

#include <cstddef>
#include <cstdint>
#include <string>

// Load a raw binary image (e.g. produced by `objcopy -O binary`) into the
// reference model's unified RAM at `base`. Returns false and fills `err` on
// failure. Never throws.
bool emu_load_binary (const std::string &path, uint32_t base,
                      std::size_t &loaded_size, std::string &err);

#endif // MINIRV_LOADER_H
