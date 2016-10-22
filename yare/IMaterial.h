#pragma once

namespace yare {

class GLProgram;
struct RenderResources;

enum class MaterialVariant : int {Normal = 1 << 0, WithSkinning = 1 << 1, EnableAOVolume = 1 << 2, EnableSDFVolume = 1 << 3};

class IMaterial
{
public:

   virtual ~IMaterial() {}
   
   virtual int requiredMeshFields(MaterialVariant material_variant) = 0;
   virtual const GLProgram& compile(MaterialVariant material_variant) = 0;
   virtual void bindTextures() = 0;
   virtual bool isTransparent() = 0;
   virtual bool hasTessellation() = 0;
   
};

}