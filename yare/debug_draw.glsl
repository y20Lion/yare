~~~~~~~~~~~~~~~~~~~VertexShader ~~~~~~~~~~~~~~~~~~~~~
layout(location = 0) in vec3 position;

layout(location = 0) uniform mat4 mat;


void main()
{
   gl_Position = mat * vec4(position,1.0) ;
}

~~~~~~~~~~~~~~~~~~FragmentShader ~~~~~~~~~~~~~~~~~~~~~~

layout(location = 0) out vec4 shading_result;

void main()
{
   shading_result = vec4(0.0, 1.0, 0.0, 1.0);
}
