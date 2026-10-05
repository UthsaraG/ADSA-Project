#include "train_network.h"

#include <algorithm>
#include <iomanip>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace {

constexpr int firstTrain = 5 * 60 + 30;
constexpr int lastTrain = 23 * 60 + 30;
constexpr int headway = 10;

std::string clockText(int minute) {
    std::ostringstream out;
    if (minute >= 24 * 60) out << "day+" << minute / (24 * 60) << ' ';
    minute %= 24 * 60;
    out << std::setfill('0') << std::setw(2) << minute / 60 << ':' << std::setw(2) << minute % 60;
    return out.str();
}

std::vector<int> orderedStops(const TrainLine& line, bool reverse) {
    auto stops = line.stops;
    if (reverse) std::reverse(stops.begin(), stops.end());
    return stops;
}
} // namespace

void addTrainEdges(Graph& graph) {
    struct Link { int from; int to; int minutes; };
    // The 13 red dashed links of the reference map.
    const std::vector<Link> links = {
        {4, 5, 5}, {5, 10, 10}, {10, 15, 5}, {15, 17, 20}, {17, 19, 20},
        {6, 3, 20}, {3, 7, 10}, {7, 11, 10}, {3, 10, 20},
        {7, 12, 5}, {12, 20, 5}, {20, 19, 10}, {12, 19, 7}
    };
    for (const auto& link : links) graph.addEdge(link.from, link.to, link.minutes, TransportMode::Train);
}

TrainNetwork::TrainNetwork(Graph graph, std::vector<TrainLine> lines)
    : graph_(std::move(graph)), lines_(std::move(lines)) {
    validateLines();
}

TrainNetwork TrainNetwork::referenceCity(const Graph& cityStops) {
    Graph graph = cityStops;
    addTrainEdges(graph);
    
    std::vector<TrainLine> lines = {
        {"T1", "West-South spine", {4, 5, 10, 15, 17, 19}, firstTrain, lastTrain, headway},
        {"T2", "North-East connector", {6, 3, 7, 11}, firstTrain, lastTrain, headway},
        {"T3", "Tech Park express", {3, 10}, firstTrain, lastTrain, headway},
        {"T4", "East coast", {7, 12, 20, 19}, firstTrain, lastTrain, headway},
        {"T5", "Lake-Industrial B shuttle", {12, 19}, firstTrain, lastTrain, headway}
    };
    return TrainNetwork(std::move(graph), std::move(lines));
}

const Graph& TrainNetwork::graph() const { return graph_; }
const std::vector<TrainLine>& TrainNetwork::lines() const { return lines_; }

const TrainLine& TrainNetwork::line(const std::string& id) const {
    for (const auto& item : lines_) if (item.id == id) return item;
    throw std::invalid_argument("Unknown train line. Use T1 to T5 in the reference city.");
}

void TrainNetwork::validateLines() const {
    std::set<std::string> ids;
    for (const auto& item : lines_) {
        if (item.id.empty() || !ids.insert(item.id).second || item.stops.size() < 2 ||
            item.headwayMinutes <= 0 || item.firstDeparture < 0 ||
            item.lastDeparture < item.firstDeparture || item.lastDeparture >= 24 * 60) {
            throw std::invalid_argument("Invalid line ID, stops or daily schedule.");
        }
        std::set<int> visited;
        for (int stop : item.stops) {
            if (!graph_.hasStop(stop) || !visited.insert(stop).second) {
                throw std::invalid_argument("A train line needs existing, distinct stops.");
            }
        }
        for (std::size_t i = 1; i < item.stops.size(); ++i) {
            graph_.travelMinutes(item.stops[i - 1], item.stops[i], TransportMode::Train);
        }
    }
}

int TrainNetwork::lineMinutes(const std::string& id) const {
    const auto& stops = line(id).stops;
    int total = 0;
    for (std::size_t i = 1; i < stops.size(); ++i) {
        total += graph_.travelMinutes(stops[i - 1], stops[i], TransportMode::Train);
    }
    return total;
}

std::vector<TrainLeg> TrainNetwork::timetable(const std::string& id, bool reverse,
                                              int terminusDeparture) const {
    const auto& service = line(id);
    if (terminusDeparture < service.firstDeparture || terminusDeparture > service.lastDeparture ||
        (terminusDeparture - service.firstDeparture) % service.headwayMinutes != 0) {
        throw std::invalid_argument("Departure must be on this line's terminus timetable.");
    }
    const auto stops = orderedStops(service, reverse);
    std::vector<TrainLeg> legs;
    int clock = terminusDeparture;
    for (std::size_t i = 1; i < stops.size(); ++i) {
        const int duration = graph_.travelMinutes(stops[i - 1], stops[i], TransportMode::Train);
        legs.push_back({stops[i - 1], stops[i], clock, clock + duration});
        clock += duration;
    }
    return legs;
}

TrainTrip TrainNetwork::planTrip(const std::string& id, int from, int to, int requestedAt) const {
    if (requestedAt < 0 || requestedAt >= 24 * 60 || from == to) {
        throw std::invalid_argument("Choose different stops and a time in the current day.");
    }
    const auto& service = line(id);
    const auto fromIt = std::find(service.stops.begin(), service.stops.end(), from);
    const auto toIt = std::find(service.stops.begin(), service.stops.end(), to);
    if (fromIt == service.stops.end() || toIt == service.stops.end()) {
        throw std::invalid_argument("Both stops must be on the selected train line.");
    }
    TrainTrip trip;
    trip.lineId = id;
    trip.reverse = fromIt > toIt;
    trip.requestedAt = requestedAt;
    const auto stops = orderedStops(service, trip.reverse);
    const auto fromIndex = static_cast<std::size_t>(std::find(stops.begin(), stops.end(), from) - stops.begin());
    const auto toIndex = static_cast<std::size_t>(std::find(stops.begin(), stops.end(), to) - stops.begin());
    int offset = 0;
    for (std::size_t i = 1; i <= fromIndex; ++i) {
        offset += graph_.travelMinutes(stops[i - 1], stops[i], TransportMode::Train);
    }
    // Next train at the boarding stop, accounting for upstream travel time.
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

std::vector<TrainPosition> TrainNetwork::vehiclesAt(int minute) const {
    if (minute < 0 || minute >= 24 * 60) throw std::invalid_argument("Snapshot time must be in the current day.");
    std::vector<TrainPosition> positions;
    for (const auto& service : lines_) {
        for (bool reverse : {false, true}) {
            for (int departure = service.firstDeparture; departure <= service.lastDeparture;
                 departure += service.headwayMinutes) {
                if (departure > minute) break;
                for (const auto& leg : timetable(service.id, reverse, departure)) {
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

void TrainNetwork::printNetwork(std::ostream& out) const {
    out << "TRAIN NETWORK: " << graph_.edgeCount(TransportMode::Train) << " undirected train links\n";
    std::size_t served = 0;
    for (const auto& entry : graph_.stops()) {
        bool first = true;
        for (const auto& edge : graph_.neighbors(entry.first)) {
            if (edge.mode != TransportMode::Train) continue;
            if (first) out << entry.first << " " << entry.second.name << " -> ";
            else out << "; ";
            out << edge.to << " " << graph_.stop(edge.to).name << " (" << edge.minutes << " min)";
            first = false;
        }
        if (!first) { out << '\n'; ++served; }
    }
    out << served << " of " << graph_.stops().size() << " stops are served by train.\n";
}

void TrainNetwork::printLines(std::ostream& out) const {
    out << "TRAIN LINES (assumed; operate in both directions)\n";
    for (const auto& service : lines_) {
        out << service.id << " - " << service.name << " - " << lineMinutes(service.id) << " min\n  ";
        for (std::size_t i = 0; i < service.stops.size(); ++i) {
            if (i) out << " -> ";
            const int id = service.stops[i];
            out << id << " " << graph_.stop(id).name;
        }
        out << "\n  Depart each terminus " << clockText(service.firstDeparture) << "-"
            << clockText(service.lastDeparture) << " every " << service.headwayMinutes << " min.\n";
    }
}

void TrainNetwork::printTrip(const TrainTrip& trip, std::ostream& out) const {
    if (!trip.available) {
        out << "No remaining " << trip.lineId << " train in this direction at the boarding stop today.\n";
        return;
    }
    out << "TRAIN TRIP " << trip.lineId << (trip.reverse ? " reverse" : " forward")
        << " | requested " << clockText(trip.requestedAt) << '\n';
    out << "Wait " << trip.departure - trip.requestedAt << " min; board " << clockText(trip.departure)
        << "; arrive " << clockText(trip.arrival) << "; ride " << trip.arrival - trip.departure
        << " min; total " << trip.arrival - trip.requestedAt << " min.\n";
    for (const auto& leg : trip.legs) {
        out << "  " << clockText(leg.departure) << " " << graph_.stop(leg.from).name << " -> "
            << clockText(leg.arrival) << " " << graph_.stop(leg.to).name << " ("
            << leg.arrival - leg.departure << " min)\n";
    }
}

void TrainNetwork::printSnapshot(int minute, std::ostream& out) const {
    const auto positions = vehiclesAt(minute);
    out << "TRAIN SNAPSHOT " << clockText(minute) << ": " << positions.size() << " active services\n";
    for (const auto& train : positions) {
        out << "  " << train.vehicleId << " " << (train.reverse ? "reverse" : "forward") << " | "
            << graph_.stop(train.from).name << " -> " << graph_.stop(train.to).name << " | "
            << train.minutesOnLeg << '/' << train.legMinutes << " min completed";
        if (train.minutesOnLeg == 0) out << " [departing stop]";
        out << '\n';
    }
    out << "Each ID represents one scheduled trip; fleet reuse/capacity are not modelled.\n";
}
