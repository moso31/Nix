#include "FBXMeshLoader.h"
#include "NXPrimitive.h"
#include "NXPrefab.h"
#include "NXScene.h"
#include "NXSubMeshGeometryEditor.h"

#ifdef IOS_REF
	#undef  IOS_REF
	#define IOS_REF (*(pManager->GetIOSettings()))
#endif

void FBXMeshLoader::LoadFBXFile(std::string filepath, NXPrefab* pOutPrefab, bool bAutoCalcTangents)
{
	FbxManager* lSdkManager = NULL;
	FbxScene* lScene = NULL;
	bool lResult = false;

	// Prepare the FBX SDK.
	InitializeSdkObjects(lSdkManager, lScene);
	// Load the scene.

	// The example can take a FBX file as an argument.
	FbxString lFilePath(filepath.data());

	if (lFilePath.IsEmpty())
		return;

	lResult = LoadScene(lSdkManager, lScene, lFilePath.Buffer());
	if (!lResult)
		return;

	FbxNode* lNode = lScene->GetRootNode();
	if (lNode)
	{
		int lChildCount = lNode->GetChildCount();
		for (int i = 0; i < lChildCount; i++)
		{
			FbxNode* pChildNode = lNode->GetChild(i);
			LoadRenderableObjects(pChildNode, pOutPrefab, bAutoCalcTangents);
		}
	}

	lNode->Destroy(true);
	lScene->Destroy();
	lSdkManager->Destroy();
}

void FBXMeshLoader::LoadRenderableObjects(FbxNode* pNode, NXRenderableObject* pParentMesh, bool bAutoCalcTangents)
{
	NXRenderableObject* pRenderableObject = nullptr;
	if (pNode->GetNodeAttribute() == NULL)
	{
	}
	else
	{
		FbxNodeAttribute::EType nodeType = pNode->GetNodeAttribute()->GetAttributeType();
		switch (nodeType)
		{
		case FbxNodeAttribute::eMesh:
		{
			pRenderableObject = new NXPrimitive();
			pRenderableObject->SetParent(pParentMesh);

			// 如果 Mesh 的 scale 存在奇数个(1个 或 3个)负值，则需要反转polygon，否则生成的Mesh会朝向相反的方向。 
			FbxVector4 lTmpVector = pNode->EvaluateLocalScaling() * pNode->GetGeometricScaling(FbxNode::eSourcePivot);
			bool bFlipPolygon = ((int)(lTmpVector[0] < 0.0f) + (int)(lTmpVector[1] < 0.0f) + (int)(lTmpVector[2] < 0.0f)) % 2;

			EncodePrimitiveData(pNode, (NXPrimitive*)pRenderableObject, bAutoCalcTangents, bFlipPolygon);
			break;
		}
		default:
		{
			pRenderableObject = new NXPrefab();
			pRenderableObject->SetParent(pParentMesh);
			break;
		}
		}
	}

	if (pRenderableObject)
	{
		pRenderableObject->SetName((char*)pNode->GetName());

		EFbxRotationOrder lRotationOrder;
		pNode->GetRotationOrder(FbxNode::eSourcePivot, lRotationOrder);

		switch (lRotationOrder)
		{
		case eEulerXYZ:
			FBXSDK_printf("Euler XYZ\n");
			break;
		case eEulerXZY:
			FBXSDK_printf("Euler XZY\n");
			break;
		case eEulerYZX:
			FBXSDK_printf("Euler YZX\n");
			break;
		case eEulerYXZ:
			FBXSDK_printf("Euler YXZ\n");
			break;
		case eEulerZXY:
			FBXSDK_printf("Euler ZXY\n");
			break;
		case eEulerZYX:
			FBXSDK_printf("Euler ZYX\n");
			break;
		case eSphericXYZ:
			FBXSDK_printf("Spheric XYZ\n");
			break;
		default:
			FBXSDK_printf("NONE\n");
			break;
		}

		SetGeometricTransform(pNode, pRenderableObject);
	}

	int lChildCount = pNode->GetChildCount();
	for (int i = 0; i < lChildCount; i++)
	{
		FbxNode* pChildNode = pNode->GetChild(i);
		LoadRenderableObjects(pChildNode, pRenderableObject, bAutoCalcTangents);
	}
}

void FBXMeshLoader::SetGeometricTransform(FbxNode* pNode, NXRenderableObject* pRenderableObject)
{
	FbxVector4 lTmpVector;
	pNode->SetRotationOrder(FbxNode::eDestinationPivot, FbxEuler::eOrderZXY);

	lTmpVector = pNode->EvaluateLocalTranslation();
	Vector3 translation((float)lTmpVector[0], (float)lTmpVector[2], (float)lTmpVector[1]);

	lTmpVector = pNode->EvaluateLocalRotation();
	Vector3 rotation((float)lTmpVector[0], (float)lTmpVector[2], (float)lTmpVector[1]);
	rotation *= XM_PI / 180.0f;
	Matrix m = Matrix::CreateFromXZY(-rotation);
	//pRenderableObject->SetRotationMatrix(m);
	rotation = m.EulerRollPitchYaw();
	rotation = -rotation;
	
	lTmpVector = pNode->EvaluateLocalScaling();
	Vector3 scale((float)lTmpVector[0], (float)lTmpVector[2], (float)lTmpVector[1]);

	pRenderableObject->SetTranslation(translation);
	pRenderableObject->SetRotation(rotation);
	pRenderableObject->SetScale(scale);

	lTmpVector = pNode->GetGeometricTranslation(FbxNode::eSourcePivot);
	translation = Vector3((float)lTmpVector[0], (float)lTmpVector[2], (float)lTmpVector[1]);

	lTmpVector = pNode->GetGeometricRotation(FbxNode::eSourcePivot);
	rotation = Vector3((float)lTmpVector[0], (float)lTmpVector[2], (float)lTmpVector[1]);
	rotation *= XM_PI / 180.0f;
	m = Matrix::CreateFromXZY(-rotation);
	rotation = m.EulerRollPitchYaw();
	rotation = -rotation;

	lTmpVector = pNode->GetGeometricScaling(FbxNode::eSourcePivot);
	scale = Vector3((float)lTmpVector[0], (float)lTmpVector[2], (float)lTmpVector[1]);

	printf("        geoTranslation: %f %f %f\n", translation.x, translation.y, translation.z);
	printf("        geoRotation: %f %f %f\n", rotation.x, rotation.y, rotation.z);
	printf("        geoScale: %f %f %f\n", scale.x, scale.y, scale.z);
	
	// geoMatrix = geoS * geoR * geoT
	pRenderableObject->SetGeoTransform(
		Matrix::CreateScale(scale) *
		Matrix::CreateFromZXY(rotation) *
		Matrix::CreateTranslation(translation));
}

void FBXMeshLoader::EncodePrimitiveData(FbxNode* pNode, NXPrimitive* pPrimitive, bool bAutoCalcTangents, bool bFlipPolygon)
{
	FbxMesh* pMesh = (FbxMesh*)pNode->GetNodeAttribute();
	FbxVector4* pControlPoints = pMesh->GetControlPoints();
	if (pMesh)
	{
		int vertexId = 0;
		int materialCount = -1;

		// 2022.4.23
		// 需要先确认有几个SubMesh，然后分两种情况：
		// 1. 当前 Primitive 有 1 个 SubMesh
		// 2. 当前 Primitive 有 n 个 SubMesh
		// 情况 1. 使用专门方案加载，可以提高一些速度（国内建模班出身的可能更多会使用情况 1.）
		// 情况 2. Unity Ball 样例使用了这种方法 可能出 Demo 比较方便。

		// 确定 SubMesh 个数
		FbxNode* pNode = pMesh->GetNode();
		if (pNode) materialCount = max(pNode->GetMaterialCount(), 1);	// 有时候3ds中没有指定材质，这种情况下填充一个默认submesh（至少保证有一个submesh）。

		static int noNameMaterialIdx = 0;
		std::vector<NXSubMeshBase*> pSubMeshes;
		pSubMeshes.reserve(materialCount);
		for (int i = 0; i < materialCount; i++)
		{
			auto fbxMaterial = pNode->GetMaterial(i);

			std::string subMeshName;
			if (fbxMaterial)
			{
				subMeshName = fbxMaterial->GetName() + '_' + std::to_string(i);
			}
			else
			{
				subMeshName = "Material_" + std::to_string(noNameMaterialIdx) + "_" + std::to_string(i);
				noNameMaterialIdx++;
			}
			pSubMeshes.push_back(new NXSubMeshStandard(pPrimitive, subMeshName));
		}

		if (materialCount == 1)
		{
			int polygonCount = pMesh->GetPolygonCount();
			for (int polygonIndex = 0; polygonIndex < polygonCount; polygonIndex++)
			{
				EncodePolygonData(pMesh, pSubMeshes[0], polygonIndex, vertexId, bFlipPolygon);
			}
		}
		else
		{

			int polygonCount = pMesh->GetPolygonCount();
			for (int polygonIndex = 0; polygonIndex < polygonCount; polygonIndex++)
			{
				for (int elementMaterialIndex = 0; elementMaterialIndex < pMesh->GetElementMaterialCount(); elementMaterialIndex++)
				{
					// 拿材质id
					FbxGeometryElementMaterial* lMaterialElement = pMesh->GetElementMaterial(elementMaterialIndex);
					int lMatId = lMaterialElement->GetIndexArray().GetAt(polygonIndex);

					// 2022.4.23
					// 这里还能拿到材质的具体纹理信息，不过现在暂时还用不上
					// 将来如果想要对 fbx材质 做导入的话 可以考虑用下面的部分
					//FbxSurfaceMaterial* lMaterial = NULL;
					//lMaterial = pMesh->GetNode()->GetMaterial(lMaterialElement->GetIndexArray().GetAt(polygonIndex));
					//if (lMatId >= 0)
					//{
					//	DisplayMaterialTextureConnections(lMaterial, header, lMatId, elementMaterialIndex);
					//}

					EncodePolygonData(pMesh, pSubMeshes[lMatId], polygonIndex, vertexId, bFlipPolygon);
				}
			}
		}

		// 将 submesh 录入到 primitive
		pPrimitive->ResizeSubMesh(materialCount);
		for (int i = 0; i < materialCount; i++)
			pPrimitive->ReloadSubMesh(i, pSubMeshes[i]);

		// 重新计算切线
		if (bAutoCalcTangents) pPrimitive->CalculateTangents();

		// 按 SubMesh 生成 VB/IB
		UINT subMeshCount = pPrimitive->GetSubMeshCount();
		for (UINT i = 0; i < subMeshCount; i++)
		{
			NXSubMeshBase* pSubMesh = pPrimitive->GetSubMesh(i);
			pSubMesh->TryAddBuffers();
		}
	}
}

void FBXMeshLoader::EncodePolygonData(FbxMesh* pMesh, NXSubMeshBase* pSubMesh, int polygonIndex, int& vertexId, bool bFlipPolygon)
{
	NXSubMeshStandard* pSubMeshFBX = dynamic_cast<NXSubMeshStandard*>(pSubMesh);
	if (!pSubMeshFBX)
		return;

	int polygonSize = pMesh->GetPolygonSize(polygonIndex);

	std::vector<VertexPNTT> vertexDataArray;
	std::vector<UINT> indexDataArray;

	vertexDataArray.reserve(polygonSize);
	indexDataArray.reserve(polygonSize);

	FbxVector4* controlPoints = pMesh->GetControlPoints();
	for (int polygonVertexIndex = 0; polygonVertexIndex < polygonSize; polygonVertexIndex++)
	{
		int controlPointIndex = pMesh->GetPolygonVertex(polygonIndex, polygonVertexIndex);

		VertexPNTT vertexData;
		FBXMeshVertexData fbxMeshVertexData;

		EncodeVertexPosition(fbxMeshVertexData, pMesh, controlPoints, controlPointIndex);
		EncodeVertexColors(fbxMeshVertexData, pMesh, controlPointIndex, vertexId);
		EncodeVertexUVs(fbxMeshVertexData, pMesh, polygonIndex, polygonVertexIndex, controlPointIndex, vertexId);
		EncodeVertexNormals(fbxMeshVertexData, pMesh, vertexId);
		EncodeVertexTangents(fbxMeshVertexData, pMesh, vertexId);
		EncodeVertexBiTangents(fbxMeshVertexData, pMesh, vertexId);

		ConvertVertexFormat(fbxMeshVertexData, vertexData);
		vertexDataArray.push_back(vertexData);
		indexDataArray.push_back((UINT)controlPointIndex);

		vertexId++;
	}

	// 将多边形拆分成若干三角形
	// polygonSize = vertexDataArray.size()
	for (int i = 1; i < polygonSize - 1; i++)
	{
		pSubMeshFBX->m_vertices.push_back(vertexDataArray[0]);
		pSubMeshFBX->m_vertices.push_back(vertexDataArray[i]);
		pSubMeshFBX->m_vertices.push_back(vertexDataArray[i + 1]);

		UINT lastIndex = (UINT)pSubMeshFBX->m_indices.size();
		pSubMeshFBX->m_indices.push_back(lastIndex);
		if (bFlipPolygon)
		{
			pSubMeshFBX->m_indices.push_back(lastIndex + 1);
			pSubMeshFBX->m_indices.push_back(lastIndex + 2);
		}
		else
		{
			pSubMeshFBX->m_indices.push_back(lastIndex + 2);
			pSubMeshFBX->m_indices.push_back(lastIndex + 1);
		}
	}
}

void FBXMeshLoader::EncodeVertexPosition(FBXMeshVertexData& inoutVertexData, FbxMesh* pMesh, FbxVector4* pControlPoints, int controlPointIndex)
{
	FbxVector4 position = pControlPoints[controlPointIndex];
	inoutVertexData.Position = Vector3((float)position[0], (float)position[1], (float)position[2]);
}

void FBXMeshLoader::EncodeVertexColors(FBXMeshVertexData& inoutVertexData, FbxMesh* pMesh, int controlPointIndex, int vertexId)
{
	int elementVertexColorCount = pMesh->GetElementVertexColorCount();
	inoutVertexData.VertexColors.resize(elementVertexColorCount);

	for (int l = 0; l < elementVertexColorCount; l++)
	{
		FbxColor color;
		FbxGeometryElementVertexColor* leVtxc = pMesh->GetElementVertexColor(l);

		switch (leVtxc->GetMappingMode())
		{
		default:
			break;
		case FbxGeometryElement::eByControlPoint:
			switch (leVtxc->GetReferenceMode())
			{
			case FbxGeometryElement::eDirect:
				color = leVtxc->GetDirectArray().GetAt(controlPointIndex);
				break;
			case FbxGeometryElement::eIndexToDirect:
			{
				int id = leVtxc->GetIndexArray().GetAt(controlPointIndex);
				color = leVtxc->GetDirectArray().GetAt(id);
			}
			break;
			default:
				break; // other reference modes not shown here!
			}
			break;

		case FbxGeometryElement::eByPolygonVertex:
		{
			switch (leVtxc->GetReferenceMode())
			{
			case FbxGeometryElement::eDirect:
				color = leVtxc->GetDirectArray().GetAt(vertexId);
				break;
			case FbxGeometryElement::eIndexToDirect:
			{
				int id = leVtxc->GetIndexArray().GetAt(vertexId);
				color = leVtxc->GetDirectArray().GetAt(id);
			}
			break;
			default:
				break; // other reference modes not shown here!
			}
		}
		break;

		case FbxGeometryElement::eByPolygon: // doesn't make much sense for UVs
		case FbxGeometryElement::eAllSame:   // doesn't make much sense for UVs
		case FbxGeometryElement::eNone:       // doesn't make much sense for UVs
			break;
		}

		inoutVertexData.VertexColors[l] = Vector4((float)color.mRed, (float)color.mGreen, (float)color.mBlue, (float)color.mAlpha);
	}
}

void FBXMeshLoader::EncodeVertexUVs(FBXMeshVertexData& inoutVertexData, FbxMesh* pMesh, int polygonIndex, int polygonVertexIndex, int controlPointIndex, int vertexId)
{
	int elementUVCount = pMesh->GetElementUVCount();
	inoutVertexData.UVs.resize(elementUVCount);

	for (int l = 0; l < elementUVCount; ++l)
	{
		FbxGeometryElementUV* leUV = pMesh->GetElementUV(l);
		FbxVector2 uv;

		switch (leUV->GetMappingMode())
		{
		default:
			break;
		case FbxGeometryElement::eByControlPoint:
			switch (leUV->GetReferenceMode())
			{
			case FbxGeometryElement::eDirect:
				uv = leUV->GetDirectArray().GetAt(controlPointIndex);
				break;
			case FbxGeometryElement::eIndexToDirect:
			{
				int id = leUV->GetIndexArray().GetAt(controlPointIndex);
				uv = leUV->GetDirectArray().GetAt(id);
			}
			break;
			default:
				break; // other reference modes not shown here!
			}
			break;

		case FbxGeometryElement::eByPolygonVertex:
		{
			int lTextureUVIndex = pMesh->GetTextureUVIndex(polygonIndex, polygonVertexIndex);
			switch (leUV->GetReferenceMode())
			{
			case FbxGeometryElement::eDirect:
			case FbxGeometryElement::eIndexToDirect:
			{
				uv = leUV->GetDirectArray().GetAt(lTextureUVIndex);
			}
			break;
			default:
				break; // other reference modes not shown here!
			}
		}
		break;

		case FbxGeometryElement::eByPolygon: // doesn't make much sense for UVs
		case FbxGeometryElement::eAllSame:   // doesn't make much sense for UVs
		case FbxGeometryElement::eNone:       // doesn't make much sense for UVs
			break;
		}

		// uv flip Y axis(for directX)
		inoutVertexData.UVs[l] = Vector2((float)uv[0], 1.0f - (float)uv[1]);
	}
}

void FBXMeshLoader::EncodeVertexNormals(FBXMeshVertexData& inoutVertexData, FbxMesh* pMesh, int vertexId)
{
	int elementNormalCount = pMesh->GetElementNormalCount();
	inoutVertexData.Normals.resize(elementNormalCount);

	for (int l = 0; l < elementNormalCount; ++l)
	{
		FbxGeometryElementNormal* leNormal = pMesh->GetElementNormal(l);
		FbxVector4 normal;

		if (leNormal->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
		{
			switch (leNormal->GetReferenceMode())
			{
			case FbxGeometryElement::eDirect:
				normal = leNormal->GetDirectArray().GetAt(vertexId);
				break;
			case FbxGeometryElement::eIndexToDirect:
			{
				int id = leNormal->GetIndexArray().GetAt(vertexId);
				normal = leNormal->GetDirectArray().GetAt(id);
			}
			break;
			default:
				break; // other reference modes not shown here!
			}
		}

		normal.Normalize();
		bool isNaN = normal.SquareLength() < 1e-5f;
		inoutVertexData.Normals[l] = isNaN ? Vector3(0.0f, 0.0f, 1.0f) : 
			Vector3((float)normal[0], (float)normal[1], (float)normal[2]);
	}
}

void FBXMeshLoader::EncodeVertexTangents(FBXMeshVertexData& inoutVertexData, FbxMesh* pMesh, int vertexId)
{
	int elementTangentCount = pMesh->GetElementTangentCount();
	inoutVertexData.Tangents.resize(elementTangentCount);

	for (int l = 0; l < elementTangentCount; ++l)
	{
		FbxGeometryElementTangent* leTangent = pMesh->GetElementTangent(l);
		FbxVector4 tangent;

		if (leTangent->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
		{
			switch (leTangent->GetReferenceMode())
			{
			case FbxGeometryElement::eDirect:
				tangent = leTangent->GetDirectArray().GetAt(vertexId);
				break;
			case FbxGeometryElement::eIndexToDirect:
			{
				int id = leTangent->GetIndexArray().GetAt(vertexId);
				tangent = leTangent->GetDirectArray().GetAt(id);
			}
			break;
			default:
				break; // other reference modes not shown here!
			}
		}

		tangent.Normalize();
		bool isNaN = tangent.SquareLength() < 1e-5f;
		inoutVertexData.Tangents[l] = isNaN ? Vector3(1.0f, 0.0f, 0.0f) :
			Vector3((float)tangent[0], (float)tangent[1], (float)tangent[2]);
	}
}

void FBXMeshLoader::EncodeVertexBiTangents(FBXMeshVertexData& inoutVertexData, FbxMesh* pMesh, int vertexId)
{
	int elementBiTangentCount = pMesh->GetElementBinormalCount();
	inoutVertexData.BiTangents.resize(elementBiTangentCount);
	for (int l = 0; l < elementBiTangentCount; ++l)
	{
		FbxGeometryElementBinormal* leBinormal = pMesh->GetElementBinormal(l);
		FbxVector4 bitangent;

		if (leBinormal->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
		{
			switch (leBinormal->GetReferenceMode())
			{
			case FbxGeometryElement::eDirect:
				bitangent = leBinormal->GetDirectArray().GetAt(vertexId);
				break;
			case FbxGeometryElement::eIndexToDirect:
			{
				int id = leBinormal->GetIndexArray().GetAt(vertexId);
				bitangent = leBinormal->GetDirectArray().GetAt(id);
			}
			break;
			default:
				break; // other reference modes not shown here!
			}
		}

		bitangent.Normalize();
		bool isNaN = bitangent.SquareLength() < 1e-5f;
		inoutVertexData.BiTangents[l] = isNaN ? Vector3(0.0f, 1.0f, 0.0f) :
			Vector3((float)bitangent[0], (float)bitangent[1], (float)bitangent[2]);
	}
}

void FBXMeshLoader::ConvertVertexFormat(FBXMeshVertexData inVertexData, VertexPNTT& outVertexData)
{
	outVertexData.pos		= VectorPermute3DSToNix(inVertexData.Position);
	outVertexData.tex		= inVertexData.UVs.empty() ? Vector2(0.0f, 0.0f) : inVertexData.UVs[0];
	outVertexData.norm		= inVertexData.Normals.empty() ? Vector3(0.0f, 0.0f, 1.0f) : VectorPermute3DSToNix(inVertexData.Normals[0]);
	outVertexData.tangent	= inVertexData.Tangents.empty() ? Vector3(1.0f, 0.0f, 0.0f) : VectorPermute3DSToNix(inVertexData.Tangents[0]);
}

Vector3 FBXMeshLoader::VectorPermute3DSToNix(const Vector3 value)
{
	return Vector3(value.x, value.z, value.y);
}

void FBXMeshLoader::InitializeSdkObjects(FbxManager*& pManager, FbxScene*& pScene)
{
	//The first thing to do is to create the FBX Manager which is the object allocator for almost all the classes in the SDK
	pManager = FbxManager::Create();
	if (!pManager)
	{
		FBXSDK_printf("Error: Unable to create FBX Manager!\n");
		exit(1);
	}
	else FBXSDK_printf("Autodesk FBX SDK version %s\n", pManager->GetVersion());

	//Create an IOSettings object. This object holds all import/export settings.
	FbxIOSettings* ios = FbxIOSettings::Create(pManager, IOSROOT);
	pManager->SetIOSettings(ios);

	//Load plugins from the executable directory (optional)
	FbxString lPath = FbxGetApplicationDirectory();
	pManager->LoadPluginsDirectory(lPath.Buffer());

	//Create an FBX scene. This object holds most objects imported/exported from/to files.
	pScene = FbxScene::Create(pManager, "My Scene");
	if (!pScene)
	{
		FBXSDK_printf("Error: Unable to create FBX scene!\n");
		exit(1);
	}
}

bool FBXMeshLoader::LoadScene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename)

{
	int lFileMajor, lFileMinor, lFileRevision;
	int lSDKMajor, lSDKMinor, lSDKRevision;
	//int lFileFormat = -1;
	//int lAnimStackCount;
	bool lStatus;
	//char lPassword[1024];

	// Get the file version number generate by the FBX SDK.
	FbxManager::GetFileFormatVersion(lSDKMajor, lSDKMinor, lSDKRevision);

	// Create an importer.
	FbxImporter* lImporter = FbxImporter::Create(pManager, "");

	// Initialize the importer by providing a filename.
	const bool lImportStatus = lImporter->Initialize(pFilename, -1, pManager->GetIOSettings());
	lImporter->GetFileVersion(lFileMajor, lFileMinor, lFileRevision);

	if (!lImportStatus)
	{
		FbxString error = lImporter->GetStatus().GetErrorString();
		FBXSDK_printf("Call to FbxImporter::Initialize() failed.\n");
		FBXSDK_printf("Error returned: %s\n\n", error.Buffer());

		if (lImporter->GetStatus().GetCode() == FbxStatus::eInvalidFileVersion)
		{
			FBXSDK_printf("FBX file format version for this FBX SDK is %d.%d.%d\n", lSDKMajor, lSDKMinor, lSDKRevision);
			FBXSDK_printf("FBX file format version for file '%s' is %d.%d.%d\n\n", pFilename, lFileMajor, lFileMinor, lFileRevision);
		}

		return false;
	}

	//FBXSDK_printf("FBX file format version for this FBX SDK is %d.%d.%d\n", lSDKMajor, lSDKMinor, lSDKRevision);

	if (lImporter->IsFBX())
	{
		//FBXSDK_printf("FBX file format version for file '%s' is %d.%d.%d\n\n", pFilename, lFileMajor, lFileMinor, lFileRevision);

		//// From this point, it is possible to access animation stack information without
		//// the expense of loading the entire file.

		//FBXSDK_printf("Animation Stack Information\n");

		//lAnimStackCount = lImporter->GetAnimStackCount();

		//FBXSDK_printf("    Number of Animation Stacks: %d\n", lAnimStackCount);
		//FBXSDK_printf("    Current Animation Stack: \"%s\"\n", lImporter->GetActiveAnimStackName().Buffer());
		//FBXSDK_printf("\n");

		//for (int i = 0; i < lAnimStackCount; i++)
		//{
		//	FbxTakeInfo* lTakeInfo = lImporter->GetTakeInfo(i);

		//	FBXSDK_printf("    Animation Stack %d\n", i);
		//	FBXSDK_printf("         Name: \"%s\"\n", lTakeInfo->mName.Buffer());
		//	FBXSDK_printf("         Description: \"%s\"\n", lTakeInfo->mDescription.Buffer());

		//	// Change the value of the import name if the animation stack should be imported 
		//	// under a different name.
		//	FBXSDK_printf("         Import Name: \"%s\"\n", lTakeInfo->mImportName.Buffer());

		//	// Set the value of the import state to false if the animation stack should be not
		//	// be imported. 
		//	FBXSDK_printf("         Import State: %s\n", lTakeInfo->mSelect ? "true" : "false");
		//	FBXSDK_printf("\n");
		//}

		// Set the import states. By default, the import states are always set to 
		// true. The code below shows how to change these states.
		IOS_REF.SetBoolProp(IMP_FBX_MATERIAL, true);
		IOS_REF.SetBoolProp(IMP_FBX_TEXTURE, true);
		IOS_REF.SetBoolProp(IMP_FBX_LINK, true);
		IOS_REF.SetBoolProp(IMP_FBX_SHAPE, true);
		IOS_REF.SetBoolProp(IMP_FBX_GOBO, true);
		IOS_REF.SetBoolProp(IMP_FBX_ANIMATION, true);
		IOS_REF.SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);
	}

	// Import the scene.
	lStatus = lImporter->Import(pScene);

	// Destroy the importer.
	lImporter->Destroy();

	return lStatus;
}
