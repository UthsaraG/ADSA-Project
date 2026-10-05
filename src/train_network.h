#ifndef ADSA_TRAIN_NETWORK_H
#define ADSA_TRAIN_NETWORK_H

#include "graph.h"

#include <ostream>
#include <string>
#include <vector>

// Mirrors the bus module so Members 3 and 4 can treat both modes the same way.
// Times are integer minutes after midnight.
struct TrainLine {
    std::string id;
    std::string name;
    std::vector<int> stops;
    int firstDeparture; // At each terminus.
    int lastDeparture;
    int headwayMinutes;
};

struct TrainLeg {
    int from;
    int to;
    int departure;
    int arrival;
};

struct TrainTrip {
    bool available = false;
    std::string lineId;
    bool reverse = false;
    int requestedAt = 0;
    int departure = 0;
    int arrival = 0;
    std::vector<TrainLeg> legs;
};

struct TrainPosition {
    std::string vehicleId;
    std::string lineId;
    bool reverse;
    int terminusDeparture;
    int from;
    int to;
    int minutesOnLeg;
    int legMinutes;
};

// Adds the 13 red dashed train links to a graph that already holds the 20 stops.
void addTrainEdges(Graph& graph);

class TrainNetwork {
public:
    TrainNetwork(Graph graph, std::vector<TrainLine> lines);
    // cityStops: a graph with the 20 stops (e.g. BusNetwork::referenceCity().graph()).
    // The returned network's graph() holds the input edges plus the train edges.
    static TrainNetwork referenceCity(const Graph& cityStops);
    const Graph& graph() const;
    const std::vector<TrainLine>& lines() const;
    const TrainLine& line(const std::string& id) const;
    int lineMinutes(const std::string& id) const;
    std::vector<TrainLeg> timetable(const std::string& id, bool reverse,
                                    int terminusDeparture) const;
    TrainTrip planTrip(const std::string& id, int from, int to, int requestedAt) const;
    std::vector<TrainPosition> vehiclesAt(int minute) const;
    void printNetwork(std::ostream& out) const;
    void printLines(std::ostream& out) const;
    void printTrip(const TrainTrip& trip, std::ostream& out) const;
    void printSnapshot(int minute, std::ostream& out) const;

private:
    Graph graph_;
    std::vector<TrainLine> lines_;
    void validateLines() const;
};

#endif
