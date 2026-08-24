#include "planner.hpp"
#include <cmath>
#include <algorithm>
#include <limits>
#include <sstream>

// Internal node structure for A* priority queue
struct AStarNode {
    int node_id;
    double f_score;

    // Strict deterministic tie-breaker:
    // If f_scores are equal, fall back to node_id comparison.
    // Guarantees identical expansion order across compilers & platforms.
    bool operator>(const AStarNode& other) const {
        if (std::abs(f_score - other.f_score) > 1e-7) {
            return f_score > other.f_score;
        }
        return node_id > other.node_id;
    }
};


// Declare the network first
LanePlanner::LanePlanner(const LaneNetwork& network) : network_(network) {}

// Pythagoras
double LanePlanner::heuristic(const Node& current, const Node& goal) const {
    double dx = current.x - goal.x;
    double dy = current.y - goal.y;
    return std::sqrt(dx * dx + dy * dy);
}


// Cost calculator function
double LanePlanner::calculate_edge_cost(const Node& current_node, const LaneEdge& edge, const Node& next_node) const {
    // 1. Distance cost
    double cost = edge.length;

    // 2. Heading change penalty (rewards smoother turns)
    double heading_diff = std::abs(next_node.heading - current_node.heading);
    while (heading_diff > M_PI) heading_diff -= 2.0 * M_PI;
    heading_diff = std::abs(heading_diff);
    cost += 1.5 * heading_diff; 

    // 3. Lane width preference (penalize narrow channels), inverse thing is not very good tho
    if (edge.lane_width > 0.0) {
        cost += (1.0 / edge.lane_width);
    }

    // 4. Traversals penalty for doors/lifts
    cost += edge.traversal_penalty;

    return cost;
}

// Dummy method depends on the one comming after it to do the "same" thing
PlanResult LanePlanner::plan_route(const std::string& start_name, const std::string& goal_name) {
    const Node* start_node = network_.get_node_by_name(start_name);
    const Node* goal_node = network_.get_node_by_name(goal_name);

    // Awkward Case: Unknown location name
    if (!start_node || !goal_node) {
        PlanResult res;
        res.status = PlannerStatus::UNKNOWN_LOCATION;
        res.message = "Error: Start or Goal location name not found in lane network.";
        return res;
    }

    return plan_route(start_node->id, goal_node->id);
}


// The main stuff
PlanResult LanePlanner::plan_route(int start_id, int goal_id) {
    PlanResult result;

    // Awkward Case: Start and Goal are identical
    if (start_id == goal_id) {
        result.status = PlannerStatus::START_EQUALS_GOAL;
        result.message = "Start and Goal locations are identical. Zero distance route.";
        result.path_node_ids.push_back(start_id);
        return result;
    }

    const Node* goal_node = network_.get_node(goal_id);
    if (!goal_node) {
        result.status = PlannerStatus::UNKNOWN_LOCATION;
        result.message = "Error: Invalid Goal node ID.";
        return result;
    }

    // Min-heap for A* frontier
    std::priority_queue<AStarNode, std::vector<AStarNode>, std::greater<AStarNode>> frontier;

    // Standard std::map for deterministic traversal order across platforms
    std::map<int, double> g_score;
    std::map<int, std::pair<int, LaneEdge>> came_from; // node_id -> {parent_id, traversed_edge}

    // Initialize scores
    for (const auto& [id, node] : network_.get_all_nodes()) {
        g_score[id] = std::numeric_limits<double>::infinity();
    }

    g_score[start_id] = 0.0;
    frontier.push({start_id, heuristic(*network_.get_node(start_id), *goal_node)});

    bool found = false;

    while (!frontier.empty()) {
        AStarNode current = frontier.top();
        frontier.pop();

        if (current.node_id == goal_id) {
            found = true;
            break;
        }

        if (current.f_score > g_score[current.node_id] + heuristic(*network_.get_node(current.node_id), *goal_node) + 1e-7) {
            continue;
        }

        const Node* curr_node_ptr = network_.get_node(current.node_id);

        for (const auto& edge : network_.get_outgoing_edges(current.node_id)) {
            const Node* next_node_ptr = network_.get_node(edge.to_node);
            if (!next_node_ptr) continue;

            double step_cost = calculate_edge_cost(*curr_node_ptr, edge, *next_node_ptr);
            double tentative_g = g_score[current.node_id] + step_cost;

            if (tentative_g < g_score[edge.to_node]) {
                came_from[edge.to_node] = {current.node_id, edge};
                g_score[edge.to_node] = tentative_g;
                double f = tentative_g + heuristic(*next_node_ptr, *goal_node);
                frontier.push({edge.to_node, f});
            }
        }
    }

    // Awkward Case: Unreachable due to one-way walls or disconnected graph
    if (!found) {
        result.status = PlannerStatus::NO_PATH_FOUND;
        result.message = "Error: No valid route exists between start and goal (graph disconnected or one-way restriction).";
        return result;
    }

    // Reconstruct Path & Geometry
    int curr_id = goal_id;
    std::vector<RouteSegment> rev_geometry;

    result.path_node_ids.push_back(curr_id);
    result.total_cost = g_score[goal_id];

    while (curr_id != start_id) {
        auto [parent_id, edge] = came_from[curr_id];
        
        RouteSegment seg{
            edge.from_node,
            edge.to_node,
            edge.type,
            edge.length,
            edge.arc_radius,
            edge.speed_limit
        };

        rev_geometry.push_back(seg);
        result.total_distance += edge.length;

        curr_id = parent_id;
        result.path_node_ids.push_back(curr_id);
    }

    std::reverse(result.path_node_ids.begin(), result.path_node_ids.end());
    std::reverse(rev_geometry.begin(), rev_geometry.end());

    result.route_geometry = rev_geometry;
    result.status = PlannerStatus::SUCCESS;
    result.message = "Route successfully planned.";

    return result;
}

void LanePlanner::verify_drivability(PlanResult& result, double min_turning_radius) const {
    if (result.status != PlannerStatus::SUCCESS) return;

    for (const auto& seg : result.route_geometry) {
        // If segment is an arc, directly compare its radius to min turning radius
        if (seg.type == GeometryType::ARC && seg.arc_radius < min_turning_radius) {
            result.is_drivable = false;
            std::ostringstream ss;
            ss << "Fails turn radius constraint at arc segment between Node " 
               << seg.from_node << " and Node " << seg.to_node 
               << " (Arc Radius: " << seg.arc_radius << "m < Min Turning Radius: " 
               << min_turning_radius << "m)";
            result.drivability_issue = ss.str();
            return;
        }
    }

    result.is_drivable = true;
    result.drivability_issue = "Route is drivable within vehicle turning radius bounds.";
}