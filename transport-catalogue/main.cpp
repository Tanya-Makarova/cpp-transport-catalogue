#include <iostream>
#include "input_reader.h"
#include "stat_reader.h"

int main() {
    transport_catalogue::TransportCatalogue catalogue;

    input_reader::InputReader reader;
    reader.ReadInput(std::cin);
    reader.ApplyCommands(catalogue);

    stat_reader::ReadAndPrintStat(catalogue, std::cin, std::cout);
}
