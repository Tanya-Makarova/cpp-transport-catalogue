#pragma once

#include <iosfwd>

namespace json_reader {

void Process(std::istream& input, std::ostream& output);

}  // namespace json_reader
