#include "Importer3DY.h"

#include <fstream>

#include <json/json.h>

void import(const std::string& filename)
{
	std::ifstream data_file("D:\\test.bin");
	{
		Json::Value root;
		std::ifstream json_file("D:\\test.json");
		json_file >> root;
		json_file.close();
		const auto& meshes = root["Meshes"];
		for (const auto& mesh : meshes)
		{
			
			auto val = mesh["VertexCount"].asInt();
			auto val2 = mesh["TriangleCount"].asInt();

			const auto& fields = mesh["Fields"];
			for (const auto& field : fields)
			{
				const auto& datablock = field["DataBlock"];
				auto address = datablock["Address"].asUInt64();
				auto size = datablock["Size"].asUInt64();
				data_file.seekg(address, std::ios::beg);
				float* data = new float[size];
				data_file.read((char*)data, sizeof(float) * 16);
				int a = 0;
				delete[] data;
			}
			
		}
	}
	
	
	/*std::ifstream file;
    file.open(filename, std::ios::binary | std::ios::in);
    
    file.read((char*)data, sizeof(float)*16);
    int a = 0;*/
}