#include "graph.hpp"
#include <stdexcept>

void LaneNetwork::add_node(const Node& node) {

    // Node insertion, O(Log N)
    nodes_[node.id] = node;

    // This overrites the previous duplicate entry, hence not a good thing to trust on
    if (!node.name.empty()) {
        named_locations_[node.name] = node.id;
    }
}

void LaneNetwork::add_edge(const LaneEdge& edge) {
    // Add the edge to the vector corresponding to the node ID
    adj_list_[edge.from_node].push_back(edge);
}

void LaneNetwork::register_named_location(const std::string& name, int node_id) {
    // First verify if the named location exists or not
    if (nodes_.find(node_id) != nodes_.end()) {
        named_locations_[name] = node_id;
        nodes_[node_id].name = name;
    }
}


const Node* LaneNetwork::get_node(int id) const {
    // Just query the map and get the node, and return the pointer to the node
    auto it = nodes_.find(id);
    if (it != nodes_.end()) {
        return &(it->second);
    }

    // Return nothing is we get nothing, simple
    return nullptr;
}

const Node* LaneNetwork::get_node_by_name(const std::string& name) const {
    // Same logic as the function above, the key is now the name instead of the ID number
    auto it = named_locations_.find(name);
    if (it != named_locations_.end()) {
        return get_node(it->second);
    }
    return nullptr;
}

const std::vector<LaneEdge>& LaneNetwork::get_outgoing_edges(int node_id) const {
    // Make it read only, and store it in the data segment so that it does not go out of scope when we want to use it
    static const std::vector<LaneEdge> empty_edges;
    auto it = adj_list_.find(node_id);
    if (it != adj_list_.end()) {
        return it->second;
    }
    return empty_edges;
}

int LaneNetwork::get_deterministic_node_id(const std::string& target_name) const {
    int best_id = -1;

    // Linear search to guarentee determinism, similar performance to O(N) in the scale of N we are operating in
    for (const auto& [id, node] : nodes_) {
        if (node.name == target_name) {
            if (best_id == -1 || id < best_id) {
                best_id = id;
            }
        }
    }
    return best_id;
}