#include "MovPanels.h"
#include <math.h>
using namespace ffglex;

enum ParamType : FFUInt32
{
	PT_PANELS,
	PT_GAP,
	PT_ARCH,
	PT_ARCH_HEIGHT,
	PT_SOFT,
	PT_CONTENT,
	PT_MOTION,
	PT_SPEED,
	PT_STAGGER,
	PT_OFFSET,
	PT_ZOOM,
	PT_MIRROR,
	PT_BG_RED,
	PT_BG_GREEN,
	PT_BG_BLUE,
	PT_BG_ALPHA,
	PT_COUNT
};

static CFFGLPluginInfo PluginInfo(
	PluginFactory< MovPanels >,
	"MvPn",
	"Mov Panels",
	2,
	1,
	1,
	0,
	FF_EFFECT,
	"Paineis (retos ou em arco) com movimento independente, para LED",
	"MovPanels"
);

static const char _vertexShaderCode[] = R"(#version 410 core
layout( location = 0 ) in vec4 vPosition;
layout( location = 1 ) in vec2 vUV;

out vec2 uv;

void main()
{
	gl_Position = vPosition;
	uv = vUV;
}
)";

static const char _fragmentShaderCode[] = R"(#version 410 core
uniform sampler2D InputTexture;
uniform vec2 MaxUV;
uniform vec2 Res;
uniform float Time;

uniform float Panels;
uniform float Gap;
uniform float ArchOn;
uniform float ArchStart;
uniform float Soft;
uniform int   Content;
uniform int   Motion;
uniform float Speed;
uniform float Stagger;
uniform float Offset;
uniform float Zoom;
uniform float MirrorOn;
uniform vec4  BgColor; // premultiplied

in vec2 uv;
out vec4 fragColor;

float wrapT( float t, float mir )
{
	float m = 1.0 - abs( 1.0 - mod( t, 2.0 ) );
	return mix( fract( t ), m, step( 0.5, mir ) );
}

void main()
{
	float cols  = max( 1.0, floor( Panels + 0.5 ) );
	float x     = uv.x * cols;
	float idx   = floor( x );
	float local = fract( x );

	float cellW = Res.x / cols;
	float pw    = cellW * ( 1.0 - Gap );
	float r     = pw * 0.5;
	float px    = ( local - 0.5 ) * cellW;
	float py    = uv.y * Res.y;

	float d;
	if( ArchOn > 0.5 )
	{
		float yc = min( ArchStart * Res.y, Res.y - r );
		yc       = max( yc, r );
		if( py > yc )
			d = r - length( vec2( px, py - yc ) );
		else
			d = r - abs( px );
		d = min( d, py );
	}
	else
	{
		d = min( min( r - abs( px ), py ), Res.y - py );
	}
	float edge = max( Soft * r, 0.0 ) + 1.0;
	float a    = smoothstep( 0.0, edge, d );

	vec2 p;
	if( Content == 0 )
		p = vec2( px / pw + 0.5, uv.y );
	else
		p = uv;
	p = ( p - 0.5 ) / Zoom + 0.5;

	float ph = idx * Stagger;
	float t  = Time * Speed + Offset;
	vec2 mv  = vec2( 0.0 );
	if( Motion == 0 )
		mv.y = t + ph;
	else if( Motion == 1 )
		mv.y = -( t + ph );
	else if( Motion == 2 )
	{
		float dir = ( mod( idx, 2.0 ) < 0.5 ) ? 1.0 : -1.0;
		mv.y      = dir * t + ph;
	}
	else if( Motion == 3 )
		mv.y = sin( Time * Speed * 3.14159 + idx * ( 0.4 + Stagger * 3.0 ) ) * 0.5 + Offset;
	else
	{
		float dir = ( mod( idx, 2.0 ) < 0.5 ) ? 1.0 : -1.0;
		mv.x      = dir * t + ph;
	}
	p += mv;
	p = vec2( wrapT( p.x, MirrorOn ), wrapT( p.y, MirrorOn ) );

	// InputTexture is premultiplied; output must be premultiplied too.
	vec4 img = texture( InputTexture, p * MaxUV );
	fragColor = mix( BgColor, img, a );
}
)";

MovPanels::MovPanels()
{
	// defaults (normalised 0..1 host values)
	values[ PT_PANELS ]       = 4.0f / 11.0f;// 5 panels
	values[ PT_GAP ]          = 0.12f / 0.8f;
	values[ PT_ARCH ]         = 1.0f;
	values[ PT_ARCH_HEIGHT ]  = 0.6f;
	values[ PT_SOFT ]         = 0.02f;
	values[ PT_CONTENT ]      = 0.0f;
	values[ PT_MOTION ]       = 2.0f;
	values[ PT_SPEED ]        = ( 0.25f + 2.0f ) / 4.0f;
	values[ PT_STAGGER ]      = 0.15f;
	values[ PT_OFFSET ]       = 0.5f;
	values[ PT_ZOOM ]         = 0.5f;// 1.0x
	values[ PT_MIRROR ]       = 0.0f;
	values[ PT_BG_RED ]       = 0.0f;
	values[ PT_BG_GREEN ]     = 0.0f;
	values[ PT_BG_BLUE ]      = 0.0f;
	values[ PT_BG_ALPHA ]     = 1.0f;

	SetMinInputs( 1 );
	SetMaxInputs( 1 );

	SetParamInfof( PT_PANELS, "Panels", FF_TYPE_STANDARD );
	SetParamInfof( PT_GAP, "Gap", FF_TYPE_STANDARD );
	SetParamInfo( PT_ARCH, "Arch", FF_TYPE_BOOLEAN, true );
	SetParamInfof( PT_ARCH_HEIGHT, "Arch Height", FF_TYPE_STANDARD );
	SetParamInfof( PT_SOFT, "Edge Softness", FF_TYPE_STANDARD );

	SetOptionParamInfo( PT_CONTENT, "Content", 2, values[ PT_CONTENT ] );
	SetParamElementInfo( PT_CONTENT, 0, "Full image per panel", 0.0f );
	SetParamElementInfo( PT_CONTENT, 1, "Continuous slice", 1.0f );

	SetOptionParamInfo( PT_MOTION, "Motion", 5, values[ PT_MOTION ] );
	SetParamElementInfo( PT_MOTION, 0, "Up", 0.0f );
	SetParamElementInfo( PT_MOTION, 1, "Down", 1.0f );
	SetParamElementInfo( PT_MOTION, 2, "Alternate Up/Down", 2.0f );
	SetParamElementInfo( PT_MOTION, 3, "Wave", 3.0f );
	SetParamElementInfo( PT_MOTION, 4, "Horizontal", 4.0f );

	SetParamInfof( PT_SPEED, "Speed", FF_TYPE_STANDARD );
	SetParamInfof( PT_STAGGER, "Stagger", FF_TYPE_STANDARD );
	SetParamInfof( PT_OFFSET, "Manual Position", FF_TYPE_STANDARD );
	SetParamInfof( PT_ZOOM, "Zoom", FF_TYPE_STANDARD );
	SetParamInfo( PT_MIRROR, "Mirror Edges", FF_TYPE_BOOLEAN, false );

	SetParamInfof( PT_BG_RED, "Background Red", FF_TYPE_RED );
	SetParamInfof( PT_BG_GREEN, "Background Green", FF_TYPE_GREEN );
	SetParamInfof( PT_BG_BLUE, "Background Blue", FF_TYPE_BLUE );
	SetParamInfof( PT_BG_ALPHA, "Background Alpha", FF_TYPE_ALPHA );

	FFGLLog::LogToHost( "Created MovPanels effect" );
}

MovPanels::~MovPanels()
{
}

FFResult MovPanels::InitGL( const FFGLViewportStruct* vp )
{
	if( !shader.Compile( _vertexShaderCode, _fragmentShaderCode ) )
	{
		DeInitGL();
		return FF_FAIL;
	}
	if( !quad.Initialise() )
	{
		DeInitGL();
		return FF_FAIL;
	}
	return CFFGLPlugin::InitGL( vp );
}

FFResult MovPanels::ProcessOpenGL( ProcessOpenGLStruct* pGL )
{
	if( pGL->numInputTextures < 1 || pGL->inputTextures[ 0 ] == NULL )
		return FF_FAIL;

	ScopedShaderBinding shaderBinding( shader.GetGLID() );
	ScopedSamplerActivation activateSampler( 0 );
	Scoped2DTextureBinding textureBinding( pGL->inputTextures[ 0 ]->Handle );

	shader.Set( "InputTexture", 0 );

	FFGLTexCoords maxCoords = GetMaxGLTexCoords( *pGL->inputTextures[ 0 ] );
	shader.Set( "MaxUV", maxCoords.s, maxCoords.t );
	shader.Set( "Res", (float)currentViewport.width, (float)currentViewport.height );
	shader.Set( "Time", (float)fmod( hostTime, 100000.0 ) );

	shader.Set( "Panels", 1.0f + floorf( values[ PT_PANELS ] * 11.0f + 0.5f ) );
	shader.Set( "Gap", values[ PT_GAP ] * 0.8f );
	shader.Set( "ArchOn", values[ PT_ARCH ] > 0.5f ? 1.0f : 0.0f );
	shader.Set( "ArchStart", values[ PT_ARCH_HEIGHT ] );
	shader.Set( "Soft", values[ PT_SOFT ] );
	shader.Set( "Content", (int)( values[ PT_CONTENT ] + 0.5f ) );
	shader.Set( "Motion", (int)( values[ PT_MOTION ] + 0.5f ) );
	shader.Set( "Speed", values[ PT_SPEED ] * 4.0f - 2.0f );
	shader.Set( "Stagger", values[ PT_STAGGER ] );
	shader.Set( "Offset", values[ PT_OFFSET ] * 2.0f - 1.0f );
	shader.Set( "Zoom", 0.25f * powf( 16.0f, values[ PT_ZOOM ] ) );
	shader.Set( "MirrorOn", values[ PT_MIRROR ] > 0.5f ? 1.0f : 0.0f );

	const float a = values[ PT_BG_ALPHA ];
	shader.Set( "BgColor", values[ PT_BG_RED ] * a, values[ PT_BG_GREEN ] * a, values[ PT_BG_BLUE ] * a, a );

	quad.Draw();
	return FF_SUCCESS;
}

FFResult MovPanels::DeInitGL()
{
	shader.FreeGLResources();
	quad.Release();
	return FF_SUCCESS;
}

FFResult MovPanels::SetFloatParameter( unsigned int index, float value )
{
	if( index >= PT_COUNT )
		return FF_FAIL;
	values[ index ] = value;
	return FF_SUCCESS;
}

float MovPanels::GetFloatParameter( unsigned int index )
{
	if( index >= PT_COUNT )
		return 0.0f;
	return values[ index ];
}
