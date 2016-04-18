#pragma once

class StandardMaterial
{
public:
    glm::vec3 diffuse_colour;
    GLTexture2DSptr diffuse_map;
    glm::mat3x2 diffuse_map_transform;
    
    float reflection_roughness;
    glm::vec3 reflection_colour;
    GLTexture2DSptr reflection_map;
    glm::mat3x2 reflection_map_transform;

    GLTexture2DSptr normal_map;
    glm::mat3x2 normal_map_transform;
    GLTexture2DSptr normal_scale;
};