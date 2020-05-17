#include "NXBSDF.h"

void NXBSDF::AddReflectionModel(const shared_ptr<NXReflectionModel>& pReflectionModel)
{
	m_reflectionModels.push_back(pReflectionModel);
}
