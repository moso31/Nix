#include "FBXMeshLoader.h"
#include "NXMesh.h"
#include "NXScene.h"

void FBXMeshLoader::LoadContent(FbxNode* pNode, std::shared_ptr<NXMesh>& pEngineMesh, std::vector<std::shared_ptr<NXMesh>>& outMeshes)
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
	pEngineMesh->Init();
	LoadNodeTransformInfo(pNode, pEngineMesh);
	outMeshes.push_back(pEngineMesh);

	for (i = 0; i < pNode->GetChildCount(); i++)
	{
		std::shared_ptr<NXMesh> pChildMesh = std::make_shared<NXMesh>();
		LoadContent(pNode->GetChild(i), pChildMesh, outMeshes);
		pChildMesh->SetParent(pEngineMesh);
	}
}

void FBXMeshLoader::LoadNodeTransformInfo(FbxNode* pNode, std::shared_ptr<NXMesh>& pEngineMesh)
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

void FBXMeshLoader::LoadMesh(FbxNode* pNode, std::shared_ptr<NXMesh>& pEngineMesh)
{
	FbxMesh* lMesh = (FbxMesh*)pNode->GetNodeAttribute();

	LoadPolygons(lMesh, pEngineMesh);
}

void FBXMeshLoader::LoadPolygons(FbxMesh* pMesh, std::shared_ptr<NXMesh>& pEngineMesh)
{
	int i, j, lPolygonCount = pMesh->GetPolygonCount();
	FbxVector4* lControlPoints = pMesh->GetControlPoints();
	char header[100];

	//DisplayString("    Polygons");

	int vertexId = 0;
	for (i = 0; i < lPolygonCount; i++)
	{
		//DisplayInt("        Polygon ", i);
		int l;

		for (l = 0; l < pMesh->GetElementPolygonGroupCount(); l++)
		{
			FbxGeometryElementPolygonGroup* lePolgrp = pMesh->GetElementPolygonGroup(l);
			switch (lePolgrp->GetMappingMode())
			{
			case FbxGeometryElement::eByPolygon:
				if (lePolgrp->GetReferenceMode() == FbxGeometryElement::eIndex)
				{
					FBXSDK_sprintf(header, 100, "        Assigned to group: ");
					int polyGroupId = lePolgrp->GetIndexArray().GetAt(i);
					//DisplayInt(header, polyGroupId);
					break;
				}
			default:
				// any other mapping modes don't make sense
				//DisplayString("        \"unsupported group assignment\"");
				break;
			}
		}

		VertexPNT vertex;
		int lPolygonSize = pMesh->GetPolygonSize(i);

		for (j = 0; j < lPolygonSize; j++)
		{
			int lControlPointIndex = pMesh->GetPolygonVertex(i, j);
			FbxVector4 posData = lControlPoints[lControlPointIndex];
			vertex.pos = Vector3((float)posData[0], (float)posData[1], (float)posData[2]);

			for (l = 0; l < pMesh->GetElementVertexColorCount(); l++)
			{
				FbxGeometryElementVertexColor* leVtxc = pMesh->GetElementVertexColor(l);
				FBXSDK_sprintf(header, 100, "            Color vertex: ");

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
				FBXSDK_sprintf(header, 100, "            Texture UV: ");

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

				vertex.tex = Vector2((float)texData[0], (float)texData[1]);
			}

			for (l = 0; l < pMesh->GetElementNormalCount(); ++l)
			{
				FbxGeometryElementNormal* leNormal = pMesh->GetElementNormal(l);
				FBXSDK_sprintf(header, 100, "            Normal: ");

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

				vertex.norm = Vector3((float)normData[0], (float)normData[1], (float)normData[2]);
			}
			for (l = 0; l < pMesh->GetElementTangentCount(); ++l)
			{
				FbxGeometryElementTangent* leTangent = pMesh->GetElementTangent(l);
				FBXSDK_sprintf(header, 100, "            Tangent: ");

				if (leTangent->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
				{
					switch (leTangent->GetReferenceMode())
					{
					case FbxGeometryElement::eDirect:
						//Display3DVector(header, leTangent->GetDirectArray().GetAt(vertexId));
						break;
					case FbxGeometryElement::eIndexToDirect:
					{
						int id = leTangent->GetIndexArray().GetAt(vertexId);
						//Display3DVector(header, leTangent->GetDirectArray().GetAt(id));
					}
					break;
					default:
						break; // other reference modes not shown here!
					}
				}

			}
			for (l = 0; l < pMesh->GetElementBinormalCount(); ++l)
			{

				FbxGeometryElementBinormal* leBinormal = pMesh->GetElementBinormal(l);

				FBXSDK_sprintf(header, 100, "            Binormal: ");
				if (leBinormal->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
				{
					switch (leBinormal->GetReferenceMode())
					{
					case FbxGeometryElement::eDirect:
						//Display3DVector(header, leBinormal->GetDirectArray().GetAt(vertexId));
						break;
					case FbxGeometryElement::eIndexToDirect:
					{
						int id = leBinormal->GetIndexArray().GetAt(vertexId);
						//Display3DVector(header, leBinormal->GetDirectArray().GetAt(id));
					}
					break;
					default:
						break; // other reference modes not shown here!
					}
				}
			}
			vertexId++;

			pEngineMesh->m_vertices.push_back(vertex);
			int meshIdx = i * 3 + j;
			pEngineMesh->m_indices.push_back(meshIdx);
		} // for polygonSize
	} // for polygonCount

	//check visibility for the edges of the mesh
	for (int l = 0; l < pMesh->GetElementVisibilityCount(); ++l)
	{
		FbxGeometryElementVisibility* leVisibility = pMesh->GetElementVisibility(l);
		//FBXSDK_sprintf(header, 100, "    Edge Visibility : ");
		//DisplayString(header);
		switch (leVisibility->GetMappingMode())
		{
		default:
			break;
			//should be eByEdge
		case FbxGeometryElement::eByEdge:
			//should be eDirect
			for (j = 0; j != pMesh->GetMeshEdgeCount(); ++j)
			{
				//DisplayInt("        Edge ", j);
				//DisplayBool("              Edge visibility: ", leVisibility->GetDirectArray().GetAt(j));
			}

			break;
		}
	}
	//DisplayString("");
}

void FBXMeshLoader::LoadFBXFile(std::string filepath, std::shared_ptr<NXScene> pRenderScene, std::vector<std::shared_ptr<NXMesh>>& outMeshes)
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
			auto pMesh = std::make_shared<NXMesh>();
			LoadContent(lNode->GetChild(i), pMesh, outMeshes);
		}
	}
}
