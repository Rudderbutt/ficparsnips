#include "OtterInterpolator.h"
#include "OtterMath.h"


void OtterInterpolator::InterpolateKeyframe(unsigned int current, unsigned int next, unsigned int animIndex, float deltaTime)
{
	anim[animIndex].currentFrame = anim[animIndex].keyframes[current];
	if (!anim[animIndex].keyframes[current].hasDecomposed)
	{
		DirectX::XMVECTOR currS, currR, currP;
		DirectX::XMMatrixDecompose(&currS, &currR, &currP, anim[animIndex].keyframes[current].transform);
		anim[animIndex].keyframes[current].m.pos = currP;
		anim[animIndex].keyframes[current].m.rot = currR;
		anim[animIndex].keyframes[current].m.scale = currS;
	}

	if (!anim[animIndex].keyframes[next].hasDecomposed)
	{
		DirectX::XMVECTOR nextS, nextR, nextP;
		DirectX::XMMatrixDecompose(&nextS, &nextR, &nextP, anim[animIndex].keyframes[next].transform);
		anim[animIndex].keyframes[next].m.pos = nextP;
		anim[animIndex].keyframes[next].m.rot = nextR;
		anim[animIndex].keyframes[next].m.scale = nextS;
	}

	DirectX::XMVECTOR rotate = DirectX::XMQuaternionSlerp(anim[animIndex].keyframes[current].m.rot, anim[animIndex].keyframes[next].m.rot, deltaTime);
	DirectX::XMVECTOR scale = DirectX::XMVectorLerp(anim[animIndex].keyframes[current].m.scale, anim[animIndex].keyframes[next].m.scale, deltaTime);
	DirectX::XMVECTOR translate = DirectX::XMVectorLerp(anim[animIndex].keyframes[current].m.pos, anim[animIndex].keyframes[next].m.pos, deltaTime);

	anim[animIndex].currentFrame.transform = DirectX::XMMatrixAffineTransformation(scale, DirectX::XMVectorZero(), rotate, translate);

	if (!anim[animIndex].keyframes[current].hasDecomposed)
		anim[animIndex].keyframes[current].hasDecomposed = true;

	if (!anim[animIndex].keyframes[next].hasDecomposed)
		anim[animIndex].keyframes[next].hasDecomposed = true;
}

void OtterInterpolator::GenerateAnimationFromSkeleton(WaffleSkeleton skeleton)
{
	anim.resize(skeleton.joints.size());

	for (int q = 0; q < skeleton.joints.size(); q++)
	{
		WaffleKeyframe* key = skeleton.joints[q].anim;
		if (skeleton.joints[q].name == "L_Toe_J" || skeleton.joints[q].name == "Weapon_Attach_J" || skeleton.joints[q].name == "Nose_J" || skeleton.joints[q].name == "R_Toe_J")
			continue;
		int keyCount = 0;
		while (key)
		{
			keyCount++;
			key = key->next;
		}

		anim[q].keyframes.resize(keyCount);

		for (int i = 0; i < anim[q].keyframes.size(); i++)
		{
			WaffleKeyframe* k = skeleton.joints[q].anim;
			int current = 0;
			while (current != i)
			{
				k = k->next;
				current++;
			}
			Keyframe frame;
			frame.keyNum = k->keyNum;
			frame.frameDelta = (float)k->frameDelta;
			frame.transform = OtterMath::buildTransposedXMMatrix(k->trans);
			frame.hasDecomposed = false;
			anim[q].keyframes[i] = frame;
		}

		anim[q].current = 1;
		anim[q].next = anim[q].current + 1;
		anim[q].currentFrame = anim[q].keyframes[1];

		key = skeleton.joints[q].anim;
		while (key)
		{
			WaffleKeyframe* tmp = key;
			key = key->next;
			delete tmp;
		}
	}
}

void OtterInterpolator::Process(float _deltaTime)
{
	for (int i = 0; i < anim.size(); i++)
	{
		if (!anim[i].keyframes.size())
			continue;

		float frameTween = 0;
		if (anim[i].next < anim[i].current)
			frameTween = anim[i].keyframes[anim[i].next].frameDelta;
		else
			frameTween = anim[i].keyframes[anim[i].next].frameDelta - anim[i].keyframes[anim[i].current].frameDelta;

		anim[i].elapsedTime += _deltaTime;

		while (anim[i].elapsedTime > frameTween)
		{
			anim[i].elapsedTime -= frameTween;
			anim[i].current = anim[i].next++;
			if (anim[i].current >= anim[i].keyframes.size())
			{
				anim[i].current = 1;
				anim[i].next = anim[i].current + 1;
			}
			if (anim[i].next >= anim[i].keyframes.size())
				anim[i].next = 1;
		}

		float tween = anim[i].elapsedTime / frameTween;
		InterpolateKeyframe(anim[i].current, anim[i].next, i, tween);
	}
}
