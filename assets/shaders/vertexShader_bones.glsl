// Vertex shader
#version 420

// Coming in from VAO
in vec4 vPosition;
in vec4 vColour;
in vec4 vNormal;
in vec4 vUV2;
in vec4 vTangent;
in vec4 vBiNormal;
in vec4 vBoneID;
in vec4 vBoneWeight;

// Out to the geometry shader
out vec4 gColour;
out vec4 gNormal;
out vec4 gWorldLocation;
out vec4 gUV2;
out vec4 gTangent;
out vec4 gBiNormal;
out vec4 gBoneID;
out vec4 gBoneWeight;

// Uniforms
uniform mat4 Model;
uniform mat4 ModelInverse;
uniform mat4 View;
uniform mat4 Projection;

bool hasBones;

// BoneMatrix is Transforms, Rotations, and Scale
uniform mat4 BoneMatrices[52];

// Only include Rotations, for updating the normals
uniform mat4 BoneRotationMatrices[52];

void main()
{
	vec3 vertPosition = vPosition.xyz;

	// MVP
	mat4 MVP = Projection * View * Model;

	// Bones

	if (hasBones) {

 		mat4 boneTransform = BoneMatrices[int(vBoneID[0])] * vBoneWeight[0];
		boneTransform += BoneMatrices[int(vBoneID[1])] * vBoneWeight[1];
		boneTransform += BoneMatrices[int(vBoneID[2])] * vBoneWeight[2];
		boneTransform += BoneMatrices[int(vBoneID[3])] * vBoneWeight[3];
		vec4 position = boneTransform * vPosition;
	}

	gl_Position = MVP * vec4(vertPosition, 1.f);

	// World Location
	gWorldLocation.xyz = (Model * vec4(vPosition)).xyz;
	gWorldLocation.w = 1.f;

	gNormal.xyz = normalize(ModelInverse * vec4(vNormal.xyz, 1.f)).xyz;
	gNormal.w = 1.f;

	// Copy over to the Geometry Shader
	gColour = vColour;
	gUV2 = vUV2;
	gTangent = vTangent;
	gBiNormal = vBiNormal;
	gBoneID = vBoneID;
	gBoneWeight = vBoneWeight;
}
