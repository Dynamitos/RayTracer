#pragma once
#include "Model.h"

class ModelLoader
{
public:
	static std::vector<PModel> loadModel(std::string_view filename);
private:
};