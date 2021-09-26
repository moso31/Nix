#include "FBXMeshLoader.h"
#include "NXPrimitive.h"
#include "NXScene.h"
#include "NXSubMeshGeometryEditor.h"

#ifdef IOS_REF
	#undef  IOS_REF
	#define IOS_REF (*(pManager->GetIOSettings()))
#endif

void FBXMeshLoader::LoadContent(FbxNode* pNode, NXPrimitive* pEngineMesh, std::vector<NXPrimitive*>& outMeshes, bool bAutoCalcTangents)
{
	FbxNodeAttribute::EType lAttributeType;
	int i;

	if (pNode->GetNodeAttribute() == NULL)
	{
		FBXSDK_printf("NULL Node Attribute\n\n");
	}
	else
	{
		lAttributeType = (pNode->GetNodeAttribute()->GetAttributeType());

		switch (lAttributeType)
		{
		default:
			break;

		case FbxNodeAttribute::eMesh:
			LoadMesh(pNode, pEngineMesh);
			break;

		case FbxNodeAttribute::eCamera:
			//DisplayCamera(pNode);
			break;

		case FbxNodeAttribute::eLight:
			//DisplayLight(pNode);
			break;
		}
	}

	pEngineMesh->SetName(pNode->GetName());

	// 如果开启了自动计算切线，则一定要在Init之前计算，不然值无法传到VB/IB
	if (bAutoCalcTangents) 
		pEngineMesh->CalculateTangents();

	LoadNodeTransformInfo(pNode, pEngineMesh);
	outMeshes.push_back(pEngineMesh);

	for (i = 0; i < pNode->GetChildCount(); i++)
	{
		NXPrimitive* pChildMesh = new NXPrimitive();
		LoadContent(pNode->GetChild(i), pChildMesh, outMeshes, bAutoCalcTangents);
		pChildMesh->SetParent(pEngineMesh);
	}
}

void FBXMeshLoader::LoadNodeTransformInfo(FbxNode* pNode, NXPrimitive* pEngineMesh)
{
	FbxDouble3 fVec = pNode->LclTranslation.Get();
	Vector3 vec = { (float)fVec[0], (float)fVec[1], (float)fVec[2] };
	pEngineMesh->SetTranslation(vec);

	fVec = pNode->LclRotation.Get();
	vec = { (float)fVec[0], (float)fVec[1], (float)fVec[2] };
	pEngineMesh->SetRotation(vec);

	fVec = pNode->LclScaling.Get();
	vec = { (float)fVec[0], (float)fVec[1], (float)fVec[2] };
	pEngineMesh->SetScale(vec);
}

void FBXMeshLoader::LoadMesh(FbxNode* pNode, NXPrimitive* pEngineMesh)
{
	FbxMesh* lMesh = (FbxMesh*)pNode->GetNodeAttribute();
	int lSubMeshCount = pNode->GetMaterialCount();

	LoadPolygons(lMesh, pEngineMesh, lSubMeshCount);
}

void FBXMeshLoader::LoadPolygons(FbxMesh* pMesh, NXPrimitive* pEngineMesh, int lSubMeshCount)
{
	int i, j, lPolygonCount = pMesh->GetPolygonCount();
	FbxVector4* lControlPoints = pMesh->GetControlPoints();

	// SubMesh数量已经有了，
	// 接下来，先统计每个SubMesh的Polygon/Vertex/Index数量，再读取每个顶点的VertexPNTT数据。
	UINT* pSubMeshPolygonsCounts = new UINT[lSubMeshCount];
	UINT* pSubMeshVerticesCounts = new UINT[lSubMeshCount];
	UINT* pSubMeshIndicesCounts = new UINT[lSubMeshCount];
	memset(pSubMeshPolygonsCounts, 0, sizeof(UINT) * lSubMeshCount);
	memset(pSubMeshVerticesCounts, 0, sizeof(UINT) * lSubMeshCount);
	memset(pSubMeshIndicesCounts, 0, sizeof(UINT) * lSubMeshCount);

	for (i = 0; i < lPolygonCount; i++)
	{
		int lPolygonSize = pMesh->GetPolygonSize(i);
		assert(lPolygonSize >= 3);

		FbxGeometryElementMaterial* leMat = pMesh->GetElementMaterial(0);
		assert(leMat);

		switch (leMat->GetMappingMode())
		{
		case FbxGeometryElement::eByPolygon:
			if (leMat->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
			{
				int leMatIndex = leMat->GetIndexArray().GetAt(i);
				assert(leMatIndex != -1);

				pSubMeshPolygonsCounts[leMatIndex]++;
				pSubMeshVerticesCounts[leMatIndex] += (lPolygonSize - 2) * 3;
				break;
			}
		}
	}

	for (int i = 0; i < lSubMeshCount; i++) pSubMeshIndicesCounts[i] = pSubMeshVerticesCounts[i];

	// 分配Vertex/Index data内存空间
	VertexPNTT** ppMeshVerticesData = new VertexPNTT*[lSubMeshCount];
	UINT** ppMeshIndicesData = new UINT*[lSubMeshCount];
	for (int i = 0; i < lSubMeshCount; i++)
	{
		ppMeshVerticesData[i] = new VertexPNTT[pSubMeshVerticesCounts[i]];
		ppMeshIndicesData[i] = new UINT[pSubMeshIndicesCounts[i]];

		memset(ppMeshVerticesData[i], 0, sizeof(UINT) * pSubMeshVerticesCounts[i]);
		memset(ppMeshIndicesData[i], 0, sizeof(UINT) * pSubMeshIndicesCounts[i]);
	}

	// 递增id
	UINT* ppSubMeshVertexId = new UINT[lSubMeshCount];
	UINT* ppSubMeshIndexId = new UINT[lSubMeshCount];
	memset(ppSubMeshVertexId, 0, sizeof(UINT) * lSubMeshCount);
	memset(ppSubMeshIndexId, 0, sizeof(UINT) * lSubMeshCount);

	// 读取所有多边形数据
	int vertexId = 0, indexId = 0;
	for (i = 0; i < lPolygonCount; i++)
	{
		int l;
		int lSubMeshId = -1;

		FbxGeometryElementMaterial* leMat = pMesh->GetElementMaterial(0);

		switch (leMat->GetMappingMode())
		{
		case FbxGeometryElement::eByPolygon:
			if (leMat->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
			{
				lSubMeshId = leMat->GetIndexArray().GetAt(i);
				assert(lSubMeshId != -1);
				break;
			}
		}

		for (l = 0; l < pMesh->GetElementPolygonGroupCount(); l++)
		{
			FbxGeometryElementPolygonGroup* lePolgrp = pMesh->GetElementPolygonGroup(l);
			switch (lePolgrp->GetMappingMode())
			{
			case FbxGeometryElement::eByPolygon:
				if (lePolgrp->GetReferenceMode() == FbxGeometryElement::eIndex)
				{
					int polyGroupId = lePolgrp->GetIndexArray().GetAt(i);
					break;
				}
			default:
				break;
			}
		}

		int lPolygonSize = pMesh->GetPolygonSize(i);
		VertexPNTT* pPolygonVerticesData = new VertexPNTT[lPolygonSize];

		for (j = 0; j < lPolygonSize; j++)
		{
			int lControlPointIndex = pMesh->GetPolygonVertex(i, j);
			FbxVector4 posData = lControlPoints[lControlPointIndex];
			pPolygonVerticesData[j].pos = Vector3((float)posData[0], (float)posData[1], (float)posData[2]);

			for (l = 0; l < pMesh->GetElementVertexColorCount(); l++)
			{
				FbxGeometryElementVertexColor* leVtxc = pMesh->GetElementVertexColor(l);

				switch (leVtxc->GetMappingMode())
				{
				default:
					break;
				case FbxGeometryElement::eByControlPoint:
					switch (leVtxc->GetReferenceMode())
					{
					case FbxGeometryElement::eDirect:
						//DisplayColor(header, leVtxc->GetDirectArray().GetAt(lControlPointIndex));
						break;
					case FbxGeometryElement::eIndexToDirect:
					{
						int id = leVtxc->GetIndexArray().GetAt(lControlPointIndex);
						//DisplayColor(header, leVtxc->GetDirectArray().GetAt(id));
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
						//DisplayColor(header, leVtxc->GetDirectArray().GetAt(vertexId));
						break;
					case FbxGeometryElement::eIndexToDirect:
					{
						int id = leVtxc->GetIndexArray().GetAt(vertexId);
						//DisplayColor(header, leVtxc->GetDirectArray().GetAt(id));
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
			}
			for (l = 0; l < pMesh->GetElementUVCount(); ++l)
			{
				FbxGeometryElementUV* leUV = pMesh->GetElementUV(l);

				FbxVector2 texData(0.0, 0.0);

				switch (leUV->GetMappingMode())
				{
				default:
					break;
				case FbxGeometryElement::eByControlPoint:
					switch (leUV->GetReferenceMode())
					{
					case FbxGeometryElement::eDirect:
						texData = leUV->GetDirectArray().GetAt(lControlPointIndex);
						break;
					case FbxGeometryElement::eIndexToDirect:
					{
						int id = leUV->GetIndexArray().GetAt(lControlPointIndex);
						texData = leUV->GetDirectArray().GetAt(id);
					}
					break;
					default:
						break; // other reference modes not shown here!
					}
					break;

				case FbxGeometryElement::eByPolygonVertex:
				{
					int lTextureUVIndex = pMesh->GetTextureUVIndex(i, j);
					switch (leUV->GetReferenceMode())
					{
					case FbxGeometryElement::eDirect:
					case FbxGeometryElement::eIndexToDirect:
					{
						texData = leUV->GetDirectArray().GetAt(lTextureUVIndex);
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

				// 暂时默认所有来自fbx的UV全部是反转的。
				bool bFlipUV = true;
				pPolygonVerticesData[j].tex = Vector2((float)texData[0], bFlipUV ? 1.0f - (float)texData[1] : (float)texData[1]);
			}

			for (l = 0; l < pMesh->GetElementNormalCount(); ++l)
			{
				FbxGeometryElementNormal* leNormal = pMesh->GetElementNormal(l);

				FbxVector4 normData;
				if (leNormal->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
				{
					switch (leNormal->GetReferenceMode())
					{
					case FbxGeometryElement::eDirect:
						normData = leNormal->GetDirectArray().GetAt(vertexId);
						break;
					case FbxGeometryElement::eIndexToDirect:
					{
						int id = leNormal->GetIndexArray().GetAt(vertexId);
						normData = leNormal->GetDirectArray().GetAt(id);
					}
					break;
					default:
						break; // other reference modes not shown here!
					}
				}

				pPolygonVerticesData[j].norm = Vector3((float)normData[0], (float)normData[1], (float)normData[2]);
			}
			for (l = 0; l < pMesh->GetElementTangentCount(); ++l)
			{
				FbxGeometryElementTangent* leTangent = pMesh->GetElementTangent(l);

				FbxVector4 tangentData;
				if (leTangent->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
				{
					switch (leTangent->GetReferenceMode())
					{
					case FbxGeometryElement::eDirect:
						tangentData = leTangent->GetDirectArray().GetAt(vertexId);
						break;
					case FbxGeometryElement::eIndexToDirect:
					{
						int id = leTangent->GetIndexArray().GetAt(vertexId);
						tangentData = leTangent->GetDirectArray().GetAt(id);
					}
					break;
					default:
						break; // other reference modes not shown here!
					}
				}

				pPolygonVerticesData[j].tangent = Vector3((float)tangentData[0], (float)tangentData[1], (float)tangentData[2]);
			}
			for (l = 0; l < pMesh->GetElementBinormalCount(); ++l)
			{
				FbxGeometryElementBinormal* leBinormal = pMesh->GetElementBinormal(l);
				FbxVector4 binormalData;
				if (leBinormal->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
				{
					switch (leBinormal->GetReferenceMode())
					{
					case FbxGeometryElement::eDirect:
						binormalData = leBinormal->GetDirectArray().GetAt(vertexId);
						break;
					case FbxGeometryElement::eIndexToDirect:
					{
						int id = leBinormal->GetIndexArray().GetAt(vertexId);
						binormalData = leBinormal->GetDirectArray().GetAt(id);
					}
					break;
					default:
						break; // other reference modes not shown here!
					}
				}
			}
			vertexId++;
		} // for polygonSize


		// 一个Polygon读取完以后，根据此Polygon的SubMesh索引，将顶点和索引数据指定给对应的SubMesh
		for (int j = 0; j < lPolygonSize - 2; j++)
		{
			UINT& lSubMeshVertexId = ppSubMeshVertexId[lSubMeshId];
			UINT& lSubMeshIndexId = ppSubMeshIndexId[lSubMeshId];

			ppMeshVerticesData[lSubMeshId][lSubMeshVertexId++] = pPolygonVerticesData[0];
			ppMeshVerticesData[lSubMeshId][lSubMeshVertexId++] = pPolygonVerticesData[j + 1];
			ppMeshVerticesData[lSubMeshId][lSubMeshVertexId++] = pPolygonVerticesData[j + 2];

			ppMeshIndicesData[lSubMeshId][lSubMeshIndexId++] = lSubMeshIndexId;
			ppMeshIndicesData[lSubMeshId][lSubMeshIndexId++] = lSubMeshIndexId;
			ppMeshIndicesData[lSubMeshId][lSubMeshIndexId++] = lSubMeshIndexId;

			auto ta1 = pPolygonVerticesData[0];
			auto ta2 = pPolygonVerticesData[j + 1];
			auto ta3 = pPolygonVerticesData[j + 2];

			auto tb1 = ppMeshIndicesData[lSubMeshId][lSubMeshIndexId - 3];
			auto tb2 = ppMeshIndicesData[lSubMeshId][lSubMeshIndexId - 2];
			auto tb3 = ppMeshIndicesData[lSubMeshId][lSubMeshIndexId - 1];

			int gg = 1;
		}

		delete[] pPolygonVerticesData;
	} // for polygonCount

	NXSubMeshGeometryEditor::CreateFBXMesh(pEngineMesh, lSubMeshCount, ppMeshVerticesData, pSubMeshVerticesCounts, ppMeshIndicesData, pSubMeshIndicesCounts);

	delete[] ppSubMeshVertexId;
	delete[] ppSubMeshIndexId;
	for (int i = 0; i < lSubMeshCount; i++)
	{
		delete[] ppMeshVerticesData[i];
		delete[] ppMeshIndicesData[i];
	}
	delete[] ppMeshVerticesData;
	delete[] ppMeshIndicesData;
	delete[] pSubMeshPolygonsCounts;
	delete[] pSubMeshVerticesCounts;
	delete[] pSubMeshIndicesCounts;
}

void FBXMeshLoader::LoadFBXFile(std::string filepath, NXScene* pRenderScene, std::vector<NXPrimitive*>& outMeshes, bool bAutoCalcTangents)
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
		for (int i = 0; i < lNode->GetChildCount(); i++)
		{
			auto pMesh = new NXPrimitive();
			LoadContent(lNode->GetChild(i), pMesh, outMeshes, bAutoCalcTangents);
		}
	}
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
	int lAnimStackCount;
	bool lStatus;
	char lPassword[1024];

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

	FBXSDK_printf("FBX file format version for this FBX SDK is %d.%d.%d\n", lSDKMajor, lSDKMinor, lSDKRevision);

	if (lImporter->IsFBX())
	{
		FBXSDK_printf("FBX file format version for file '%s' is %d.%d.%d\n\n", pFilename, lFileMajor, lFileMinor, lFileRevision);

		// From this point, it is possible to access animation stack information without
		// the expense of loading the entire file.

		FBXSDK_printf("Animation Stack Information\n");

		lAnimStackCount = lImporter->GetAnimStackCount();

		FBXSDK_printf("    Number of Animation Stacks: %d\n", lAnimStackCount);
		FBXSDK_printf("    Current Animation Stack: \"%s\"\n", lImporter->GetActiveAnimStackName().Buffer());
		FBXSDK_printf("\n");

		for (int i = 0; i < lAnimStackCount; i++)
		{
			FbxTakeInfo* lTakeInfo = lImporter->GetTakeInfo(i);

			FBXSDK_printf("    Animation Stack %d\n", i);
			FBXSDK_printf("         Name: \"%s\"\n", lTakeInfo->mName.Buffer());
			FBXSDK_printf("         Description: \"%s\"\n", lTakeInfo->mDescription.Buffer());

			// Change the value of the import name if the animation stack should be imported 
			// under a different name.
			FBXSDK_printf("         Import Name: \"%s\"\n", lTakeInfo->mImportName.Buffer());

			// Set the value of the import state to false if the animation stack should be not
			// be imported. 
			FBXSDK_printf("         Import State: %s\n", lTakeInfo->mSelect ? "true" : "false");
			FBXSDK_printf("\n");
		}

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
	if (lStatus == true)
	{
		// Check the scene integrity!
		FbxStatus status;
		FbxArray< FbxString*> details;
		FbxSceneCheckUtility sceneCheck(FbxCast<FbxScene>(pScene), &status, &details);
		lStatus = sceneCheck.Validate(FbxSceneCheckUtility::eCkeckData);
		bool lNotify = (!lStatus && details.GetCount() > 0) || (lImporter->GetStatus().GetCode() != FbxStatus::eSuccess);
		if (lNotify)
		{
			FBXSDK_printf("\n");
			FBXSDK_printf("********************************************************************************\n");
			if (details.GetCount())
			{
				FBXSDK_printf("Scene integrity verification failed with the following errors:\n");
				for (int i = 0; i < details.GetCount(); i++)
					FBXSDK_printf("   %s\n", details[i]->Buffer());

				FbxArrayDelete<FbxString*>(details);
			}

			if (lImporter->GetStatus().GetCode() != FbxStatus::eSuccess)
			{
				FBXSDK_printf("\n");
				FBXSDK_printf("WARNING:\n");
				FBXSDK_printf("   The importer was able to read the file but with errors.\n");
				FBXSDK_printf("   Loaded scene may be incomplete.\n\n");
				FBXSDK_printf("   Last error message:'%s'\n", lImporter->GetStatus().GetErrorString());
			}
			FBXSDK_printf("********************************************************************************\n");
			FBXSDK_printf("\n");
		}
	}

	if (lStatus == false && lImporter->GetStatus().GetCode() == FbxStatus::ePasswordError)
	{
		FBXSDK_printf("Please enter password: ");

		lPassword[0] = '\0';

		FBXSDK_CRT_SECURE_NO_WARNING_BEGIN
			scanf("%s", lPassword);
		FBXSDK_CRT_SECURE_NO_WARNING_END

			FbxString lString(lPassword);

		IOS_REF.SetStringProp(IMP_FBX_PASSWORD, lString);
		IOS_REF.SetBoolProp(IMP_FBX_PASSWORD_ENABLE, true);

		lStatus = lImporter->Import(pScene);

		if (lStatus == false && lImporter->GetStatus().GetCode() == FbxStatus::ePasswordError)
		{
			FBXSDK_printf("\nPassword is wrong, import aborted.\n");
		}
	}

	// Destroy the importer.
	lImporter->Destroy();

	return lStatus;
}
