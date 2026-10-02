#include "../src/bus_network.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <tuple>

namespace {
int checks = 0;
void check(bool condition, const char* message) {
    ++checks;
    if (!condition) throw std::runtime_error(message);
}
void rejects(const std::function<void()>& operation, const char* message) {
    bool threw = false;
    try { operation(); } catch (const std::exception&) { threw = true; }
    check(threw, message);
}
}

int main() {
    try {
        const auto network = BusNetwork::referenceCity();
        const auto& graph = network.graph();
        check(graph.stops().size() == 20, "Reference city must have 20 stops.");
        check(graph.edgeCount(TransportMode::Bus) == 23, "Reference has 23 blue links.");
        check(graph.edgeCount(TransportMode::Train) == 0, "Do not turn red train links into bus links.");
        check(graph.neighbors(4).empty(), "Industrial Zone A has no blue edge.");
        check(graph.stop(6).name == "Public Park A", "Resolve reference naming mismatch.");
        std::set<int> transfers;
        for (const auto& entry : graph.stops()) if (entry.second.transferStop) transfers.insert(entry.first);
        check(transfers == std::set<int>{5, 10, 17}, "Preserve all three transfer locations.");
        const auto components = graph.connectedComponents(TransportMode::Bus);
        check(components.size() == 2 && components[0].size() == 19 && components[1] == std::vector<int>{4},
              "There must be one 19-stop bus component and isolated stop 4.");
        check(graph.travelMinutes(1, 7, TransportMode::Bus) == 15, "Agreed assumed time.");
        for (const auto& entry : graph.stops()) {
            for (const auto& edge : graph.neighbors(entry.first)) {
                check(graph.travelMinutes(edge.to, entry.first, edge.mode) == edge.minutes,
                      "Every bus edge must be symmetric.");
            }
        }
        const std::vector<int> expectedDurations{20, 25, 25, 35, 10, 20, 30};
        std::set<std::pair<int, int>> covered;
        for (std::size_t i = 0; i < network.routes().size(); ++i) {
            const auto& service = network.routes()[i];
            check(network.routeMinutes(service.id) == expectedDurations.at(i), "Wrong service duration.");
            for (std::size_t j = 1; j < service.stops.size(); ++j) {
                covered.insert(std::minmax(service.stops[j - 1], service.stops[j]));
            }
        }
        check(covered.size() == 23, "Service lines must cover every blue edge.");

        const auto forward = network.timetable("B3", false, 420);
        check(forward.size() == 4 && forward.front().from == 1 && forward.back().to == 12 &&
              forward.back().arrival == 445, "07:00 airport bus must reach lake at 07:25.");
        const auto reverse = network.timetable("B3", true, 420);
        check(reverse.front().from == 12 && reverse.back().to == 1 && reverse.back().arrival == 445,
              "Reverse timetable must reverse the ordered stops.");
        const auto intermediate = network.planTrip("B3", 6, 12, 422);
        check(intermediate.available && intermediate.departure == 425 && intermediate.arrival == 445 &&
              intermediate.legs.size() == 3, "Intermediate boarding must include upstream offset.");
        const auto exact = network.planTrip("B3", 6, 12, 425);
        check(exact.departure == 425, "A passenger exactly at departure can board.");
        const auto missed = network.planTrip("B3", 6, 12, 426);
        check(missed.departure == 440 && missed.arrival == 460, "Missed bus requires next headway.");
        const auto reversedTrip = network.planTrip("B6", 20, 13, 421);
        check(reversedTrip.reverse && reversedTrip.departure == 435 && reversedTrip.arrival == 455,
              "Reverse trip must catch the next return service.");
        const auto beforeService = network.planTrip("B3", 6, 12, 300);
        check(beforeService.departure == 365, "Before service, wait for first bus to reach boarding stop.");
        check(!network.planTrip("B3", 1, 12, 1321).available, "No terminus departure after 22:00.");
        const auto afterLastTerminus = network.planTrip("B3", 6, 12, 1323);
        check(afterLastTerminus.available && afterLastTerminus.departure == 1325 && afterLastTerminus.arrival == 1345,
              "Last bus remains boardable downstream after 22:00.");
        check(!network.planTrip("B3", 6, 12, 1326).available, "No bus after last downstream departure.");

        check(network.vehiclesAt(359).empty(), "No active services before 06:00.");
        check(network.vehiclesAt(360).size() == 14, "First service starts at both termini on seven lines.");
        const auto snapshot = network.vehiclesAt(367);
        const auto bus = std::find_if(snapshot.begin(), snapshot.end(), [](const BusPosition& p) {
            return p.vehicleId == "B3-F-360";
        });
        check(bus != snapshot.end() && bus->from == 6 && bus->to == 10 && bus->minutesOnLeg == 2,
              "At 06:07 the first B3 bus must be two minutes past Public Park A.");
        check(network.vehiclesAt(1355).empty(), "All final services finish by 22:35.");
        const auto atBoundary = network.vehiclesAt(365);
        const auto boundary = std::find_if(atBoundary.begin(), atBoundary.end(), [](const BusPosition& p) {
            return p.vehicleId == "B3-F-360";
        });
        check(boundary != atBoundary.end() && boundary->from == 6 && boundary->minutesOnLeg == 0,
              "At an arrival boundary, advance to the next leg.");

        rejects([&] { network.planTrip("B3", 4, 12, 420); }, "Train-only stop cannot be boarded on B3.");
        rejects([&] { network.planTrip("B3", 1, 1, 420); }, "Reject same origin and destination.");
        rejects([&] { network.route("B99"); }, "Reject unknown route.");
        rejects([&] { network.timetable("B3", false, 421); }, "Reject off-schedule terminus time.");
        rejects([&] { network.vehiclesAt(-1); }, "Reject negative snapshot time.");
        rejects([&] { network.planTrip("B3", 1, 12, 1440); }, "Reject out-of-day request.");
        check(parseTime("00:00") == 0 && parseTime("23:59") == 1439, "Time boundaries.");
        for (const std::string invalid : {"7:00", "24:00", "07:60", "ab:cd", "07:00x", "-1:00"}) {
            rejects([&] { parseTime(invalid); }, "Reject malformed time.");
        }
        check(formatTime(1505) == "day+1 01:05", "Show next-day arrivals correctly.");

        Graph mixed = graph;
        mixed.addEdge(1, 3, 7, TransportMode::Train);
        check(mixed.travelMinutes(1, 3, TransportMode::Train) == 7 &&
              mixed.travelMinutes(1, 3, TransportMode::Bus) == 10, "Allow separate modes with same endpoints.");
        rejects([&] { mixed.addEdge(3, 1, 10, TransportMode::Bus); }, "Reject reversed duplicate edge.");
        rejects([&] { mixed.addEdge(1, 99, 5, TransportMode::Bus); }, "Reject missing endpoint.");
        rejects([&] { mixed.addEdge(1, 1, 5, TransportMode::Bus); }, "Reject self-loop.");
        rejects([&] { mixed.addEdge(4, 1, 0, TransportMode::Bus); }, "Reject non-positive travel time.");
        rejects([&] { mixed.addStop(graph.stop(1)); }, "Reject duplicate stop.");
        auto invalidRoutes = network.routes();
        invalidRoutes[0].stops = {1, 4};
        rejects([&] { BusNetwork invalid(graph, invalidRoutes); }, "Reject a service using missing bus edges.");
        invalidRoutes = network.routes();
        invalidRoutes[0].headwayMinutes = 0;
        rejects([&] { BusNetwork invalid(graph, invalidRoutes); }, "Reject zero headway.");
        std::ostringstream output;
        network.exportCity(output);
        check(output.str().find("\"assumed_time\": true") != std::string::npos, "Export marks assumed link.");
        std::cout << "PASS: " << checks << " checks (reference topology, coverage, trips, schedules, movement, validation).\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL after " << checks << " checks: " << error.what() << '\n';
        return 1;
    }
}
