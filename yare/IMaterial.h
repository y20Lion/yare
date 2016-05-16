#pragma once

namespace yare {

class GLVertexSource;

class IMaterial
{
public:

   virtual ~IMaterial() {}
      
   virtual void render(const GLVertexSource& mesh_source) = 0;
   virtual int requiredMeshFields() = 0;
};

}