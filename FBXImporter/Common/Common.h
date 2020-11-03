/****************************************************************************************

   Copyright (C) 2015 Autodesk, Inc.
   All rights reserved.

   Use of this software is subject to the terms of the Autodesk license agreement
   provided at the time of installation or download, or which otherwise accompanies
   this software in either electronic or hard copy form.

****************************************************************************************/
#ifndef _COMMON_H
#define _COMMON_H

#if defined(DEBUG) | defined(_DEBUG)
    #undef DEBUG_NEW
    #undef new
#endif

#include <fbxsdk.h>

void InitializeSdkObjects(FbxManager*& pManager, FbxScene*& pScene);
void DestroySdkObjects(FbxManager* pManager, bool pExitStatus);
void CreateAndFillIOSettings(FbxManager* pManager);

bool SaveScene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename, int pFileFormat=-1, bool pEmbedMedia=false);
bool LoadScene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename);

#if defined(DEBUG) | defined(_DEBUG)
    #define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__)
    #define new DEBUG_NEW 
#endif

#endif // #ifndef _COMMON_H
