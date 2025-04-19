#include "NXTerrain.h"
#include "NXGlobalDefinitions.h"
#include "NXScene.h"
#include "NXAllocatorManager.h"
#include "NXCamera.h"
#include "NXTimer.h"

NXTerrain::NXTerrain(int rawSize, int gridSize, int worldSize, const std::filesystem::path& rawFile) :
	m_rawSize(rawSize),
	m_gridSize(gridSize),
	m_worldSize(worldSize),
	m_rawPath(rawFile),
	m_rawData(rawSize * rawSize) 
{
}

void NXTerrain::AddSubMesh(NXSubMeshBase* pSubMesh)
{
	m_pSubMesh = std::shared_ptr<NXSubMeshBase>(pSubMesh);
}

void NXTerrain::InitAABB()
{
	m_pSubMesh->CalcLocalAABB();
	AABB::CreateMerged(m_localAABB, m_localAABB, m_pSubMesh->GetLocalAABB());

	NXRenderableObject::InitAABB();
}

void NXTerrain::Update(ID3D12GraphicsCommandList* pCmdList)
{
	auto* pCamera = NXResourceManager::GetInstance()->GetCameraManager()->GetCamera("Main Camera");
	auto& mxView = pCamera->GetViewMatrix();
	auto& mxWorld = m_worldMatrix;
	auto& mxWorldView = mxWorld * mxView;

	m_cbDataObject.world = mxWorld.Transpose();
	m_cbDataObject.worldInverseTranspose = mxWorld.Invert(); // it actually = m_worldMatrix.Invert().Transpose().Transpose();
	m_cbDataObject.worldView = mxWorldView.Transpose();
	m_cbDataObject.worldViewInverseTranspose = (mxWorldView).Invert();
	m_cbDataObject.globalData.time = NXGlobalApp::Timer->GetGlobalTimeSeconds();

	m_cbObject.Update(m_cbDataObject);

	pCmdList->SetGraphicsRootConstantBufferView(0, m_cbObject.CurrentGPUAddress());
}
