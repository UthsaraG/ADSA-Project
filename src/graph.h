#ifndef ADSA_GRAPH_H
#define ADSA_GRAPH_H

#include <cstddef>
#include <map>
#include <string>
#include <vector>

// Shared graph core. Stable city IDs are not vector indices.
enum class TransportMode { Bus, Train };

struct Stop {
    int id;
    std::string name;
    double x;
    double y;
    std::string zone;
    bool transferStop;
};

struct Edge {
    int to;
    int minutes;
    TransportMode mode;
};

// Weighted, undirected graph; bus and train edges may share endpoints.
class Graph {
public:
    void addStop(const Stop& stop);
    void addEdge(int from, int to, int minutes, TransportMode mode);
    bool hasStop(int id) const;
    const Stop& stop(int id) const;
    const std::map<int, Stop>& stops() const;
    const std::vector<Edge>& neighbors(int id) const;
    int travelMinutes(int from, int to, TransportMode mode) const;
    std::size_t edgeCount(TransportMode mode) const;
    // A mode change requires a marked stop with both services.
    bool canTransfer(int id, TransportMode from, TransportMode to) const;
    std::vector<int> transferStops() const;
    std::vector<std::vector<int>> connectedComponents(TransportMode mode) const;

private:
    std::map<int, Stop> stops_;
    std::map<int, std::vector<Edge>> adjacency_;
};

const char* modeName(TransportMode mode);

#endif
