#pragma once
#include <FFGLSDK.h>
#include <string>

class ShapeMasksMixer : public ffglqs::Plugin
{
public:
    ShapeMasksMixer();
    ~ShapeMasksMixer();
    FFResult InitGL(const FFGLViewportStruct* vp) override;
    FFResult ProcessOpenGL(ProcessOpenGLStruct* pGL) override;
    FFResult DeInitGL() override;
    const char* GetShortName() override { return "MkM2"; }
private:
    ffglex::FFGLShader shader;
    ffglex::FFGLScreenQuad quad;
};
