#include "graph.hpp"
#include <stdexcept>

void LaneNetwork::add_node(const Node& node) {
    nodes_[node.id] = node;
    if (!node.name.empty()) {
        named_locations_[node.name] = node.id;
    }
}

void LaneNetwork::add_edge(const LaneEdge& edge) {
    adj_list_[edge.from_node].push_back(edge);
}

void LaneNetwork::register_named_location(const std::string& name, int node_id) {
    if (nodes_.find(node_id) != nodes_.end()) {
        named_locations_[name] = node_id;
        nodes_[node_id].name = name;
    }
}

const Node* LaneNetwork::get_node(int id) const {
    auto it = nodes_.find(id);
    if (it != nodes_.end()) {
        return &(it->second);
    }
    return nullptr;
}

const Node* LaneNetwork::get_node_by_name(const std::string& name) const {
    auto it = named_locations_.find(name);
    if (it != named_locations_.end()) {
        return get_node(it->second);
    }
    return nullptr;
}

const std::vector<LaneEdge>& LaneNetwork::get_outgoing_edges(int node_id) const {
    static const std::vector<LaneEdge> empty_edges;
    auto it = adj_list_.find(node_id);
    if (it != adj_list_.end()) {
        return it->second;
    }
    return empty_edges;
}

int LaneNetwork::get_deterministic_node_id(const std::string& target_name) const {
    int best_id = -1;
    for (const auto& [id, node] : nodes_) {
        if (node.name == target_name) {
            if (best_id == -1 || id < best_id) {
                best_id = id;
            }
        }
    }
    return best_id;
}