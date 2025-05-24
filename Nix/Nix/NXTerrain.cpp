#include "NXTerrain.h"
#include "NXGlobalDefinitions.h"
#include "NXScene.h"
#include "NXAllocatorManager.h"
#include "NXCamera.h"
#include "NXTimer.h"

NXTerrain::NXTerrain(int gridSize, int worldSize) :
	m_gridSize(gridSize),
	m_worldSize(worldSize),
	m_heightRange(0, 1000)
{
}

void NXTerrain::AddSubMesh(NXSubMeshBase* pSubMesh)
{
	m_pSubMesh.reset(pSubMesh);
}


bool NXTerrain::RayCastPrimitive(const Ray& worldRay, NXHit& outHitInfo, float& outDist)
{
	NXHit hit;
	hit.pSubMesh = m_pSubMesh.get();
	outDist = 0.0f;
	outHitInfo = hit;
	return true;
}

void NXTerrain::InitAABB()
{
	Vector3 vMin(0.0f, m_heightRange.x, 0.0f);
	Vector3 vMax((float)m_worldSize, m_heightRange.y, (float)m_worldSize);
	m_localAABB = AABB(vMin, vMax);

	NXRenderableObject::InitAABB();
}

void NXTerrain::Update(ID3D12GraphicsCommandList* pCmdList)
{
	if (!m_bGenerated)
		std::runtime_error("NXTerrain has not been Generated(). Terrain should be call this method once always.");

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
