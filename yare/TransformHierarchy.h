#pragma once

#include <vector>
#include <glm/mat4x3.hpp>

#include "tools.h"
#include "Transform.h"

namespace yare {

using namespace glm;

struct TransformHierarchyNode
{
   std::string object_name;
   Transform local_transform;
   mat4x3 parent_to_node_matrix;
   int children_count;
   int first_child;
};

class TransformHierarchy
{
public:
   explicit TransformHierarchy(std::vector<TransformHierarchyNode> nodes);

   void updateNodesWorldToLocalMatrix();

   Transform& nodeParentToLocalTransform(int node_index) { return _nodes[node_index].local_transform; }
   const mat4x3& nodeWorldToLocalMatrix(int node_index) const { return _nodes_world_to_local_matrices[node_index]; }

private:
   void _updateNodeChildren(const mat4x3 world_to_node_matrix, int node_index);

private:
   DISALLOW_COPY_AND_ASSIGN(TransformHierarchy)
   std::vector<TransformHierarchyNode> _nodes;
   std::vector<mat4x3> _nodes_world_to_local_matrices;
};

}