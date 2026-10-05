// Reproducible Part II demonstration.
// Build: g++ -std=c++17 src/graph.cpp src/bus_network.cpp src/train_network.cpp tests/train_demo.cpp -o train_demo
#include "../src/bus_network.h"
#include "../src/train_network.h"

#include <iostream>

namespace {
void showTimetable(const TrainNetwork& train, const std::string& id, bool reverse, int departure) {
    const auto legs = train.timetable(id, reverse, departure);
    std::cout << "TIMETABLE " << id << (reverse ? " reverse" : " forward")
              << " | terminus departure " << formatTime(departure) << '\n';
    std::cout << "  " << formatTime(legs.front().departure) << " " << train.graph().stop(legs.front().from).name << '\n';
    for (const auto& leg : legs) {
        std::cout << "  " << formatTime(leg.arrival) << " " << train.graph().stop(leg.to).name << '\n';
    }
}
} // namespace

int main() {
    const BusNetwork bus = BusNetwork::referenceCity();
    const TrainNetwork train = TrainNetwork::referenceCity(bus.graph());
    std::cout << "SMART CITY TRANSPORT - PART II: TRAIN ROUTES AND TRAIN NETWORK\n"
                 "C++17 | 20 city locations | red dashed links from the reference map\n"
                 "Assumed service: 05:30-23:30, every 20 min, both directions, zero dwell time.\n\n";
    train.printNetwork(std::cout);
    std::size_t multi = 0;
    for (const auto& component : train.graph().connectedComponents(TransportMode::Train)) {
        if (component.size() > 1) ++multi;
    }
    std::cout << "Train components with at least one link: " << multi << ".\n";
    std::cout << "Train-only stop: Industrial Zone A (4) has no bus link; it is reached only by train.\n\n";
    train.printLines(std::cout);
    std::cout << '\n';
    showTimetable(train, "T1", false, parseTime("07:10"));
    std::cout << '\n';
    train.printTrip(train.planTrip("T1", 5, 17, parseTime("07:02")), std::cout);
    std::cout << '\n';
    train.printTrip(train.planTrip("T1", 5, 17, parseTime("07:16")), std::cout);
    std::cout << '\n';
    train.printTrip(train.planTrip("T4", 19, 7, parseTime("07:01")), std::cout);
    std::cout << '\n';
    train.printTrip(train.planTrip("T5", 12, 19, parseTime("23:30")), std::cout);
    std::cout << '\n';
    train.printTrip(train.planTrip("T5", 12, 19, parseTime("23:45")), std::cout);
    std::cout << '\n';
    train.printSnapshot(parseTime("05:37"), std::cout);
}
