#include "simulate.h"
#include <ctime>
#include <iostream>
#include <random>

std::vector<Passenger> generatePassengers(int hour) {
  std::vector<Passenger> hourly_passengers;

  Graph g = loadGraph("data/city.json");

  std::mt19937 rng(std::time(nullptr) + hour);

  int num_passengers = 0;

  if ((hour >= 7 && hour <= 9) || (hour >= 17 && hour <= 19)) {
    std::uniform_int_distribution<int> dist(80, 150);
    num_passengers = dist(rng);
  } else {
    std::uniform_int_distribution<int> dist(10, 40);
    num_passengers = dist(rng);
  }

  std::uniform_int_distribution<int> node_dist(1, 20);
  std::uniform_int_distribution<int> min_dist(0, 59);

  for (int i = 0; i < num_passengers; ++i) {
    Passenger p;
    p.id = (hour * 1000) + i;

    int orig = node_dist(rng);
    int dest = node_dist(rng);

    while (orig == dest) {
      dest = node_dist(rng);
    }

    p.origin = orig;
    p.destination = dest;

    p.departureTime = (hour * 60) + min_dist(rng);

    hourly_passengers.push_back(p);
  }

  std::cout << "Hour " << hour << " : Generated " << num_passengers
            << " passengers.\n";

  return hourly_passengers;
}