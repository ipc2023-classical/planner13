#ifndef ALGORITHMS_SCCS_H
#define ALGORITHMS_SCCS_H

#include <vector>

#include <forward_list>

/*
  This implements Tarjan's linear-time algorithm for finding the maximal
  strongly connected components. It takes time proportional to the sum
  of the number of vertices and arcs.

  Call SCC with a graph represented as a vector of vectors, where
  graph[i] is the vector of successors of vertex i.

  Method compute_scc_equivalence() returns a vector of strongly
  connected components, each of which is a list of vertices (ints).
  This is a partitioning of all vertices where each SCC is a maximal
  subset such that each node in an SCC is reachable from all other
  nodes in the SCC.  Note that the derived graph where each SCC is a
  single "supernode" is necessarily acyclic. The SCCs returned by
  get_result() are in a topological sort order with regard to this
  derived DAG.
*/

#include <vector>
#include <set>
#include <algorithm>
#include "equivalence_relation.h"


namespace sccs {
/*
  This function implements Tarjan's algorithm for finding the strongly
  connected components of a directed graph. The runtime is O(n + m) for a
  directed graph with n vertices and m arcs.

  Input: a directed graph represented as a vector of vectors, where graph[i] is
  the vector of successors of vertex i.

  Output: a vector of strongly connected components, each of which is a vector
  of vertices (ints). This is a partitioning of all vertices where each SCC is
  a maximal subset such that each vertex in an SCC is reachable from all other
  vertexs in the SCC. Note that the derived graph where each SCC is a single
  "supervertex" is necessarily acyclic. The SCCs returned by this function are
  in a topological sort order with regard to this derived DAG.
*/
std::vector<std::vector<int>> compute_maximal_sccs(
    const std::vector<std::vector<int>> &graph);


class EquivalenceRelation;


template<class T> 
static int get_successor (const T & successor) { 
    return successor.get_successor();
}

 template<> inline int get_successor<int> (const int & successor) {
    return successor;
}


//TODO: It could be convinient to use template functions here to allow
//returning the SCC result in different ways (vector<vector<int> >,
//EquivalenceRelation, or even vector <int> state_to_group. However,
//it is not so easy to do so since we need to change the whole
//insertion of sccs into the result
template<class Q>
class SCC {
    template <class T> static void insert_in (std::vector<T> & container, T item) {
	container.push_back(item);
    }

    template <class T> static void insert_in (std::forward_list<T> & container, T item) {
	container.push_front(item);
    }


    template <class T>
	static void dfs_equivalence(const std::vector<std::vector<Q> > &graph, 
				    int vertex, int & current_dfs_number,
				    std::vector<int> & stack,
				    std::vector<int> & stack_indices,
				    std::vector<int> & dfs_numbers,
				    std::vector<int> & dfs_minima,
				    std::vector<T > & final_sccs, 
				    std::vector<bool> * is_goal){
	int vertex_dfs_number = current_dfs_number++;
	dfs_numbers[vertex] = dfs_minima[vertex] = vertex_dfs_number;
	stack_indices[vertex] = stack.size();
	stack.push_back(vertex);

	const std::vector<Q> &successors = graph[vertex];
	for (size_t i = 0; i < successors.size(); i++) {
	    int succ = get_successor (successors[i]);
	    int succ_dfs_number = dfs_numbers[succ];
	    if (succ_dfs_number == -1) {
		dfs_equivalence(graph, succ, current_dfs_number, stack, stack_indices,
				dfs_numbers, dfs_minima, final_sccs, is_goal);
		dfs_minima[vertex] = std::min(dfs_minima[vertex], dfs_minima[succ]);
	    } else if (succ_dfs_number < vertex_dfs_number && stack_indices[succ] != -1) {
		dfs_minima[vertex] = std::min(dfs_minima[vertex], succ_dfs_number);
	    }
	    if(is_goal && (*is_goal)[succ]){
		(*is_goal)[vertex] = true;
	    }
	}

	if (dfs_minima[vertex] == vertex_dfs_number) {
	    int stack_index = stack_indices[vertex];
	    
	    final_sccs.push_back(T());
	    T & scc = final_sccs.back();
	    for (size_t i = stack_index; i < stack.size(); i++) {
		insert_in(scc, stack[i]);
		stack_indices[stack[i]] = -1;
	    }
	    stack.erase(stack.begin() + stack_index, stack.end());
	}
    }


    // Input graph. TODO: Change type? 
    // Should we have a LabelledTransitionClass for those cases?
    // TODO: Using a const reference is not completely safe. 
    // A shared_ptr could be more appropiate. 
    const std::vector<std::vector<Q> > &graph;
    
    // Output relative at the SCCs and their connection
    std::vector<std::vector<int> > sccs;


    // Additional output that we might want (TODO: move to somewhere
    // else?)
    std::vector <int> vertex_scc; // SCC index for each vertex
    std::vector <int> scc_layer; // Layer of each SCC
    std::vector <int> vertex_layer; // LayerSCC of each vertex
    std::vector <std::set<int> > scc_graph;

    //Auxiliar functions to compute additional output on demand
    void compute_scc_graph();
    

public:
    //Allows to access scc equivalence computation without creating a new class
    //TODO: is_goal is an optional parameter (use std::optional?)
    template <class T>
        static void compute_scc_equivalence(const std::vector<std::vector<Q> > &graph, 
					    std::vector<T> & result, 
					    std::vector<bool> *is_goal = NULL){
    int node_count = graph.size();

    // The following three are indexed by vertex number.
    std::vector<int> dfs_numbers(node_count, -1);
    std::vector<int> dfs_minima (node_count, -1);
    std::vector<int> stack_indices (node_count, -1);
    std::vector<int> stack; // This is indexed by the level of recursion.
    stack.reserve(node_count);
    int current_dfs_number = 0;

    for (int i = 0; i < node_count; i++)
	if (dfs_numbers[i] == -1)
	    dfs_equivalence(graph, i, current_dfs_number, stack, stack_indices, 
			    dfs_numbers, dfs_minima, result, is_goal);

    std::reverse(result.begin(), result.end());
    }

    SCC(const std::vector<std::vector<Q > > & _graph) : graph(_graph){
        compute_scc_equivalence(graph, sccs);
    }

    const std::vector<std::vector<int> > & get_sccs() const{
      return sccs;
    }

    const std::vector <int> & get_vertex_scc() {
      if(vertex_scc.empty()) compute_scc_graph();
      return vertex_scc;
    }

    const std::vector <int> & get_scc_layer() {
      if(vertex_scc.empty()) compute_scc_graph();
      return scc_layer;
    }

    const std::vector <int> & get_vertex_layer() {
      if(vertex_scc.empty()) compute_scc_graph();
      return vertex_layer;
    }

    const std::vector <std::set<int> > & get_scc_graph() {
      if(vertex_scc.empty()) compute_scc_graph();
      return scc_graph;
    }
};
}
#endif
