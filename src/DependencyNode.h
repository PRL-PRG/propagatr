#ifndef PROMISEDYNTRACER_DEPENDENCY_NODE_H
#define PROMISEDYNTRACER_DEPENDENCY_NODE_H

class DependencyNode {

  public:
  explicit DependencyNode(const function_id_t & fn_id, int param_pos) :
    fn_id_(fn_id), param_pos_(param_pos) {}

  const function_id_t & get_function_id() const {
    return fn_id_;
  }

  int get_formal_parameter_position() const {
    return param_pos_;
  }

  bool operator==(const DependencyNode & node) const {
    return get_function_id() == node.get_function_id() &&
           get_formal_parameter_position() == node.get_formal_parameter_position();
  }

  bool operator!=(const DependencyNode & node) const {
    return ! operator==(node);
  }

  bool operator<(const DependencyNode & node) const {
    return get_function_id() < node.get_function_id() ||
           get_formal_parameter_position() < node.get_formal_parameter_position();
  }

  private:
    function_id_t fn_id_;
    int param_pos_;

};

struct DependencyNodeHasher
{
 std::size_t operator()(const DependencyNode& k) const
 {
   return (std::hash<std::string>()(k.get_function_id())
             ^ (std::hash<int>()(k.get_formal_parameter_position()) << 1));
 }
};

#endif
