#pragma once

#include <cstdint>
#include <iosfwd>
#include <vector>

std::vector<uint8_t> decompress(std::istream& input);
