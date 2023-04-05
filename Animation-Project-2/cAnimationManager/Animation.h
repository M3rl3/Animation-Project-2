#pragma once

#include <vector>
#include <map>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <string>

#include "BoneHierarchy.h"

enum EasingType
{
	EaseIn,
	EaseOut,
	EaseInOut,
	None
};

struct BoneInfo
{
	std::string name;
	glm::mat4 boneOffset;
	glm::mat4 globalTransformation;
	glm::mat4 finalTransformation; // <- For rendering
};

struct BoneVertexData
{
	BoneVertexData()
	{
		ids[0] = 0;
		ids[1] = 0;
		ids[2] = 0;
		ids[3] = 0;
		weights[0] = 0.f;
		weights[1] = 0.f;
		weights[2] = 0.f;
		weights[3] = 0.f;
	}

	unsigned int ids[4];
	float weights[4];

	void AddBoneInfo(int id, float weight)
	{
		int numIds = sizeof(ids) / sizeof(ids[0]);
		for (int i = 0; i < numIds; i++)
		{
			if (weights[i] == 0.f)
			{
				ids[i] = id;
				weights[i] = weight;
				return;
			}
		}

		assert(0);
	}
};

struct PositionKeyFrame
{
	PositionKeyFrame()
		: value(0.f), time(0.f), type(None) { }

	PositionKeyFrame(glm::vec3 value, float time, EasingType type = None)
		: value(value), time(time), type(type) { }

	glm::vec3 value;
	float time;
	EasingType type;
};

struct ScaleKeyFrame
{
	ScaleKeyFrame()
		: value(0.f), time(0.f), type(None) { }

	ScaleKeyFrame(glm::vec3 value, float time, EasingType type = None)
		: value(value), time(time), type(type) { }

	glm::vec3 value;
	float time;
	EasingType type;
};

struct RotationKeyFrame
{
	RotationKeyFrame()
		: value(1.0f, 0.f, 0.f, 0.f), time(0.f), useSlerp(true), type(None) { }

	RotationKeyFrame(glm::quat value, float time, bool useSlerp, EasingType type = None)
		: value(value), time(time), useSlerp(useSlerp), type(type) { }

	glm::quat value;
	float time;
	bool useSlerp;
	EasingType type;
};

struct NullKeyFrame 
{
	NullKeyFrame()
		: data(""), time(0.f) { }

	NullKeyFrame(std::string data, float time) 
		: data(data), time(time) { }

	std::string data;
	float time;
};

struct AnimationData
{
	std::vector<PositionKeyFrame> PositionKeyFrames;
	std::vector<ScaleKeyFrame> ScaleKeyFrames;
	std::vector<RotationKeyFrame> RotationKeyFrames;
	std::vector<NullKeyFrame> NullKeyFrames;
	std::string Name;
	double Duration;
	double TicksPerSecond;
};

class aiScene;

struct CharacterAnimationData
{
	CharacterAnimationData(const aiScene* scene) : AIScene(scene) { }
	const aiScene* AIScene;											 

	BoneHierarchy* BoneHierarchy;			// Utilized for Animation 

	std::vector<BoneInfo> boneInfoVec;
	std::map<std::string, int> boneNameToIdMap;

	std::vector<AnimationData*> Channels;
	std::string Name;
	double Duration;
	double TicksPerSecond;
};

struct Animation
{
	std::string AnimationType;
	float AnimationTime;
	bool IsPlaying;
	bool IsLooping;
	float Speed;

	EasingType currentEasing;
};