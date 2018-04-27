#include "FBXImportExport.h"
#include <fbxsdk.h>
#include <assert.h>

#define numBuckets 1000

void FBXExporter::RecursiveStartSkeletonProcessing(fbxsdk::FbxNode* root)
{
	for (int i = 0; i < root->GetChildCount(); i++)
	{
		RecursiveLoadSkeleton(root->GetChild(i), 0, -1);
	}
}

void FBXExporter::RecursiveLoadSkeleton(FbxNode* node, int ndx, int pndx)
{
	if (!node)
		return;

	if (!std::strcmp(node->GetName(),"L_Toe_J") || !std::strcmp(node->GetName(),"Weapon_Attach_J")|| !std::strcmp(node->GetName(), "Nose_J")|| !std::strcmp(node->GetName(), "R_Toe_J"))
		return;

	if (node->GetNodeAttribute() && node->GetNodeAttribute()->GetAttributeType() && node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
	{
		WaffleJoint currJoint;
		currJoint.pIndex = pndx;
		currJoint.name = std::string(node->GetName());
		skeleton.joints.push_back(currJoint);
	}

	for (int i = 0; i < node->GetChildCount(); i++)
	{
		RecursiveLoadSkeleton(node->GetChild(i), (int)skeleton.joints.size(), ndx);
	}
}

// For hash *(int*)&float
unsigned int hashVec3(float x, float y, float z)
{
	return (((*(unsigned int*)&x) ^ (*(unsigned int*)&y) ^ (*(unsigned int*)&z)) % numBuckets);
}

unsigned int hashVec2(float x, float y)
{
	return (((*(unsigned int*)&x) ^ (*(unsigned int*)&y)) % numBuckets);
}

void FBXExporter::ProcessJoints(fbxsdk::FbxMesh * mesh, fbxsdk::FbxScene* scene)
{
	unsigned int defCount = mesh->GetDeformerCount();

	for (auto deformerIndex = 0; (unsigned)deformerIndex < defCount; deformerIndex++)
	{
		FbxSkin* skin = reinterpret_cast<FbxSkin*>(mesh->GetDeformer(deformerIndex, FbxDeformer::eSkin));
		if (!skin)
			continue;

		unsigned int clusterCount = skin->GetClusterCount();

		for (auto clusterIndex = 0; (unsigned)clusterIndex < clusterCount; clusterIndex++)
		{
			FbxCluster* cluster = skin->GetCluster(clusterIndex);
			std::string jointName = cluster->GetLink()->GetName();
			unsigned int jIndex = 0;
			for (int ind = 0; ind < skeleton.joints.size(); ind++)
			{
				if (jointName == skeleton.joints[ind].name)
				{
					jIndex = ind;
					break;
				}
			}
			FbxAMatrix trans, link, bind;

			cluster->GetTransformMatrix(trans);
			cluster->GetTransformLinkMatrix(link);
			bind = link.Inverse() * trans;

			for (int m = 0; m < 4; m++)
			{
				for (int n = 0; n < 4; n++)
				{
					skeleton.joints[jIndex].bindInverse[m][n] = bind.Get(n, m);
				}
			}

			unsigned int cpCount = cluster->GetControlPointIndicesCount();

			for (unsigned int cp = 0; cp < cpCount; cp++)
			{
				WaffleBlendingPair currPair;
				currPair.index = jIndex;
				currPair.weight = cluster->GetControlPointWeights()[cp];
				cpoints[cluster->GetControlPointIndices()[cp]].blendingInfo.push_back(currPair);
			}

			FbxAnimStack* stack = scene->GetSrcObject<FbxAnimStack>(0);
			FbxString stackName = stack->GetName();
			animName = stackName.Buffer();
			FbxTakeInfo* take = scene->GetTakeInfo(stackName);
			FbxTime start = take->mLocalTimeSpan.GetStart();
			FbxTime end = take->mLocalTimeSpan.GetStop();
			animTime = end.GetFrameCount(FbxTime::eFrames24) - start.GetFrameCount(FbxTime::eFrames24) + 1;
			WaffleKeyframe** anim = &skeleton.joints[jIndex].anim;

			for (auto i = start.GetFrameCount(FbxTime::eFrames24); i <= end.GetFrameCount(FbxTime::eFrames24); i++)
			{
				FbxTime curr;
				curr.SetFrame(i, FbxTime::eFrames24);
				*anim = new WaffleKeyframe();
				(*anim)->keyNum = i;
				FbxAMatrix offset;
			}
		}
	}
}

void FBXExporter::ProcessControl(fbxsdk::FbxMesh * mesh)
{
	unsigned int cp = mesh->GetControlPointsCount();

	for (int i = 0; (unsigned)i < cp; i++)
	{
		WaffleControl c;
		c.pos[0] = (float)(mesh->GetControlPointAt(i).mData[0]);
		c.pos[1] = (float)(mesh->GetControlPointAt(i).mData[1]);
		c.pos[2] = (float)(mesh->GetControlPointAt(i).mData[2]);

		cpoints.push_back(c);
	}
}

FBXExporter::FBXExporter()
{
	sdkManager = FbxManager::Create();
}

FBXExporter::~FBXExporter()
{
}

bool FBXExporter::ReadFBX(std::string _filePath, std::string _texturePath)
{
	FbxIOSettings* fbxIOS = FbxIOSettings::Create(sdkManager, IOSROOT);

	sdkManager->SetIOSettings(fbxIOS);

	FbxImporter* in = FbxImporter::Create(sdkManager, "");

	bool status = in->Initialize(_filePath.c_str(), -1, sdkManager->GetIOSettings());

	if (!status)
	{
		printf("Importer failed initialization. \nCode: %i\n Err: %s\n", in->GetStatus().GetCode(), in->GetStatus().GetErrorString());
		return false;
	}

	FbxScene* scene = FbxScene::Create(sdkManager, "scene");

	FbxAxisSystem sys = FbxAxisSystem(FbxAxisSystem::EPreDefinedAxisSystem::eDirectX);

	in->Import(scene);

	scene->GetGlobalSettings().SetAxisSystem(sys);

	//sys.ConvertScene(scene);

	in->Destroy();

	FbxNode* root = scene->GetRootNode();

	std::queue<FbxNode*> open;

	open.push(root);

	RecursiveStartSkeletonProcessing(root);

	while (!open.empty())
	{
		FbxNode* n = open.front();
		open.pop();

		if (n->GetNodeAttribute())
		{
			switch (n->GetNodeAttribute()->GetAttributeType())
			{
			case FbxNodeAttribute::eLight:
			{
				WaffleLight l;

				for (int i = 0; i < 3; i++)
				{
					l.t.t[i] = (float)n->LclTranslation.Get().mData[i];
					l.t.r[i] = (float)n->LclRotation.Get().mData[i];
					l.t.s[i] = (float)n->LclScaling.Get().mData[i];
				}

				FbxLight* light = (FbxLight*)n->GetNodeAttribute();

				for (int i = 0; i < 3; i++)
					l.color[i] = (float)light->Color.Get()[i];

				l.hotSpot = (float)light->InnerAngle.Get();
				l.coneAngle = (float)light->OuterAngle.Get();
				l.naStart = (float)light->NearAttenuationStart.Get();
				l.naEnd = (float)light->NearAttenuationEnd.Get();
				l.faStart = (float)light->FarAttenuationStart.Get();
				l.faEnd = (float)light->FarAttenuationEnd.Get();
				l.intensity = (float)light->Intensity.Get();

				switch (light->LightType.Get())
				{
				case FbxLight::eSpot:
					l.type = WaffleLight::LightType::SPOT;
					break;
				case FbxLight::eDirectional:
					l.type = WaffleLight::LightType::DIR;
					break;
				case FbxLight::ePoint:
					l.type = WaffleLight::LightType::POINT;
					break;
				}

				lights.push_back(l);
			}
			break;
			case FbxNodeAttribute::eCamera:
			{
				WaffleCamera c;

				for (int i = 0; i < 3; i++)
				{
					c.t.t[i] = (float)n->LclTranslation.Get().mData[i];
					c.t.r[i] = (float)n->LclRotation.Get().mData[i];
					c.t.s[i] = (float)n->LclScaling.Get().mData[i];
				}

				FbxCamera* cam = (FbxCamera*)n->GetNodeAttribute();

				for (int i = 0; i < 3; i++)
				{
					c.pos[i] = (float)cam->Position.Get()[i];
					c.up[i] = (float)cam->UpVector.Get()[i];
					c.interest[i] = (float)cam->InterestPosition.Get()[i];
				}

				c.roll = (float)cam->Roll.Get();
				c.aspectW = (float)cam->AspectWidth.Get();
				c.aspectH = (float)cam->AspectHeight.Get();
				c.pAspectR = (float)cam->PixelAspectRatio.Get();
				c.fovDeg = (float)cam->FieldOfView.Get();
				c.fovX = (float)cam->FieldOfViewX.Get();
				c.fovY = (float)cam->FieldOfViewY.Get();
				c.focal = (float)cam->FocalLength.Get();
				c.nearPlane = (float)cam->NearPlane.Get();
				c.farPlane = (float)cam->FarPlane.Get();


				cameras.push_back(c);
			}
			break;
			case FbxNodeAttribute::eMesh:
			{
				WaffleModel w;

				for (int i = 0; i < 3; i++)
				{
					w.t.t[i] = (float)n->LclTranslation.Get().mData[i];
					w.t.r[i] = (float)n->LclRotation.Get().mData[i];
					w.t.s[i] = (float)n->LclScaling.Get().mData[i];
				}

				FbxMesh* mesh = (FbxMesh*)n->GetMesh();

				ProcessControl(mesh);


				unsigned int defCount = mesh->GetDeformerCount();

				for (auto deformerIndex = 0; (unsigned)deformerIndex < defCount; deformerIndex++)
				{
					FbxSkin* skin = reinterpret_cast<FbxSkin*>(mesh->GetDeformer(deformerIndex, FbxDeformer::eSkin));
					if (!skin)
						continue;

					unsigned int clusterCount = skin->GetClusterCount();

					for (auto clusterIndex = 0; (unsigned)clusterIndex < clusterCount; clusterIndex++)
					{
						FbxCluster* cluster = skin->GetCluster(clusterIndex);
						std::string jointName = cluster->GetLink()->GetName();
						unsigned int jIndex = 0;
						for (int ind = 0; ind < skeleton.joints.size(); ind++)
						{
							if (jointName == skeleton.joints[ind].name)
							{
								jIndex = ind;
								break;
							}
						}
						FbxAMatrix trans, link, bind;

						cluster->GetTransformMatrix(trans);
						cluster->GetTransformLinkMatrix(link);
						bind = link.Inverse() * trans;

						for (int m = 0; m < 4; m++)
						{
							for (int n = 0; n < 4; n++)
							{
								skeleton.joints[jIndex].bindInverse[m][n] = bind.Get(n, m);
							}
						}

						unsigned int cpCount = cluster->GetControlPointIndicesCount();

						for (unsigned int cp = 0; cp < cpCount; cp++)
						{
							WaffleBlendingPair currPair;
							currPair.index = jIndex;
							currPair.weight = cluster->GetControlPointWeights()[cp];
							cpoints[cluster->GetControlPointIndices()[cp]].blendingInfo.push_back(currPair);
						}

						FbxAnimStack* stack = scene->GetSrcObject<FbxAnimStack>(0);
						FbxString stackName = stack->GetName();
						animName = stackName.Buffer();
						FbxTakeInfo* take = scene->GetTakeInfo(stackName);
						FbxTime start = take->mLocalTimeSpan.GetStart();
						FbxTime end = take->mLocalTimeSpan.GetStop();
						animTime = end.GetFrameCount(FbxTime::eFrames24) - start.GetFrameCount(FbxTime::eFrames24) + 1;
						WaffleKeyframe** anim = &skeleton.joints[jIndex].anim;

						for (auto frame = start.GetFrameCount(FbxTime::eFrames24); frame <= end.GetFrameCount(FbxTime::eFrames24); frame++)
						{
							FbxTime curr;
							curr.SetFrame(frame, FbxTime::eFrames24);
							*anim = new WaffleKeyframe();
							(*anim)->keyNum = frame;
							(*anim)->frameDelta = curr.GetSecondDouble();
							FbxAMatrix offset = n->EvaluateGlobalTransform(curr);
							offset = offset.Inverse() * cluster->GetLink()->EvaluateGlobalTransform(curr);
							for (int m = 0; m < 4; m++)
							{
								for (int n = 0; n < 4; n++)
								{
									(*anim)->trans[m][n] = offset.Get(n, m);
								}
							}
							anim = &((*anim)->next);
						}
					}
				}

				WaffleBlendingPair dummy;
				dummy.index = 0;
				dummy.weight = 0;
				for (auto it = cpoints.begin(); it != cpoints.end(); it++)
				{
					for (int blend = (int)it->blendingInfo.size(); blend <= 4; blend++)
						it->blendingInfo.push_back(dummy);
				}

				skeletons.push_back(skeleton);

				FbxVector4* verts = mesh->GetControlPoints();

				int vc = 0;

				int pc = mesh->GetPolygonCount();

				for (int i = 0; i < mesh->GetPolygonCount(); i++)
				{
					int numVerts = mesh->GetPolygonSize(i);
					assert(numVerts == 3);

					FbxLayerElementArrayTemplate<FbxVector2>* uvs = 0;
					mesh->GetTextureUV(&uvs, FbxLayerElement::eTextureDiffuse);

					for (int j = 0; j < numVerts; j++)
					{
						int cpi = mesh->GetPolygonVertex(i, j);
						WaffleControl control = cpoints[cpi];

						WaffleVertex v;


						// Vertices
						v.x = (float)verts[cpi].mData[0];
						v.y = (float)verts[cpi].mData[1];
						v.z = (float)verts[cpi].mData[2];
						v.w = 1.0f;

						FbxVector4 norm;

						// Normals
						if (mesh->GetPolygonVertexNormal(cpi, j, norm))
						{
							v.nX = (float)norm.mData[0];
							v.nY = (float)norm.mData[1];
							v.nZ = (float)norm.mData[2];
						}

						// Uvs
						v.u = (float)uvs->GetAt(mesh->GetTextureUVIndex(i, j)).mData[0];
						v.v = (float)uvs->GetAt(mesh->GetTextureUVIndex(i, j)).mData[1];

						for (int blend = 0; blend < control.blendingInfo.size(); blend++)
						{
							v.blendingInformation.push_back(control.blendingInfo[blend]);
						}

						w.verts.push_back(v);
						//w.indices.push_back(vc);
						vc++;
					}
				}
				w.indices.resize(mesh->GetPolygonVertexCount());
				for (int q = 0; q < mesh->GetPolygonVertexCount(); q++)
					w.indices[q] = mesh->GetPolygonVertices()[q];

				w.texturePath = _texturePath;

				std::vector<WaffleVertex> uverts;
				std::vector<WaffleVertex> unorms;
				std::vector<WaffleVertex> uuvs;
				w.indices.clear();

				bool first = true;
				std::vector<HashWaffle> uVertHash[numBuckets];
				std::vector<HashWaffle> uNormHash[numBuckets];
				std::vector<HashWaffle> uUVHash[numBuckets];

#pragma region Verts
				unsigned int hash;
				for (int i = 0; i < w.verts.size(); i++)
				{
					// Verts
					bool ignorePush = false;
					hash = hashVec3(w.verts[i].x, w.verts[i].y, w.verts[i].z);
					for (int j = 0; j < uVertHash[hash].size(); j++)
					{
						if (uVertHash[hash][j].w.x == w.verts[i].x && uVertHash[hash][j].w.y == w.verts[i].y && uVertHash[hash][j].w.z == w.verts[i].z)
						{
							ignorePush = true;
							break;
						}
					}

					if (!ignorePush)
					{
						uVertHash[hash].push_back({ w.verts[i], (unsigned int)uverts.size() });
						uverts.push_back(w.verts[i]);
					}

					// Normals
					ignorePush = false;
					hash = hashVec3(w.verts[i].nX, w.verts[i].nY, w.verts[i].nZ);
					for (int j = 0; j < uNormHash[hash].size(); j++)
					{
						if (uNormHash[hash][j].w.nX == w.verts[i].nX && uNormHash[hash][j].w.nY == w.verts[i].nY && uNormHash[hash][j].w.nZ == w.verts[i].nZ)
						{
							ignorePush = true;
							break;
						}
					}

					if (!ignorePush)
					{
						uNormHash[hash].push_back({ w.verts[i], (unsigned int)unorms.size() });
						unorms.push_back(w.verts[i]);
					}

					// UVs
					ignorePush = false;
					hash = hashVec2(w.verts[i].u, w.verts[i].v);
					for (int j = 0; j < uUVHash[hash].size(); j++)
					{
						if (uUVHash[hash][j].w.u == w.verts[i].u && uUVHash[hash][j].w.v == w.verts[i].v)
						{
							ignorePush = true;
							break;
						}
					}

					if (!ignorePush)
					{
						uUVHash[hash].push_back({ w.verts[i], (unsigned int)uuvs.size() });
						uuvs.push_back(w.verts[i]);
					}
				}

#pragma endregion

				for (int i = 0; i < w.verts.size(); i++)
				{
					// Vertex Indices
					bool ignorePush = false;
					hash = hashVec3(w.verts[i].x, w.verts[i].y, w.verts[i].z);
					for (int j = 0; j < uVertHash[hash].size(); j++)
					{
						if (uVertHash[hash][j].w.x == w.verts[i].x && uVertHash[hash][j].w.y == w.verts[i].y && uVertHash[hash][j].w.z == w.verts[i].z)
						{
							w.indices.push_back(uVertHash[hash][j].ind);
							ignorePush = true;
							break;
						}
					}

					//if (!ignorePush)
					//	w.indices.push_back(i);

					// Normal Indices
					ignorePush = false;
					hash = hashVec3(w.verts[i].nX, w.verts[i].nY, w.verts[i].nZ);
					for (int j = 0; j < uNormHash[hash].size(); j++)
					{
						if (uNormHash[hash][j].w.nX == w.verts[i].nX && uNormHash[hash][j].w.nY == w.verts[i].nY && uNormHash[hash][j].w.nZ == w.verts[i].nZ)
						{
							w.nIndices.push_back(uNormHash[hash][j].ind);
							ignorePush = true;
							break;
						}
					}

					if (!ignorePush)
						w.nIndices.push_back(i);

					ignorePush = false;
					hash = hashVec2(w.verts[i].u, w.verts[i].v);
					for (int j = 0; j < uUVHash[hash].size(); j++)
					{
						if (uUVHash[hash][j].w.u == w.verts[i].u && uUVHash[hash][j].w.v == w.verts[i].v)
						{
							w.uvIndices.push_back(uUVHash[hash][j].ind);
							ignorePush = true;
							break;
						}
					}

					//if (!ignorePush)
					//	w.uvIndices.push_back(i);
				}

				w.verts = uverts;
				w.norms = unorms;
				w.uvs = uuvs;

				for (auto iter = w.indices.begin(); iter != w.indices.end(); iter += 3)
					std::swap(*iter, *(iter + 2));

				for (auto iter = w.nIndices.begin(); iter != w.nIndices.end(); iter += 3)
					std::swap(*iter, *(iter + 2));

				for (auto iter = w.uvIndices.begin(); iter != w.uvIndices.end(); iter += 3)
					std::swap(*iter, *(iter + 2));

				models.push_back(w);
			}
			break;
			}

		}


		for (int i = 0; i < n->GetChildCount(); i++)
			open.push(n->GetChild(i));
	}

	return true;
}

bool FBXExporter::ExportWafl(std::string _filePath)
{
	return false;
}

std::vector<WaffleModel> FBXExporter::GetModels()
{
	return models;
}

WaffleCamera* FBXExporter::GetCameras()
{
	return nullptr;
}

WaffleLight* FBXExporter::GetLights()
{
	return nullptr;
}

//FBXEporter::NodeType* FBXEporter::GetNodeTypeList()
//{
//	for (int i = 0; i < nodes.size(); i++)
//	{
//		switch (nodes[i]->GetNodeAttribute()->GetAttributeType())
//		{
//		case FbxNodeAttribute::eLight:
//			typeList.push_back(NodeType::LIGHT);
//			break;
//		case FbxNodeAttribute::eCamera:
//			typeList.push_back(NodeType::CAMERA);
//			break;
//		case FbxNodeAttribute::eMesh:
//			typeList.push_back(NodeType::MODEL);
//			break;
//		default:
//			typeList.push_back((NodeType)UINT_MAX);
//			break;
//		}
//	}
//
//	return &typeList[0];
//}

//XMMATRIX FBXEporter::GetNodeMatrix(FbxNode * _node)
//{
//	FbxDouble3 fT, fR, fS;
//
//	XMMATRIX m;
//
//	fT = _node->LclTranslation;
//	fR = _node->LclRotation;
//	fS = _node->LclScaling;
//
//	return m;
//}
