#include "ShapeMasksMixer.h"
#include <vector>
using namespace ffglex;
using namespace ffglqs;

enum MixerParam : unsigned int
{
    PARAM_POS_X_1 = 0,
    PARAM_POS_Y_1,
    PARAM_POS_X_2,
    PARAM_POS_Y_2
};

static CFFGLPluginInfo PluginInfo(
    PluginFactory< ShapeMasksMixer >,
    "MkM2", "Mascara MultiPainel Mixer",
    2, 1, 0, 0, FF_MIXER,
    "Mascara MultiPainel Mixer: duas entradas com posicionamento independente",
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
uniform vec2 PositionA;
uniform vec2 PositionB;
in vec2 uv;
out vec4 fragColor;

bool insidePanel(vec2 p, vec2 center)
{
    vec2 local = (p - (center - vec2(0.25, 0.5))) / vec2(0.5, 1.0);
    return all(greaterThanEqual(local, vec2(0.0))) &&
           all(lessThanEqual(local, vec2(1.0)));
}

void main()
{
    vec2 centerA = vec2(0.25, 0.5) + PositionA;
    vec2 centerB = vec2(0.75, 0.5) + PositionB;

    if (insidePanel(uv, centerB))
    {
        vec2 localB = (uv - (centerB - vec2(0.25, 0.5))) / vec2(0.5, 1.0);
        fragColor = texture(InputB, localB * MaxUVB);
    }
    else if (insidePanel(uv, centerA))
    {
        vec2 localA = (uv - (centerA - vec2(0.25, 0.5))) / vec2(0.5, 1.0);
        fragColor = texture(InputA, localA * MaxUVA);
    }
    else
    {
        fragColor = vec4(0.0);
    }
}
)";

ShapeMasksMixer::ShapeMasksMixer()
{
    SetMinInputs(2);
    SetMaxInputs(2);

    AddParam(ParamRange::Create("Pos X Painel 1", 0.0f, ParamRange::Range(-0.5f, 0.5f)));
    AddParam(ParamRange::Create("Pos Y Painel 1", 0.0f, ParamRange::Range(-0.5f, 0.5f)));
    AddParam(ParamRange::Create("Pos X Painel 2", 0.0f, ParamRange::Range(-0.5f, 0.5f)));
    AddParam(ParamRange::Create("Pos Y Painel 2", 0.0f, ParamRange::Range(-0.5f, 0.5f)));

    FFGLLog::LogToHost("Mascara MultiPainel Mixer: posicao X/Y independente dos dois paineis");
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

    shader.Set("PositionA", GetFloatParameter(PARAM_POS_X_1), GetFloatParameter(PARAM_POS_Y_1));
    shader.Set("PositionB", GetFloatParameter(PARAM_POS_X_2), GetFloatParameter(PARAM_POS_Y_2));

    quad.Draw();
    return FF_SUCCESS;
}

FFResult ShapeMasksMixer::DeInitGL()
{
    shader.FreeGLResources();
    quad.Release();
    return FF_SUCCESS;
}
