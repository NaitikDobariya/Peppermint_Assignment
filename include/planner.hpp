#pragma once

#include "graph.hpp"
#include <vector>
#include <string>
#include <map>
#include <queue>

// Planner status codes for handling awkward cases, gives status code instead of empty arrays
enum class PlannerStatus {
    SUCCESS,
    START_EQUALS_GOAL,
    UNKNOWN_LOCATION,
    NO_PATH_FOUND
};

// Extracted segment for controller execution, gives some info that the local controller can use
// Can figure out heading angle, node geometry, and other constraints that can be useful
struct RouteSegment {
    int from_node;
    int to_node;
    GeometryType type;
    double length;
    double arc_radius;
    double speed_limit;
};

// Plan result output
struct PlanResult {
    // Status check
    PlannerStatus status{PlannerStatus::NO_PATH_FOUND};
    std::string message;

    // The net cost and the distance, obviously both are not equal all the time
    double total_cost{0.0};
    double total_distance{0.0};

    // The final path
    std::vector<int> path_node_ids;
    std::vector<RouteSegment> route_geometry;

    // Drivability status, turning radius needs to be satisfied acroess the entire path
    bool is_drivable{true};
    std::string drivability_issue;
};

class LanePlanner {
public:
    // foo(LanePlanner(my_lane_planner_instance)), explicit instantiation
    explicit LanePlanner(const LaneNetwork& network);

    // Primary route planning method, same kaam different naam. The 2nd one is deterministic tho
    PlanResult plan_route(const std::string& start_name, const std::string& goal_name);
    PlanResult plan_route(int start_id, int goal_id);

    // Vehicle turning radius feasibility check
    void verify_drivability(PlanResult& result, double min_turning_radius) const;

private:
    const LaneNetwork& network_;

    // Composite cost function beyond raw distance
    double calculate_edge_cost(const Node& current_node, const LaneEdge& edge, const Node& next_node) const;

    // Euclidean distance heuristic, very basic, but works in our case
    double heuristic(const Node& current, const Node& goal) const;
};