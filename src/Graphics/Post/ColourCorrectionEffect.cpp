#include "ColourCorrectionEffect.h"

void ColourCorrectionEffect::Init(unsigned width, unsigned height)
{
	effectCube.loadFromFile("cubes/neutral.cube");
	int index = int(_buffers.size());
	_buffers.push_back(new Framebuffer());
	_buffers[index]->AddColorTarget(GL_RGBA8);
	_buffers[index]->AddDepthTarget();
	_buffers[index]->Init(width, height);

	index = int(_shaders.size());
	_shaders.push_back(Shader::Create());
	_shaders[index]->LoadShaderPartFromFile("shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
	_shaders[index]->LoadShaderPartFromFile("shaders/Post/colour_correction_frag.glsl", GL_FRAGMENT_SHADER);
	_shaders[index]->Link();

	PostEffect::Init(width, height);
}

void ColourCorrectionEffect::ApplyEffect(PostEffect* buffer)
{
	BindShader(0);
	buffer->BindColorAsTexture(0, 0, 0);
	effectCube.bind(30);
	_buffers[0]->RenderToFSQ();
	effectCube.unbind(30);
	buffer->UnbindTexture(0);
	UnbindShader();
}

/*void ColourCorrectionEffect::DrawToScreen()
{
	BindShader(0);
	BindColorAsTexture(0, 0, 0);
	//neutralCube.bind(30);
	_buffers[0]->DrawFullscreenQuad();
	//neutralCube.unbind(30);
	UnbindTexture(0);
	UnbindShader();
}*/

LUT3D ColourCorrectionEffect::GetEffectCube()
{
	return effectCube;
}

void ColourCorrectionEffect::SetEffectCube(LUT3D _effect)
{
	effectCube = _effect;
}
