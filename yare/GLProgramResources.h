#pragma once

#include <GL/glew.h>
#include <map>

class GLTexture;

class GLProgramResources
{
public:
    GLProgramResources();
    virtual ~GLProgramResources();

    void setTexture(int bind_slot, const GLTexture& texture);
    //void setImage(int bind_slot, const GLTexture& texture, int level);
    //void setUniformBuffer(int bind_slot, );

    void bindResources() const;

private:
    std::map<GLuint, GLuint> _bind_slot_to_texture; 
};