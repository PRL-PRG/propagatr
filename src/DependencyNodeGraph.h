#ifndef PROMISEDYNTRACER_DEPENDENCY_NODE_GRAPH_H
#define PROMISEDYNTRACER_DEPENDENCY_NODE_GRAPH_H

#include "DependencyNode.h"

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

  // param_pos is -1 for return values
  void add_return(SEXP value, function_id_t fn_id) {
    // lmao
    add_argument(value, fn_id, -1);
  }

  // for things that get gcd
  void remove_value(SEXP value) {
    // dependencies_ will still have the dependencies tracked
    arguments_.erase(value);
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
