#pragma once
#include <string>
#include <FFGLSDK.h>

class ShapeMasks : public CFFGLPlugin
{
public:
	ShapeMasks();
	~ShapeMasks();

	FFResult InitGL( const FFGLViewportStruct* vp ) override;
	FFResult ProcessOpenGL( ProcessOpenGLStruct* pGL ) override;
	FFResult DeInitGL() override;

	FFResult SetFloatParameter( unsigned int index, float value ) override;
	float GetFloatParameter( unsigned int index ) override;

private:
	ffglex::FFGLShader shader;
	ffglex::FFGLScreenQuad quad;

	static const unsigned int NUM_PARAMS = 18;
	float values[ NUM_PARAMS ];
};
