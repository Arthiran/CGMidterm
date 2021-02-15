#pragma once

#include "Graphics/Post/PostEffect.h"
#include "Graphics/LUT.h"

class ColourCorrectionEffect : public PostEffect
{
public:
	//Initializes framebuffer
	//Overrides post effect Init
	void Init(unsigned width, unsigned height) override;

	//Applies the effect to this buffer
	//Passes the previous framebuffer with the texture to apply as a parameter
	void ApplyEffect(PostEffect* buffer) override;

	//Applies the effect to the screen
	//void DrawToScreen() override;

	//Getters
	LUT3D GetEffectCube();

	//Setters
	void SetEffectCube(LUT3D _effect);

	LUT3D effectCube;
};