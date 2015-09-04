#ifndef BASE64_HH
#define BASE64_HH

#include "MemBuffer.hh"
#include <string>
#include <cstdint>
#include <utility>

namespace Base64 {
	std::string encode(const uint8_t* input, size_t len);
	std::pair<openmsx::MemBuffer<uint8_t>, size_t> decode(const std::string& input);
}

#endif
