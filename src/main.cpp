#include "bus_network.h"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
int integer(const std::string& value) {
    if (value.empty()) throw std::invalid_argument("Enter a whole number.");
    for (char c : value) if (c < '0' || c > '9') throw std::invalid_argument("Enter a non-negative whole number.");
    std::size_t length = 0;
    const int result = std::stoi(value, &length);
    if (length != value.size()) throw std::invalid_argument("Invalid whole number.");
    return result;
}

bool direction(const std::string& value) {
    if (value == "F" || value == "f") return false;
    if (value == "R" || value == "r") return true;
    throw std::invalid_argument("Direction must be F (forward) or R (reverse).");
}

std::string ask(const std::string& prompt) {
    std::cout << prompt;
    std::string answer;
    if (!std::getline(std::cin, answer)) throw std::runtime_error("Input ended.");
    return answer;
}

void showTimetable(const BusNetwork& network, const std::string& id, bool reverse, int departure) {
    const auto legs = network.timetable(id, reverse, departure);
    std::cout << "TIMETABLE " << id << (reverse ? " reverse" : " forward")
              << " | terminus departure " << formatTime(departure) << '\n';
    std::cout << "  " << formatTime(legs.front().departure) << " " << network.graph().stop(legs.front().from).name << '\n';
    for (const auto& leg : legs) {
        std::cout << "  " << formatTime(leg.arrival) << " " << network.graph().stop(leg.to).name << '\n';
    }
}

void simulate(const BusNetwork& network, int start, int end, int step) {
    if (start > end || step <= 0 || step > 24 * 60) {
        throw std::invalid_argument("Choose an end at/after start and a step from 1 to 1440 minutes.");
    }
    for (int minute = start; minute <= end; minute += step) {
        network.printSnapshot(minute, std::cout);
        std::cout << '\n';
    }
}

void demo(const BusNetwork& network) {
    std::cout << "SMART CITY TRANSPORT - PART I: BUS ROUTES AND BUS NETWORK\n"
                 "C++17 | 20 city locations | blue links from the reference map\n"
                 "Assumed service: 06:00-22:00, every 15 min, both directions, zero dwell time.\n\n";
    network.printNetwork(std::cout);
    std::cout << '\n';
    network.printRoutes(std::cout);
    std::cout << '\n';
    showTimetable(network, "B3", false, parseTime("07:00"));
    std::cout << '\n';
    network.printTrip(network.planTrip("B3", 6, 12, parseTime("07:02")), std::cout);
    std::cout << '\n';
    network.printTrip(network.planTrip("B6", 20, 13, parseTime("07:01")), std::cout);
    std::cout << '\n';
    network.printTrip(network.planTrip("B3", 1, 12, parseTime("22:01")), std::cout);
    std::cout << '\n';
    network.printSnapshot(parseTime("06:07"), std::cout);
}

void help() {
    std::cout << "Usage: transport [command]\n"
                 "  (no command)                         Interactive menu\n"
                 "  --demo                               Reproducible Part I demonstration\n"
                 "  --stops | --network | --routes        Inspect reference city\n"
                 "  --timetable ROUTE F|R HH:MM           One scheduled terminus departure\n"
                 "  --trip ROUTE FROM_ID TO_ID HH:MM      Next direct bus on a chosen line\n"
                 "  --snapshot HH:MM                      Moving buses at one time\n"
                 "  --simulate START END STEP_MINUTES    Advance simulation clock\n"
                 "  --export-city PATH                    Export compiled reference city as JSON\n"
                 "  --help                               Show this help\n"
                 "Times are 24-hour HH:MM; IDs are 1-20; service lines are B1-B7.\n";
}

void interactive(const BusNetwork& network) {
    std::cout << "SMART CITY TRANSPORT - PART I (BUS)\n"
                 "Use stop IDs 1-20 and route IDs B1-B7; times use HH:MM.\n";
    while (true) {
        std::cout << "\n1. City stops\n2. Bus graph\n3. Bus service lines\n4. Timetable\n"
                     "5. Direct bus trip\n6. Bus snapshot\n7. Demonstration\n8. Advance simulation clock\n0. Exit\nChoice: ";
        std::string choice;
        if (!std::getline(std::cin, choice) || choice == "0") return;
        try {
            if (choice == "1") network.printStops(std::cout);
            else if (choice == "2") network.printNetwork(std::cout);
            else if (choice == "3") network.printRoutes(std::cout);
            else if (choice == "4") {
                const auto route = ask("Route ID: ");
                const bool reverse = direction(ask("Direction F/R: "));
                const int departure = parseTime(ask("Terminus departure HH:MM: "));
                showTimetable(network, route, reverse, departure);
            } else if (choice == "5") {
                const auto route = ask("Route ID: ");
                const int from = integer(ask("Boarding stop ID: "));
                const int to = integer(ask("Destination stop ID: "));
                const int requested = parseTime(ask("Ready to board HH:MM: "));
                network.printTrip(network.planTrip(route, from, to, requested), std::cout);
            } else if (choice == "6") network.printSnapshot(parseTime(ask("Snapshot HH:MM: ")), std::cout);
            else if (choice == "7") demo(network);
            else if (choice == "8") {
                const int start = parseTime(ask("Start HH:MM: "));
                const int end = parseTime(ask("End HH:MM: "));
                const int step = integer(ask("Step in minutes: "));
                simulate(network, start, end, step);
            } else std::cout << "Choose 0 to 8.\n";
        } catch (const std::exception& error) {
            if (std::cin.eof()) return;
            std::cout << "Input error: " << error.what() << '\n';
        }
    }
}
} // namespace

int main(int argc, char* argv[]) {
    try {
        const auto network = BusNetwork::referenceCity();
        if (argc == 1) { interactive(network); return 0; }
        const std::string command = argv[1];
        if (command == "--help" && argc == 2) help();
        else if (command == "--demo" && argc == 2) demo(network);
        else if (command == "--stops" && argc == 2) network.printStops(std::cout);
        else if (command == "--network" && argc == 2) network.printNetwork(std::cout);
        else if (command == "--routes" && argc == 2) network.printRoutes(std::cout);
        else if (command == "--timetable" && argc == 5) {
            showTimetable(network, argv[2], direction(argv[3]), parseTime(argv[4]));
        } else if (command == "--trip" && argc == 6) {
            network.printTrip(network.planTrip(argv[2], integer(argv[3]), integer(argv[4]), parseTime(argv[5])), std::cout);
        } else if (command == "--snapshot" && argc == 3) network.printSnapshot(parseTime(argv[2]), std::cout);
        else if (command == "--simulate" && argc == 5) simulate(network, parseTime(argv[2]), parseTime(argv[3]), integer(argv[4]));
        else if (command == "--export-city" && argc == 3) {
            std::ofstream file(argv[2]);
            if (!file) throw std::runtime_error("Cannot open the JSON output path.");
            network.exportCity(file);
            file.close();
            if (!file) throw std::runtime_error("Failed to write the JSON output.");
            std::cout << "Exported reference city to " << argv[2] << '\n';
        } else { help(); throw std::invalid_argument("Unknown command or wrong number of arguments."); }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
