#pragma once
// чтение запросов к уже созданной базе и вывод ответа пользователю

#include <iosfwd>
#include <string_view>

#include "transport_catalogue.h"

namespace stat_reader {

void ParseAndPrintStat(const transport_catalogue::TransportCatalogue& transport_catalogue,
                       std::string_view request,
                       std::ostream& output);

void ReadAndPrintStat(const transport_catalogue::TransportCatalogue& transport_catalogue,
                      std::istream& input,
                      std::ostream& output);

}  // namespace stat_reader
