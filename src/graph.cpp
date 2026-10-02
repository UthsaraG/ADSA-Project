#include "graph.h"

#include <algorithm>
#include <cmath>
#include <queue>
#include <set>
#include <stdexcept>

void Graph::addStop(const Stop& value) {
    if (value.id <= 0 || value.name.empty() || value.zone.empty() ||
        !std::isfinite(value.x) || !std::isfinite(value.y)) {
        throw std::invalid_argument("A stop needs a positive ID, name, zone and finite coordinates.");
    }
    if (hasStop(value.id)) throw std::invalid_argument("Duplicate stop ID.");
    stops_.emplace(value.id, value);
    adjacency_.emplace(value.id, std::vector<Edge>{});
}

void Graph::addEdge(int from, int to, int minutes, TransportMode mode) {
    if (!hasStop(from) || !hasStop(to)) {
        throw std::invalid_argument("Both edge endpoints must exist.");
    }
    if (from == to || minutes <= 0) {
        throw std::invalid_argument("An edge needs different endpoints and a positive travel time.");
    }
    for (const auto& edge : adjacency_.at(from)) {
        if (edge.to == to && edge.mode == mode) {
            throw std::invalid_argument("Duplicate edge for this transport mode.");
        }
    }
    adjacency_.at(from).push_back({to, minutes, mode});
    adjacency_.at(to).push_back({from, minutes, mode});
}

bool Graph::hasStop(int id) const { return stops_.count(id) != 0; }
const Stop& Graph::stop(int id) const { return stops_.at(id); }
const std::map<int, Stop>& Graph::stops() const { return stops_; }
const std::vector<Edge>& Graph::neighbors(int id) const { return adjacency_.at(id); }

int Graph::travelMinutes(int from, int to, TransportMode mode) const {
    stop(from);
    stop(to);
    for (const auto& edge : neighbors(from)) {
        if (edge.to == to && edge.mode == mode) return edge.minutes;
    }
    throw std::invalid_argument("These stops have no direct link for the selected mode.");
}

std::size_t Graph::edgeCount(TransportMode mode) const {
    std::size_t count = 0;
    for (const auto& entry : adjacency_) {
        for (const auto& edge : entry.second) if (edge.mode == mode) ++count;
    }
    return count / 2; // Each undirected edge is stored at both endpoints.
}

std::vector<std::vector<int>> Graph::connectedComponents(TransportMode mode) const {
    std::set<int> visited;
    std::vector<std::vector<int>> result;
    for (const auto& entry : stops_) {
        if (!visited.insert(entry.first).second) continue;
        std::queue<int> pending;
        pending.push(entry.first);
        std::vector<int> component;
        while (!pending.empty()) {
            const int current = pending.front();
            pending.pop();
            component.push_back(current);
            for (const auto& edge : neighbors(current)) {
                if (edge.mode == mode && visited.insert(edge.to).second) pending.push(edge.to);
            }
        }
        std::sort(component.begin(), component.end());
        result.push_back(component);
    }
    return result;
}

const char* modeName(TransportMode mode) {
    return mode == TransportMode::Bus ? "bus" : "train";
}
