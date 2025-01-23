#pragma once
#include "Model.h"
#include <string_view>

class ModelLoader
{
public:
	static std::vector<PModel> loadModel(std::string_view filename);
private:
};