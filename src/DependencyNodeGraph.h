#ifndef PROMISEDYNTRACER_DEPENDENCY_NODE_GRAPH_H
#define PROMISEDYNTRACER_DEPENDENCY_NODE_GRAPH_H

#include "DependencyNode.h"

#include <sstream> // for serializing
#include <string>  // for serializing

class DependencyNodeGraph {

public:
  explicit DependencyNodeGraph() {}

  void add_argument(SEXP value, function_id_t fn_id, int param_pos) {
    /*
        some things that we need to check:
        - is the argument already in the table? then add edges w.r.t. all other positiongs
        - if it's a return value, want to add a dependency cause it can become an argument
          to something else
    */

    auto iter = arguments_.find(value);
    DependencyNode new_node(fn_id, param_pos);

    if (iter != arguments_.end()) {
      // it was present

      for (const DependencyNode & node : iter->second) {
        add_dependency_(new_node, node);
      }

      // handles adding to arguments_ list
      iter->second.insert(new_node);
    } else {
      // not present, add for first time
      arguments_.insert({value, {new_node}});
    }

    // in case the argument was the return of some other function, figure that out
    // it's dealt with actually
  }

  // for tracking traces
  void add_argument(SEXP value, function_id_t fn_id, int param_pos, std::size_t trace_hash) {
    /*
        some things that we need to check:
        - is the argument already in the table? then add edges w.r.t. all other positiongs
        - if it's a return value, want to add a dependency cause it can become an argument
          to something else
    */

    auto iter = arguments_.find(value);
    DependencyNode new_node(fn_id, param_pos, trace_hash);

    if (iter != arguments_.end()) {
      // it was present

      for (const DependencyNode & node : iter->second) {
        add_dependency_(new_node, node);
      }

      // handles adding to arguments_ list
      iter->second.insert(new_node);
    } else {
      // not present, add for first time
      arguments_.insert({value, {new_node}});
    }

    // in case the argument was the return of some other function, figure that out
    // it's dealt with actually
  }

  // param_pos is -1 for return values
  void add_return(SEXP value, function_id_t fn_id) {
    // lmao
    add_argument(value, fn_id, -1);
  }

  // for tracking return types
  void add_return(SEXP value, function_id_t fn_id, std::size_t trace_hash) {
    // lmao
    add_argument(value, fn_id, -1, trace_hash);
  }

  // for things that get gcd
  void remove_value(SEXP value) {
    // dependencies_ will still have the dependencies tracked
    arguments_.erase(value);
  }

  std::stringstream serialize() {
    
    int size_of_dependencies_as_int = dependencies_.size();
    std::stringstream out;
    
    std::stringstream add_me, add_me_too;

    // for each node 
    // (fn_id, p_pos) : (fn_id, p_pos) - (fn_id, p_pos) - ... - (fn_id, p_pos)
    for ( auto iter = dependencies_.begin(); iter != dependencies_.end(); ++iter) {
      // serialize the node
      
      add_me.str(std::string());

      if (iter->first.get_trace_hash() != 0) {
        add_me << "," << iter->first.get_trace_hash();
      }

      add_me << " : ";

      out << iter->first.get_function_id() << "," << iter->first.get_formal_parameter_position() << add_me.rdbuf();

      // TODO serialize the edges
      for ( auto edge_iter = iter->second.begin(); edge_iter != iter->second.end(); /* ++edge_iter */) {

        add_me_too.str(std::string());

        add_me_too << edge_iter->get_formal_parameter_position();

        if (edge_iter->get_trace_hash() != 0) {
          add_me_too << "," << edge_iter->get_trace_hash();
        }

        out << edge_iter->get_function_id() << "," << add_me_too.rdbuf();
        if (++edge_iter != iter->second.end()) {
          out << " - ";
        }
      }

      out << "\n";
    }

    return out;

  }

private:
  std::unordered_map<SEXP, std::set<DependencyNode>> arguments_;
  std::unordered_map<DependencyNode, std::set<DependencyNode>, DependencyNodeHasher> dependencies_;

  void add_dependency_(DependencyNode key, DependencyNode value) {
    auto iter = dependencies_.find(key);

    if (iter == dependencies_.end()) {
      // first time
      dependencies_.insert({key, {value}});
    } else {
      iter->second.insert(value);
    }
  }

};

#endif
