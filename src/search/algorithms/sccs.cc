#include "sccs.h"

#include <algorithm>

using namespace std;

namespace sccs {

void dfs(
    const vector<vector<int>> &graph,
    int vertex,
    vector<int> &dfs_numbers,
    vector<int> &dfs_minima,
    vector<int> &stack_indices,
    vector<int> &stack,
    int &current_dfs_number,
    vector<vector<int>> &sccs) {
    int vertex_dfs_number = current_dfs_number++;
    dfs_numbers[vertex] = dfs_minima[vertex] = vertex_dfs_number;
    stack_indices[vertex] = stack.size();
    stack.push_back(vertex);

    const vector<int> &successors = graph[vertex];
    for (size_t i = 0; i < successors.size(); i++) {
        int succ = successors[i];
        int succ_dfs_number = dfs_numbers[succ];
        if (succ_dfs_number == -1) {
            dfs(graph, succ, dfs_numbers, dfs_minima, stack_indices, stack, current_dfs_number, sccs);
            dfs_minima[vertex] = min(dfs_minima[vertex], dfs_minima[succ]);
        } else if (succ_dfs_number < vertex_dfs_number && stack_indices[succ] != -1) {
            dfs_minima[vertex] = min(dfs_minima[vertex], succ_dfs_number);
        }
    }

    if (dfs_minima[vertex] == vertex_dfs_number) {
        int stack_index = stack_indices[vertex];
        vector<int> scc;
        for (size_t i = stack_index; i < stack.size(); i++) {
            scc.push_back(stack[i]);
            stack_indices[stack[i]] = -1;
        }
        stack.erase(stack.begin() + stack_index, stack.end());
        sccs.push_back(scc);
    }
}

vector<vector<int>> compute_maximal_sccs(
    const vector<vector<int>> &graph) {
    int node_count = graph.size();
    vector<int> dfs_numbers(node_count, -1);
    vector<int> dfs_minima(node_count, -1);
    vector<int> stack_indices(node_count, -1);
    vector<int> stack;
    stack.reserve(node_count);
    int current_dfs_number = 0;

    vector<vector<int>> sccs;
    for (int i = 0; i < node_count; i++) {
        if (dfs_numbers[i] == -1) {
            dfs(graph, i, dfs_numbers, dfs_minima, stack_indices, stack, current_dfs_number, sccs);
        }
    }

    reverse(sccs.begin(), sccs.end());
    return sccs;
}

template<class Q>
void SCC<Q>::compute_scc_graph() {
    if(!vertex_scc.empty()) return; //Avoid recomputing

    int node_count = graph.size();
    //Compute the SCC associated with each vertex
    vertex_scc.resize(node_count, -1);
    for (size_t i = 0; i < sccs.size(); i++) {
    for (size_t j = 0; j < sccs[i].size(); j++){
        vertex_scc[sccs[i][j]] = i;
    }
    }

    scc_graph.resize(sccs.size());
    //Compute the layer in the CG of each scc and var.
    scc_layer.resize(sccs.size(), -1);
    vertex_layer.resize(node_count, 0);
    //Initially all the sccs are considered to have layer 0 (root)
    for (size_t i = 0; i < sccs.size(); i++) {
    //If no one pointed this scc, it is a root.
    if(scc_layer[i] == -1)
        scc_layer[i] = 0;
    int layer = scc_layer[i];
    for (size_t j = 0; j < sccs[i].size(); j++){
        int var = sccs[i][j];
        vertex_layer[var] = layer; //Assign the layer of the scc to its
        //variables

        //Each element pointed by var and not in scc i, it is a
        //descendant of scc i
        for(size_t k = 0; k < graph[var].size(); k++){
        size_t var2 = graph[var][k];
        size_t scc_var2 = vertex_scc[var2];
        //If this is the first time we point it, we have found a
        //shortest path from the root to scc_var2
        if(scc_var2 != i){
            scc_graph[i].insert(scc_var2);
            if(scc_layer[scc_var2] == -1){
            //cout << var << " => " << var2 << ": " << scc_var2 << " updated to " << layer +1 << endl;
            scc_layer[scc_var2] = layer + 1;
            }
        }
        }
    }
    }
}
}
