#include "bus_network.h"

#include <algorithm>
#include <iomanip>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace {
constexpr int firstBus = 6 * 60;
constexpr int lastBus = 22 * 60;
constexpr int headway = 15;

std::vector<int> orderedStops(const BusRoute& route, bool reverse) {
    auto stops = route.stops;
    if (reverse) std::reverse(stops.begin(), stops.end());
    return stops;
}

std::string jsonString(const std::string& value) {
    std::ostringstream out;
    out << '"';
    for (unsigned char c : value) {
        switch (c) {
        case '"': out << "\\\""; break;
        case '\\': out << "\\\\"; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default:
            if (c < 32) out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                            << static_cast<int>(c) << std::dec << std::setfill(' ');
            else out << c;
        }
    }
    out << '"';
    return out.str();
}
} // namespace

BusNetwork::BusNetwork(Graph graph, std::vector<BusRoute> routes)
    : graph_(std::move(graph)), routes_(std::move(routes)) {
    validateRoutes();
}

BusNetwork BusNetwork::referenceCity() {
    Graph graph;
    // Normalized coordinates and zones from the supplied node table.
    const std::vector<Stop> stops = {
        {1, "Airport", 5, 9.2, "North", false},
        {2, "Residential Zone A", 1.5, 8.7, "North-West", false},
        {3, "Tech Park", 8.2, 8.7, "North-East", false},
        {4, "Industrial Zone A", 0.6, 6.7, "West", false},
        {5, "Transport Hub A", 2.8, 7.3, "North-West Center", true},
        {6, "Public Park A", 5, 7.3, "North Center", false},
        {7, "Economic Zone", 8.7, 6.3, "East", false},
        {8, "University", 1.2, 4.9, "West", false},
        {9, "City Hall", 3.2, 5.3, "Center West", false},
        {10, "Multimodal Transport Center", 5, 4.9, "Center", true},
        {11, "City Area", 6.6, 5.3, "Center East", false},
        {12, "Lake", 9, 4.4, "Far East", false},
        {13, "Hospital", 1.6, 3.3, "West", false},
        {14, "Residential Zone B", 3.2, 3.3, "Center West", false},
        {15, "Public Park B", 5, 3.1, "Center", false},
        {16, "Mall", 6.6, 3.3, "Center East", false},
        {17, "Transport Hub B", 5, 1.5, "South Center", true},
        {18, "Residential Zone C", 2.2, 0.9, "South-West", false},
        {19, "Industrial Zone B", 7.5, 1.2, "South-East", false},
        {20, "Port", 9, 2, "Far South-East", false}
    };
    for (const auto& stop : stops) graph.addStop(stop);
    struct Link { int from; int to; int minutes; };
    // Only solid blue links. Airport--Economic Zone is the agreed 15-min assumption.
    const std::vector<Link> links = {
        {2, 1, 10}, {2, 5, 5}, {5, 1, 5}, {1, 6, 5}, {1, 3, 10}, {1, 7, 15},
        {5, 6, 5}, {5, 9, 5}, {6, 10, 5}, {8, 9, 5}, {9, 10, 5}, {10, 11, 5},
        {11, 12, 10}, {8, 13, 5}, {13, 14, 5}, {14, 15, 5}, {15, 16, 5},
        {16, 20, 5}, {13, 18, 5}, {18, 17, 10}, {14, 17, 10}, {16, 17, 10},
        {16, 19, 10}
    };
    for (const auto& link : links) graph.addEdge(link.from, link.to, link.minutes, TransportMode::Bus);
    // Proposed service lines, not labels from the diagram. Cover every blue edge.
    std::vector<BusRoute> routes = {
        {"B1", "Northern connector", {2, 1, 3}, firstBus, lastBus, headway},
        {"B2", "Airport and economy", {2, 5, 1, 7}, firstBus, lastBus, headway},
        {"B3", "Airport to lake", {1, 6, 10, 11, 12}, firstBus, lastBus, headway},
        {"B4", "Western connector", {6, 5, 9, 8, 13, 18, 17}, firstBus, lastBus, headway},
        {"B5", "University shuttle", {8, 9, 10}, firstBus, lastBus, headway},
        {"B6", "Hospital to port", {13, 14, 15, 16, 20}, firstBus, lastBus, headway},
        {"B7", "Southern connector", {14, 17, 16, 19}, firstBus, lastBus, headway}
    };
    return BusNetwork(std::move(graph), std::move(routes));
}

const Graph& BusNetwork::graph() const { return graph_; }
const std::vector<BusRoute>& BusNetwork::routes() const { return routes_; }

const BusRoute& BusNetwork::route(const std::string& id) const {
    for (const auto& route : routes_) if (route.id == id) return route;
    throw std::invalid_argument("Unknown bus route. Use B1 to B7 in the reference city.");
}

void BusNetwork::validateRoutes() const {
    std::set<std::string> ids;
    for (const auto& route : routes_) {
        if (route.id.empty() || !ids.insert(route.id).second || route.stops.size() < 2 ||
            route.headwayMinutes <= 0 || route.firstDeparture < 0 ||
            route.lastDeparture < route.firstDeparture || route.lastDeparture >= 24 * 60) {
            throw std::invalid_argument("Invalid route ID, stops or daily schedule.");
        }
        std::set<int> visited;
        for (int stop : route.stops) {
            if (!graph_.hasStop(stop) || !visited.insert(stop).second) {
                throw std::invalid_argument("A service route needs existing, distinct stops.");
            }
        }
        for (std::size_t i = 1; i < route.stops.size(); ++i) {
            graph_.travelMinutes(route.stops[i - 1], route.stops[i], TransportMode::Bus);
        }
    }
}

int BusNetwork::routeMinutes(const std::string& id) const {
    const auto& stops = route(id).stops;
    int total = 0;
    for (std::size_t i = 1; i < stops.size(); ++i) {
        total += graph_.travelMinutes(stops[i - 1], stops[i], TransportMode::Bus);
    }
    return total;
}

std::vector<BusLeg> BusNetwork::timetable(const std::string& id, bool reverse,
                                        int terminusDeparture) const {
    const auto& service = route(id);
    if (terminusDeparture < service.firstDeparture || terminusDeparture > service.lastDeparture ||
        (terminusDeparture - service.firstDeparture) % service.headwayMinutes != 0) {
        throw std::invalid_argument("Departure must be on this route's terminus timetable.");
    }
    const auto stops = orderedStops(service, reverse);
    std::vector<BusLeg> legs;
    int clock = terminusDeparture;
    for (std::size_t i = 1; i < stops.size(); ++i) {
        const int duration = graph_.travelMinutes(stops[i - 1], stops[i], TransportMode::Bus);
        legs.push_back({stops[i - 1], stops[i], clock, clock + duration});
        clock += duration;
    }
    return legs;
}

BusTrip BusNetwork::planTrip(const std::string& id, int from, int to, int requestedAt) const {
    if (requestedAt < 0 || requestedAt >= 24 * 60 || from == to) {
        throw std::invalid_argument("Choose different stops and a time in the current day.");
    }
    const auto& service = route(id);
    const auto fromIt = std::find(service.stops.begin(), service.stops.end(), from);
    const auto toIt = std::find(service.stops.begin(), service.stops.end(), to);
    if (fromIt == service.stops.end() || toIt == service.stops.end()) {
        throw std::invalid_argument("Both stops must be on the selected bus route.");
    }
    BusTrip trip;
    trip.routeId = id;
    trip.reverse = fromIt > toIt;
    trip.requestedAt = requestedAt;
    const auto stops = orderedStops(service, trip.reverse);
    const auto fromIndex = static_cast<std::size_t>(std::find(stops.begin(), stops.end(), from) - stops.begin());
    const auto toIndex = static_cast<std::size_t>(std::find(stops.begin(), stops.end(), to) - stops.begin());
    int offset = 0;
    for (std::size_t i = 1; i <= fromIndex; ++i) {
        offset += graph_.travelMinutes(stops[i - 1], stops[i], TransportMode::Bus);
    }
    // Next bus at the boarding stop, accounting for upstream travel.
    int departureAtTerminus = service.firstDeparture;
    const int earliestTerminus = requestedAt - offset;
    if (earliestTerminus > departureAtTerminus) {
        const int difference = earliestTerminus - departureAtTerminus;
        departureAtTerminus += ((difference + service.headwayMinutes - 1) /
                               service.headwayMinutes) * service.headwayMinutes;
    }
    if (departureAtTerminus > service.lastDeparture) return trip;
    const auto allLegs = timetable(id, trip.reverse, departureAtTerminus);
    trip.legs.assign(allLegs.begin() + static_cast<std::ptrdiff_t>(fromIndex),
                     allLegs.begin() + static_cast<std::ptrdiff_t>(toIndex));
    trip.available = true;
    trip.departure = trip.legs.front().departure;
    trip.arrival = trip.legs.back().arrival;
    return trip;
}

std::vector<BusPosition> BusNetwork::vehiclesAt(int minute) const {
    if (minute < 0 || minute >= 24 * 60) throw std::invalid_argument("Snapshot time must be in the current day.");
    std::vector<BusPosition> positions;
    for (const auto& service : routes_) {
        for (bool reverse : {false, true}) {
            for (int departure = service.firstDeparture; departure <= service.lastDeparture;
                 departure += service.headwayMinutes) {
                if (departure > minute) break;
                for (const auto& leg : timetable(service.id, reverse, departure)) {
                    // Arrivals immediately start the next leg; final arrivals end service.
                    if (leg.departure <= minute && minute < leg.arrival) {
                        positions.push_back({service.id + (reverse ? "-R-" : "-F-") +
                            std::to_string(departure), service.id, reverse, departure,
                            leg.from, leg.to, minute - leg.departure, leg.arrival - leg.departure});
                        break;
                    }
                }
            }
        }
    }
    return positions;
}

void BusNetwork::printStops(std::ostream& out) const {
    out << "CITY STOPS (coordinates from the reference table)\n";
    for (const auto& entry : graph_.stops()) {
        const auto& stop = entry.second;
        out << std::setw(2) << stop.id << "  " << stop.name << "  (" << stop.x << ", "
            << stop.y << ")  " << stop.zone;
        if (stop.transferStop) out << "  [BUS/TRAIN TRANSFER LOCATION]";
        out << '\n';
    }
    out << "Stop 4 has no bus edge in the reference map.\n";
}

void BusNetwork::printNetwork(std::ostream& out) const {
    out << "BUS NETWORK: " << graph_.stops().size() << " city stops, "
        << graph_.edgeCount(TransportMode::Bus) << " undirected bus links\n";
    for (const auto& entry : graph_.stops()) {
        out << entry.first << " " << entry.second.name << " -> ";
        bool first = true;
        for (const auto& edge : graph_.neighbors(entry.first)) {
            if (edge.mode != TransportMode::Bus) continue;
            if (!first) out << "; ";
            out << edge.to << " " << graph_.stop(edge.to).name << " (" << edge.minutes << " min)";
            first = false;
        }
        if (first) out << "no bus connection";
        out << '\n';
    }
    const auto components = graph_.connectedComponents(TransportMode::Bus);
    out << "Bus connected components: " << components.size() << " (sizes";
    for (const auto& component : components) out << ' ' << component.size();
    out << ").\nAirport--Economic Zone: 15 min, agreed assumption (unlabelled in map).\n";
}

void BusNetwork::printRoutes(std::ostream& out) const {
    out << "BUS SERVICE LINES (assumed; operate in both directions)\n";
    for (const auto& service : routes_) {
        out << service.id << " - " << service.name << " - " << routeMinutes(service.id) << " min\n  ";
        for (std::size_t i = 0; i < service.stops.size(); ++i) {
            if (i) out << " -> ";
            const int id = service.stops[i];
            out << id << " " << graph_.stop(id).name;
        }
        out << "\n  Depart each terminus " << formatTime(service.firstDeparture) << "-"
            << formatTime(service.lastDeparture) << " every " << service.headwayMinutes << " min.\n";
    }
}

void BusNetwork::printTrip(const BusTrip& trip, std::ostream& out) const {
    if (!trip.available) {
        out << "No remaining " << trip.routeId << " bus in this direction at the boarding stop today.\n";
        return;
    }
    out << "BUS TRIP " << trip.routeId << (trip.reverse ? " reverse" : " forward")
        << " | requested " << formatTime(trip.requestedAt) << '\n';
    out << "Wait " << trip.departure - trip.requestedAt << " min; board " << formatTime(trip.departure)
        << "; arrive " << formatTime(trip.arrival) << "; ride " << trip.arrival - trip.departure
        << " min; total " << trip.arrival - trip.requestedAt << " min.\n";
    for (const auto& leg : trip.legs) {
        out << "  " << formatTime(leg.departure) << " " << graph_.stop(leg.from).name << " -> "
            << formatTime(leg.arrival) << " " << graph_.stop(leg.to).name << " ("
            << leg.arrival - leg.departure << " min)\n";
    }
}

void BusNetwork::printSnapshot(int minute, std::ostream& out) const {
    const auto positions = vehiclesAt(minute);
    out << "BUS SNAPSHOT " << formatTime(minute) << ": " << positions.size() << " active services\n";
    for (const auto& bus : positions) {
        out << "  " << bus.vehicleId << " " << (bus.reverse ? "reverse" : "forward") << " | "
            << graph_.stop(bus.from).name << " -> " << graph_.stop(bus.to).name << " | "
            << bus.minutesOnLeg << '/' << bus.legMinutes << " min completed";
        if (bus.minutesOnLeg == 0) out << " [departing stop]";
        out << '\n';
    }
    out << "Each ID represents one scheduled trip; fleet reuse/capacity are not modelled in Part I.\n";
}

void BusNetwork::exportCity(std::ostream& out) const {
    out << "{\n  \"schema_version\": 1,\n  \"part\": \"I - bus network\",\n"
           "  \"coordinate_units\": \"normalized map positions, not measured distances\",\n"
           "  \"assumptions\": [\n"
           "    \"All bus links are bidirectional with constant travel times.\",\n"
           "    \"Airport to Economic Zone takes 15 minutes; its map link is unlabelled.\",\n"
           "    \"Public Park in the node table means Public Park A (stop 6).\",\n"
           "    \"Seven service lines are proposed; each departs both termini 06:00-22:00 every 15 minutes.\",\n"
           "    \"No dwell time, congestion, fleet reuse, passenger capacity or train operation in Part I.\"\n"
           "  ],\n  \"stops\": [\n";
    std::size_t count = 0;
    for (const auto& entry : graph_.stops()) {
        const auto& stop = entry.second;
        out << "    {\"id\": " << stop.id << ", \"name\": " << jsonString(stop.name)
            << ", \"x\": " << stop.x << ", \"y\": " << stop.y << ", \"zone\": "
            << jsonString(stop.zone) << ", \"transfer_stop\": " << (stop.transferStop ? "true" : "false") << '}';
        out << (++count == graph_.stops().size() ? "\n" : ",\n");
    }
    out << "  ],\n  \"bus_edges\": [\n";
    count = 0;
    for (const auto& entry : graph_.stops()) {
        for (const auto& edge : graph_.neighbors(entry.first)) {
            if (edge.mode != TransportMode::Bus || entry.first >= edge.to) continue;
            const bool assumed = entry.first == 1 && edge.to == 7;
            out << "    {\"from\": " << entry.first << ", \"to\": " << edge.to
                << ", \"minutes\": " << edge.minutes << ", \"bidirectional\": true, \"assumed_time\": "
                << (assumed ? "true" : "false") << '}';
            out << (++count == graph_.edgeCount(TransportMode::Bus) ? "\n" : ",\n");
        }
    }
    out << "  ],\n  \"bus_routes\": [\n";
    for (std::size_t i = 0; i < routes_.size(); ++i) {
        const auto& service = routes_[i];
        out << "    {\"id\": " << jsonString(service.id) << ", \"name\": " << jsonString(service.name)
            << ", \"stops\": [";
        for (std::size_t j = 0; j < service.stops.size(); ++j) {
            if (j) out << ", ";
            out << service.stops[j];
        }
        out << "], \"first_departure_minutes\": " << service.firstDeparture
            << ", \"last_departure_minutes\": " << service.lastDeparture
            << ", \"headway_minutes\": " << service.headwayMinutes << ", \"bidirectional\": true}";
        out << (i + 1 == routes_.size() ? "\n" : ",\n");
    }
    out << "  ]\n}\n";
}

int parseTime(const std::string& value) {
    if (value.size() != 5 || value[2] != ':' || value[0] < '0' || value[0] > '9' ||
        value[1] < '0' || value[1] > '9' || value[3] < '0' || value[3] > '9' ||
        value[4] < '0' || value[4] > '9') {
        throw std::invalid_argument("Time must be HH:MM, for example 07:00.");
    }
    const int hours = (value[0] - '0') * 10 + value[1] - '0';
    const int minutes = (value[3] - '0') * 10 + value[4] - '0';
    if (hours > 23 || minutes > 59) throw std::invalid_argument("Time must be 00:00 to 23:59.");
    return hours * 60 + minutes;
}

std::string formatTime(int minute) {
    if (minute < 0) throw std::invalid_argument("Time cannot be negative.");
    std::ostringstream out;
    if (minute >= 24 * 60) out << "day+" << minute / (24 * 60) << ' ';
    minute %= 24 * 60;
    out << std::setfill('0') << std::setw(2) << minute / 60 << ':' << std::setw(2) << minute % 60;
    return out.str();
}
