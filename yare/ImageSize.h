#pragma once

namespace yare {

struct ImageSize
{
   ImageSize() : width(0), height(0) {}
   ImageSize(int width, int height) : width(width), height(height) {}
   ImageSize(const ImageSize& other) : width(other.width), height(other.height) {};

   int width;
   int height;
};

}
