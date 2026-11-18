#include "ShapeMasksMixer.h"
#include <vector>
using namespace ffglex;
using namespace ffglqs;

enum MixerParam : unsigned int
{
    PARAM_POS_X_1 = 0,
    PARAM_POS_Y_1,
    PARAM_POS_X_2,
    PARAM_POS_Y_2,
    PARAM_BORDER_ENABLED,
    PARAM_BORDER_THICKNESS,
    PARAM_BORDER_OPACITY,
    PARAM_BORDER_1_R,
    PARAM_BORDER_1_G,
    PARAM_BORDER_1_B,
    PARAM_BORDER_2_R,
    PARAM_BORDER_2_G,
    PARAM_BORDER_2_B
};

static CFFGLPluginInfo PluginInfo(
    PluginFactory< ShapeMasksMixer >,
    "MkM2", "Mascara MultiPainel Mixer",
    2, 1, 0, 0, FF_MIXER,
    "Mascara MultiPainel Mixer: duas entradas, posicao independente e bordas opcionais",
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
uniform int BorderEnabled;
uniform float BorderThickness;
uniform float BorderOpacity;
uniform vec3 BorderColorA;
uniform vec3 BorderColorB;
in vec2 uv;
out vec4 fragColor;

bool insidePanel(vec2 p, vec2 center)
{
    vec2 local = (p - (center - vec2(0.25, 0.5))) / vec2(0.5, 1.0);
    return all(greaterThanEqual(local, vec2(0.0))) &&
           all(lessThanEqual(local, vec2(1.0)));
}

bool onBorder(vec2 local)
{
    float edge = min(min(local.x, 1.0 - local.x),
                     min(local.y, 1.0 - local.y));
    return edge <= BorderThickness;
}

void main()
{
    vec2 centerA = vec2(0.25, 0.5) + PositionA;
    vec2 centerB = vec2(0.75, 0.5) + PositionB;

    if (insidePanel(uv, centerB))
    {
        vec2 localB = (uv - (centerB - vec2(0.25, 0.5))) / vec2(0.5, 1.0);
        vec4 color = texture(InputB, localB * MaxUVB);
        if (BorderEnabled == 1 && onBorder(localB))
            color.rgb = mix(color.rgb, BorderColorB, BorderOpacity);
        fragColor = color;
    }
    else if (insidePanel(uv, centerA))
    {
        vec2 localA = (uv - (centerA - vec2(0.25, 0.5))) / vec2(0.5, 1.0);
        vec4 color = texture(InputA, localA * MaxUVA);
        if (BorderEnabled == 1 && onBorder(localA))
            color.rgb = mix(color.rgb, BorderColorA, BorderOpacity);
        fragColor = color;
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

    AddParam(ParamRange::Create("Borda Ligada", 0.0f, ParamRange::Range(0.0f, 1.0f)));
    AddParam(ParamRange::Create("Espessura da Borda", 0.006f, ParamRange::Range(0.001f, 0.05f)));
    AddParam(ParamRange::Create("Transparencia da Borda", 1.0f, ParamRange::Range(0.0f, 1.0f)));

    AddParam(ParamRange::Create("Borda Painel 1 - Vermelho", 1.0f, ParamRange::Range(0.0f, 1.0f)));
    AddParam(ParamRange::Create("Borda Painel 1 - Verde", 1.0f, ParamRange::Range(0.0f, 1.0f)));
    AddParam(ParamRange::Create("Borda Painel 1 - Azul", 1.0f, ParamRange::Range(0.0f, 1.0f)));

    AddParam(ParamRange::Create("Borda Painel 2 - Vermelho", 1.0f, ParamRange::Range(0.0f, 1.0f)));
    AddParam(ParamRange::Create("Borda Painel 2 - Verde", 1.0f, ParamRange::Range(0.0f, 1.0f)));
    AddParam(ParamRange::Create("Borda Painel 2 - Azul", 1.0f, ParamRange::Range(0.0f, 1.0f)));

    FFGLLog::LogToHost("Mascara MultiPainel Mixer: posicao X/Y e bordas opcionais");
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

    shader.Set("BorderEnabled", GetFloatParameter(PARAM_BORDER_ENABLED) >= 0.5f ? 1 : 0);
    shader.Set("BorderThickness", GetFloatParameter(PARAM_BORDER_THICKNESS));
    shader.Set("BorderOpacity", GetFloatParameter(PARAM_BORDER_OPACITY));
    shader.Set("BorderColorA",
        GetFloatParameter(PARAM_BORDER_1_R),
        GetFloatParameter(PARAM_BORDER_1_G),
        GetFloatParameter(PARAM_BORDER_1_B));
    shader.Set("BorderColorB",
        GetFloatParameter(PARAM_BORDER_2_R),
        GetFloatParameter(PARAM_BORDER_2_G),
        GetFloatParameter(PARAM_BORDER_2_B));

    quad.Draw();
    return FF_SUCCESS;
}

FFResult ShapeMasksMixer::DeInitGL()
{
    shader.FreeGLResources();
    quad.Release();
    return FF_SUCCESS;
}
