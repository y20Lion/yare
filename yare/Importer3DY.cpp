#include "Importer3DY.h"

#include <fstream>

void import(const std::string& filename)
{
    std::ifstream file;
    file.open(filename, std::ios::binary | std::ios::in);
    float data[16];
    file.read((char*)data, sizeof(float)*16);
    int a = 0;
}