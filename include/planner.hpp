#pragma once

#include "graph.hpp"
#include <vector>
#include <string>
#include <map>
#include <queue>

// Planner status codes for handling awkward cases
enum class PlannerStatus {
    SUCCESS,
    START_EQUALS_GOAL,
    UNKNOWN_LOCATION,
    NO_PATH_FOUND
};

// Extracted segment for controller execution
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
    PlannerStatus status{PlannerStatus::NO_PATH_FOUND};
    std::string message;
    double total_cost{0.0};
    double total_distance{0.0};
    std::vector<int> path_node_ids;
    std::vector<RouteSegment> route_geometry;

    // Task 5: Drivability status
    bool is_drivable{true};
    std::string drivability_issue;
};

class LanePlanner {
public:
    explicit LanePlanner(const LaneNetwork& network);

    // Primary route planning method (Task 2 & 3)
    PlanResult plan_route(const std::string& start_name, const std::string& goal_name);
    PlanResult plan_route(int start_id, int goal_id);

    // Vehicle turning radius feasibility check (Task 5)
    void verify_drivability(PlanResult& result, double min_turning_radius) const;

private:
    const LaneNetwork& network_;

    // Composite cost function beyond raw distance
    double calculate_edge_cost(const Node& current_node, const LaneEdge& edge, const Node& next_node) const;

    // Euclidean distance heuristic
    double heuristic(const Node& current, const Node& goal) const;
};