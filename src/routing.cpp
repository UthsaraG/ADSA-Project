#include "routing.h"
#include <queue>
#include <map>
#include <iostream>
#include <algorithm>
#include <climits>

using namespace std;

PathResults bfs(const Graph&g, int startId, int endId){
    if(startId==endId){
        return {{startId},0,true};
    }

    map<int,bool>visited;
    map<int, int>parent;

    queue<int>q;
    q.push(startId);
    visited[startId]=true;
    parent[startId]= -1;

    while(!q.empty()){
        
        int current = q.front();
        q.pop();
        for( const Edge& edge : g.neighbors(current)){
            int neighbor = edge.to;
            if(visited[neighbor]){
                continue;
            }
            visited[neighbor] = true;
            parent[neighbor] = current;

            if(neighbor == endId){
                vector<int>path;
                int step = endId;

                while( step!= -1){
                    path.push_back(step);
                    step = parent[step];                    
                }
                reverse(path.begin(),path.end());

                int totalTime = 0;
                for (int i = 0; i + 1 < (int)path.size(); i++) {
                    for (const Edge& e : g.neighbors(path[i])) {
                        if (e.to == path[i + 1]) {
                            totalTime += e.minutes;
                            break;
                        }
                    }
                }

                return {path, totalTime, true};
            }
            q.push(neighbor);
        }
    }
    return PathResults{{},0,false};
}

PathResults dijkstra(const Graph&g, int startId, int endId){
    if(startId==endId){
        return {{startId},0,true};
    }
    map<int, int>dist;
    for(auto entry : g.stops()){
        dist[entry.first] = INT_MAX;
    }
    dist[startId] = 0;

    map<int, int>parent;
    parent[startId] = -1;
    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int,int>>> pq;
    pq.push({0,startId});

    while(!pq.empty()){

        int currentDist = pq.top().first;
        int current = pq.top().second;
        pq.pop();

        if(current==endId) break;

        if(currentDist > dist[current]) continue;

        for(const Edge&edge : g.neighbors(current)){
            int newDist = dist[current] + edge.minutes;
            
            int neighbor = edge.to;
            if(newDist < dist[neighbor]){
                dist[neighbor] = newDist;
                parent[neighbor] = current;
                pq.push({newDist, neighbor});
            }            
        }
    }

    if(dist[endId]==INT_MAX){
            return PathResults{{},0,false};
    }
    vector<int>path;
                int step = endId;

                while( step!= -1){
                    path.push_back(step);
                    step = parent[step];                    
                }
                reverse(path.begin(),path.end());
    return PathResults{path, dist[endId], true};
}

void profileNetwork(const Graph&g, vector<pair<int,int>>passengerTrips){
    int totalTime = 0;
    int tripsFound = 0;
    int tripsNotFound = 0;
    map<int,int> stopUsage;

    for( const auto& trip : passengerTrips){
        PathResults result = dijkstra(g, trip.first, trip.second);

        if(!result.found){
            tripsNotFound++;
            continue;
        }

        tripsFound++;
        totalTime += result.totalMinutes;

        for(int stopId : result.stopIds) 
            stopUsage[stopId]++;
    }

    int busiestId = -1;
    int busiestCount = 0;
    
    for(const auto&entry : stopUsage){
        if(entry.second > busiestCount){
            busiestCount = entry.second;
            busiestId = entry.first;
        }
    }

    cout << "======Statistics======"<<endl;
    cout << "Total Passengers: " << passengerTrips.size() << endl;
    cout << "Trips Found: " << tripsFound << endl;
    cout << "Trips Not Found: " << tripsNotFound << endl;

    if(tripsFound > 0){
    cout << "Average travel time: " << totalTime/tripsFound << "mins" << endl;
    }

    if(busiestId != -1){
        cout << "Busiest stop: " << g.stop(busiestId).name << endl;
    }

    cout << "======================" << endl;

}