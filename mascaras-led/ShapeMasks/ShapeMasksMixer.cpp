#include "ShapeMasksMixer.h"
using namespace ffglex;
using namespace ffglqs;

static CFFGLPluginInfo PluginInfo(
    PluginFactory< ShapeMasksMixer >,
    "MkM2", "Mascara MultiPainel Mixer",
    2, 1, 0, 0, FF_MIXER,
    "Mascara MultiPainel Mixer: teste de duas entradas independentes",
    "@leandrone_x"
);

static const char _vertexShaderCode[] = R"(#version 410 core
layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec2 vUV;
out vec2 uv;
void main() { gl_Position = vPosition; uv = vUV; }
)";

static const char _fragmentShaderCode[] = R"(#version 410 core
uniform sampler2D InputA;
uniform sampler2D InputB;
uniform vec2 MaxUVA;
uniform vec2 MaxUVB;
in vec2 uv;
out vec4 fragColor;
void main()
{
    fragColor = (uv.x < 0.5)
        ? texture(InputA, uv * MaxUVA)
        : texture(InputB, uv * MaxUVB);
}
)";

ShapeMasksMixer::ShapeMasksMixer()
{
    SetMinInputs(2);
    SetMaxInputs(2);
    FFGLLog::LogToHost("Mascara MultiPainel Mixer: duas entradas prontas");
}
ShapeMasksMixer::~ShapeMasksMixer() {}

FFResult ShapeMasksMixer::InitGL(const FFGLViewportStruct* vp)
{
    if (!shader.Compile(_vertexShaderCode, _fragmentShaderCode)) { DeInitGL(); return FF_FAIL; }
    if (!quad.Initialise()) { DeInitGL(); return FF_FAIL; }
    return CFFGLPlugin::InitGL(vp);
}

FFResult ShapeMasksMixer::ProcessOpenGL(ProcessOpenGLStruct* pGL)
{
    if (pGL->numInputTextures < 2 || pGL->inputTextures[0] == nullptr || pGL->inputTextures[1] == nullptr)
        return FF_FAIL;

    ScopedShaderBinding shaderBinding(shader.GetGLID());
    ScopedSamplerActivation activateSampler0(0);
    Scoped2DTextureBinding textureBinding0(pGL->inputTextures[0]->Handle);
    ScopedSamplerActivation activateSampler1(1);
    Scoped2DTextureBinding textureBinding1(pGL->inputTextures[1]->Handle);

    shader.Set("InputA", 0);
    shader.Set("InputB", 1);
    FFGLTexCoords maxCoords = GetMaxGLTexCoords(*pGL->inputTextures[0]);
    shader.Set("MaxUVA", maxCoords.s, maxCoords.t);
    maxCoords = GetMaxGLTexCoords(*pGL->inputTextures[1]);
    shader.Set("MaxUVB", maxCoords.s, maxCoords.t);

    quad.Draw();
    return FF_SUCCESS;
}

FFResult ShapeMasksMixer::DeInitGL()
{
    shader.FreeGLResources();
    quad.Release();
    return FF_SUCCESS;
}
