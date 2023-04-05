#pragma once

#include "../OpenGL.h"

#include "../cMeshInfo/cMeshInfo.h"
#include "../sCamera.h"
#include "../cVAOManager/cVAOManager.h"
#include "../cBasicTextureManager/cBasicTextureManager.h"

#include <vector>
#include <glm/mat4x4.hpp>

// Draw function
void DrawMesh(cMeshInfo* currentMesh,
    glm::mat4 model,
    GLuint shaderID,
    cBasicTextureManager* TextureManager,
    cVAOManager* VAOManager,
    sCamera* camera,
    GLint modelULoc,
    GLint modelInverseULoc,
    GLuint BoneMatricesLocation[4],
    GLuint BoneRotationMatricesLocation[4]
);

void ReadSceneDescription(std::vector<cMeshInfo*>& meshArray);