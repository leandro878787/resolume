#include "ShapeMasks.h"
#include <math.h>
#include <stdio.h>
using namespace ffglex;

enum ParamType : FFUInt32
{
	PT_SHAPE,
	PT_COLUMNS,
	PT_ROWS,
	PT_GAP,
	PT_ROUNDNESS,
	PT_ARCH_HEIGHT,
	PT_POINTS,
	PT_FATNESS,
	PT_ROTATION,
	PT_OFFSET_ROWS,
	PT_FLIP_ALT,
	PT_INVERT,
	PT_SOFT,
	PT_CONTENT,
	PT_BG_RED,
	PT_BG_GREEN,
	PT_BG_BLUE,
	PT_BG_ALPHA,
	PT_COUNT,
	PT_PANEL_SELECT = PT_COUNT,
	PT_PANEL_DATA_START,
	PT_PANEL_DATA_END = PT_PANEL_DATA_START + 96 * 4,
	PT_BORDER_ENABLED,
	PT_BORDER_THICKNESS,
	PT_BORDER_OPACITY,
	PT_BORDER_RED,
	PT_BORDER_GREEN,
	PT_BORDER_BLUE,
	PT_TOTAL_PARAMS
};

static CFFGLPluginInfo PluginInfo(
	PluginFactory< ShapeMasks >,
	"MkSh",
	"Mascara MultiPainel",
	2,
	1,
	1,
	0,
	FF_EFFECT,
	"Mascara MultiPainel: mascaras geometricas para video e paineis de LED",
	"ShapeMasks"
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
uniform vec2  MaxUV;
uniform vec2  Res;

uniform int   Shape;
uniform float Cols;
uniform float Rows;
uniform float Gap;
uniform float Roundness;
uniform float ArchHeight;
uniform float Points;
uniform float Fatness;
uniform float Rotation;
uniform float OffsetRows;
uniform float FlipAlt;
uniform float Invert;
uniform float Soft;
uniform int   Content;
uniform vec4  BgColor; // premultiplied
uniform vec2  PanelScale[96];
uniform vec2  PanelPosition[96];
uniform int   BorderEnabled;
uniform float BorderThickness;
uniform float BorderOpacity;
uniform vec3  BorderColor;

in vec2 uv;
out vec4 fragColor;

float sdTriangle( vec2 p, vec2 p0, vec2 p1, vec2 p2 )
{
	vec2 e0 = p1 - p0, e1 = p2 - p1, e2 = p0 - p2;
	vec2 v0 = p - p0, v1 = p - p1, v2 = p - p2;
	vec2 pq0 = v0 - e0 * clamp( dot( v0, e0 ) / dot( e0, e0 ), 0.0, 1.0 );
	vec2 pq1 = v1 - e1 * clamp( dot( v1, e1 ) / dot( e1, e1 ), 0.0, 1.0 );
	vec2 pq2 = v2 - e2 * clamp( dot( v2, e2 ) / dot( e2, e2 ), 0.0, 1.0 );
	float s = sign( e0.x * e2.y - e0.y * e2.x );
	vec2 d = min( min( vec2( dot( pq0, pq0 ), s * ( v0.x * e0.y - v0.y * e0.x ) ),
	                   vec2( dot( pq1, pq1 ), s * ( v1.x * e1.y - v1.y * e1.x ) ) ),
	              vec2( dot( pq2, pq2 ), s * ( v2.x * e2.y - v2.y * e2.x ) ) );
	return -sqrt( d.x ) * sign( d.y );
}

float sdHexagon( vec2 p, float r )
{
	const vec3 k = vec3( -0.866025404, 0.5, 0.577350269 );
	p = abs( p );
	p -= 2.0 * min( dot( k.xy, p ), 0.0 ) * k.xy;
	p -= vec2( clamp( p.x, -k.z * r, k.z * r ), r );
	return length( p ) * sign( p.y );
}

float sdStar( vec2 p, float r, float n, float m )
{
	float an  = 3.141593 / n;
	float en  = 3.141593 / m;
	vec2  acs = vec2( cos( an ), sin( an ) );
	vec2  ecs = vec2( cos( en ), sin( en ) );
	float bn  = mod( atan( p.x, p.y ), 2.0 * an ) - an;
	p         = length( p ) * vec2( cos( bn ), abs( sin( bn ) ) );
	p -= r * acs;
	p += ecs * clamp( -dot( p, ecs ), 0.0, r * acs.y / ecs.y );
	return length( p ) * sign( p.x );
}

// distancia ate a borda da forma (positivo = dentro), em pixels. h = meia largura/altura da forma
float shapeDist( vec2 p, vec2 h )
{
	float mn = min( h.x, h.y );
	if( Shape == 0 ) // retangulo
	{
		return min( h.x - abs( p.x ), h.y - abs( p.y ) );
	}
	else if( Shape == 1 ) // retangulo arredondado
	{
		float r = Roundness * mn;
		vec2  q = abs( p ) - ( h - r );
		return -( length( max( q, 0.0 ) ) + min( max( q.x, q.y ), 0.0 ) - r );
	}
	else if( Shape == 2 ) // arco
	{
		float r  = h.x;
		float sy = 2.0 * h.y;
		float py = p.y + h.y;
		float yc = clamp( ArchHeight * sy, 0.0, max( sy - r, 0.0 ) );
		float d;
		if( py > yc )
			d = r - length( vec2( p.x, py - yc ) );
		else
			d = r - abs( p.x );
		return min( min( d, py ), sy - py );
	}
	else if( Shape == 3 ) // circulo
	{
		return mn - length( p );
	}
	else if( Shape == 4 ) // oval
	{
		return ( 1.0 - length( p / h ) ) * mn;
	}
	else if( Shape == 5 ) // triangulo para cima
	{
		return -sdTriangle( p, vec2( -h.x, -h.y ), vec2( h.x, -h.y ), vec2( 0.0, h.y ) );
	}
	else if( Shape == 6 ) // triangulo para baixo
	{
		return -sdTriangle( p, vec2( -h.x, h.y ), vec2( h.x, h.y ), vec2( 0.0, -h.y ) );
	}
	else if( Shape == 7 ) // losango
	{
		return ( h.x * h.y - h.y * abs( p.x ) - h.x * abs( p.y ) ) / length( h );
	}
	else if( Shape == 8 ) // hexagono
	{
		return -sdHexagon( p, min( h.y, h.x * 0.8660254 ) );
	}
	else if( Shape == 9 ) // estrela
	{
		float n = floor( Points + 0.5 );
		float m = mix( 2.0, n, Fatness );
		return -sdStar( p, mn, n, m );
	}
	else // cruz
	{
		float d1 = min( h.x - abs( p.x ), h.y * 0.34 - abs( p.y ) );
		float d2 = min( h.x * 0.34 - abs( p.x ), h.y - abs( p.y ) );
		return max( d1, d2 );
	}
}

void main()
{
	float cols = max( 1.0, floor( Cols + 0.5 ) );
	float rows = max( 1.0, floor( Rows + 0.5 ) );

	vec2 maskUV = uv;
	float yg  = maskUV.y * rows;
	float row = floor( yg );
	float xg  = maskUV.x * cols;
	if( OffsetRows > 0.5 && mod( row, 2.0 ) > 0.5 )
		xg += 0.5;
	float col = floor( xg );

	vec2 cell  = vec2( Res.x / cols, Res.y / rows );
	vec2 local = vec2( fract( xg ) - 0.5, fract( yg ) - 0.5 ) * cell; // pixels, centro da celula = 0
	int panelRow = int( clamp( row, 0.0, rows - 1.0 ) );
	int panelCol = int( clamp( col, 0.0, cols - 1.0 ) );
	int panelIndex = clamp( panelRow * int( cols ) + panelCol, 0, 95 );
	vec2 panelScale = PanelScale[ panelIndex ];
	vec2 panelPosition = PanelPosition[ panelIndex ];
	local -= panelPosition * Res;
	vec2 sz    = cell * ( 1.0 - Gap ) * panelScale;
	vec2 h     = sz * 0.5;

	vec2 q = local;
	if( FlipAlt > 0.5 && mod( col + row, 2.0 ) > 0.5 )
		q.y = -q.y;
	float ang = Rotation * 6.2831853;
	float cs = cos( ang ), sn = sin( ang );
	q = vec2( cs * q.x - sn * q.y, sn * q.x + cs * q.y );

	float d    = shapeDist( q, h );
	float edge = Soft * min( h.x, h.y ) * 0.5 + 1.0;
	float a    = smoothstep( 0.0, edge, d );
	if( Invert > 0.5 )
		a = 1.0 - a;
	float insideFrame = step( 0.0, maskUV.x ) * step( maskUV.x, 1.0 )
	                  * step( 0.0, maskUV.y ) * step( maskUV.y, 1.0 );
	a *= insideFrame;

	vec2 suv = uv;
	if( Content == 1 )
		suv = local / sz + 0.5;

	// InputTexture e premultiplicada; a saida tambem precisa ser.
	vec4 img = texture( InputTexture, suv * MaxUV );

	if( BorderEnabled == 1 && BorderThickness > 0.0 )
	{
		float borderInner = smoothstep( 0.0, edge, d );
		float borderOuter = 1.0 - smoothstep( BorderThickness, BorderThickness + edge, d );
		float borderMask = borderInner * borderOuter;
		vec4 borderColor = vec4( BorderColor * img.a, img.a );
		img = mix( img, borderColor, borderMask * BorderOpacity );
	}
	fragColor = mix( BgColor, img, a );
}
)";

static const char* ShapeNames[] = {
	"Retangulo", "Retangulo Arredondado", "Arco", "Circulo", "Oval", "Triangulo para Cima",
	"Triangulo para Baixo", "Losango", "Hexagono", "Estrela", "Cruz"
};

ShapeMasks::ShapeMasks()
{
	values[ PT_SHAPE ]       = 2.0f;// Arch
	values[ PT_COLUMNS ]     = 5.0f;
	values[ PT_ROWS ]        = 1.0f;
	values[ PT_GAP ]         = 0.15f;
	values[ PT_ROUNDNESS ]   = 0.4f;
	values[ PT_ARCH_HEIGHT ] = 0.6f;
	values[ PT_POINTS ]      = 5.0f;
	values[ PT_FATNESS ]     = 0.33f;
	values[ PT_ROTATION ]    = 0.0f;
	values[ PT_OFFSET_ROWS ] = 0.0f;
	values[ PT_FLIP_ALT ]    = 0.0f;
	values[ PT_INVERT ]      = 0.0f;
	values[ PT_SOFT ]        = 0.02f;
	values[ PT_CONTENT ]     = 0.0f;
	values[ PT_BG_RED ]      = 0.0f;
	values[ PT_BG_GREEN ]    = 0.0f;
	values[ PT_BG_BLUE ]     = 0.0f;
	values[ PT_BG_ALPHA ]    = 1.0f;
	values[ PT_PANEL_SELECT ] = 1.0f;
	values[ PT_BORDER_ENABLED ] = 0.0f;
	values[ PT_BORDER_THICKNESS ] = 2.0f;
	values[ PT_BORDER_OPACITY ] = 1.0f;
	values[ PT_BORDER_RED ] = 1.0f;
	values[ PT_BORDER_GREEN ] = 1.0f;
	values[ PT_BORDER_BLUE ] = 1.0f;
	for( unsigned int i = PT_PANEL_DATA_START; i < PT_PANEL_DATA_END; ++i )
		values[ i ] = 1.0f;

	SetMinInputs( 1 );
	SetMaxInputs( 1 );

	SetOptionParamInfo( PT_SHAPE, "Forma", 11, values[ PT_SHAPE ] );
	for( unsigned int i = 0; i < 11; ++i )
		SetParamElementInfo( PT_SHAPE, i, ShapeNames[ i ], (float)i );

	SetOptionParamInfo( PT_COLUMNS, "Colunas", 12, values[ PT_COLUMNS ] );
	for( unsigned int i = 1; i <= 12; ++i )
	{
		char buf[ 8 ];
		snprintf( buf, sizeof( buf ), "%u", i );
		SetParamElementInfo( PT_COLUMNS, i - 1, buf, (float)i );
	}
	SetOptionParamInfo( PT_ROWS, "Linhas", 8, values[ PT_ROWS ] );
	for( unsigned int i = 1; i <= 8; ++i )
	{
		char buf[ 8 ];
		snprintf( buf, sizeof( buf ), "%u", i );
		SetParamElementInfo( PT_ROWS, i - 1, buf, (float)i );
	}

	SetParamInfof( PT_GAP, "Espacamento", FF_TYPE_STANDARD );
	SetParamInfof( PT_ROUNDNESS, "Arredondamento dos Cantos", FF_TYPE_STANDARD );
	SetParamInfof( PT_ARCH_HEIGHT, "Altura do Arco", FF_TYPE_STANDARD );

	SetOptionParamInfo( PT_POINTS, "Pontas da Estrela", 6, values[ PT_POINTS ] );
	for( unsigned int i = 3; i <= 8; ++i )
	{
		char buf[ 8 ];
		snprintf( buf, sizeof( buf ), "%u", i );
		SetParamElementInfo( PT_POINTS, i - 3, buf, (float)i );
	}
	SetParamInfof( PT_FATNESS, "Espessura da Estrela", FF_TYPE_STANDARD );
	SetParamInfof( PT_ROTATION, "Rotacao", FF_TYPE_STANDARD );

	SetParamInfo( PT_OFFSET_ROWS, "Deslocar Linhas", FF_TYPE_BOOLEAN, false );
	SetParamInfo( PT_FLIP_ALT, "Espelhar Alternadamente", FF_TYPE_BOOLEAN, false );
	SetParamInfo( PT_INVERT, "Inverter Mascara", FF_TYPE_BOOLEAN, false );
	SetParamInfof( PT_SOFT, "Suavidade das Bordas", FF_TYPE_STANDARD );

	SetOptionParamInfo( PT_CONTENT, "Conteudo", 2, values[ PT_CONTENT ] );
	SetParamElementInfo( PT_CONTENT, 0, "Video Original", 0.0f );
	SetParamElementInfo( PT_CONTENT, 1, "Ajustar dentro de cada forma", 1.0f );

	SetParamInfof( PT_BG_RED, "Fundo - Vermelho", FF_TYPE_RED );
	SetParamInfof( PT_BG_GREEN, "Fundo - Verde", FF_TYPE_GREEN );
	SetParamInfof( PT_BG_BLUE, "Fundo - Azul", FF_TYPE_BLUE );
	SetParamInfof( PT_BG_ALPHA, "Fundo - Opacidade", FF_TYPE_ALPHA );

	SetOptionParamInfo( PT_PANEL_SELECT, "Painel ativo", 96, values[ PT_PANEL_SELECT ] );
	for( unsigned int i = 0; i < 96; ++i )
	{
		char panelName[ 16 ];
		snprintf( panelName, sizeof( panelName ), "Painel %02u", i + 1 );
		SetParamElementInfo( PT_PANEL_SELECT, i, panelName, (float)( i + 1 ) );
		char widthName[ 16 ], heightName[ 16 ];
		snprintf( widthName, sizeof( widthName ), "Largura P%02u", i + 1 );
		snprintf( heightName, sizeof( heightName ), "Altura P%02u", i + 1 );
		char posXName[ 16 ], posYName[ 16 ];
		snprintf( posXName, sizeof( posXName ), "Posicao X P%02u", i + 1 );
		snprintf( posYName, sizeof( posYName ), "Posicao Y P%02u", i + 1 );
		unsigned int baseIndex = PT_PANEL_DATA_START + i * 4;
		SetParamInfo( baseIndex, widthName, FF_TYPE_STANDARD, 1.0f );
		SetParamInfo( baseIndex + 1, heightName, FF_TYPE_STANDARD, 1.0f );
		SetParamInfo( baseIndex + 2, posXName, FF_TYPE_STANDARD, 0.0f );
		SetParamInfo( baseIndex + 3, posYName, FF_TYPE_STANDARD, 0.0f );
		SetParamRange( baseIndex, 0.1f, 2.0f );
		SetParamRange( baseIndex + 1, 0.1f, 2.0f );
		SetParamRange( baseIndex + 2, -1.0f, 1.0f );
		SetParamRange( baseIndex + 3, -1.0f, 1.0f );
		SetParamGroup( baseIndex, "Tamanho individual" );
		SetParamGroup( baseIndex + 1, "Tamanho individual" );
		SetParamGroup( baseIndex + 2, "Posicao individual" );
		SetParamGroup( baseIndex + 3, "Posicao individual" );
		for( unsigned int j = 0; j < 4; ++j )
			SetParamVisibility( baseIndex + j, false, false );
	}
	SetParamInfo( PT_BORDER_ENABLED, "Ativar Borda", FF_TYPE_BOOLEAN, false );
	SetParamInfof( PT_BORDER_THICKNESS, "Espessura da Borda", FF_TYPE_STANDARD );
	SetParamRange( PT_BORDER_THICKNESS, 0.0f, 20.0f );
	SetParamInfof( PT_BORDER_OPACITY, "Opacidade da Borda", FF_TYPE_ALPHA );
	SetParamInfof( PT_BORDER_RED, "Borda - Vermelho", FF_TYPE_RED );
	SetParamInfof( PT_BORDER_GREEN, "Borda - Verde", FF_TYPE_GREEN );
	SetParamInfof( PT_BORDER_BLUE, "Borda - Azul", FF_TYPE_BLUE );
	SetParamGroup( PT_BORDER_ENABLED, "Borda dos Paineis" );
	SetParamGroup( PT_BORDER_THICKNESS, "Borda dos Paineis" );
	SetParamGroup( PT_BORDER_OPACITY, "Borda dos Paineis" );
	SetParamGroup( PT_BORDER_RED, "Borda dos Paineis" );
	SetParamGroup( PT_BORDER_GREEN, "Borda dos Paineis" );
	SetParamGroup( PT_BORDER_BLUE, "Borda dos Paineis" );

	UpdatePanelParameterVisibility( false );
	FFGLLog::LogToHost( "Efeito Mascara MultiPainel criado" );
}

ShapeMasks::~ShapeMasks()
{
}

FFResult ShapeMasks::InitGL( const FFGLViewportStruct* vp )
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

FFResult ShapeMasks::ProcessOpenGL( ProcessOpenGLStruct* pGL )
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

	shader.Set( "Shape", (int)( values[ PT_SHAPE ] + 0.5f ) );
	shader.Set( "Cols", values[ PT_COLUMNS ] );
	shader.Set( "Rows", values[ PT_ROWS ] );
	shader.Set( "Gap", values[ PT_GAP ] * 0.8f );
	shader.Set( "Roundness", values[ PT_ROUNDNESS ] );
	shader.Set( "ArchHeight", values[ PT_ARCH_HEIGHT ] );
	shader.Set( "Points", values[ PT_POINTS ] );
	shader.Set( "Fatness", values[ PT_FATNESS ] );
	shader.Set( "Rotation", values[ PT_ROTATION ] );
	shader.Set( "OffsetRows", values[ PT_OFFSET_ROWS ] > 0.5f ? 1.0f : 0.0f );
	shader.Set( "FlipAlt", values[ PT_FLIP_ALT ] > 0.5f ? 1.0f : 0.0f );
	shader.Set( "Invert", values[ PT_INVERT ] > 0.5f ? 1.0f : 0.0f );
	shader.Set( "Soft", values[ PT_SOFT ] );
	shader.Set( "Content", (int)( values[ PT_CONTENT ] + 0.5f ) );
	shader.Set( "BorderEnabled", values[ PT_BORDER_ENABLED ] > 0.5f ? 1 : 0 );
	shader.Set( "BorderThickness", values[ PT_BORDER_THICKNESS ] );
	shader.Set( "BorderOpacity", values[ PT_BORDER_OPACITY ] );
	shader.Set( "BorderColor", values[ PT_BORDER_RED ], values[ PT_BORDER_GREEN ], values[ PT_BORDER_BLUE ] );

	const float a = values[ PT_BG_ALPHA ];
	shader.Set( "BgColor", values[ PT_BG_RED ] * a, values[ PT_BG_GREEN ] * a, values[ PT_BG_BLUE ] * a, a );
	for( unsigned int i = 0; i < 96; ++i )
	{
		char uniformName[ 32 ];
		snprintf( uniformName, sizeof( uniformName ), "PanelScale[%u]", i );
		unsigned int baseIndex = PT_PANEL_DATA_START + i * 4;
		shader.Set( uniformName, values[ baseIndex ], values[ baseIndex + 1 ] );
		snprintf( uniformName, sizeof( uniformName ), "PanelPosition[%u]", i );
		shader.Set( uniformName, values[ baseIndex + 2 ], values[ baseIndex + 3 ] );
	}

	quad.Draw();
	return FF_SUCCESS;
}

FFResult ShapeMasks::DeInitGL()
{
	shader.FreeGLResources();
	quad.Release();
	return FF_SUCCESS;
}

void ShapeMasks::UpdatePanelParameterVisibility( bool raiseEvents )
{
	unsigned int selected = (unsigned int)( values[ PT_PANEL_SELECT ] + 0.5f );
	if( selected < 1 ) selected = 1;
	if( selected > 96 ) selected = 96;
	for( unsigned int i = 0; i < 96; ++i )
	{
		bool visible = ( i + 1 == selected );
		unsigned int baseIndex = PT_PANEL_DATA_START + i * 4;
		for( unsigned int j = 0; j < 4; ++j )
			SetParamVisibility( baseIndex + j, visible, raiseEvents );
	}
}

FFResult ShapeMasks::SetFloatParameter( unsigned int index, float value )
{
	if( index >= PT_TOTAL_PARAMS )
		return FF_FAIL;
	if( index == PT_PANEL_SELECT )
	{
		value = fmaxf( 1.0f, fminf( 96.0f, value ) );
		values[ index ] = value;
		UpdatePanelParameterVisibility( true );
		return FF_SUCCESS;
	}
	if( index >= PT_PANEL_DATA_START && index < PT_PANEL_DATA_END )
	{
		unsigned int field = ( index - PT_PANEL_DATA_START ) % 4;
		if( field < 2 )
			value = fmaxf( 0.1f, fminf( 2.0f, value ) );
		else
			value = fmaxf( -1.0f, fminf( 1.0f, value ) );
	}
	if( index == PT_BORDER_THICKNESS )
		value = fmaxf( 0.0f, fminf( 20.0f, value ) );
	if( index == PT_BORDER_OPACITY || ( index >= PT_BORDER_RED && index <= PT_BORDER_BLUE ) )
		value = fmaxf( 0.0f, fminf( 1.0f, value ) );
	values[ index ] = value;
	return FF_SUCCESS;
}

float ShapeMasks::GetFloatParameter( unsigned int index )
{
	if( index >= PT_TOTAL_PARAMS )
		return 0.0f;
	return values[ index ];
}
