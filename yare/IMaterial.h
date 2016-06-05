#pragma once

namespace yare {

class GLVertexSource;
struct RenderResources;

class IMaterial
{
public:

   virtual ~IMaterial() {}
      
   virtual void render(const RenderResources& resources, const GLVertexSource& mesh_source) = 0;
   virtual int requiredMeshFields() = 0;
};

}