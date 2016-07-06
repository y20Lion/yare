#include "TransformHierarchy.h"

#include "matrix_math.h"

namespace yare {

TransformHierarchy::TransformHierarchy(std::vector<TransformHierarchyNode> nodes)
 : _nodes(std::move(nodes))
{
   _nodes_world_to_local_matrices.resize(_nodes.size());
}

void TransformHierarchy::updateNodesWorldToLocalMatrix()
{
   const int root = 0;
   _nodes_world_to_local_matrices[root] = toMat4x3(_nodes[root].local_transform.toMatrix());
   _updateNodeChildren(_nodes_world_to_local_matrices[root], 0);
}

void TransformHierarchy::_updateNodeChildren(const mat4x3 world_to_node_matrix, int node_index)
{
   const TransformHierarchyNode& node = _nodes[node_index];

   // update all children at the same time as they are placed next to each other in memory
   int node_after_last_child = node.first_child + node.children_count;
   
   for (int child_node_index = node.first_child; child_node_index < node_after_last_child; ++child_node_index)
   {
      const TransformHierarchyNode& child_node = _nodes[child_node_index];
      _nodes_world_to_local_matrices[child_node_index] = composeAS(composeAS(world_to_node_matrix, child_node.parent_to_node_matrix), toMat4x3(child_node.local_transform.toMatrix()));
   }

   for (int child_node_index = node.first_child; child_node_index < node_after_last_child; ++child_node_index)
   {
      _updateNodeChildren(_nodes_world_to_local_matrices[child_node_index], child_node_index);
   }
}

}