#pragma once
#include "NXReflectionModel.h"

class NXBSDF
{
public:
	NXBSDF() {}
	~NXBSDF() {}

	void AddReflectionModel(const shared_ptr<NXReflectionModel>& pReflectionModel);

private:
	vector<shared_ptr<NXReflectionModel>> m_reflectionModels;
};
