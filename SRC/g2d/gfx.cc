#include "gfx.h"
#include "utilfuncs.h"
#include "sceneresize.h"
#include "m3.h"
#include "stb_image.h"
#include "engine.h"
#include "assetloader.h"
#include "logger.h"
#include "textrender.h"

#include <algorithm>
#include <cmath>

_G2D_NAMESPACE_BEGIN_

static constexpr bool  SI_COMPARE_MODE       = false;
static constexpr bool  SI_BYPASS_POSTPROCESS = false;
static constexpr float SI_LIGHT_BRIGHTNESS   = 0.99f;
static constexpr float SI_AMBIENT_LIGHT      = 0.f;
static constexpr float LOCAL_LIGHT_HEIGHT    = 160.0f;

static GLint  s_ppLightComparisonUniform = -1;
static float  s_lightBgOffsetX = 0.0f;
static float  s_lightBgOffsetY = 0.0f;
static int    s_uploadedLightCount = 0;
static GLint  s_maxVertexAttribs = 0;

#define SAFE_ENABLE_ATTRIB(loc) \
    do { GLint _loc = (loc); if (_loc >= 0) glEnableVertexAttribArray(_loc); } while(0)

#define SAFE_ATTRIB_POINTER(loc, size, type, normalized, stride, offset) \
    do { GLint _loc = (loc); if (_loc >= 0) glVertexAttribPointer(_loc, size, type, normalized, stride, (void*)(offset)); } while(0)

#ifdef TARGET_EMSCRIPTEN

EM_JS(void, call_texImage2D, (uint32_t imageHandle), {
    GLctx.texImage2D(GLctx.TEXTURE_2D, 0, GLctx.RGBA, GLctx.RGBA, GLctx.UNSIGNED_BYTE, window.gfx_imgs[imageHandle]);
});

EM_JS(GLuint, jsSetupShaders, (const char* vShaderStr, const char* fShaderStr), {
    var vSource = UTF8ToString(vShaderStr);
    var fSource = UTF8ToString(fShaderStr);

    function createShader(gl, type, source) {
        var shader = gl.createShader(type);
        gl.shaderSource(shader, source);
        gl.compileShader(shader);
        if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
            console.error('Shader compile error:', gl.getShaderInfoLog(shader));
            gl.deleteShader(shader);
            return null;
        }
        return shader; 
    }

    var vs = createShader(GLctx, GLctx.VERTEX_SHADER, vSource);
    var fs = createShader(GLctx, GLctx.FRAGMENT_SHADER, fSource);

    var program = GLctx.createProgram();
    GLctx.attachShader(program, vs);
    GLctx.attachShader(program, fs);
    GLctx.linkProgram(program);

    if (!GLctx.getProgramParameter(program, GLctx.LINK_STATUS)) {
        console.error('Program link error:', GLctx.getProgramInfoLog(program));
        return 0;
    }

    program.uniformSizeAndIdsByName = {};

    var id = GL.getNewId(GL.programs);
    GL.programs[id] = program;
    program.name = id;
    program.maxUniformLength = program.maxAttributeLength = program.maxUniformBlockNameLength = 0;
    program.uniformIdCounter = 1;

    return id;
});

EM_JS(void, jsSetCanvSize, (void), {
    const canvas     = document.getElementById('canvas');
    const multiplier = 1;
    const width      = canvas.clientWidth  * multiplier | 0;
    const height     = canvas.clientHeight * multiplier | 0;
    if (canvas.width !== width ||  canvas.height !== height) 
    {
      canvas.width  = width;
      canvas.height = height;
    }
});

#endif

#ifdef TARGET_WIN
namespace
{
    auto& getGfxLog()
    {
        return std::cout;
    }

    void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id,
                                    GLenum severity, GLsizei length,
                                    const GLchar* message, const void* userParam)
    {
        std::string msg;
        if (message)
            msg = std::string(message, length);
    }

    void logAssert_glError(int nId)
    {
        auto err = glGetError();
        assert(err == GL_NO_ERROR);
        if (err != GL_NO_ERROR)
        {
            getGfxLog() << "glGetError = " << err << " id = " << nId << std::endl;
            getGfxLog().flush();
        }
    }
};
#else
#ifndef NDEBUG
void logAssert_glError(int nId)
{
    auto err = glGetError();
    if (err != GL_NO_ERROR)
    {
        std::cout << "glGetError = " << err << " id = " << nId << std::endl;
    }
}
#else
#define logAssert_glError(...)        ((void)0)
#endif
#endif

class RootContainer : public CContainer
{
public:
    RootContainer() : CContainer()
    {
        setClass(E_EL_ROOT);
    }
};

CGfx* CGfx::_instance = NULL;

CGfx* CGfx::getInstance()
{
    if (CGfx::_instance == NULL)
        CGfx::_instance = new CGfx();
    return CGfx::_instance;
}

CGfx::CGfx()
    : _isSetupDone(false)
    , _progId(0)
    , _verteciesInMemBuff(0)
    , _pVerBuffMem(NULL)
    , _pIdxBuffMem(nullptr)
    , _vertexSize(0)
    , _vertexOffset(0)
    , _indexOffset(0)
    , _nLastActiveTexture(-1)
    , _numIndicesToDraw(0)
{
    _gameRoot       = std::make_shared<RootContainer>();
    _gameIface      = std::make_shared<RootContainer>();
    _bgRoot         = std::make_shared<RootContainer>();
    _fgRoot         = std::make_shared<RootContainer>();
    _fgControlsRoot = std::make_shared<RootContainer>();

    memset(&(_attribsLocations), 0, sizeof(_attribsLocations));
}

#ifdef TARGET_WIN
void CGfx::printProgramLog(GLuint program)
{
    if (glIsProgram(program))
    {
        int infoLogLength = 0;
        int maxLength = infoLogLength;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);
        char* infoLog = new char[maxLength];
        glGetProgramInfoLog(program, maxLength, &infoLogLength, infoLog);
        if (infoLogLength > 0)
        {
            printf("%s\n", infoLog);
        }
        delete[] infoLog;
    }
    else
    {
        printf("Name %d is not a program\n", program);
    }
}

void CGfx::printShaderLog(GLuint shader)
{
    if (glIsShader(shader))
    {
        int infoLogLength = 0;
        int maxLength = infoLogLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
        char* infoLog = new char[maxLength];
        glGetShaderInfoLog(shader, maxLength, &infoLogLength, infoLog);
        if (infoLogLength > 0)
        {
            printf("%s\n", infoLog);
        }
        delete[] infoLog;
    }
    else
    {
        printf("Name %d is not a shader\n", shader);
    }
}
#endif

bool CGfx::setupShaders()
{
#ifdef TARGET_EMSCRIPTEN
    const char* preamble =
        "#version 300 es\n"
        "precision highp float;\n"
        "precision highp int;\n";
#else
    const char* preamble = "#version 330 core\n";
#endif

    const char* vertexBody = R"(
layout(location = 0) in vec2  a_position;
layout(location = 1) in vec2  a_texCoord;
layout(location = 2) in mat3  a_mat3;
layout(location = 5) in vec4  a_tintColor;
layout(location = 6) in vec4  a_reflectionType;
layout(location = 7) in vec4  a_isNormalBlend;
layout(location = 8) in vec4  a_addRGB;
layout(location = 9) in vec4  a_texMuls;
layout(location = 10) in vec3  a_params;

flat out int v_layer;
flat out int v_normalLayer;
out vec2 v_worldPos;
out vec4 v_v1;
out vec4 v_v2;
out vec4 v_v3;
out vec4 v_v4;
out vec4 v_v5;
out vec2 v_v6;
out float v_skipLight;
out float v_lightMul;

void main()
{
    v_v1 = vec4(a_texCoord, a_isNormalBlend.a, a_params.x);
    v_v2 = a_tintColor;
    v_v3 = a_reflectionType;
    v_v4 = a_addRGB;
    v_v5 = a_texMuls;
    v_v6 = a_params.yz;

    v_skipLight = a_reflectionType.w;
    v_lightMul  = a_reflectionType.z;

    v_layer = int(a_texMuls.x);
    v_normalLayer = int(a_texMuls.y);

    vec3 pos = a_mat3 * vec3(a_position, 1.0);
    v_worldPos = pos.xy;
    gl_Position = vec4(pos.xy, 0.0, 1.0);
}
)";

    const char* fragmentBody = R"(
in vec4 v_v1;
in vec4 v_v2;
in vec4 v_v3;
in vec4 v_v4;
in vec4 v_v5;
in vec2 v_v6;

flat in int v_layer;
flat in int v_normalLayer;

in vec2 v_worldPos;
in float v_skipLight;
in float v_lightMul;

out vec4 fragColor;

uniform sampler2D u_images[8];

uniform float u_shockTime, u_lowHPEffect, u_time, u_aberration;
uniform vec2  u_shockCenter, u_resolution, u_tutRectSize;
uniform vec4  u_tutParams;

uniform vec3  u_hit0; uniform float u_hitI0;
uniform vec3  u_hit1; uniform float u_hitI1;
uniform vec3  u_hit2; uniform float u_hitI2;
uniform vec3  u_hit3; uniform float u_hitI3;
uniform vec3  u_hit4; uniform float u_hitI4;
uniform vec3  u_hitColor;

#define MAX_UBO_LIGHTS 204

layout(std140) uniform LightBlock
{
    vec4 u_lights[MAX_UBO_LIGHTS * 5];
};

uniform float u_specularEnabled;
uniform float u_hasLights;
uniform float u_ambientLight;

vec4 sampleAlbedo(vec2 uv)
{
    switch (v_layer)
    {
        case 0:  return texture(u_images[0], uv);
        case 1:  return texture(u_images[1], uv);
        case 2:  return texture(u_images[2], uv);
        case 3:  return texture(u_images[3], uv);
        case 4:  return texture(u_images[4], uv);
        case 5:  return texture(u_images[5], uv);
        case 6:  return texture(u_images[6], uv);
        case 7:  return texture(u_images[7], uv);
        default: return vec4(0.0);
    }
}

vec4 sampleLightSprite(int layer, vec2 uv)
{
    switch (layer)
    {
        case 0:  return texture(u_images[0], uv);
        case 1:  return texture(u_images[1], uv);
        case 2:  return texture(u_images[2], uv);
        case 3:  return texture(u_images[3], uv);
        case 4:  return texture(u_images[4], uv);
        case 5:  return texture(u_images[5], uv);
        case 6:  return texture(u_images[6], uv);
        case 7:  return texture(u_images[7], uv);
        default: return vec4(1.0);
    }
}

const bool  NORMAL_Z_FULL_RANGE  = true;
const bool  NORMAL_Y_INVERTED    = false;
const bool  NORMAL_ALPHA_IS_SPECULAR = false;

const float NORMAL_BOOST = 1.0;
const float DIFFUSE_MIN   = 0.45;
const float DIFFUSE_MAX   = 1.0;
const float DIFFUSE_WRAP  = 0.7;
const float SPEC_POWER    = 12.0;
const float SPEC_STRENGTH = 0.30;
const float SPEC_MASK_FALLBACK = 0.0;

vec3 safeNormal(vec3 value)
{
    float len2 = dot(value, value);
    return len2 > 1e-12 ? value * inversesqrt(len2) : vec3(0.0, 0.0, 1.0);
}

mat2 spriteNormalFrame(vec2 uv)
{
    vec2 p = v_worldPos * (0.5 * u_resolution);
    vec2 px = dFdx(p);
    vec2 py = dFdy(p);
    vec2 ux = dFdx(uv);
    vec2 uy = dFdy(uv);

    float determinant = ux.x * uy.y - ux.y * uy.x;
    float uvScale = max(length(ux) * length(uy), 1e-30);

    if (abs(determinant) <= 1e-6 * uvScale)
        return mat2(1.0);

    vec2 tangent   = (px * uy.y - py * ux.y) / determinant;
    vec2 bitangent = (px * uy.x - py * ux.x) / determinant;

    float tx2 = dot(tangent, tangent);
    float by2 = dot(bitangent, bitangent);

    if (min(tx2, by2) < 1e-12)
        return mat2(1.0);

    tangent   *= inversesqrt(tx2);
    bitangent *= inversesqrt(by2);

    float basisDet = tangent.x * bitangent.y - tangent.y * bitangent.x;
    if (abs(basisDet) < 1e-4)
        return mat2(1.0);

    return mat2(vec2(bitangent.y, -bitangent.x),
                vec2(-tangent.y,   tangent.x)) / basisDet;
}

vec4 sampleNormalAndSpec(vec2 uv, mat2 normalFrame)
{
    if (v_normalLayer < 0 || v_normalLayer > 7)
        return vec4(0.0, 0.0, 1.0, 0.0);

    vec4 raw = sampleLightSprite(v_normalLayer, uv);

    if (dot(raw.rgb, vec3(1.0)) < 0.03)
        return vec4(0.0, 0.0, 1.0, 0.0);

    vec3 tNormal = vec3(raw.rg * 2.0 - 1.0,
                        NORMAL_Z_FULL_RANGE ? raw.b : raw.b * 2.0 - 1.0);

    if (NORMAL_Y_INVERTED)
        tNormal.y = -tNormal.y;

    tNormal.xy *= NORMAL_BOOST;
    tNormal = safeNormal(tNormal);

    vec3 normal = safeNormal(vec3(normalFrame * tNormal.xy, tNormal.z));

    float specMask = NORMAL_ALPHA_IS_SPECULAR ? clamp(raw.a, 0.0, 1.0)
                                              : SPEC_MASK_FALLBACK;

    return vec4(normal, specMask);
}

uniform float u_zLayer;
uniform int   u_itemCullingMask;

vec3 calcLighting(vec3 normal, float specMask, vec2 screenPos,
                  out float localLightBrightness, out vec3 localLight)
{
    localLightBrightness = 0.0;
    localLight = vec3(0.0);

    int lightCount = min(int(u_hasLights), MAX_UBO_LIGHTS);
    if (lightCount <= 0)
        return vec3(u_ambientLight > 0.0 ? u_ambientLight : 1.0);

    vec3 lighting = vec3(u_ambientLight);

    for (int i = 0; i < lightCount; ++i)
    {

        vec4 zinfo = u_lights[i * 5 + 4];

        if (u_zLayer < zinfo.x || u_zLayer > zinfo.y)
            continue;

        if ((u_itemCullingMask & int(zinfo.z + 0.5)) == 0)
            continue;

        vec4 geometry = u_lights[i * 5 + 0];
        vec4 emission = u_lights[i * 5 + 1];

        float flags = geometry.w;

        bool hasSprite      = mod(flags, 2.0) > 0.5;
        bool referenceLight = mod(floor(flags / 2.0), 2.0) > 0.5;
        bool explicitSprite = mod(floor(flags / 4.0), 2.0) > 0.5;

        float radius = geometry.z;
        vec2 lightCenter = geometry.xy;

        vec3 incident = vec3(0.0);

        if (hasSprite)
        {
            vec4 texInfo  = u_lights[i * 5 + 2];
            vec4 texInfo2 = u_lights[i * 5 + 3];

            int sampler = int(texInfo.x + 0.5);
            if (sampler < 0 || sampler > 7)
                continue;

            if (explicitSprite)
            {
                vec2 edgeX = vec2(texInfo2.y, texInfo2.z);
                vec2 edgeY = vec2(texInfo2.w, geometry.z);

                float det = edgeX.x * edgeY.y - edgeX.y * edgeY.x;
                if (abs(det) < 1e-8)
                    continue;

                lightCenter = geometry.xy + 0.5 * (edgeX + edgeY);

                vec2 d = screenPos - geometry.xy;
                float localX = (d.x * edgeY.y - d.y * edgeY.x) / det;
                float localY = (edgeX.x * d.y - edgeX.y * d.x) / det;

                if (localX < -0.001 || localX > 1.001 ||
                    localY < -0.001 || localY > 1.001)
                {
                    continue;
                }

                localX = clamp(localX, 0.0, 1.0);
                localY = clamp(localY, 0.0, 1.0);

                vec2 uv0 = vec2(texInfo.y, texInfo.z);
                vec2 uv1 = vec2(texInfo.w, texInfo2.x);
                vec2 atlasUV = uv0 + vec2(localX, localY) * (uv1 - uv0);

                vec4 lightTex = sampleLightSprite(sampler, atlasUV);
                float lum = max(max(lightTex.r, lightTex.g), lightTex.b);

                if (lightTex.a <= 0.001 && lum <= 0.001)
                    continue;

                incident = max(emission.rgb, vec3(0.0)) * lightTex.rgb * lightTex.a;
                if (dot(incident, vec3(1.0)) <= 0.0)
                    continue;
            }
            else
            {
                vec2 size = texInfo2.yz;
                if (size.x <= 0.0 || size.y <= 0.0)
                    continue;

                vec2 d = screenPos - lightCenter;
                float rot = texInfo2.w;

                if (abs(rot) > 0.0001)
                {
                    float c = cos(rot);
                    float s = sin(rot);
                    d = vec2(c * d.x + s * d.y,
                             -s * d.x + c * d.y);
                }

                vec2 localUV;
                localUV.x = d.x / size.x + 0.5;
                localUV.y = 0.5 - d.y / size.y;

                if (localUV.x < 0.0 || localUV.x > 1.0 ||
                    localUV.y < 0.0 || localUV.y > 1.0)
                {
                    continue;
                }

                vec2 uv0 = vec2(texInfo.y, texInfo.z);
                vec2 uv1 = vec2(texInfo.w, texInfo2.x);
                vec2 atlasUV = uv0 + localUV * (uv1 - uv0);

                vec4 lightTex = sampleLightSprite(sampler, atlasUV);
                float lum = max(max(lightTex.r, lightTex.g), lightTex.b);

                if (lightTex.a <= 0.001 && lum <= 0.001)
                    continue;

                incident = max(emission.rgb, vec3(0.0)) * lightTex.rgb * lightTex.a;
                if (dot(incident, vec3(1.0)) <= 0.0)
                    continue;
            }
        }
        else
        {
            if (!referenceLight && radius <= 0.0)
                continue;

            float d = referenceLight ? 0.0 : length(lightCenter - screenPos) / radius;
            if (!referenceLight && d >= 1.0)
                continue;

            float atten = pow(1.0 - smoothstep(0.0, 1.0, d), 1.8);
            incident = max(emission.rgb, vec3(0.0)) * atten;
        }

        vec2 toLight = lightCenter - screenPos;
        localLightBrightness += dot(incident, vec3(0.0333));

        vec3 L = safeNormal(vec3(toLight, max(emission.a, 0.0)));
        float NdotL = dot(normal, L);

        float wrapped = clamp((NdotL + DIFFUSE_WRAP) / (1.0 + DIFFUSE_WRAP), 0.0, 1.0);
        float diffuse = mix(DIFFUSE_MIN, DIFFUSE_MAX, wrapped);

        float specular = 0.0;
        if (!referenceLight && u_specularEnabled > 0.5 && NdotL > 0.0)
        {
            vec3 H = safeNormal(L + vec3(0.0, 0.0, 1.0));
            specular = pow(max(dot(normal, H), 0.0), SPEC_POWER)
                     * max(specMask, 0.0) * SPEC_STRENGTH;
        }

        vec3 contribution = incident * (diffuse + specular);
        localLight += contribution;
        lighting   += contribution;
    }

    return lighting;
}

float sampleAlpha(vec2 uv)
{
    return sampleAlbedo(uv).a;
}

vec4 cleanSprite(vec2 uv)
{
    vec4 c = sampleAlbedo(uv);
    if (c.a < 0.01)
        return vec4(0.0);

    vec2 off = vec2(0.003, 0.0);

    float aN = sampleAlpha(uv + off.xy);
    float aS = sampleAlpha(uv - off.xy);
    float aE = sampleAlpha(uv + off.yx);
    float aW = sampleAlpha(uv - off.yx);

    float maxNeighbor = max(max(aN, aS), max(aE, aW));
    if (c.a < 0.25 && maxNeighbor < 0.2)
        return vec4(0.0);

    return c;
}

void main()
{
    vec2  uv_main      = v_v1.xy;
    float alphaBlend   = v_v1.z;
    float sparkLife    = v_v1.w;

    vec4  tintColor    = v_v2;
    vec4  addRGB       = v_v4;
    vec4  texMuls      = v_v5;

    float effectType   = v_v6.x;
    float tutIntensity = v_v6.y;

    vec2 uv_distorted = uv_main;

    if (effectType > 6.5 && effectType < 7.5)
    {
        float p_jitter = fract(sin(dot(uv_distorted, vec2(12.9, 78.2))) * 437.5);
        uv_distorted += (p_jitter - 0.5) * 0.003 * sparkLife;
    }

    if (u_shockTime > 0.0)
    {
        if (u_shockTime < 0.2)
        {
            uv_distorted.x += sin(uv_distorted.y * 40.0) * u_aberration * 0.1;
        }

        vec2 shock_dir = uv_distorted - u_shockCenter;
        float dist = length(shock_dir);

        if (dist < u_shockTime + 0.1)
        {
            float mask_shock = smoothstep(u_shockTime - 0.1, u_shockTime, dist) *
                               (1.0 - smoothstep(u_shockTime, u_shockTime + 0.1, dist));

            uv_distorted += normalize(shock_dir + 0.0001) * (mask_shock * 0.05 * (1.0 - u_shockTime));
        }
    }

    mat2 normalFrame = spriteNormalFrame(uv_main);

    float localLightBrightness = 0.0;
    vec3 localLightContribution = vec3(0.0);

    vec4 color_out = sampleAlbedo(uv_main);

    if (v_skipLight < 0.5)
    {
        vec4 nm = sampleNormalAndSpec(uv_main, normalFrame);
        vec3 lighting = calcLighting(nm.xyz, nm.w, gl_FragCoord.xy, localLightBrightness, localLightContribution);

        float lightMul = clamp(v_lightMul, 0.0, 1.0);
        vec3 adjustedLighting = lighting - localLightContribution + localLightContribution * lightMul;

        color_out.rgb *= adjustedLighting;
    }

    if (effectType > 26.5 && effectType < 30.5)
    {
        color_out = cleanSprite(uv_main);
    }

    if (texMuls.z > 0.001)
    {
        float lum = dot(color_out.rgb, vec3(0.299, 0.587, 0.114));
        color_out.rgb = mix(color_out.rgb, vec3(lum), clamp(texMuls.z, 0.0, 1.0));
    }

    vec2 p = gl_FragCoord.xy / u_resolution;
    float aspect = u_resolution.x / u_resolution.y;
    vec2 p_asp = p * vec2(aspect, 1.0);

    float glow = 0.0;

    if (u_hitI0 > 0.001)
    {
        float m = smoothstep(u_hit0.z, 0.0, distance(p_asp, u_hit0.xy * vec2(aspect, 1.0)));
        glow += (m * m * m * m) * u_hitI0;
    }

    if (u_hitI1 > 0.001)
    {
        float m = smoothstep(u_hit1.z, 0.0, distance(p_asp, u_hit1.xy * vec2(aspect, 1.0)));
        glow += (m * m * m * m) * u_hitI1;
    }

    if (u_hitI2 > 0.001)
    {
        float m = smoothstep(u_hit2.z, 0.0, distance(p_asp, u_hit2.xy * vec2(aspect, 1.0)));
        glow += (m * m * m * m) * u_hitI2;
    }

    if (u_hitI3 > 0.001)
    {
        float m = smoothstep(u_hit3.z, 0.0, distance(p_asp, u_hit3.xy * vec2(aspect, 1.0)));
        glow += (m * m * m * m) * u_hitI3;
    }

    if (u_hitI4 > 0.001)
    {
        float m = smoothstep(u_hit4.z, 0.0, distance(p_asp, u_hit4.xy * vec2(aspect, 1.0)));
        glow += (m * m * m * m) * u_hitI4;
    }

    vec3 final_glow = u_hitColor * glow * 1.5;
    color_out.rgb = 1.0 - (1.0 - color_out.rgb) * (1.0 - final_glow);

    color_out *= tintColor;
    color_out.rgb += addRGB.rgb;

    if (alphaBlend > 1.5 && alphaBlend < 2.5)
    {
        float brightness = dot(color_out.rgb, vec3(0.299, 0.587, 0.114));
        brightness = clamp(brightness, 0.0, 1.0);
        brightness = mix(1.0, brightness, color_out.a);

        color_out.rgb = vec3(0.0);
        color_out.a = 1.0 - brightness;
    }
    else
    {
        color_out.rgb *= color_out.a;
        color_out.a *= alphaBlend;
    }

    if (u_lowHPEffect > 0.0)
    {
        float hp_dist = distance(uv_main, vec2(0.5));
        float pulse = smoothstep(0.3, 0.8, hp_dist) * ((sin(u_time * 5.0) * 0.5 + 0.5) * u_lowHPEffect) * color_out.a;
        color_out.rgb = mix(color_out.rgb, vec3(0.8, 0.0, 0.0) * color_out.a, pulse * 0.7);
    }

    if (tutIntensity > 0.0)
    {
        vec2 aspect_vec = vec2(u_resolution.x / (u_resolution.y + 0.001), 1.0);
        vec2 p_tut = (gl_FragCoord.xy / u_resolution - u_tutParams.xy) * aspect_vec;
        vec2 d_tut = abs(p_tut) - (u_tutRectSize * aspect_vec) + u_tutParams.z;

        float tut_dist = length(max(d_tut, 0.0)) + min(max(d_tut.x, d_tut.y), 0.0) - u_tutParams.z;
        float mask = smoothstep(0.0, 0.005, tut_dist);

        color_out.rgb *= mix(1.0, 1.0 - tutIntensity, mask);

        float pulse_tut = sin(u_time * 4.0) * 0.1 + 1.0;
        color_out.rgb *= mix(pulse_tut, 1.0, mask);

        float edge_glow = smoothstep(0.004, 0.0, abs(tut_dist)) *
                          smoothstep(0.4, 0.5, sin(p_tut.y * 15.0 - u_time * 8.0)) *
                          0.5 * tutIntensity;

        color_out.rgb += edge_glow;
    }

    fragColor = color_out;
}
)";

#ifdef TARGET_EMSCRIPTEN
    std::string vertStr = std::string(preamble) + vertexBody;
    std::string fragStr = std::string(preamble) + fragmentBody;

    _progId = jsSetupShaders(vertStr.c_str(), fragStr.c_str());
    return _progId != 0;
#endif

#ifdef TARGET_WIN
    bool bRes = true;

    _progId = glCreateProgram();

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

    std::string vertSrc = std::string(preamble) + vertexBody;
    std::string fragSrc = std::string(preamble) + fragmentBody;

    const GLchar* vSrcPtr = vertSrc.c_str();
    const GLchar* fSrcPtr = fragSrc.c_str();

    glShaderSource(vertexShader, 1, &vSrcPtr, NULL);
    glCompileShader(vertexShader);

    GLint vShaderCompiled = GL_FALSE;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &vShaderCompiled);
    if (vShaderCompiled != GL_TRUE)
    {
        printf("Unable to compile vertex shader %d!\n", vertexShader);
        bRes = false;
        printShaderLog(vertexShader);
        assert(false);
    }

    if (bRes)
    {
        glAttachShader(_progId, vertexShader);

        glShaderSource(fragmentShader, 1, &fSrcPtr, NULL);
        glCompileShader(fragmentShader);

        GLint fShaderCompiled = GL_FALSE;
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &fShaderCompiled);
        if (fShaderCompiled != GL_TRUE)
        {
            printf("Unable to compile fragment shader %d!\n", fragmentShader);
            printShaderLog(fragmentShader);
            assert(false);
            bRes = false;
        }
    }

    if (bRes)
    {
        glAttachShader(_progId, fragmentShader);
        glLinkProgram(_progId);

        GLint programSuccess = GL_TRUE;
        glGetProgramiv(_progId, GL_LINK_STATUS, &programSuccess);
        if (programSuccess != GL_TRUE)
        {
            printf("Error linking program %d!\n", _progId);
            printProgramLog(_progId);
            bRes = false;
        }
    }

    if (vertexShader)   glDeleteShader(vertexShader);
    if (fragmentShader) glDeleteShader(fragmentShader);

    return bRes;
#endif
}

bool CGfx::initOpenGL()
{
    bool bRes = false;

#ifdef TARGET_EMSCRIPTEN
    jsSetCanvSize();

    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);

    attrs.alpha                     = false;
    attrs.depth                     = false;
    attrs.stencil                   = false;
    attrs.antialias                 = false;
    attrs.premultipliedAlpha        = true;
    attrs.preserveDrawingBuffer     = false;
    attrs.enableExtensionsByDefault = false;
    attrs.majorVersion              = 2;

    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx = emscripten_webgl_create_context("canvas", &attrs);
    if (ctx != 0)
    {
        _isWebGL2 = true;
        bRes = true;

        emscripten_webgl_make_context_current(ctx);

        setViewPort(_fOrigCx, _fOrigCy);

        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glColorMask(true, true, true, true);
        glEnable(GL_BLEND);
    }
    else
    {
        _isWebGL2 = false;
        attrs.majorVersion = 1;

        EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx = emscripten_webgl_create_context("canvas", &attrs);
        if (ctx != 0)
        {
            bRes = true;

            emscripten_webgl_make_context_current(ctx);

            setViewPort(_fOrigCx, _fOrigCy);

            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            glColorMask(true, true, true, true);
            glEnable(GL_BLEND);
        }
    }
#endif

#ifdef TARGET_WIN
    getGfxLog() << "--- BEGIN LOG ---" << std::endl;
    getGfxLog() << "GL_VERSION = " << glGetString(GL_VERSION) << std::endl;

    bRes = true;

    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    logAssert_glError(1);

    glColorMask(true, true, true, true);
    logAssert_glError(2);

    glEnable(GL_BLEND);
    logAssert_glError(3);

    glEnable(GL_TEXTURE_2D);
    logAssert_glError(4);
#endif

    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &s_maxVertexAttribs);
    if (s_maxVertexAttribs <= 0)
        s_maxVertexAttribs = 8;

    return bRes;
}

void CGfx::onEvent(int nEvent, void* pData1, void* pData2, void* pData3)
{
    switch (nEvent)
    {
        case CSceneResize::EVT_RESOLUTION_CHANGED:
        {
            _gameRoot->removeSelfTweens();

            if (_cbOnCameraAnimComplete)
            {
                _cbOnCameraAnimComplete();
                _cbOnCameraAnimComplete = {};
            }

            resetCamera();
            updateViewPort();

            float w = (float)CSceneResize::getInstance()->getScreenWidth();
            float h = (float)CSceneResize::getInstance()->getScreenHeight();

            resizePostProcess(w, h);
            resizeLightBuffer(w, h);
        }
        break;
    }
}

bool CGfx::setup()
{
#ifdef TARGET_EMSCRIPTEN
    _gpuTier = EM_ASM_INT({ return (window.GPU_TIER !== null && window.GPU_TIER !== undefined) ? window.GPU_TIER : 1;});
#endif

    bool bRes = false;

    if (!_isSetupDone)
    {
        auto& cfg = Engine::getCfg();

        _fOrigCx = cfg.INIT_SCR_CX;
        _fOrigCy = cfg.INIT_SCR_CY;

        CSceneResize::getInstance()->addListener(this);
        resetCamera();

        _isSetupDone = true;

        bRes = initOpenGL();
        if (bRes)
        {
            setupShaders();
            lookupUniforms();
            setupAttributes();

            glUseProgram(_progId);

            createTempTextures();
            updateUniforms();

            if (!_ppInitialized)
                initPostProcess();

            if (!_lightBufferInitialized)
                initLightBuffer();
        }
    }

    return bRes;
}

bool CGfx::initPostProcess()
{
#ifdef TARGET_EMSCRIPTEN
    if (!_isWebGL2)
        return false;
#endif

    if (_ppInitialized)
        return true;

    _ppWidth = _viewPortCx > 0 ? _viewPortCx : _fOrigCx;
    _ppHeight = _viewPortCy > 0 ? _viewPortCy : _fOrigCy;

    glGenFramebuffers(1, &_ppFBO);
    if (!_ppFBO)
        return false;

    glGenTextures(1, &_ppTexture);

    glBindTexture(GL_TEXTURE_2D, _ppTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)_ppWidth, (GLsizei)_ppHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, _ppFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _ppTexture, 0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        glDeleteFramebuffers(1, &_ppFBO); _ppFBO = 0;
        glDeleteTextures(1, &_ppTexture); _ppTexture = 0;
        return false;
    }

    if (!setupPostProcessShaders())
    {
        glDeleteFramebuffers(1, &_ppFBO); _ppFBO = 0;
        glDeleteTextures(1, &_ppTexture); _ppTexture = 0;
        return false;
    }

    lookupPostProcessUniforms();

    float quadData[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
         1.0f,  1.0f, 1.0f, 1.0f
    };

    glGenBuffers(1, &_ppVBO);
    glBindBuffer(GL_ARRAY_BUFFER, _ppVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadData), quadData, GL_STATIC_DRAW);

    glGenVertexArrays(1, &_ppVAO);
    glBindVertexArray(_ppVAO);

    glBindBuffer(GL_ARRAY_BUFFER, _ppVBO);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);

    _ppInitialized = true;

    initBloom();

    return true;
}

bool CGfx::setupPostProcessShaders()
{
#ifdef TARGET_EMSCRIPTEN
    const char* preamble =
        "#version 300 es\n"
        "precision mediump float;\n"
        "precision mediump sampler2D;\n"
        "precision mediump int;\n";
#else
    const char* preamble = "#version 330 core\n";
#endif

    const char* vBody = R"(
layout(location = 0) in vec2 a_pos;
layout(location = 1) in vec2 a_uv;

out vec2 v_uv;

void main()
{
    gl_Position = vec4(a_pos, 0.0, 1.0);
    v_uv = a_uv;
}
)";

    const char* fBody = R"(
in vec2 v_uv;
out vec4 fragColor;

uniform sampler2D u_sceneTex;
uniform sampler2D u_bloomTex;
uniform sampler2D u_grainTex;

uniform vec2 u_resolution;
uniform float u_time;

uniform float u_lightComparison;

uniform float u_vignetteIntensity;
uniform float u_vignetteRadius;
uniform float u_vignetteSmoothness;

uniform vec3  u_tintColor;
uniform float u_saturation;
uniform float u_contrast;
uniform float u_brightness;

uniform float u_grainIntensity;
uniform float u_aberration;
uniform float u_bloomIntensity;

uniform float u_fisheyeIntensity;
uniform float u_distortionIntensity;
uniform float u_rainIntensity;
uniform float u_bloodDrops;

float rand(vec2 co)
{
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

float rf_hash1(float n)
{
    return fract(sin(n * 127.1) * 43758.5453);
}

vec3 N13(float p)
{
    vec3 p3 = fract(vec3(p) * vec3(.1031, .11369, .13787));
    p3 += dot(p3, p3.yzx + 19.19);
    return fract(vec3((p3.x + p3.y) * p3.z, (p3.x + p3.z) * p3.y, (p3.y + p3.z) * p3.x));
}

float N(float t)
{
    return fract(sin(t * 12345.564) * 7658.76);
}

vec2 DropLayer2(vec2 uv, float t)
{
    vec2 UV = uv;
    uv.y += t * 0.75;
    vec2 a = vec2(6., 1.);
    vec2 grid = a * 2.;
    vec2 id = floor(uv * grid);
    float colShift = N(id.x);
    uv.y += colShift;
    id = floor(uv * grid);
    vec3 n = N13(id.x * 35.2 + id.y * 2376.1);
    vec2 st = fract(uv * grid) - vec2(.5, 0);
    float x = n.x - .5;
    float y = UV.y * 20.;
    float wiggle = sin(y + sin(y));
    x += wiggle * (.5 - abs(x)) * (n.z - .5);
    x *= .7;
    float ti = fract(t + n.z);
    y = (smoothstep(0., .85, ti) * smoothstep(1., .85, ti) - .5) * .9 + .5;
    vec2 p = vec2(x, y);
    float d = length((st - p) * a.yx);
    float mainDrop = smoothstep(.4, .0, d);
    float r = sqrt(smoothstep(1., y, st.y));
    float cd = abs(st.x - x);
    float trail = smoothstep(.23 * r, .15 * r * r, cd);
    float trailFront = smoothstep(-.02, .02, st.y - y);
    trail *= trailFront * r * r;
    y = UV.y;
    float trail2 = smoothstep(.2 * r, .0, cd);
    float droplets = max(0., (sin(y * (1. - y) * 120.) - st.y)) * trail2 * trailFront * n.z;
    y = fract(y * 10.) + (st.y - .5);
    float dd = length(st - vec2(x, y));
    droplets = smoothstep(.3, 0., dd);
    return vec2(mainDrop + droplets * r * trailFront, trail);
}

float StaticDrops(vec2 uv, float t)
{
    uv *= 40.;
    vec2 id = floor(uv);
    uv = fract(uv) - .5;
    vec3 n = N13(id.x * 107.45 + id.y * 3543.654);
    vec2 p = (n.xy - .5) * .7;
    float d = length(uv - p);
    float fade = smoothstep(0., .025, fract(t + n.z)) * smoothstep(1., .025, fract(t + n.z));
    return smoothstep(.3, 0., d) * fract(n.z * 10.) * fade;
}

vec2 Drops(vec2 uv, float t, float l0, float l1, float l2)
{
    float s = StaticDrops(uv, t) * l0;
    vec2 m1 = DropLayer2(uv, t) * l1;
    vec2 m2 = DropLayer2(uv * 1.85, t) * l2;
    float c = smoothstep(.3, 1., s + m1.x + m2.x);
    return vec2(c, max(m1.y * l0, m2.y * l1));
}

float rainDropFall(vec2 uv, float t, float speed, float density, float stretch, float aspect)
{
    float id = floor(uv.x * density);
    float cell = id / density;
    float seed = floor(t * speed);
    float rnd  = rf_hash1(id * 7.0 + seed * 13.0);
    float rnd2 = rf_hash1(id * 3.0 + seed * 29.0);
    float rnd3 = rf_hash1(id * 11.0 + seed * 7.0);
    float startY = rnd2;
    float fallSpeed = speed * (0.4 + rnd3 * 0.6);
    float y = fract(startY - t * fallSpeed);
    float x = cell + (rnd - 0.5) * 0.6 / density;
    x += (y - 0.5) * 0.025;
    float dx = (uv.x - x) * aspect * density * 12.0;
    float dy = (uv.y - y) * stretch;
    float drop = exp(-(dx * dx + dy * dy) * 4.0);
    float tail = 0.0;
    float ty = uv.y - y;
    if (ty > 0.0 && ty < 0.06 * stretch / 25.0)
    {
        float tailLen = 0.06 * stretch / 25.0;
        float fade = 1.0 - ty / tailLen;
        float tx = abs(uv.x - x) * aspect * density * 8.0;
        tail = fade * fade * exp(-tx * tx * 3.0) * 0.35;
    }
    return drop + tail;
}

vec3 renderFarRain(vec2 uv, float t, float aspect, float intensity)
{
    float r1 = rainDropFall(uv, t, 0.6, 100.0, 18.0, aspect);
    float r2 = rainDropFall(uv, t, 1.2, 60.0, 22.0, aspect);
    vec3 farColor = vec3(0.12, 0.14, 0.17);
    vec3 midColor = vec3(0.25, 0.28, 0.32);
    return (farColor * r1 * 0.6 + midColor * r2 * 1.2) * intensity;
}

float bld_hash21(vec2 p)
{
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

vec3 bld_hash13(float p)
{
    vec3 p3 = fract(vec3(p) * vec3(.1031, .11369, .13787));
    p3 += dot(p3, p3.yzx + 19.19);
    return fract(vec3((p3.x + p3.y) * p3.z, (p3.x + p3.z) * p3.y, (p3.y + p3.z) * p3.x));
}

float bld_rainDrops(vec2 st, float time, float size, vec2 resolution)
{
    vec2 uv = st * size;
    uv.x *= resolution.x / resolution.y;
    vec2 gridUv = fract(uv) - .5;
    vec2 id = floor(uv);
    vec3 h = (bld_hash13(id.x * 467.983 + id.y * 1294.387) - .5) * .8;
    vec2 dropUv = gridUv - h.xy;
    float noiseG = bld_hash21(id);
    float noiseB = bld_hash21(id + 45.32);
    float drop = smoothstep(.25, 0., length(dropUv)) *
                 max(0., 1. - fract(time * (noiseB + .1) * .2 + noiseG) * 2.);
    return drop;
}

vec2 bld_wigglyDrops(vec2 st, float time, float size, vec2 resolution)
{
    vec2 wigglyDropAspect = vec2(2., 1.);
    vec2 uv = st * size * wigglyDropAspect;
    uv.x *= resolution.x / resolution.y;
    uv.y += time * .23;
    vec2 gridUv = fract(uv) - .5;
    vec2 id = floor(uv);
    float h = bld_hash21(id);
    time += h * 2. * 3.14159265;
    float w = st.y * 10.;
    float dx = (h - .5) * .8;
    dx += (.3 - abs(dx)) * pow(sin(w), 2.) * sin(2. * w) * pow(cos(w), 3.) * 1.05;
    float dy = -sin(time + sin(time + sin(time) * .5)) * .45;
    dy -= (gridUv.x - dx) * (gridUv.x - dx);
    vec2 dropUv = (gridUv - vec2(dx, dy)) / wigglyDropAspect;
    float drop = smoothstep(.06, .0, length(dropUv));
    vec2 trailUv = (gridUv - vec2(dx, time * .23)) / wigglyDropAspect;
    trailUv.y = (fract((trailUv.y) * 8.) - .5) / 8.;
    float trailDrop = smoothstep(.03, .0, length(trailUv));
    trailDrop *= smoothstep(-.05, .05, dropUv.y) * smoothstep(.4, dy, gridUv.y) * (1. - step(.4, gridUv.y));
    float fogTrail = smoothstep(-.05, .05, dropUv.y) * smoothstep(.4, dy, gridUv.y) *
                     smoothstep(.05, .01, abs(dropUv.x)) * (1. - step(.4, gridUv.y));
    return vec2(drop + trailDrop, fogTrail);
}

vec2 bld_getDrops(vec2 st, float time, vec2 resolution)
{
    vec2 largeDrops = bld_wigglyDrops(st, time * 2., 1.6, resolution);
    vec2 mediumDrops = bld_wigglyDrops(st + 2.65, (time + 1296.675) * 1.4, 2.5, resolution);
    vec2 smallDrops = bld_wigglyDrops(st - 1.67, time - 896.431, 3.6, resolution);
    float rain = bld_rainDrops(st, time, 20., resolution);
    vec2 drops;
    drops.y = max(largeDrops.y, max(mediumDrops.y, smallDrops.y));
    drops.x = smoothstep(.4, 2., (1. - drops.y) * rain + largeDrops.x + mediumDrops.x + smallDrops.x);
    return drops;
}

void main()
{
    if (u_lightComparison > 0.5)
    {
        fragColor = texture(u_sceneTex, v_uv);
        return;
    }

    vec2 uv = v_uv;

    if (u_fisheyeIntensity > 0.0)
    {
        vec2 d = uv - 0.5;
        float d2 = dot(d, d);
        float maxFactor = 1.0 + u_fisheyeIntensity * (0.5 * 1.5);
        d = d / maxFactor;
        float factor = 1.0 + u_fisheyeIntensity * (d2 * 1.5);
        uv = 0.5 + d * factor;
    }

    vec2 dir = uv - vec2(0.5);
    float dist = length(dir);

    vec2 distortedUV = uv;
    if (u_distortionIntensity > 0.0)
    {
        float t = u_time * 2.0;
        float w1 = sin(uv.y * 14.0 + t) * 0.01;
        float w2 = cos(uv.x * 11.0 + t * 0.8) * 0.01;
        float w3 = sin((uv.x + uv.y) * 9.0 - t * 1.3) * 0.008;
        float edgeAmp = smoothstep(0.0, 0.5, dist);
        vec2 warp = vec2(w1 + w3, w2 + w3) * u_distortionIntensity * (1.0 + edgeAmp * 3.0);
        distortedUV += warp;
    }

    vec2 dropOffset = vec2(0.0);
    if (u_rainIntensity > 0.0)
    {
        float dropTime = u_time * 0.2;
        float aspect = u_resolution.x / u_resolution.y;
        vec2 dropUV = (uv - 0.5) * vec2(aspect, 1.0) * 0.7;
        float staticDrops = smoothstep(-0.5, 1.0, u_rainIntensity) * 2.0;
        float layer1 = smoothstep(0.25, 0.75, u_rainIntensity);
        float layer2 = smoothstep(0.0, 0.5, u_rainIntensity);
        vec2 c = Drops(dropUV, dropTime, staticDrops, layer1, layer2);
        vec2 e = vec2(0.001, 0.0);
        float cx = Drops(dropUV + e, dropTime, staticDrops, layer1, layer2).x;
        float cy = Drops(dropUV + e.yx, dropTime, staticDrops, layer1, layer2).x;
        vec2 n = vec2(cx - c.x, cy - c.x);
        dropOffset = n * 0.06 * u_rainIntensity;
    }

    vec2 bloodOffset = vec2(0.0);
    vec2 bloodData = vec2(0.0);
    if (u_bloodDrops > 0.0)
    {
        float bloodTime = mod(u_time + 100., 7200.);
        vec2 bldUV = uv;
        vec2 drops = bld_getDrops(bldUV, bloodTime, u_resolution);
        vec2 dropsX = bld_getDrops(bldUV + vec2(.001, 0.), bloodTime, u_resolution);
        vec2 dropsY = bld_getDrops(bldUV + vec2(0., .001), bloodTime, u_resolution);
        vec3 bldNormal = vec3(dropsX.x - drops.x, dropsY.x - drops.x, 0.);
        bldNormal.z = sqrt(max(0.0, 1. - bldNormal.x * bldNormal.x - bldNormal.y * bldNormal.y));
        bldNormal = normalize(bldNormal);
        bloodOffset = bldNormal.xy * 3.0 * u_bloodDrops;
        bloodData = drops;
    }

    vec2 abOff = dir * u_aberration;
    vec2 baseUV = distortedUV + dropOffset + bloodOffset;

    float r = texture(u_sceneTex, baseUV + abOff).r;
    float g = texture(u_sceneTex, baseUV + abOff * 0.5).g;
    float b = texture(u_sceneTex, baseUV - abOff).b;
    vec3 color = vec3(r, g, b);

    float lum = dot(color, vec3(0.299, 0.587, 0.114));
    vec3 gray = vec3(lum);
    color = mix(gray, color, u_saturation);
    color = (color - 0.5) * u_contrast + 0.5 + u_brightness;
    color *= u_tintColor;

    vec3 bloom = texture(u_bloomTex, uv).rgb;
    color += bloom * u_bloomIntensity;
    color = clamp(color, 0.0, 1.0);

    float grainFps = 24.0;
    float grainTime = floor(u_time * grainFps) / grainFps;
    vec2 grainOffset = vec2(
        rand(vec2(grainTime, 1.234)),
        rand(vec2(2.345, grainTime))
    );
    vec2 grainUv = (uv * u_resolution / 128.0) + grainOffset;
    vec3 grainTexColor = texture(u_grainTex, grainUv).rgb;
    if (dot(color, vec3(1.0)) > 0.01)
    {
        vec3 grainBlend = color * (grainTexColor + 0.5);
        color = mix(color, grainBlend, u_grainIntensity * 2.0);
    }

    if (u_rainIntensity > 0.0)
    {
        float aspect = u_resolution.x / u_resolution.y;
        color += renderFarRain(uv, u_time, aspect, u_rainIntensity);
    }

    if (u_bloodDrops > 0.0)
    {
        color += (bloodData.y > 0. ? vec3(.5, -.1, -.15) * bloodData.y * u_bloodDrops : vec3(0.));
        color *= (bloodData.x > 0. ? vec3(.8, .2, .1) * (1. - bloodData.x * u_bloodDrops) : vec3(1.));
    }

    float vigMask = 1.0 - smoothstep(u_vignetteRadius, u_vignetteRadius + u_vignetteSmoothness, dist);
    vec3 vignetteTargetColor = color * (1.0 - u_vignetteIntensity);
    color = mix(vignetteTargetColor, color, vigMask);

    fragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
)";

#ifdef TARGET_EMSCRIPTEN
    std::string vSrc = std::string(preamble) + vBody;
    std::string fSrc = std::string(preamble) + fBody;
    _ppProgId = jsSetupShaders(vSrc.c_str(), fSrc.c_str());
    return _ppProgId != 0;
#endif

#ifdef TARGET_WIN
    _ppProgId = glCreateProgram();
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    std::string vSrc = std::string(preamble) + vBody;
    std::string fSrc = std::string(preamble) + fBody;
    const char* vPtr = vSrc.c_str();
    const char* fPtr = fSrc.c_str();
    glShaderSource(vs, 1, &vPtr, NULL);
    glCompileShader(vs);
    glShaderSource(fs, 1, &fPtr, NULL);
    glCompileShader(fs);
    glAttachShader(_ppProgId, vs);
    glAttachShader(_ppProgId, fs);
    glLinkProgram(_ppProgId);
    GLint linked = GL_TRUE;
    glGetProgramiv(_ppProgId, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE)
    {
        printProgramLog(_ppProgId);
        glDeleteShader(vs);
        glDeleteShader(fs);
        return false;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return true;
#endif
}

void CGfx::lookupPostProcessUniforms()
{
    s_ppLightComparisonUniform = glGetUniformLocation(_ppProgId, "u_lightComparison");
    _ppUSceneTex               = glGetUniformLocation(_ppProgId, "u_sceneTex");
    _ppUGrainTex               = glGetUniformLocation(_ppProgId, "u_grainTex");
    _ppUResolution             = glGetUniformLocation(_ppProgId, "u_resolution");
    _ppUTime                   = glGetUniformLocation(_ppProgId, "u_time");
    _ppUVignetteIntensity      = glGetUniformLocation(_ppProgId, "u_vignetteIntensity");
    _ppUVignetteRadius         = glGetUniformLocation(_ppProgId, "u_vignetteRadius");
    _ppUVignetteSmoothness     = glGetUniformLocation(_ppProgId, "u_vignetteSmoothness");
    _ppUTintColor              = glGetUniformLocation(_ppProgId, "u_tintColor");
    _ppUSaturation             = glGetUniformLocation(_ppProgId, "u_saturation");
    _ppUContrast               = glGetUniformLocation(_ppProgId, "u_contrast");
    _ppUBrightness             = glGetUniformLocation(_ppProgId, "u_brightness");
    _ppUGrain                  = glGetUniformLocation(_ppProgId, "u_grainIntensity");
    _ppUAberration             = glGetUniformLocation(_ppProgId, "u_aberration");
    _ppUBloomTex               = glGetUniformLocation(_ppProgId, "u_bloomTex");
    _ppUBloomIntensity         = glGetUniformLocation(_ppProgId, "u_bloomIntensity");

    _ppUFisheyeIntensity       = glGetUniformLocation(_ppProgId, "u_fisheyeIntensity");
    _ppUDistortionIntensity    = glGetUniformLocation(_ppProgId, "u_distortionIntensity");
    _ppURainIntensity          = glGetUniformLocation(_ppProgId, "u_rainIntensity");
    _ppUBloodDrops             = glGetUniformLocation(_ppProgId, "u_bloodDrops");

    _ppAPos                    = glGetAttribLocation(_ppProgId, "a_pos");
    _ppAUV                     = glGetAttribLocation(_ppProgId, "a_uv");
}

void CGfx::resizePostProcess(float w, float h)
{
    if (!_ppInitialized)
        return;

    _ppWidth = w;
    _ppHeight = h;

    if (_ppTexture && _ppFBO)
    {
        glBindTexture(GL_TEXTURE_2D, _ppTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)w, (GLsizei)h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

        glBindFramebuffer(GL_FRAMEBUFFER, _ppFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _ppTexture, 0);

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            _ppInitialized = false;
        }
    }

    resizeBloom(w, h);
    resizeLightBuffer(w, h);
}

void CGfx::restoreVertexFormat()
{
    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vertexBuff);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _indexBuff);
}

CTexturePtr CGfx::createRenderTexture(float w, float h)
{
    if (w < 1.0f) w = 1.0f;
    if (h < 1.0f) h = 1.0f;

    GLuint fbo = 0, texHandle = 0;

    glGenFramebuffers(1, &fbo);
    glGenTextures(1, &texHandle);

    glBindTexture(GL_TEXTURE_2D, texHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)w, (GLsizei)h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texHandle, 0);

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        glDeleteFramebuffers(1, &fbo);
        glDeleteTextures(1, &texHandle);
        return nullptr;
    }

    CTexturePtr ptr = std::make_shared<CTexture>();
    ptr->_handle = texHandle;
    ptr->_cx     = (int)w;
    ptr->_cy     = (int)h;
    ptr->setUploaded();
    ptr->setPath("");
    ptr->unsetSampler();

    _rttFBOs[ptr.get()] = fbo;

    return ptr;
}

void CGfx::destroyRenderTexture(CTexturePtr tex)
{
    if (!tex)
        return;

    auto it = _rttFBOs.find(tex.get());
    if (it != _rttFBOs.end())
    {
        glDeleteFramebuffers(1, &it->second);
        _rttFBOs.erase(it);
    }

    if (tex->isUploaded())
    {
        for (size_t i = 0; i < _activeTextures.size(); ++i)
        {
            if (_activeTextures[i].get() == tex.get())
            {
                _activeTextures[i]->unsetSampler();
                _activeTextures.erase(_activeTextures.begin() + i);
                break;
            }
        }

        glDeleteTextures(1, &tex->_handle);
        tex->setUploaded(false);
    }
}

void CGfx::setRenderTarget(CTexturePtr tex)
{
    if (!tex || !tex->isUploaded())
        return;

    auto it = _rttFBOs.find(tex.get());
    if (it == _rttFBOs.end())
        return;

    flush();

    for (size_t i = 0; i < _activeTextures.size(); ++i)
    {
        if (_activeTextures[i].get() == tex.get())
        {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, 0);
            _activeTextures[i]->unsetSampler();
            _activeTextures.erase(_activeTextures.begin() + i);
            --i;
        }
    }

    if (tex->getSampler() >= 0)
    {
        glActiveTexture(GL_TEXTURE0 + tex->getSampler());
        glBindTexture(GL_TEXTURE_2D, 0);
        tex->unsetSampler();
    }

    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &_rttPrevFBO);
    glGetIntegerv(GL_VIEWPORT, _rttPrevViewport);

    glBindFramebuffer(GL_FRAMEBUFFER, it->second);
    glViewport(0, 0, (GLsizei)tex->getCx(), (GLsizei)tex->getCy());
}

void CGfx::restoreRenderTarget()
{
    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)_rttPrevFBO);
    glViewport(_rttPrevViewport[0], _rttPrevViewport[1],
               _rttPrevViewport[2], _rttPrevViewport[3]);
}

void CGfx::clearRenderTexture(CTexturePtr tex, float r, float g, float b, float a)
{
    if (!tex || !tex->isUploaded())
        return;

    auto it = _rttFBOs.find(tex.get());
    if (it == _rttFBOs.end())
        return;

    GLint prevFBO = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);

    GLint prevViewport[4];
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    glBindFramebuffer(GL_FRAMEBUFFER, it->second);
    glViewport(0, 0, (GLsizei)tex->getCx(), (GLsizei)tex->getCy());

    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)prevFBO);
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
}

void CGfx::allocVertBuffMem()
{
    assert(_vertexSize);

    if (_vertexSize)
    {
        _nBuffMemSz  = MAX_VERTICIES * _vertexSize;
        _pVerBuffMem = malloc(_nBuffMemSz);
        memset(_pVerBuffMem, 0, _nBuffMemSz);

        _nIdxBuffMemSz = MAX_INDICES * sizeof(uint32_t);
        _pIdxBuffMem   = malloc(_nIdxBuffMemSz);
        memset(_pIdxBuffMem, 0, _nIdxBuffMemSz);
    }
}

bool CGfx::setupAttributes()
{
    glGenBuffers(1, &(_vertexBuff));
    logAssert_glError(223);

    glGenBuffers(1, &(_indexBuff));
    logAssert_glError(224);

    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);

    glBindBuffer(GL_ARRAY_BUFFER, _vertexBuff);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _indexBuff);

    _attribsLocations.a_position       = glGetAttribLocation(_progId, "a_position");
    _attribsLocations.a_texCoord       = glGetAttribLocation(_progId, "a_texCoord");
    _attribsLocations.a_mat3           = glGetAttribLocation(_progId, "a_mat3");
    _attribsLocations.a_tintColor      = glGetAttribLocation(_progId, "a_tintColor");
    _attribsLocations.a_reflectionType = glGetAttribLocation(_progId, "a_reflectionType");
    _attribsLocations.a_isNormalBlend  = glGetAttribLocation(_progId, "a_isNormalBlend");
    _attribsLocations.a_addRGB         = glGetAttribLocation(_progId, "a_addRGB");
    _attribsLocations.a_texMuls        = glGetAttribLocation(_progId, "a_texMuls");
    _attribsLocations.a_params         = glGetAttribLocation(_progId, "a_params");

    logAssert_glError(234);

    SAFE_ENABLE_ATTRIB(_attribsLocations.a_position);
    SAFE_ENABLE_ATTRIB(_attribsLocations.a_texCoord);
    SAFE_ENABLE_ATTRIB(_attribsLocations.a_mat3);
    SAFE_ENABLE_ATTRIB(_attribsLocations.a_mat3 + 1);
    SAFE_ENABLE_ATTRIB(_attribsLocations.a_mat3 + 2);
    SAFE_ENABLE_ATTRIB(_attribsLocations.a_tintColor);
    SAFE_ENABLE_ATTRIB(_attribsLocations.a_reflectionType);
    SAFE_ENABLE_ATTRIB(_attribsLocations.a_isNormalBlend);
    SAFE_ENABLE_ATTRIB(_attribsLocations.a_addRGB);
    SAFE_ENABLE_ATTRIB(_attribsLocations.a_texMuls);
    SAFE_ENABLE_ATTRIB(_attribsLocations.a_params);

    logAssert_glError(247);

    _vertexSize   = VERTEX_SIZE;
    _vertexOffset = 0;
    _indexOffset  = 0;

    allocVertBuffMem();

    logAssert_glError(248);

    uint32_t offset = 0;
    uint32_t stride = _vertexSize;

    SAFE_ATTRIB_POINTER(_attribsLocations.a_position, 2, GL_FLOAT, GL_FALSE, stride, offset);
    offset += 8;

    SAFE_ATTRIB_POINTER(_attribsLocations.a_texCoord, 2, GL_FLOAT, GL_FALSE, stride, offset);
    offset += 8;

    for (int i = 0; i < 3; ++i)
    {
        SAFE_ATTRIB_POINTER(_attribsLocations.a_mat3 + i, 3, GL_FLOAT, GL_FALSE, stride, offset);
        offset += 12;
    }

    SAFE_ATTRIB_POINTER(_attribsLocations.a_tintColor, 4, GL_FLOAT, GL_FALSE, stride, offset);
    offset += 16;

    SAFE_ATTRIB_POINTER(_attribsLocations.a_reflectionType, 4, GL_FLOAT, GL_FALSE, stride, offset);
    offset += 16;

    SAFE_ATTRIB_POINTER(_attribsLocations.a_isNormalBlend, 4, GL_FLOAT, GL_FALSE, stride, offset);
    offset += 16;

    SAFE_ATTRIB_POINTER(_attribsLocations.a_addRGB, 4, GL_FLOAT, GL_FALSE, stride, offset);
    offset += 16;

    SAFE_ATTRIB_POINTER(_attribsLocations.a_texMuls, 4, GL_FLOAT, GL_FALSE, stride, offset);
    offset += 16;

    SAFE_ATTRIB_POINTER(_attribsLocations.a_params, 3, GL_FLOAT, GL_FALSE, stride, offset);
    offset += 12;

    logAssert_glError(259);

    glBindVertexArray(0);

    return true;
}

bool CGfx::ensureSpace(size_t nVerts, size_t nIndices)
{
    if (VERTEX_SIZE * nVerts > _nBuffMemSz || nIndices * sizeof(uint32_t) > _nIdxBuffMemSz)
        return false;

    if (_vertexOffset * sizeof(float) + VERTEX_SIZE * nVerts > _nBuffMemSz ||
        _indexOffset * sizeof(uint32_t) + nIndices * sizeof(uint32_t) > _nIdxBuffMemSz)
    {
        flush();
    }

    return true;
}

void CGfx::appendQuadIndices(GLuint baseVertex, bool particleTopology)
{
    uint32_t* p = ((uint32_t*)_pIdxBuffMem) + _indexOffset;

    if (!particleTopology)
    {
        p[0] = baseVertex + 0;
        p[1] = baseVertex + 1;
        p[2] = baseVertex + 2;
        p[3] = baseVertex + 2;
        p[4] = baseVertex + 1;
        p[5] = baseVertex + 3;
    }
    else
    {
        p[0] = baseVertex + 1;
        p[1] = baseVertex + 2;
        p[2] = baseVertex + 0;
        p[3] = baseVertex + 0;
        p[4] = baseVertex + 2;
        p[5] = baseVertex + 3;
    }

    _indexOffset += 6;
    _numIndicesToDraw += 6;
}

void CGfx::appendSequentialIndices(GLuint baseVertex, size_t count)
{
    uint32_t* p = ((uint32_t*)_pIdxBuffMem) + _indexOffset;

    for (size_t i = 0; i < count; ++i)
        p[i] = baseVertex + (GLuint)i;

    _indexOffset += count;
    _numIndicesToDraw += (int)count;
}

void CGfx::appendIndexArray(GLuint baseVertex,  spine::Vector<unsigned short>* indices)
{
    uint32_t* p = ((uint32_t*)_pIdxBuffMem) + _indexOffset;

    for (size_t i = 0; i < indices->size(); ++i)
        p[i] = baseVertex + (*indices)[i];

    _indexOffset += indices->size();
    _numIndicesToDraw += (int)indices->size();
}

void CGfx::setScissor(Rect* pScissor)
{
    if (pScissor && !_bScissorEnabled)
        enableScissor(true);

    bool bFlush = false;

    if (_bScissorEnabled)
    {
        if (pScissor)
        {
            if (_bIsScissorsRectEmpty)
            {
                _rcScissor.copyFrom(pScissor);
                _bIsScissorsRectEmpty = false;
                bFlush = true;
            }
            else if (!(*pScissor == _rcScissor))
            {
                bFlush = true;
                _rcScissor.copyFrom(pScissor);
            }

            assert(_rcScissor.cx > 0 && _rcScissor.cy > 0);
        }
        else
        {
            _rcScissor.set(0, 0, _viewPortCx, _viewPortCy);

            if (!_bIsScissorsRectEmpty)
            {
                _bIsScissorsRectEmpty = true;
                bFlush = true;
            }
        }

        if (bFlush)
            flush();

        glScissor(_rcScissor.x, _viewPortCy - (_rcScissor.cy + _rcScissor.y), _rcScissor.cx, _rcScissor.cy);
    }
}

bool CGfx::getScissors(Rect* rcOut)
{
    if (_bScissorEnabled)
    {
        *rcOut = _rcScissor;
    }

    return _bScissorEnabled;
}

float CGfx::getCurrLayerOffsetX(void)
{
    return getLayerOffsetX(_eCurrLayer);
}

float CGfx::getCurrLayerOffsetY(void)
{
    return getLayerOffsetY(_eCurrLayer);
}

float CGfx::getLayerOffsetX(eRenderLayer eLayer)
{
    switch (eLayer)
    {
        case eRenderLayer::GAME:
            return CSceneResize::getInstance()->getGameOffsX();

        case eRenderLayer::BACKGORUND:
            return CSceneResize::getInstance()->getBgOffsX();

        default:
            assert(false);
            break;
    }

    return 0.0f;
}

float CGfx::getLayerOffsetY(eRenderLayer eLayer)
{
    switch (eLayer)
    {
        case eRenderLayer::GAME:
            return CSceneResize::getInstance()->getGameOffsY();

        case eRenderLayer::BACKGORUND:
            return CSceneResize::getInstance()->getBgOffsY();

        default:
            assert(false);
            break;
    }

    return 0.0f;
}

void CGfx::resetCamera(void)
{
    _camInfo = calculateCamera(0, 0, 1);
}

void CGfx::setCenterCamera(Point ptFocus, float fZoom)
{
    _camInfo = calculateCamera(ptFocus.x, ptFocus.y, fZoom);
}

void CGfx::enableScissor(bool bEnabled)
{
    if (_bScissorEnabled != bEnabled)
    {
        _bScissorEnabled = bEnabled;

        if (bEnabled)
        {
            glEnable(GL_SCISSOR_TEST);
            setScissor(NULL);
        }
        else
        {
            glDisable(GL_SCISSOR_TEST);
        }
    }
}

void CGfx::setViewPort(float cx, float cy)
{
    glViewport(0, 0, cx, cy);

    _viewPortCx = cx;
    _viewPortCy = cy;
}

void CGfx::updateViewPort()
{
    float fWidth  = CSceneResize::getInstance()->getScreenWidth();
    float fHeight = CSceneResize::getInstance()->getScreenHeight();

    setViewPort(fWidth, fHeight);
}

void CGfx::triggerShockWave(float x, float y)
{
    _wave.time = 0.0f;
    _wave.centerX = x / (float)CSceneResize::getInstance()->getScreenWidth();
    _wave.centerY = y / (float)CSceneResize::getInstance()->getScreenHeight();
}

bool CGfx::updateUniforms()
{
    for (size_t i = 0; i < _activeTextures.size() && i < SAMPLERS_COUNT; i++)
    {
        glUniform1i(_samplersUniforms[i], i);
        logAssert_glError(259);
    }

    return true;
}

bool CGfx::lookupUniforms()
{
    for (size_t i = 0; i < SAMPLERS_COUNT; i++)
    {
        std::string strUImage = std::format("u_images[{}]", i);
        _samplersUniforms[i] = glGetUniformLocation(_progId, strUImage.c_str());
        logAssert_glError(260);
    }
    _uZLayer          = glGetUniformLocation(_progId, "u_zLayer");
    _uItemCullingMask = glGetUniformLocation(_progId, "u_itemCullingMask");

    _shockTimeUniform    = glGetUniformLocation(_progId, "u_shockTime");
    _shockCenterUniform  = glGetUniformLocation(_progId, "u_shockCenter");
    _lowHPEffectUniform  = glGetUniformLocation(_progId, "u_lowHPEffect");
    _timeUniform         = glGetUniformLocation(_progId, "u_time");
    _tutParamsUniform    = glGetUniformLocation(_progId, "u_tutParams");
    _resolutionUniform   = glGetUniformLocation(_progId, "u_resolution");
    _tutRectSizeUniform  = glGetUniformLocation(_progId, "u_tutRectSize");

    for (int i = 0; i < 5; i++)
    {
        std::string posName = "u_hit" + std::to_string(i);
        std::string intensityName = "u_hitI" + std::to_string(i);

        _uHitPos[i] = glGetUniformLocation(_progId, posName.c_str());
        _uHitIntensity[i] = glGetUniformLocation(_progId, intensityName.c_str());
    }

    _uHitColor = glGetUniformLocation(_progId, "u_hitColor");

    _uLightBuffer     = glGetUniformLocation(_progId, "u_lightBuffer");
    _uLightVecBuffer  = -1;
    _uHasLights       = glGetUniformLocation(_progId, "u_hasLights");
    _uSpecularEnabled = glGetUniformLocation(_progId, "u_specularEnabled");
    _uAmbientLight    = glGetUniformLocation(_progId, "u_ambientLight");

    logAssert_glError(261);

    return true;
}

bool CGfx::flush()
{
    if (_vertexOffset && _indexOffset)
    {
        _drawCalls++;

        glBindVertexArray(_vao);

        glBindBuffer(GL_ARRAY_BUFFER, _vertexBuff);
        logAssert_glError(25);

        glBufferData(GL_ARRAY_BUFFER, _vertexOffset * sizeof(float), _pVerBuffMem, GL_DYNAMIC_DRAW);
        logAssert_glError(26);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _indexBuff);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, _indexOffset * sizeof(uint32_t), _pIdxBuffMem, GL_DYNAMIC_DRAW);

        glDrawElements(GL_TRIANGLES, _numIndicesToDraw, GL_UNSIGNED_INT, (void*)0);
        logAssert_glError(27);
    }

    _vertexOffset = 0;
    _indexOffset = 0;
    _numIndicesToDraw = 0;

    return true;
}

CSpritePtr CGfx::spriteFromTexture(CTexturePtr ptrTex)
{
    CSpritePtr ptrRes;

    assert(ptrTex);

    float cx = ptrTex->getCx();
    float cy = ptrTex->getCy();

    float uvs[CSprite::VERT_COUNT] = {
        0, 1, 0, 0, 1, 0, 1, 1
    };

    float verts[CSprite::VERT_COUNT] = {
        0, cy, 0, 0, cx, 0, cx, cy
    };

    ptrRes = CSprite::fromData(ptrTex, verts, uvs);

    return ptrRes;
}

CSpritePtr CGfx::spriteFromTexture(LPCTSTR lpszTexName)
{
    CSpritePtr ptrRes;

    auto s = std::format("{}/{}", Engine::getCfg().RES_DIR, lpszTexName);

    auto it = _all.find(s.c_str());

    assert(it != _all.end());

    if (it != _all.end())
    {
        float cx = it->second->getCx();
        float cy = it->second->getCy();

        float uvs[CSprite::VERT_COUNT] = {
            0, 1, 0, 0, 1, 0, 1, 1
        };

        float verts[CSprite::VERT_COUNT] = {
            0, cy, 0, 0, cx, 0, cx, cy
        };

        ptrRes = CSprite::fromData(it->second, verts, uvs);
    }

    return ptrRes;
}

bool CGfx::batchParticles(CTexturePtr pTex,
                          const ParticleQuadData* quads,
                          int numQuads,
                          float isNormalBlend,
                          bool skipLight)
{
    if (!skipLight)
        skipLight = !isLightAvailable();
    if (numQuads <= 0)
        return true;

    if (!pTex || !pTex->isUploaded())
        return false;

    pTex->updateLastUse();

    if (pTex->getSampler() < 0)
    {
        flush();
        setActiveTexture(pTex.get());

        return batchParticles(pTex, quads, numQuads, isNormalBlend, skipLight);
    }

    float normalSamplerIdx = -1.0f;

    if (_pFlatNormalTex && _pFlatNormalTex->isUploaded())
    {
        _pFlatNormalTex->updateLastUse();

        if (_pFlatNormalTex->getSampler() < 0)
        {
            flush();
            setActiveTexture(_pFlatNormalTex.get());

            return batchParticles(pTex, quads, numQuads, isNormalBlend, skipLight);
        }

        normalSamplerIdx = (float)_pFlatNormalTex->getSampler();
    }

    float* m33          = _matStack.top();
    float samplerIdx    = (float)pTex->getSampler();
    float tutIntensity  = _bIgnoreTutorialBox ? 0.0f : _tutorialBoxIntencity;
    float lightMul      = _lightIntensityMul;

    for (int q = 0; q < numQuads; ++q)
    {
        const ParticleQuadData& quad = quads[q];

        if (!ensureSpace(4, 6))
            return false;

        GLuint baseVertex = (GLuint)(_vertexOffset / VERTEX_FLOATS);
        float* pVertMem = (float*)(_pVerBuffMem) + _vertexOffset;
        int vo = 0;

        for (int c = 0; c < 4; ++c)
        {
            pVertMem[vo++] = quad.verts[c * 2];
            pVertMem[vo++] = quad.verts[c * 2 + 1];

            pVertMem[vo++] = quad.uvs[c * 2];
            pVertMem[vo++] = quad.uvs[c * 2 + 1];

            for (int i = 0; i < 9; ++i)
                pVertMem[vo++] = m33[i];

            pVertMem[vo++] = quad.rgba[0];
            pVertMem[vo++] = quad.rgba[1];
            pVertMem[vo++] = quad.rgba[2];
            pVertMem[vo++] = quad.rgba[3];

            pVertMem[vo++] = 0.0f;
            pVertMem[vo++] = 0.0f;
            pVertMem[vo++] = lightMul;
            pVertMem[vo++] = skipLight ? 1.0f : 0.0f;

            pVertMem[vo++] = isNormalBlend;
            pVertMem[vo++] = isNormalBlend;
            pVertMem[vo++] = isNormalBlend;
            pVertMem[vo++] = isNormalBlend;

            pVertMem[vo++] = 0.0f;
            pVertMem[vo++] = 0.0f;
            pVertMem[vo++] = 0.0f;
            pVertMem[vo++] = 0.0f;

            pVertMem[vo++] = samplerIdx;
            pVertMem[vo++] = normalSamplerIdx;
            pVertMem[vo++] = 0.0f;
            pVertMem[vo++] = 0.0f;

            pVertMem[vo++] = 0.0f;
            pVertMem[vo++] = 0.0f;
            pVertMem[vo++] = tutIntensity;
        }

        _vertexOffset += vo;
        appendQuadIndices(baseVertex, true);
    }

    return true;
}

bool CGfx::batchSprite(CTexturePtr pTex,
                       float* verts,
                       float* uvs,
                       float* rgba,
                       bool bGrayScale,
                       float fIsNormalBlend,
                       float* addRGB,
                       float effectType,
                       float lifeTime,
                       std::optional<float> optTutBoxIntencity,
                       CTexturePtr pNormalMapTex,
                       bool skipLight)
{
    constexpr float addR = 0.f;
    constexpr float addG = 0.f;
    constexpr float addB = 0.f;
    constexpr float addA = 0.f;

    if (!skipLight)
        skipLight = !isLightAvailable();

    if (!pTex->isUploaded())
        return false;

    pTex->updateLastUse();

    if (pTex->getSampler() < 0)
    {
        flush();
        setActiveTexture(pTex.get());

        return batchSprite(pTex, verts, uvs, rgba, bGrayScale, fIsNormalBlend, addRGB, effectType, lifeTime, optTutBoxIntencity, pNormalMapTex, skipLight);
    }

    float normalSamplerIdx = -1.f;

    if (pNormalMapTex && pNormalMapTex->isUploaded())
    {
        pNormalMapTex->updateLastUse();

        if (pNormalMapTex->getSampler() < 0)
        {
            flush();
            setActiveTexture(pNormalMapTex.get());

            return batchSprite(pTex, verts, uvs, rgba, bGrayScale, fIsNormalBlend, addRGB, effectType, lifeTime, optTutBoxIntencity, pNormalMapTex, skipLight);
        }

        normalSamplerIdx = (float)pNormalMapTex->getSampler();
    }
    else
    {
        if (_pFlatNormalTex && _pFlatNormalTex->isUploaded())
        {
            _pFlatNormalTex->updateLastUse();

            if (_pFlatNormalTex->getSampler() < 0)
            {
                flush();
                setActiveTexture(_pFlatNormalTex.get());

                return batchSprite(pTex, verts, uvs, rgba, bGrayScale, fIsNormalBlend, addRGB, effectType, lifeTime, optTutBoxIntencity, pNormalMapTex, skipLight);
            }

            normalSamplerIdx = (float)_pFlatNormalTex->getSampler();
        }
    }

    if (!ensureSpace(4, 6))
        return false;

    GLuint baseVertex = (GLuint)(_vertexOffset / VERTEX_FLOATS);
    float* pVertMem  = (float*)(_pVerBuffMem) + _vertexOffset;
    int vertexOffset = 0;

    float* m33 = _matStack.top();

    float samplerIdx = (pTex->getSampler() >= 0) ? float(pTex->getSampler()) : -1.f;
    float lightMul = _lightIntensityMul;

    int vx[4] = { CSprite::VERT_ULX, CSprite::VERT_URX, CSprite::VERT_BLX, CSprite::VERT_BRX };
    int vy[4] = { CSprite::VERT_ULY, CSprite::VERT_URY, CSprite::VERT_BLY, CSprite::VERT_BRY };

    for (int i = 0; i < 4; ++i)
    {
        pVertMem[vertexOffset++] = verts[vx[i]];
        pVertMem[vertexOffset++] = verts[vy[i]];

        pVertMem[vertexOffset++] = uvs[vx[i]];
        pVertMem[vertexOffset++] = uvs[vy[i]];

        for (int m = 0; m < 9; m++)
            pVertMem[vertexOffset++] = m33[m];

        pVertMem[vertexOffset++] = rgba[0];
        pVertMem[vertexOffset++] = rgba[1];
        pVertMem[vertexOffset++] = rgba[2];
        pVertMem[vertexOffset++] = rgba[3];

        pVertMem[vertexOffset++] = 0.0f;
        pVertMem[vertexOffset++] = 0.0f;
        pVertMem[vertexOffset++] = lightMul;
        pVertMem[vertexOffset++] = skipLight ? 1.0f : 0.0f;

        pVertMem[vertexOffset++] = fIsNormalBlend;
        pVertMem[vertexOffset++] = fIsNormalBlend;
        pVertMem[vertexOffset++] = fIsNormalBlend;
        pVertMem[vertexOffset++] = fIsNormalBlend;

        pVertMem[vertexOffset++] = addRGB ? addRGB[0] : addR;
        pVertMem[vertexOffset++] = addRGB ? addRGB[1] : addG;
        pVertMem[vertexOffset++] = addRGB ? addRGB[2] : addB;
        pVertMem[vertexOffset++] = addRGB ? addRGB[3] : addA;

        pVertMem[vertexOffset++] = samplerIdx;
        pVertMem[vertexOffset++] = normalSamplerIdx;
        pVertMem[vertexOffset++] = bGrayScale ? 1.f : 0.f;
        pVertMem[vertexOffset++] = 0.f;

        pVertMem[vertexOffset++] = lifeTime;
        pVertMem[vertexOffset++] = (float)effectType;

        if (optTutBoxIntencity.has_value())
            pVertMem[vertexOffset++] = *optTutBoxIntencity;
        else
            pVertMem[vertexOffset++] = _bIgnoreTutorialBox ? 0.0f : _tutorialBoxIntencity;
    }

    _vertexOffset += vertexOffset;
    appendQuadIndices(baseVertex, false);

    return true;
}

void CGfx::updateActiveTextures()
{
    for (size_t i = 0; i < _activeTextures.size(); i++)
    {
        CTexturePtr ptr = _activeTextures[i];

        ptr->setSampler(i);

        glActiveTexture(GL_TEXTURE0 + i);
        logAssert_glError(28);

        glBindTexture(GL_TEXTURE_2D, ptr->_handle);
        logAssert_glError(29);
    }
}
bool CGfx::isSamplerReserved(int idx) const
{
    return std::find(_reservedLightSamplers.begin(), _reservedLightSamplers.end(), idx)
           != _reservedLightSamplers.end();
}

void CGfx::reserveSampler(int idx)
{
    if (idx < 0)
        return;

    if (!isSamplerReserved(idx))
        _reservedLightSamplers.push_back(idx);
}

void CGfx::unreserveSampler(int idx)
{
    _reservedLightSamplers.erase(
        std::remove(_reservedLightSamplers.begin(), _reservedLightSamplers.end(), idx),
        _reservedLightSamplers.end()
    );
}

void CGfx::clearReservedSamplers()
{
    _reservedLightSamplers.clear();
}

bool CGfx::hasFreeSamplerForLightTexture()
{
    if (_activeTextures.size() < getMaxSamplers())
        return true;

    for (size_t i = 0; i < _activeTextures.size(); ++i)
    {
        if (!isSamplerReserved((int)i))
            return true;
    }

    return false;
}

bool CGfx::canReserveLightSampler()
{

    if (getMaxSamplers() <= 3)
        return true;

    return _reservedLightSamplers.size() < (getMaxSamplers() - 2);
}

void CGfx::setActiveTexture(CTexture* p)
{
    CTexturePtr ptr = p->getPtr();

    assert(ptr);

    for (size_t i = 0; i < _activeTextures.size(); ++i)
    {
        if (_activeTextures[i].get() == ptr.get())
        {
            ptr->setSampler((int)i);

            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, ptr->_handle);

            return;
        }
    }

    assert(_activeTextures.size() <= getMaxSamplers());

    if (_activeTextures.size() < getMaxSamplers())
    {
        _activeTextures.push_back(ptr);
        ptr->setSampler((int)(_activeTextures.size() - 1));

        glActiveTexture(GL_TEXTURE0 + _activeTextures.size() - 1);
        glBindTexture(GL_TEXTURE_2D, ptr->_handle);
    }
    else if (_activeTextures.size() == getMaxSamplers())
    {
        int victim = -1;

        for (size_t attempts = 0; attempts < getMaxSamplers(); ++attempts)
        {
            _nLastActiveTexture++;

            if (_nLastActiveTexture >= _activeTextures.size())
                _nLastActiveTexture = 0;

            if (!isSamplerReserved((int)_nLastActiveTexture))
            {
                victim = (int)_nLastActiveTexture;
                break;
            }
        }

        if (victim < 0)
        {
            _nLastActiveTexture++;

            if (_nLastActiveTexture >= _activeTextures.size())
                _nLastActiveTexture = 0;

            victim = (int)_nLastActiveTexture;
        }

        unreserveSampler(victim);

        _activeTextures[victim]->unsetSampler();
        _activeTextures[victim] = ptr;

        ptr->setSampler(victim);

        glActiveTexture(GL_TEXTURE0 + victim);
        glBindTexture(GL_TEXTURE_2D, ptr->_handle);
    }
}
void CGfx::createTempTextures()
{
    GLuint* pTexIds = new GLuint[getMaxSamplers()];
    uint8_t texBytes[4] = {0};

    logAssert_glError(262);

    if (SAMPLERS_COUNT)
    {
        glGenTextures(SAMPLERS_COUNT, pTexIds);
        logAssert_glError(9);

        for (size_t i = 0; i < SAMPLERS_COUNT; i++)
        {
            std::string strTextName = std::format("*$?$?_TEMP_{}", i);
            CTexturePtr ptr         = std::make_shared<CTexture>();

            _all[strTextName] = ptr;

            GLuint texHandle = pTexIds[i];

            ptr->_handle = texHandle;
            ptr->setPath(strTextName.c_str());

            ptr->_cx = 1;
            ptr->_cy = 1;
            ptr->setUploaded();

            glBindTexture(GL_TEXTURE_2D, texHandle);
            logAssert_glError(10);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, texBytes);
            logAssert_glError(11);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
            logAssert_glError(12);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
            logAssert_glError(13);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            logAssert_glError(14);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            logAssert_glError(15);

            setActiveTexture(ptr.get());
            logAssert_glError(16);
        }

        delete[] pTexIds;
    }

    {
        uint8_t flatNormalBytes[4] = {128, 128, 255, 255};

        GLuint flatNormalHandle = 0;
        glGenTextures(1, &flatNormalHandle);

        glBindTexture(GL_TEXTURE_2D, flatNormalHandle);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, flatNormalBytes);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        _pFlatNormalTex = std::make_shared<CTexture>();

        _pFlatNormalTex->_handle = flatNormalHandle;
        _pFlatNormalTex->_cx = 1;
        _pFlatNormalTex->_cy = 1;
        _pFlatNormalTex->setPath("*FLAT_NORMAL*");
        _pFlatNormalTex->setUploaded();
        _pFlatNormalTex->unsetSampler();
    }
}

CTexturePtr CGfx::getTextureById(LPCTSTR lpszTexId, bool bFullPath)
{
    LPCTSTR lpszPath = lpszTexId;
    std::string s;
    if (!bFullPath)
    {
        s = std::format("{}/{}", Engine::getCfg().RES_DIR, lpszTexId);
        lpszPath = s.c_str();
    }
    CTexturePtr pRes;

    MapTexturesIt it = _all.find(lpszPath);

    if (it != _all.end())
        pRes = it->second;

    return pRes;
}

GLuint CGfx::allocAndBindGPUTexture()
{
    GLuint texHandle = 0;

    glGenTextures(1, &texHandle);
    logAssert_glError(222);

    assert(texHandle);

    if (texHandle)
    {
        glBindTexture(GL_TEXTURE_2D, texHandle);
        logAssert_glError(17);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        logAssert_glError(18);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        logAssert_glError(19);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        logAssert_glError(20);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        logAssert_glError(21);
    }

    return texHandle;
}

void* CGfx::decodeImage(LPCTSTR lpszTexId, int bytesSize, const void* imgBytes, int& cx, int& cy)
{
    void*  ptrRes = nullptr;

    cx = 0;
    cy = 0;

    int nchans = 0;

    auto bitmapBytes = stbi_load_from_memory((stbi_uc*)imgBytes, bytesSize, &cx, &cy, &nchans, STBI_rgb_alpha);

    assert(bitmapBytes);

    if (bitmapBytes)
    {
        ptrRes = bitmapBytes;
    }

    return ptrRes;
}

void CGfx::freeDecodedImage(void* decodedBytes)
{
    stbi_image_free(decodedBytes);
}

void CGfx::uploadAssets(LPCTSTR lpszTexId, int numOfPngs)
{
    std::string strTexId;

    for (int i = 1; i <= numOfPngs; i++)
    {
        if (i == 1)
        {
            strTexId = std::format("chars/{}.png", lpszTexId);
        }
        else
        {
            strTexId = std::format("chars/{}_{}.png", lpszTexId, i);
        }

        uploadAsset(strTexId.c_str());
    }
}

CTexturePtr CGfx::uploadAsset(LPCTSTR lpszFileName, const uint8_t* pBytes, size_t nSize)
{
    CTexturePtr pRes = getTextureById(lpszFileName);

    if (!pRes)
    {
        auto itUnloaded = _unloaded.find(lpszFileName);
        if (itUnloaded != _unloaded.end())
        {
            pRes = itUnloaded->second;
            _unloaded.erase(itUnloaded);
        }

        int cx, cy;

        auto ptrDecodedBytes = decodeImage(lpszFileName, nSize, pBytes, cx, cy);

        assert(ptrDecodedBytes);

        if (ptrDecodedBytes)
        {
            flush();

            GLuint texHandle = allocAndBindGPUTexture();

            if (texHandle)
            {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, cx, cy, 0, GL_RGBA, GL_UNSIGNED_BYTE, ptrDecodedBytes);
                logAssert_glError(22);

                freeDecodedImage(ptrDecodedBytes);

                if (!pRes)
                    pRes = std::make_shared<CTexture>();

                _all[lpszFileName] = pRes;

                pRes->_handle = texHandle;
                pRes->_cx     = cx;
                pRes->_cy     = cy;

                pRes->unsetSampler();
                pRes->setPath(lpszFileName);
                pRes->setUploaded();

                LOG_TRACE_FMT("Texture %s uploaded", lpszFileName);

                updateActiveTextures();
            }
        }
    }
    else
    {
        auto it = _toUnload.find(pRes);
        if (it != _toUnload.end())
        {
            _toUnload.erase(it);
        }
    }

    return pRes;
}

CTexturePtr CGfx::uploadAsset(LPCTSTR lpszTexId, bool bFullPath)
{
    std::string strTexId = lpszTexId;

    CTexturePtr pRes = getTextureById(strTexId.c_str(), bFullPath);

    if (pRes && pRes->isUploaded())
    {
        return pRes;
    }
    else
    {
        int nSize = 0;

        auto pBytes = AssetLoader::instance().getLoadedFile(lpszTexId, nSize);

        assert(pBytes);
        assert(!pRes);

        auto itUnloaded = _unloaded.find(lpszTexId);
        if (itUnloaded != _unloaded.end())
        {
            pRes = itUnloaded->second;
            _unloaded.erase(itUnloaded);
        }

        int cx, cy;

        auto ptrDecodedBytes = decodeImage(lpszTexId, nSize, pBytes, cx, cy);

        assert(ptrDecodedBytes);

        if (ptrDecodedBytes)
        {
            flush();

            GLuint texHandle = allocAndBindGPUTexture();

            if (texHandle)
            {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, cx, cy, 0, GL_RGBA, GL_UNSIGNED_BYTE, ptrDecodedBytes);
                logAssert_glError(22);

                freeDecodedImage(ptrDecodedBytes);

                if (!pRes)
                    pRes = std::make_shared<CTexture>();

                _all[strTexId] = pRes;

                pRes->_handle = texHandle;
                pRes->_cx     = cx;
                pRes->_cy     = cy;

                pRes->unsetSampler();
                pRes->setPath(strTexId.c_str());
                pRes->setUploaded();

                LOG_TRACE_FMT("Texture %s uploaded", strTexId.c_str());

                updateActiveTextures();
            }
        }
    }

    return pRes;
}

void CGfx::uploadGrainTexture()
{
    auto& cfg = Engine::getCfg();

    if (!_ptrTexGrain)
    {
        int nSize = 0;

        auto pBytes = AssetLoader::instance().getLoadedFile("noise.png", nSize);

        assert(pBytes);

        int cx, cy;

        auto ptrDecodedBytes = decodeImage("noise.png", nSize, pBytes, cx, cy);

        assert(ptrDecodedBytes);

        if (ptrDecodedBytes)
        {
            flush();

            GLuint texHandle = allocAndBindGPUTexture();

            if (texHandle)
            {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, cx, cy, 0, GL_RGBA, GL_UNSIGNED_BYTE, ptrDecodedBytes);
                logAssert_glError(22);

                freeDecodedImage(ptrDecodedBytes);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                logAssert_glError(221);

                _ptrTexGrain = std::make_shared<CTexture>();

                _ptrTexGrain->_handle = texHandle;
                _ptrTexGrain->_cx     = cx;
                _ptrTexGrain->_cy     = cy;

                _ptrTexGrain->setUploaded();

                updateActiveTextures();
            }
        }
    }
}

CTexturePtr CGfx::uploadFontStahTexture(int cx, int cy)
{
    CTexturePtr pRes;

    flush();

    GLuint texHandle = allocAndBindGPUTexture();

    if (texHandle)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, cx, cy, 0, GL_ALPHA, GL_UNSIGNED_BYTE, 0);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        logAssert_glError(22);

        if (!pRes)
        {
            pRes = std::make_shared<CTexture>();

            pRes->_handle = texHandle;
            pRes->_cx     = cx;
            pRes->_cy     = cy;

            pRes->setUploaded();
        }

        updateActiveTextures();
    }

    return pRes;
}

void CGfx::updateFontStashTexture(CTexturePtr ptrTex, int* rect, const unsigned char* data)
{
    flush();

    int x = rect[0];
    int y = rect[1];
    int w = rect[2] - rect[0];
    int h = rect[3] - rect[1];

    int fullWidth = ptrTex->getCx();

    glBindTexture(GL_TEXTURE_2D, ptrTex->_handle);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    if (_isWebGL2)
    {
        glPixelStorei(GL_UNPACK_ROW_LENGTH, fullWidth);
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, x);
        glPixelStorei(GL_UNPACK_SKIP_ROWS, y);

        glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_ALPHA, GL_UNSIGNED_BYTE, data);

        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
        glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    }
    else
    {
        for (int i = 0; i < h; ++i)
        {
            const unsigned char* rowData = data + ((y + i) * fullWidth + x);
            glTexSubImage2D(GL_TEXTURE_2D, 0, x, y + i, w, 1, GL_ALPHA, GL_UNSIGNED_BYTE, rowData);
        }
    }

    updateActiveTextures();
}

void CGfx::deleteTexture(CTexturePtr ptrTex)
{
    if (ptrTex->isUploaded())
    {
        for (size_t i = 0; i < _activeTextures.size(); ++i)
            _activeTextures[i]->unsetSampler();

        _activeTextures.clear();

        _nLastActiveTexture = -1;

        clearReservedSamplers();

        glDeleteTextures(1, &ptrTex->_handle);

        ptrTex->setUploaded(false);

        if (ptrTex->getPath()[0])
        {
            _all.erase(ptrTex->getPath());
        }
    }
}

void CGfx::unloadAsset(LPCTSTR lpszTexId, int numOfPngs)
{
    for (int i = 1; i <= numOfPngs; i++)
    {
        if (i == 1)
        {
            auto strTexId = std::format("chars/{}.png", lpszTexId);

            auto it = _all.find(strTexId.c_str());

            assert(it != _all.end());

            if (it != _all.end())
            {
                unloadTexture(it->second);
            }
        }
        else
        {
            std::string strFilePng = std::format("chars/{}_{}.png", lpszTexId, i);

            auto it = _all.find(strFilePng.c_str());

            assert(it != _all.end());

            if (it != _all.end())
            {
                unloadTexture(it->second);
            }
        }
    }
}

void CGfx::saveLoadedTextures()
{
    assert(_saved.empty());

    for (auto& it : _all)
    {
        _saved.insert(it.second);
    }
}

void CGfx::freeAllNotSavedTextures()
{
    for (auto& it : _all)
    {
        if (_saved.find(it.second) == _saved.end())
        {
            unloadTexture(it.second);
        }
    }

    _saved.clear();
}

void CGfx::unloadTexture(CTexturePtr ptr)
{
    if (ptr->isUploaded())
        _toUnload.insert(ptr);
}

void CGfx::unloadTexture(LPCTSTR lpszTexId)
{
    auto it = _all.find(lpszTexId);

    if (it != _all.end())
        _toUnload.insert(it->second);
}

bool CGfx::batchFontStashVerts(CTexturePtr pTex,
                               const float* verts,
                               const float* tcoords,
                               const unsigned int* colors,
                               int nverts,
                               bool skipLight)
{
    if (!skipLight)
        skipLight = !isLightAvailable();

    if (!pTex->isUploaded())
        return false;

    if (pTex->getSampler() < 0)
    {
        flush();
        setActiveTexture(pTex.get());
        return batchFontStashVerts(pTex, verts, tcoords, colors, nverts, skipLight);
    }

    if (!ensureSpace((size_t)nverts, (size_t)nverts))
        return false;

    GLuint baseVertex = (GLuint)(_vertexOffset / VERTEX_FLOATS);
    float* pVertMem = (float*)(_pVerBuffMem) + _vertexOffset;
    int vo = 0;

    float* m33 = _matStack.top();
    float samplerIdx = (pTex->getSampler() >= 0) ? float(pTex->getSampler()) : -1.0f;
    float tutIntensity = _bIgnoreTutorialBox ? 0.f : _tutorialBoxIntencity;

    for (int ii = 0; ii < nverts; ++ii)
    {
        int nXIdx = ii * 2;
        int nYIdx = ii * 2 + 1;

        pVertMem[vo++] = verts[nXIdx];
        pVertMem[vo++] = verts[nYIdx];

        pVertMem[vo++] = tcoords[nXIdx];
        pVertMem[vo++] = tcoords[nYIdx];

        for (int i = 0; i < 9; i++)
            pVertMem[vo++] = m33[i];

        uint32_t dwColor = colors[ii];

        uint8_t a = dwColor & 0x000000FF;
        uint8_t b = (dwColor & 0x0000FF00) >> 8;
        uint8_t g = (dwColor & 0x00FF0000) >> 16;
        uint8_t r = (dwColor & 0xFF000000) >> 24;

        pVertMem[vo++] = 1.f;
        pVertMem[vo++] = 1.f;
        pVertMem[vo++] = 1.f;
        pVertMem[vo++] = static_cast<float>(a) / 255.f;

        pVertMem[vo++] = 0.f;
        pVertMem[vo++] = 0.f;
        pVertMem[vo++] = 1.f;
        pVertMem[vo++] = skipLight ? 1.0f : 0.0f;

        pVertMem[vo++] = 1.f;
        pVertMem[vo++] = 1.f;
        pVertMem[vo++] = 1.f;
        pVertMem[vo++] = 1.f;

        pVertMem[vo++] = static_cast<float>(r) / 255.f;
        pVertMem[vo++] = static_cast<float>(g) / 255.f;
        pVertMem[vo++] = static_cast<float>(b) / 255.f;
        pVertMem[vo++] = 1.f;

        pVertMem[vo++] = samplerIdx;
        pVertMem[vo++] = 0.f;
        pVertMem[vo++] = 0.f;
        pVertMem[vo++] = 0.f;

        pVertMem[vo++] = _fTextEffectLifeTime;
        pVertMem[vo++] = _fTextEffect;
        pVertMem[vo++] = tutIntensity;
    }

    _vertexOffset += vo;
    appendSequentialIndices(baseVertex, (size_t)nverts);

    return true;
}

void CGfx::animateWorldDarken(float dt)
{
    float fMinD     = _fMinEnvDarken;
    float fMinGameD = _fMinGameDarken;
    float fD        = 1.f;

    constexpr float fMaxDarkenTime = 1.f;

    switch (_eDarkenState)
    {
        case eWorldDarkenState::BASIC:
        {
        }
        break;

        case eWorldDarkenState::DARKENING:
        {
            _fDarkenWorldTimer += dt;

            if (_fDarkenWorldTimer >= fMaxDarkenTime)
            {
                _fDarkenWorldTimer = fMaxDarkenTime;
                _eDarkenState      = eWorldDarkenState::DARK;

                if (_onWorldDarkenCb)
                {
                    _onWorldDarkenCb();
                }
            }

            fD = std::max(0.f, 1.f - _fDarkenWorldTimer / fMaxDarkenTime);
        }
        break;

        case eWorldDarkenState::BRIGHTENING:
        {
            _fDarkenWorldTimer -= dt;

            if (_fDarkenWorldTimer <= 0.f)
            {
                _fDarkenWorldTimer = 0.f;
                _eDarkenState      = eWorldDarkenState::BASIC;

                if (_onWorldDarkenCb)
                {
                    _onWorldDarkenCb();
                    _onWorldDarkenCb = {};
                }
            }

            fD = 1.f - _fDarkenWorldTimer / fMaxDarkenTime;
        }
        break;

        case eWorldDarkenState::DARK:
        {
            fD = 0.f;
        }
        break;
    }

    float fdGame = std::max(fMinGameD, fD);
    float fdEnv  = std::max(fMinD, fD);

    _gameRoot->setTint(fdGame, fdGame, fdGame);
    _bgRoot->setTint(fdEnv, fdEnv, fdEnv);
    _fgRoot->setTint(fdEnv, fdEnv, fdEnv);
}

bool CGfx::batchRawInterleaved(CTexture* pTex,
                               const float* data,
                               int vertexCount,
                               float r, float g, float b, float a,
                               CTexturePtr pNormalMapTex,
                               bool skipLight,
                               float reflFadeStartY,
                               float reflFadeEndY)
{
    constexpr int DATA_FLOATS_PER_VERT = 6;

    if (!skipLight)
        skipLight = !isLightAvailable();

    if (vertexCount <= 0)
        return true;

    if (!pTex->isUploaded())
        return false;

    pTex->updateLastUse();

    if (pTex->getSampler() < 0)
    {
        flush();
        setActiveTexture(pTex);

        return batchRawInterleaved(pTex, data, vertexCount, r, g, b, a, pNormalMapTex, skipLight, reflFadeStartY, reflFadeEndY);
    }

    float normalSamplerIdx = -1.0f;

    if (pNormalMapTex && pNormalMapTex->isUploaded())
    {
        pNormalMapTex->updateLastUse();

        if (pNormalMapTex->getSampler() < 0)
        {
            flush();
            setActiveTexture(pNormalMapTex.get());

            return batchRawInterleaved(pTex, data, vertexCount, r, g, b, a, pNormalMapTex, skipLight, reflFadeStartY, reflFadeEndY);
        }

        normalSamplerIdx = (float)pNormalMapTex->getSampler();
    }
    else
    {
        if (_pFlatNormalTex && _pFlatNormalTex->isUploaded())
        {
            _pFlatNormalTex->updateLastUse();

            if (_pFlatNormalTex->getSampler() < 0)
            {
                flush();
                setActiveTexture(_pFlatNormalTex.get());

                return batchRawInterleaved(pTex, data, vertexCount, r, g, b, a, pNormalMapTex, skipLight, reflFadeStartY, reflFadeEndY);
            }

            normalSamplerIdx = (float)_pFlatNormalTex->getSampler();
        }
    }

    float* m33 = _matStack.top();

    float samplerIdx = (pTex->getSampler() >= 0) ? float(pTex->getSampler()) : -1.0f;
    float tutIntensity = _bIgnoreTutorialBox ? 0.f : _tutorialBoxIntencity;

    constexpr float addR = 0.f;
    constexpr float addG = 0.f;
    constexpr float addB = 0.f;
    constexpr float addA = 0.f;

    float lightMul = _lightIntensityMul;

    int processed = 0;

    while (processed < vertexCount)
    {
        int remaining = vertexCount - processed;
        int chunk = std::min(remaining, 3072);

        if (chunk > 3)
            chunk -= chunk % 3;

        if (!ensureSpace(chunk, chunk))
            return false;

        GLuint baseVertex = (GLuint)(_vertexOffset / VERTEX_FLOATS);
        float* pVertMem = (float*)(_pVerBuffMem) + _vertexOffset;
        int vo = 0;

        for (int i = 0; i < chunk; ++i)
        {
            int base = (processed + i) * DATA_FLOATS_PER_VERT;

            pVertMem[vo++] = data[base + 0];
            pVertMem[vo++] = data[base + 1];
            pVertMem[vo++] = data[base + 2];
            pVertMem[vo++] = data[base + 3];

            float fIsBlendNormal = data[base + 4];
            float fAlpha = data[base + 5];

            for (int m = 0; m < 9; m++)
                pVertMem[vo++] = m33[m];

            pVertMem[vo++] = r;
            pVertMem[vo++] = g;
            pVertMem[vo++] = b;
            pVertMem[vo++] = a * fAlpha;

            pVertMem[vo++] = 0.0f;
            pVertMem[vo++] = 0.0f;
            pVertMem[vo++] = lightMul;
            pVertMem[vo++] = skipLight ? 1.0f : 0.0f;

            pVertMem[vo++] = fIsBlendNormal;
            pVertMem[vo++] = fIsBlendNormal;
            pVertMem[vo++] = fIsBlendNormal;
            pVertMem[vo++] = fIsBlendNormal;

            pVertMem[vo++] = addR;
            pVertMem[vo++] = addG;
            pVertMem[vo++] = addB;
            pVertMem[vo++] = addA;

            pVertMem[vo++] = samplerIdx;
            pVertMem[vo++] = normalSamplerIdx;
            pVertMem[vo++] = 0.f;
            pVertMem[vo++] = 0.f;

            pVertMem[vo++] = 0.f;
            pVertMem[vo++] = 0.f;

            pVertMem[vo++] = tutIntensity;
        }

        _vertexOffset += vo;
        appendSequentialIndices(baseVertex, chunk);

        processed += chunk;
    }

    return true;
}

bool CGfx::batchSpineVect(CTexture* pTex,
                          spine::Vector<float>* verts,
                          spine::Vector<float>* uvs,
                          spine::Vector<unsigned short>* indices,
                          float r, float g, float b, float a,
                          float gsr, float gsg, float gsb, float gsa,
                          float fIsBlendNormal,
                          float fEffectType,
                          float fEffectLifeTime,
                          CTexturePtr pNormalMapTex,
                          bool skipLight,
                          float reflFadeStartY,
                          float reflFadeEndY)
{
    constexpr float addR = 0.f;
    constexpr float addG = 0.f;
    constexpr float addB = 0.f;
    constexpr float addA = 0.f;

    if (!skipLight)
        skipLight = !isLightAvailable();

    if (!pTex->isUploaded())
        return false;

    pTex->updateLastUse();

    if (pTex->getSampler() < 0)
    {
        flush();
        setActiveTexture(pTex);

        return batchSpineVect(pTex, verts, uvs, indices, r, g, b, a, gsr, gsg, gsb, gsa, fIsBlendNormal, fEffectType, fEffectLifeTime, pNormalMapTex, skipLight, reflFadeStartY, reflFadeEndY);
    }

    float normalSamplerIdx = -1.0f;

    if (pNormalMapTex && pNormalMapTex->isUploaded())
    {
        pNormalMapTex->updateLastUse();

        if (pNormalMapTex->getSampler() < 0)
        {
            flush();
            setActiveTexture(pNormalMapTex.get());

            return batchSpineVect(pTex, verts, uvs, indices, r, g, b, a, gsr, gsg, gsb, gsa, fIsBlendNormal, fEffectType, fEffectLifeTime, pNormalMapTex, skipLight, reflFadeStartY, reflFadeEndY);
        }

        normalSamplerIdx = (float)pNormalMapTex->getSampler();
    }
    else
    {
        if (_pFlatNormalTex && _pFlatNormalTex->isUploaded())
        {
            _pFlatNormalTex->updateLastUse();

            if (_pFlatNormalTex->getSampler() < 0)
            {
                flush();
                setActiveTexture(_pFlatNormalTex.get());

                return batchSpineVect(pTex, verts, uvs, indices, r, g, b, a, gsr, gsg, gsb, gsa, fIsBlendNormal, fEffectType, fEffectLifeTime, pNormalMapTex, skipLight, reflFadeStartY, reflFadeEndY);
            }

            normalSamplerIdx = (float)_pFlatNormalTex->getSampler();
        }
    }

    int vertexCount = (int)(verts->size() / 2);
    int indexCount = (int)indices->size();

    if (vertexCount <= 0 || indexCount <= 0)
        return true;

    if (!ensureSpace(vertexCount, indexCount))
        return false;

    GLuint baseVertex = (GLuint)(_vertexOffset / VERTEX_FLOATS);
    float* pVertMem = (float*)(_pVerBuffMem) + _vertexOffset;
    int vertexOffset = 0;

    float* m33 = _matStack.top();
    float samplerIdx = (pTex->getSampler() >= 0) ? float(pTex->getSampler()) : -1.0f;
    float lightMul = _lightIntensityMul;

    for (int i = 0; i < vertexCount; ++i)
    {
        int xIdx = i * 2;
        int yIdx = i * 2 + 1;

        pVertMem[vertexOffset++] = (*verts)[xIdx];
        pVertMem[vertexOffset++] = (*verts)[yIdx];

        pVertMem[vertexOffset++] = (*uvs)[xIdx];
        pVertMem[vertexOffset++] = (*uvs)[yIdx];

        for (int m = 0; m < 9; ++m)
            pVertMem[vertexOffset++] = m33[m];

        pVertMem[vertexOffset++] = r;
        pVertMem[vertexOffset++] = g;
        pVertMem[vertexOffset++] = b;
        pVertMem[vertexOffset++] = a;

        pVertMem[vertexOffset++] = 0.0f;
        pVertMem[vertexOffset++] = 0.0f;
        pVertMem[vertexOffset++] = lightMul;
        pVertMem[vertexOffset++] = skipLight ? 1.0f : 0.0f;

        for (int n = 0; n < 4; ++n)
            pVertMem[vertexOffset++] = fIsBlendNormal;

        pVertMem[vertexOffset++] = addR;
        pVertMem[vertexOffset++] = addG;
        pVertMem[vertexOffset++] = addB;
        pVertMem[vertexOffset++] = addA;

        pVertMem[vertexOffset++] = samplerIdx;
        pVertMem[vertexOffset++] = normalSamplerIdx;
        pVertMem[vertexOffset++] = 0.f;
        pVertMem[vertexOffset++] = 0.f;

        pVertMem[vertexOffset++] = fEffectLifeTime;
        pVertMem[vertexOffset++] = fEffectType;
        pVertMem[vertexOffset++] = _bIgnoreTutorialBox ? 0.f : _tutorialBoxIntencity;
    }

    _vertexOffset += vertexOffset;
    appendIndexArray(baseVertex, indices);

    return true;
}

bool CGfx::batchVert(CTexture* pTex,
                     float x, float y, float u, float v,
                     float r, float g, float b, float a,
                     float gsr, float gsg, float gsb, float gsa,
                     float isNormalBlendMode,
                     CTexturePtr pNormalMapTex,
                     bool skipLight)
{
    if (!skipLight)
        skipLight = !isLightAvailable();

    if (!pTex->isUploaded())
        return false;

    pTex->updateLastUse();

    if (pTex->getSampler() < 0)
    {
        flush();
        setActiveTexture(pTex);

        return batchVert(pTex, x, y, u, v, r, g, b, a, gsr, gsg, gsb, gsa, isNormalBlendMode, pNormalMapTex, skipLight);
    }

    float normalSamplerIdx = -1.0f;

    if (pNormalMapTex && pNormalMapTex->isUploaded())
    {
        pNormalMapTex->updateLastUse();

        if (pNormalMapTex->getSampler() < 0)
        {
            flush();
            setActiveTexture(pNormalMapTex.get());

            return batchVert(pTex, x, y, u, v, r, g, b, a, gsr, gsg, gsb, gsa, isNormalBlendMode, pNormalMapTex, skipLight);
        }

        normalSamplerIdx = (float)pNormalMapTex->getSampler();
    }
    else
    {
        if (_pFlatNormalTex && _pFlatNormalTex->isUploaded())
        {
            _pFlatNormalTex->updateLastUse();

            if (_pFlatNormalTex->getSampler() < 0)
            {
                flush();
                setActiveTexture(_pFlatNormalTex.get());

                return batchVert(pTex, x, y, u, v, r, g, b, a, gsr, gsg, gsb, gsa, isNormalBlendMode, pNormalMapTex, skipLight);
            }

            normalSamplerIdx = (float)_pFlatNormalTex->getSampler();
        }
    }

    if (!ensureSpace(1, 1))
        return false;

    GLuint baseVertex = (GLuint)(_vertexOffset / VERTEX_FLOATS);
    float* pVertMem = (float*)(_pVerBuffMem) + _vertexOffset;
    int vertexOffset = 0;

    float* m33 = _matStack.top();

    float samplerIdx = (pTex->getSampler() >= 0) ? float(pTex->getSampler()) : -1.0f;
    float lightMul = _lightIntensityMul;

    pVertMem[vertexOffset++] = x;
    pVertMem[vertexOffset++] = y;
    pVertMem[vertexOffset++] = u;
    pVertMem[vertexOffset++] = v;

    for (int i = 0; i < 9; i++)
        pVertMem[vertexOffset++] = m33[i];

    pVertMem[vertexOffset++] = r;
    pVertMem[vertexOffset++] = g;
    pVertMem[vertexOffset++] = b;
    pVertMem[vertexOffset++] = a;

    pVertMem[vertexOffset++] = 0.0f;
    pVertMem[vertexOffset++] = 0.0f;
    pVertMem[vertexOffset++] = lightMul;
    pVertMem[vertexOffset++] = skipLight ? 1.0f : 0.0f;

    for (int i = 0; i < 4; i++)
        pVertMem[vertexOffset++] = isNormalBlendMode;

    pVertMem[vertexOffset++] = 0.f;
    pVertMem[vertexOffset++] = 0.f;
    pVertMem[vertexOffset++] = 0.f;
    pVertMem[vertexOffset++] = 0.f;

    pVertMem[vertexOffset++] = samplerIdx;
    pVertMem[vertexOffset++] = normalSamplerIdx;
    pVertMem[vertexOffset++] = 0.f;
    pVertMem[vertexOffset++] = 0.f;

    pVertMem[vertexOffset++] = 0.f;
    pVertMem[vertexOffset++] = 0.f;

    pVertMem[vertexOffset++] = _bIgnoreTutorialBox ? 0.f : _tutorialBoxIntencity;

    _vertexOffset += vertexOffset;
    appendSequentialIndices(baseVertex, 1);

    return true;
}

void CGfx::begin(float dt)
{
    if (!_ppSettings.bloom.enabled)
    {
        if (_bloomInitialized)
            destroyBloom();
    }
    else
    {
        if (!_bloomInitialized)
            initBloom();
    }

    GLint iViewport[4];
    glGetIntegerv(GL_VIEWPORT, iViewport);

    float fWidth  = (float)iViewport[2];
    float fHeight = (float)iViewport[3];

    _eCurrLayer = eRenderLayer::BACKGORUND;

    float finalZoom = _camInfo.zoom + _zoomShakeIntensity;

    float offsetBgX = 0, offsetBgY = 0, offsetGmX = 0, offsetGmY = 0;

    if (_shakeIntensity > 0)
    {
        offsetBgX = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * _shakeIntensity * 0.3f;
        offsetBgY = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * _shakeIntensity * 0.3f;
        offsetGmX = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * _shakeIntensity;
        offsetGmY = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * _shakeIntensity;
    }

    if (!_lightBufferInitialized)
        initLightBuffer();

    s_uploadedLightCount = 0;

    _drawCalls = 0;

    restoreVertexFormat();

    if (_ppInitialized && isPostProcessEnebled())
    {
        GLint prevFBO = 0;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);

        _ppPrevFBO = (GLuint)prevFBO;

        glBindFramebuffer(GL_FRAMEBUFFER, _ppFBO);
        glViewport(0, 0, (GLsizei)_ppWidth, (GLsizei)_ppHeight);
    }

    glClearColor(0.0f, 0.0f, 0.f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(_progId);

    glUniform1f(_uSpecularEnabled, !SI_COMPARE_MODE && _specularEnabled ? 1.0f : 0.0f);

    if (_uZLayer >= 0)
        glUniform1f(_uZLayer, (float)_currentZLayer);

    if (_uItemCullingMask >= 0)
        glUniform1i(_uItemCullingMask, (GLint)_currentItemCullingMask);

    if (_uAmbientLight >= 0)
        glUniform1f(_uAmbientLight, SI_COMPARE_MODE ? SI_AMBIENT_LIGHT : 0.0f);

    if (_lightBufferInitialized && _lightBlockIndex != (GLint)GL_INVALID_INDEX)
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, _lightUBO);
    s_lightBgOffsetX = offsetBgX;
    s_lightBgOffsetY = offsetBgY;

    _lastFinalZoom = finalZoom;
    _lastOffsetGmX = offsetGmX;
    _lastOffsetGmY = offsetGmY;

    s_uploadedLightCount = 0;
    glUniform1f(_uHasLights, 0.0f);

    updateActiveTextures();

    CContainer::clearTrack();

    if (CSceneResize::getInstance()->onFrame(dt))
    {
        updateUniforms();
    }
}

CameraInfo& CGfx::calculateCamera(float targetX, float targetY, float desiredZoom)
{
    static CameraInfo res;

    res.zoom = desiredZoom;

    const float scrW = CSceneResize::getInstance()->getScreenWidth();
    const float scrH = CSceneResize::getInstance()->getScreenHeight();

    const float scrWG = CSceneResize::getInstance()->getScreenWidth();
    const float scrHG = CSceneResize::getInstance()->getScreenHeight();

    const float bgW  = (float)CSceneResize::getInstance()->getBgWidth();
    const float bgH  = (float)CSceneResize::getInstance()->getBgHeight();

    float viewW = scrWG / res.zoom;
    float viewH = scrHG / res.zoom;

    float halfViewW = viewW * 0.5f;
    float halfViewH = viewH * 0.5f;

    float minGameX = halfViewW;
    float maxGameX = scrW - halfViewW;

    res.gameX = (maxGameX > minGameX) ? std::clamp(targetX, minGameX, maxGameX) : scrW / 2.0f;

    float minGameY = halfViewH;
    float maxGameY = scrH - halfViewH;

    res.gameY = (maxGameY > minGameY) ? std::clamp(targetY, minGameY, maxGameY) : scrH / 2.0f;

    viewW = scrW / res.zoom;
    viewH = scrH / res.zoom;

    halfViewW = viewW * 0.5f;
    halfViewH = viewH * 0.5f;

    float minBgX = halfViewW;
    float maxBgX = bgW - halfViewW;

    if (maxBgX > minBgX)
    {
        res.bgX = std::clamp(targetX, minBgX, maxBgX);
    }
    else
    {
        res.bgX = bgW * 0.5f;
    }

    float minBgY = halfViewH;
    float maxBgY = bgH - halfViewH;

    if (maxBgY > minBgY)
    {
        res.bgY = std::clamp(targetY, minBgY, maxBgY);
    }
    else
    {
        res.bgY = bgH * 0.5f;
    }

    return res;
}

float CGfx::getElapsedSeconds()
{
    auto now = std::chrono::steady_clock::now();
    std::chrono::duration<float> elapsed = now - _startTime;

    return elapsed.count();
}

void CGfx::setTutorialElips(CContainerPtr ptrVisibleElement)
{
    _ptrTutorElement = ptrVisibleElement;

    _tutorialBoxIntencity = 0.f;
    _tutorialWait = 0.f;

    if (_ptrTutorElement)
        _isTutorActive = true;
    else
        _isTutorActive = false;
}

float CGfx::getLoopedTime()
{
    float loopDuration = 62.83185f;
    return fmodf(getElapsedSeconds(), loopDuration);
}

void CGfx::setShockTimeUniform(float fValue)
{
    glUniform1f(_shockTimeUniform, fValue);
}

void CGfx::startHit(float x, float y)
{
    for (int i = 0; i < 5; i++)
    {
        if (!_hits[i].active || _hits[i].intensity <= 0.1f)
        {
            _hits[i].x = x / CSceneResize::getInstance()->getScreenWidth();
            _hits[i].y = 1.0f - (y / CSceneResize::getInstance()->getScreenHeight());
            _hits[i].radius = 0.0f;
            _hits[i].intensity = 2.0f;
            _hits[i].active = true;

            break;
        }
    }
}

void CGfx::update(float dt)
{
    constexpr float fTutorialWait       = 1.0f;
    constexpr float fTutorialMaxIntence = 0.7;

    _lastDt = dt;

    animateWorldDarken(dt);

    TextRender::getInstance()->update(dt);

    glUniform1f(_uHasLights, (float)s_uploadedLightCount);

    if (_isTutorActive)
    {
        if (_tutorialWait < fTutorialWait)
            _tutorialWait += dt;
        else if (_tutorialBoxIntencity < fTutorialMaxIntence)
        {
            _tutorialBoxIntencity += dt * 0.5f;
        }
    }

    if (_currentHP < 100)
        _currentHP += dt * 2.5f;

    if (_shakeIntensity > 0)
    {
        _shakeIntensity -= dt * 50.0f;

        if (_shakeIntensity < 0)
            _shakeIntensity = 0;
    }

    if (_zoomShakeIntensity > 0)
    {
        _zoomShakeIntensity -= _zoomShakeIntensity * dt * 10.0f;

        if (_zoomShakeIntensity < 0.001f)
            _zoomShakeIntensity = 0;
    }

    if (_wave.time < 1.0f)
    {
        _wave.time += dt / _wave.duration;

        if (_wave.time > 1.0f)
            _wave.time = 1.0f;
    }

    glUniform1f(_shockTimeUniform, _wave.time);
    glUniform2f(_shockCenterUniform, _wave.centerX, _wave.centerY);

    switch (_eHitColor)
    {
        case eHitColor::NORMAL:
            glUniform3f(_uHitColor, 1.0f, 0.7f, 0.3f);
            break;

        case eHitColor::PURPLE_NEON:
            glUniform3f(_uHitColor, 0.8f, 0.2f, 1.0f);
            break;
    }

    for (int i = 0; i < 5; i++)
    {
        if (_hits[i].active)
        {
            _hits[i].radius += dt * 0.7f;
            _hits[i].intensity -= dt * 1.5f;

            if (_hits[i].intensity <= 0.0f)
            {
                _hits[i].active = false;
                _hits[i].intensity = 0.0f;
            }
        }

        glUniform3f(_uHitPos[i], _hits[i].x, _hits[i].y, _hits[i].radius);
        glUniform1f(_uHitIntensity[i], _hits[i].intensity);
    }

    glUniform1f(_timeUniform, getLoopedTime());

    {
        GLint currentViewport[4];
        glGetIntegerv(GL_VIEWPORT, currentViewport);

        glUniform2f(_resolutionUniform, (float)currentViewport[2], (float)currentViewport[3]);
    }

    if (_ptrTutorElement && !_bIgnoreTutorialBox)
    {
        Rect rect;

        bool bFirstUnite = true;
        bool bOk = _ptrTutorElement->calcInteractiveBounds(&rect, bFirstUnite);

        if (bOk)
        {
            rect.deflate(-28.f, -28.f);

            float screenW = (float)CSceneResize::getInstance()->getScreenWidth();
            float screenH = (float)CSceneResize::getInstance()->getScreenHeight();

            float x01 = (rect.x + rect.cx * 0.5f) / screenW;
            float y01 = 1.0f - (rect.y + rect.cy * 0.5f) / screenH;

            float halfW01 = (rect.cx * 0.5f) / screenW;
            float halfH01 = (rect.cy * 0.5f) / screenH;

            glUniform4f(_tutParamsUniform, x01, y01, 0.02f, 0.0f);
            glUniform2f(_tutRectSizeUniform, halfW01, halfH01);
        }
        else
        {
            glUniform4f(_tutParamsUniform, 0, 0, 0, 0);
            glUniform2f(_tutRectSizeUniform, 0, 0);
        }
    }
    else
    {
        glUniform4f(_tutParamsUniform, 0, 0, 0, 0);
        glUniform2f(_tutRectSizeUniform, 0, 0);
    }
}

void CGfx::initGameMatrix()
{
    float fWidth  = CSceneResize::getInstance()->getScreenWidth();
    float fHeight = CSceneResize::getInstance()->getScreenHeight();

    float finalZoom = _camInfo.zoom + _zoomShakeIntensity;

    float offsetBgX = 0;
    float offsetBgY = 0;
    float offsetGmX = 0;
    float offsetGmY = 0;

    if (_shakeIntensity > 0)
    {
        offsetBgX = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * _shakeIntensity * 0.3f;
        offsetBgY = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * _shakeIntensity * 0.3f;
        offsetGmX = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * _shakeIntensity;
        offsetGmY = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * _shakeIntensity;
    }

    _matStack.projection(fWidth, fHeight);

    _matStack.translate(CSceneResize::getInstance()->getGameOffsX(), CSceneResize::getInstance()->getGameOffsY());
    _matStack.translate(CSceneResize::getInstance()->getScreenWidth() * 0.5f + offsetGmX, CSceneResize::getInstance()->getScreenHeight() * 0.5f + offsetGmY);

    _matStack.scale(finalZoom, finalZoom);

    _matStack.translate(-_camInfo.gameX, -_camInfo.gameY);
}

void CGfx::renderGraph(float dt)
{
    clearLights();

    // Визуальные эффекты движка (шейк, хиты, туториал, шрифты) идут с кадровым dt
    update(dt);

    // --- Timestep: переменный или фиксированный ---
    const auto& cfg = Engine::getCfg();

    float simDt    = dt;
    int   simSteps = 1;

    if (cfg.TIMESTEP_MODE == eTimestepMode::FIXED)
    {
        // Накапливаем время; потолок - MAX_DT, чтобы не нагонять часы
        // после сворачивания вкладки / бряка в отладчике
        _timestepAccumulator = std::min(_timestepAccumulator + dt, cfg.MAX_DT);

        simSteps = 0;
        while (_timestepAccumulator >= cfg.FIXED_TIMESTEP &&
               simSteps < cfg.MAX_FIXED_STEPS)
        {
            _timestepAccumulator -= cfg.FIXED_TIMESTEP;
            ++simSteps;
        }

        if (simSteps == cfg.MAX_FIXED_STEPS)
            _timestepAccumulator = 0.f; // отбрасываем остаток, не нагоняем

        simDt = cfg.FIXED_TIMESTEP;

        // Доля недоделанного шага - для интерполяции при рендере
        _renderAlpha = (cfg.FIXED_TIMESTEP > 0.f)
                     ? _timestepAccumulator / cfg.FIXED_TIMESTEP
                     : 1.f;
    }
    else
    {
        _renderAlpha = 1.f;
    }

    GLint iViewport[4];
    glGetIntegerv(GL_VIEWPORT, iViewport);

    float fWidth  = (float)iViewport[2];
    float fHeight = (float)iViewport[3];

    static float fInitialRGBA[] = {1.f, 1.f, 1.f, 1.f};

    float finalZoom = _camInfo.zoom + _zoomShakeIntensity;

    float offsetBgX = 0.0f;
    float offsetBgY = 0.0f;
    float offsetGmX = 0.0f;
    float offsetGmY = 0.0f;

    if (_shakeIntensity > 0)
    {
        offsetBgX = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * _shakeIntensity * 0.3f;
        offsetBgY = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * _shakeIntensity * 0.3f;
        offsetGmX = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * _shakeIntensity;
        offsetGmY = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * _shakeIntensity;
    }

    s_lightBgOffsetX = offsetBgX;
    s_lightBgOffsetY = offsetBgY;

    _lastFinalZoom = finalZoom;
    _lastOffsetGmX = offsetGmX;
    _lastOffsetGmY = offsetGmY;

    float bgW = CSceneResize::getInstance()->getScreenWidth();
    float bgH = CSceneResize::getInstance()->getScreenHeight();

    float bgHalfW = bgW * 0.5f;
    float bgHalfH = bgH * 0.5f;

    // Симуляция: FIXED - несколько шагов по FIXED_TIMESTEP, VARIABLE - один шаг
    for (int simStep = 0; simStep < simSteps; ++simStep)
    {
    _matStack.save();
    {
        _eCurrLayer = eRenderLayer::BACKGORUND;

        _matStack.projection(fWidth, fHeight);
        _matStack.translate(fWidth * 0.5f - bgHalfW + offsetBgX,
                            fHeight * 0.5f - bgHalfH + offsetBgY);

        _matStack.translate(bgHalfW, bgHalfH);
        _matStack.scale(finalZoom, finalZoom);
        _matStack.translate(-bgHalfW, -bgHalfH);

        float diffX = _camInfo.bgX - bgHalfW;
        float diffY = _camInfo.bgY - bgHalfH;

        _matStack.translate(-diffX, -diffY);

        _bgRoot->update(simDt);
    }
    _matStack.restore();

    _matStack.save();
    {
        _eCurrLayer = eRenderLayer::GAME;

        _matStack.projection(fWidth, fHeight);
        _matStack.translate(CSceneResize::getInstance()->getGameOffsX(),
                            CSceneResize::getInstance()->getGameOffsY());

        _matStack.translate(fWidth * 0.5f + offsetGmX,
                            fHeight * 0.5f + offsetGmY);

        _matStack.scale(finalZoom, finalZoom);
        _matStack.translate(-_camInfo.gameX, -_camInfo.gameY);

        _gameRoot->update(simDt);
    }
    _matStack.restore();

    _matStack.save();
    {
        _matStack.projection(fWidth, fHeight);
        _matStack.translate(CSceneResize::getInstance()->getGameOffsX(),
                            CSceneResize::getInstance()->getGameOffsY());

        _gameIface->update(simDt);
    }
    _matStack.restore();

    _matStack.save();
    {
        _eCurrLayer = eRenderLayer::FOREGROUND;

        _matStack.projection(fWidth, fHeight);
        _matStack.translate(fWidth * 0.5f - bgHalfW + offsetBgX,
                            fHeight * 0.5f - bgHalfH + offsetBgY);

        _matStack.translate(bgHalfW, bgHalfH);
        _matStack.scale(finalZoom, finalZoom);
        _matStack.translate(-bgHalfW, -bgHalfH);

        float diffX = _camInfo.bgX - bgHalfW;
        float diffY = _camInfo.bgY - bgHalfH;

        _matStack.translate(-diffX, -diffY);

        _fgRoot->update(simDt);
    }
    _matStack.restore();

    if (_fgControlsRoot->isVisible())
    {
        _matStack.save();
        {
            _eCurrLayer = eRenderLayer::FOREGROUND_CONTROLS;

            _matStack.projection(fWidth, fHeight);

            _fgControlsRoot->update(simDt);
        }
        _matStack.restore();
    }

    }

    renderLightPass(finalZoom, offsetGmX, offsetGmY);

    glUniform1f(_uHasLights, (float)s_uploadedLightCount);

    _matStack.save();
    {
        _eCurrLayer = eRenderLayer::BACKGORUND;

        _matStack.projection(fWidth, fHeight);
        _matStack.translate(fWidth * 0.5f - bgHalfW + offsetBgX,
                            fHeight * 0.5f - bgHalfH + offsetBgY);

        _matStack.translate(bgHalfW, bgHalfH);
        _matStack.scale(finalZoom, finalZoom);
        _matStack.translate(-bgHalfW, -bgHalfH);

        float diffX = _camInfo.bgX - bgHalfW;
        float diffY = _camInfo.bgY - bgHalfH;

        _matStack.translate(-diffX, -diffY);

        _bgRoot->render(dt, fInitialRGBA);
    }
    _matStack.restore();

    _matStack.save();
    {
        _eCurrLayer = eRenderLayer::GAME;

        _matStack.projection(fWidth, fHeight);
        _matStack.translate(CSceneResize::getInstance()->getGameOffsX(),
                            CSceneResize::getInstance()->getGameOffsY());

        _matStack.translate(fWidth * 0.5f + offsetGmX,
                            fHeight * 0.5f + offsetGmY);

        _matStack.scale(finalZoom, finalZoom);
        _matStack.translate(-_camInfo.gameX, -_camInfo.gameY);

        _gameRoot->render(dt, fInitialRGBA);
        flush();
    }
    _matStack.restore();

    _matStack.save();
    {
        _matStack.projection(fWidth, fHeight);
        _matStack.translate(CSceneResize::getInstance()->getGameOffsX(),
                            CSceneResize::getInstance()->getGameOffsY());

        _gameIface->render(dt, fInitialRGBA);
    }
    _matStack.restore();

    _matStack.save();
    {
        _eCurrLayer = eRenderLayer::FOREGROUND;

        _matStack.projection(fWidth, fHeight);
        _matStack.translate(fWidth * 0.5f - bgHalfW + offsetBgX,
                            fHeight * 0.5f - bgHalfH + offsetBgY);

        _matStack.translate(bgHalfW, bgHalfH);
        _matStack.scale(finalZoom, finalZoom);
        _matStack.translate(-bgHalfW, -bgHalfH);

        float diffX = _camInfo.bgX - bgHalfW;
        float diffY = _camInfo.bgY - bgHalfH;

        _matStack.translate(-diffX, -diffY);

        _fgRoot->render(dt, fInitialRGBA);
    }
    _matStack.restore();

    if (_fgControlsRoot->isVisible())
    {
        _matStack.save();
        {
            _eCurrLayer = eRenderLayer::FOREGROUND_CONTROLS;

            _matStack.projection(fWidth, fHeight);

            _fgControlsRoot->render(dt, fInitialRGBA);
        }
        _matStack.restore();
    }
}

void CGfx::end()
{
    flush();

    if (_ppInitialized && isPostProcessEnebled())
    {
        if (_ppSettings.bloom.enabled && !_bloomInitialized)
            initBloom();

        if (_ppSettings.bloom.enabled && _bloomInitialized)
            renderBloom();

        renderPostProcess();
    }
    else
    {
        if (!_ppSettings.bloom.enabled && _bloomInitialized)
            destroyBloom();
    }

    if (!_toUnload.empty())
    {
        for (auto& it : _toUnload)
        {
            _unloaded[it->getPath()] = it;
            deleteTexture(it);
        }

        _toUnload.clear();
    }

    if (_cbOnRenderEnded)
    {
        _cbOnRenderEnded();
        _cbOnRenderEnded = {};
    }
}

void CGfx::animateCenterCamera(CContainerPtr ptrCont, float zoom, float fDuring, SimpleCallback cbOnComplete)
{
    Rect rc;
    ptrCont->calcBounds(rc, true);

    bool bRes = true;
    assert(bRes);

    Point pt(rc.x + rc.cx * 0.5f, rc.y + rc.cy * 0.5f);

    animateCenterCamera(pt, zoom, fDuring, cbOnComplete);
}

void CGfx::animateCenterCamera(Point ptFocus, float zoom, float fDuring, SimpleCallback cbOnComplete)
{
    _cbOnCameraAnimComplete = cbOnComplete;

    auto& currInfo = CGfx::getInstance()->getCameraInfo();
    auto& targInfo = CGfx::calculateCamera(ptFocus.x, ptFocus.y, zoom);

    _gameRoot->removeSelfTweens();

    _gameRoot->addSelfTween(eTweenProp::GAME_CAMERA_X, currInfo.gameX, targInfo.gameX, fDuring, Easing::linear);
    _gameRoot->addSelfTween(eTweenProp::GAME_CAMERA_Y, currInfo.gameY, targInfo.gameY, fDuring, Easing::linear, 0);

    _gameRoot->addSelfTween(eTweenProp::BG_CAMERA_X, currInfo.bgX, targInfo.bgX, fDuring, Easing::linear);
    _gameRoot->addSelfTween(eTweenProp::BG_CAMERA_Y, currInfo.bgY, targInfo.bgY, fDuring, Easing::linear, 0);

    _gameRoot->addSelfTween(eTweenProp::CAMERA_ZOOM, currInfo.zoom, targInfo.zoom, fDuring, Easing::linear, 0, [this, cbOnComplete] {
        if (cbOnComplete)
            cbOnComplete();

        _cbOnCameraAnimComplete = {};
    });
}

void CGfx::animateResetCamera(float fDuring, SimpleCallback cbOnComplete)
{
    _cbOnCameraAnimComplete = cbOnComplete;

    auto& currInfo = CGfx::getInstance()->getCameraInfo();
    auto& targInfo = CGfx::calculateCamera(0, 0, 1.f);

    _gameRoot->removeSelfTweens();

    _gameRoot->addSelfTween(eTweenProp::GAME_CAMERA_X, currInfo.gameX, targInfo.gameX, fDuring, Easing::outBounce);
    _gameRoot->addSelfTween(eTweenProp::GAME_CAMERA_Y, currInfo.gameY, targInfo.gameY, fDuring, Easing::outBounce, 0);

    _gameRoot->addSelfTween(eTweenProp::BG_CAMERA_X, currInfo.bgX, targInfo.bgX, fDuring, Easing::outBounce);
    _gameRoot->addSelfTween(eTweenProp::BG_CAMERA_Y, currInfo.bgY, targInfo.bgY, fDuring, Easing::outBounce, 0);

    _gameRoot->addSelfTween(eTweenProp::CAMERA_ZOOM, currInfo.zoom, targInfo.zoom, fDuring, Easing::outBounce, 0, [this, cbOnComplete] {
        if (cbOnComplete)
            cbOnComplete();

        _cbOnCameraAnimComplete = {};
    });
}

void CGfx::setWorldDarkenNoAnim(bool bDarken, float fMinEnvDarken, float fMinGameDarken)
{
    _fMinEnvDarken  = fMinEnvDarken;
    _fMinGameDarken = fMinGameDarken;

    float fMinD     = _fMinEnvDarken;
    float fMinGameD = _fMinGameDarken;

    float fD        = bDarken ? std::min(_fMinEnvDarken, _fMinGameDarken) : 1.f;
    _eDarkenState   = bDarken ? eWorldDarkenState::DARK : eWorldDarkenState::BASIC;

    float fdGame = std::max(fMinGameD, fD);
    float fdEnv  = std::max(fMinD, fD);

    _gameRoot->setTint(fdGame, fdGame, fdGame);
    _bgRoot->setTint(fdEnv, fdEnv, fdEnv);
    _fgRoot->setTint(fdEnv, fdEnv, fdEnv);
}

void CGfx::setWorldDarken(bool bDarken, float fMinEnvDarken, float fMinGameDarken, SimpleCallback cb)
{
    _onWorldDarkenCb = cb;

    if (bDarken)
    {
        _fMinEnvDarken  = fMinEnvDarken;
        _fMinGameDarken = fMinGameDarken;
    }

    switch (_eDarkenState)
    {
        case eWorldDarkenState::BASIC:
        {
            if (bDarken)
            {
                _eDarkenState = eWorldDarkenState::DARKENING;
                _fDarkenWorldTimer = 0.f;
            }
        }
        break;

        case eWorldDarkenState::DARKENING:
        case eWorldDarkenState::DARK:
        {
            if (!bDarken)
            {
                _eDarkenState = eWorldDarkenState::BRIGHTENING;
            }
        }
        break;

        case eWorldDarkenState::BRIGHTENING:
        {
            if (bDarken)
            {
                _eDarkenState = eWorldDarkenState::DARKENING;
            }
        }
        break;
    }
}

void CGfx::appendLights(std::span<GPULight> lights)
{
    int nStart = _lightCount;

    _lightCount = std::max(0, std::min((int)(_lightCount + lights.size()), MAX_LIGHTS));

    for (int i = nStart; i < _lightCount; ++i)
        _lights[i] = lights[i - nStart];
}

void CGfx::clearLights()
{
    _lightCount = 0;
    s_uploadedLightCount = 0;

}

void CGfx::setCurrentZLayer(int z, uint16_t itemCullingMask)
{
    if (_currentZLayer == z && _currentItemCullingMask == itemCullingMask)
        return;

    flush();

    _currentZLayer          = z;
    _currentItemCullingMask = itemCullingMask;

    GLint prevProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);

    if (_progId)
    {
        glUseProgram(_progId);

        if (_uZLayer >= 0)
            glUniform1f(_uZLayer, (float)_currentZLayer);
        if (_uItemCullingMask >= 0)
            glUniform1i(_uItemCullingMask, (GLint)_currentItemCullingMask);

        if (prevProgram != 0 && prevProgram != (GLint)_progId)
            glUseProgram(prevProgram);
    }
}

void CGfx::setCurrentItemCullingMask(uint16_t mask)
{
    setCurrentZLayer(_currentZLayer, mask);
}

bool CGfx::initLightBuffer()
{
#ifdef TARGET_EMSCRIPTEN
    if (!_isWebGL2)
        return false;
#endif

    if (_lightBufferInitialized)
        return true;

    glGenBuffers(1, &_lightUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, _lightUBO);
    glBufferData(GL_UNIFORM_BUFFER, MAX_UBO_LIGHTS * 5 * 16, NULL, GL_DYNAMIC_DRAW);

    _lightBlockIndex = glGetUniformBlockIndex(_progId, "LightBlock");
    if (_lightBlockIndex == (GLint)GL_INVALID_INDEX)
    {
        glDeleteBuffers(1, &_lightUBO);
        _lightUBO = 0;
        return false;
    }

    glUniformBlockBinding(_progId, _lightBlockIndex, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, _lightUBO);

    s_uploadedLightCount = 0;
    _lightBufferInitialized = true;
    return true;
}

void CGfx::destroyLightBuffer()
{
    if (_lightUBO)
    {
        glDeleteBuffers(1, &_lightUBO);
        _lightUBO = 0;
    }
    if (_lightVAO)
    {
        glDeleteVertexArrays(1, &_lightVAO);
        _lightVAO = 0;
    }

    if (_lightQuadVBO)
    {
        glDeleteBuffers(1, &_lightQuadVBO);
        _lightQuadVBO = 0;
    }

    if (_lightInstanceVBO)
    {
        glDeleteBuffers(1, &_lightInstanceVBO);
        _lightInstanceVBO = 0;
    }

    if (_lightFBO)
    {
        glDeleteFramebuffers(1, &_lightFBO);
        _lightFBO = 0;
    }

    for (int i = 0; i < 2; ++i)
    {
        if (_lightTex[i])
        {
            glDeleteTextures(1, &_lightTex[i]);
            _lightTex[i] = 0;
        }
    }

    if (_lightVecTexture)
    {
        glDeleteTextures(1, &_lightVecTexture);
        _lightVecTexture = 0;
    }

    if (_lightProgId)
    {
        glDeleteProgram(_lightProgId);
        _lightProgId = 0;
    }

    _lightTexture = 0;
    _lightTexIdx = 0;

    _lightBufferInitialized = false;
    s_uploadedLightCount = 0;
}

void CGfx::resizeLightBuffer(float, float)
{

}

bool CGfx::setupLightShaders()
{
    return _progId != 0;
}

void CGfx::lookupLightUniforms()
{
    _lightUResolution = -1;
}

void CGfx::renderLightPass(float finalZoom, float offsetGmX, float offsetGmY)
{
    s_uploadedLightCount = 0;

    if (!_lightBufferInitialized || _lightCount <= 0)
        return;

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    float targetWidth  = (float)viewport[2];
    float targetHeight = (float)viewport[3];

    if (targetWidth <= 0.0f || targetHeight <= 0.0f)
        return;

    static float lightGeometry[MAX_LIGHTS * 4];
    static float lightEmission[MAX_LIGHTS * 4];
    static float lightTexInfo[MAX_LIGHTS * 4];
    static float lightTexInfo2[MAX_LIGHTS * 4];
    static float lightZInfo[MAX_LIGHTS * 4];

        int count = std::min(_lightCount, MAX_UBO_LIGHTS);

    float zoom = std::abs(finalZoom);

    auto tryBindLightTexture = [&](CTexturePtr tex) -> bool
    {
        if (!tex || !tex->isUploaded())
            return false;

        tex->updateLastUse();

        if (tex->getSampler() >= 0)
        {
            reserveSampler(tex->getSampler());
            return true;
        }

        if (!canReserveLightSampler())
            return false;

        if (!hasFreeSamplerForLightTexture())
            return false;

        setActiveTexture(tex.get());

        if (tex->getSampler() < 0)
            return false;

        reserveSampler(tex->getSampler());
        return true;
    };

    for (int i = 0; i < count; ++i)
    {
        if (SI_COMPARE_MODE && s_uploadedLightCount > 0)
            break;

        const auto& light = _lights[i];

        if (light.intensity <= 0.0f)
            continue;

        const bool bScreenSpace = light.bScreenSpace;

        if (!bScreenSpace && zoom <= 0.0f)
            continue;

        float screenX = 0.0f;
        float screenY = 0.0f;

        float screenScale  = zoom;
        float screenScaleX = zoom;
        float screenScaleY = zoom;

        if (bScreenSpace)
        {
            float srcW = (float)CSceneResize::getInstance()->getScreenWidth();
            float srcH = (float)CSceneResize::getInstance()->getScreenHeight();

            float kx = (srcW > 0.0f) ? (targetWidth  / srcW) : 1.0f;
            float ky = (srcH > 0.0f) ? (targetHeight / srcH) : 1.0f;

            screenScale  = (kx + ky) * 0.5f;
            screenScaleX = kx;
            screenScaleY = ky;

            screenX = light.position.x * kx;
            screenY = light.position.y * ky;
        }
        else
        {
            switch (light.eLayer)
            {
                case eLightLayer::GAME:
                {
                    screenX = (light.position.x - _camInfo.gameX) * finalZoom
                            + targetWidth * 0.5f + offsetGmX;

                    screenY = (light.position.y - _camInfo.gameY) * finalZoom
                            + targetHeight * 0.5f + offsetGmY;
                }
                break;

                case eLightLayer::BG:
                {
                    screenX = (light.position.x - _camInfo.bgX) * finalZoom
                            + targetWidth * 0.5f + s_lightBgOffsetX;

                    screenY = (light.position.y - _camInfo.bgY) * finalZoom
                            + targetHeight * 0.5f + s_lightBgOffsetY;
                }
                break;

                default:
                    continue;
            }
        }

        float radiusScreen = bScreenSpace
                             ? (light.radius * screenScale)
                             : (light.radius * zoom);

        bool  hasSprite      = false;
        bool  explicitSprite = false;

        float samplerIdx = -1.0f;

        float u0 = 0.0f;
        float v0 = 0.0f;
        float u1 = 1.0f;
        float v1 = 1.0f;

        float wScreen = 0.0f;
        float hScreen = 0.0f;
        float rotate  = 0.0f;

        float originX = 0.0f;
        float originY = 0.0f;

        float edgeXX = 0.0f;
        float edgeXY = 0.0f;
        float edgeYX = 0.0f;
        float edgeYY = 0.0f;

        if (light.bExplicitQuad)
        {
            if (tryBindLightTexture(light.explicitTexture))
            {
                CTexturePtr tex = light.explicitTexture;

                if (tex->getSampler() >= 0)
                {
                    float kx = bScreenSpace ? screenScaleX : 1.0f;
                    float ky = bScreenSpace ? screenScaleY : 1.0f;

                    float glX[4];
                    float glY[4];

                    for (int c = 0; c < 4; ++c)
                    {
                        float sx = light.explicitX[c] * kx;
                        float sy = light.explicitY[c] * ky;

                        glX[c] = sx;
                        glY[c] = targetHeight - sy;
                    }

                    originX = glX[0];
                    originY = glY[0];

                    edgeXX = glX[3] - glX[0];
                    edgeXY = glY[3] - glY[0];

                    edgeYX = glX[1] - glX[0];
                    edgeYY = glY[1] - glY[0];

                    float area = edgeXX * edgeYY - edgeXY * edgeYX;

                    if (fabsf(area) > 1e-5f)
                    {
                        samplerIdx = (float)tex->getSampler();

                        reserveSampler((int)samplerIdx);

                        u0 = light.explicitU[0];
                        v0 = light.explicitV[0];
                        u1 = light.explicitU[2];
                        v1 = light.explicitV[2];

                        hasSprite      = true;
                        explicitSprite = true;
                    }
                }
            }

            if (!hasSprite)
                continue;
        }

        else if (light.sprite && light.sprite->_bIsLoaded)
        {
            CTexturePtr tex = light.sprite->getTexture();

            Rect rc;

            if (tex && tex->isUploaded() && light.sprite->getNotTransBounds(&rc))
            {
                float sw = std::fabs(rc.cx);
                float sh = std::fabs(rc.cy);

                const bool useExplicitSpriteScale =
                    light.spriteScaleX > 1e-5f &&
                    light.spriteScaleY > 1e-5f;

                if (useExplicitSpriteScale &&
                    sw > 1e-5f &&
                    sh > 1e-5f)
                {
                    if (tryBindLightTexture(tex))
                    {
                        wScreen = sw * light.spriteScaleX * screenScaleX;
                        hScreen = sh * light.spriteScaleY * screenScaleY;

                        if (wScreen > 1e-5f && hScreen > 1e-5f)
                        {
                            u0 = light.sprite->_uvs[CSprite::VERT_ULX];
                            v0 = light.sprite->_uvs[CSprite::VERT_ULY];
                            u1 = light.sprite->_uvs[CSprite::VERT_BRX];
                            v1 = light.sprite->_uvs[CSprite::VERT_BRY];

                            samplerIdx = (float)tex->getSampler();

                            reserveSampler((int)samplerIdx);

                            rotate     = light.rotate;

                            hasSprite  = true;
                        }
                    }
                }
                else if (sw > 1e-5f && sh > 1e-5f && radiusScreen > 0.0f)
                {
                    float spriteRadius = 0.5f * std::max(sw, sh);

                    if (spriteRadius > 1e-5f)
                    {
                        if (tryBindLightTexture(tex))
                        {
                            float scale = radiusScreen / spriteRadius;

                            wScreen = sw * scale;
                            hScreen = sh * scale;

                            u0 = light.sprite->_uvs[CSprite::VERT_ULX];
                            v0 = light.sprite->_uvs[CSprite::VERT_ULY];
                            u1 = light.sprite->_uvs[CSprite::VERT_BRX];
                            v1 = light.sprite->_uvs[CSprite::VERT_BRY];

                            samplerIdx = (float)tex->getSampler();

                            reserveSampler((int)samplerIdx);

                            rotate     = light.rotate;

                            hasSprite  = true;
                        }
                    }
                }
            }
        }

        if (!hasSprite && radiusScreen <= 0.0f)
            continue;

        int base = s_uploadedLightCount * 4;

        float flags = 0.0f;

        if (hasSprite)
            flags += 1.0f;

        if (explicitSprite)
            flags += 4.0f;

        if (SI_COMPARE_MODE)
            flags += 2.0f;

        float pivotShiftX = 0.0f;
        float pivotShiftY = 0.0f;

        if (hasSprite && !explicitSprite)
        {
            float vx = (light.spritePivotX - 0.5f) * wScreen;
            float vy = -(light.spritePivotY - 0.5f) * hScreen;

            float c = cosf(rotate);
            float s = sinf(rotate);

            pivotShiftX = c * vx - s * vy;
            pivotShiftY = s * vx + c * vy;
        }

        lightEmission[base + 0] = SI_COMPARE_MODE
                                  ? SI_LIGHT_BRIGHTNESS
                                  : std::max(0.0f, light.color.x) * light.intensity;

        lightEmission[base + 1] = SI_COMPARE_MODE
                                  ? SI_LIGHT_BRIGHTNESS
                                  : std::max(0.0f, light.color.y) * light.intensity;

        lightEmission[base + 2] = SI_COMPARE_MODE
                                  ? SI_LIGHT_BRIGHTNESS
                                  : std::max(0.0f, light.color.z) * light.intensity;

        lightEmission[base + 3] = bScreenSpace
                                  ? (LOCAL_LIGHT_HEIGHT * screenScale)
                                  : (LOCAL_LIGHT_HEIGHT * zoom);

        lightTexInfo[base + 0] = samplerIdx;
        lightTexInfo[base + 1] = u0;
        lightTexInfo[base + 2] = v0;
        lightTexInfo[base + 3] = u1;

        lightZInfo[base + 0] = (float)light.zMin;
        lightZInfo[base + 1] = (float)light.zMax;
        lightZInfo[base + 2] = (float)light.itemCullingMask;
        lightZInfo[base + 3] = 0.0f;

        if (explicitSprite)
        {
            lightGeometry[base + 0] = originX;
            lightGeometry[base + 1] = originY;
            lightGeometry[base + 2] = edgeYY;
            lightGeometry[base + 3] = flags;

            lightTexInfo2[base + 0] = v1;
            lightTexInfo2[base + 1] = edgeXX;
            lightTexInfo2[base + 2] = edgeXY;
            lightTexInfo2[base + 3] = edgeYX;
        }
        else
        {
            lightGeometry[base + 0] = screenX - pivotShiftX;
            lightGeometry[base + 1] = (targetHeight - screenY) - pivotShiftY;
            lightGeometry[base + 2] = hasSprite ? 0.0f : radiusScreen;
            lightGeometry[base + 3] = flags;

            lightTexInfo2[base + 0] = v1;
            lightTexInfo2[base + 1] = wScreen;
            lightTexInfo2[base + 2] = hScreen;
            lightTexInfo2[base + 3] = rotate;
        }

        ++s_uploadedLightCount;
    }

    {
        static bool bCurrentInitDone = false;
        std::vector<int> current;
        if (!bCurrentInitDone)
        {
            current.reserve(8);
            bCurrentInitDone = true;
        }
        current.clear();

        for (int i = 0; i < s_uploadedLightCount; ++i)
        {
            int idx = (int)(lightTexInfo[i * 4 + 0] + 0.5f);

            if (idx >= 0 &&
                std::find(current.begin(), current.end(), idx) == current.end())
            {
                current.push_back(idx);
            }
        }

        static std::vector<int> stale; 
        stale = _reservedLightSamplers;
        for (int idx : stale)
        {
            if (std::find(current.begin(), current.end(), idx) == current.end())
                unreserveSampler(idx);
        }
    }

    if (s_uploadedLightCount <= 0)
        return;

    static float lightStaging[MAX_UBO_LIGHTS * 5 * 4];

    for (int i = 0; i < s_uploadedLightCount; ++i)
    {
        memcpy(&lightStaging[(i * 5 + 0) * 4], &lightGeometry[i * 4], 16);
        memcpy(&lightStaging[(i * 5 + 1) * 4], &lightEmission[i * 4], 16);
        memcpy(&lightStaging[(i * 5 + 2) * 4], &lightTexInfo[i * 4], 16);
        memcpy(&lightStaging[(i * 5 + 3) * 4], &lightTexInfo2[i * 4], 16);
        memcpy(&lightStaging[(i * 5 + 4) * 4], &lightZInfo[i * 4], 16);
    }

    glBindBuffer(GL_UNIFORM_BUFFER, _lightUBO);

    glBufferData(GL_UNIFORM_BUFFER, MAX_UBO_LIGHTS * 5 * 16, NULL, GL_DYNAMIC_DRAW);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, s_uploadedLightCount * 5 * 16, lightStaging);
}

void CGfx::renderPostProcess()
{
    if (!_ppInitialized || !isPostProcessEnebled())
        return;

    uploadGrainTexture();

    GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    GLboolean scissorEnabled = glIsEnabled(GL_SCISSOR_TEST);

    GLint prevProgram = 0;
    GLint prevVBO = 0;
    GLint prevFBO = 0;

    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prevVBO);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);

    glBindFramebuffer(GL_FRAMEBUFFER, _ppPrevFBO);
    glViewport(0, 0, (GLsizei)_viewPortCx, (GLsizei)_viewPortCy);

    if (scissorEnabled)
        glDisable(GL_SCISSOR_TEST);

    glUseProgram(_ppProgId);
    glBindVertexArray(_ppVAO);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _ppTexture);

    glActiveTexture(GL_TEXTURE1);
    if (_bloomInitialized && _ppSettings.bloom.enabled)
        glBindTexture(GL_TEXTURE_2D, _bloomTexture[1]);
    else
        glBindTexture(GL_TEXTURE_2D, _ppTexture);

    glActiveTexture(GL_TEXTURE2);
    if (_ptrTexGrain && _ptrTexGrain->_handle)
        glBindTexture(GL_TEXTURE_2D, _ptrTexGrain->_handle);
    else
        glBindTexture(GL_TEXTURE_2D, _ppTexture);

    auto set1i = [](GLint loc, GLint v)
    {
        if (loc >= 0)
            glUniform1i(loc, v);
    };

    auto set1f = [](GLint loc, GLfloat v)
    {
        if (loc >= 0)
            glUniform1f(loc, v);
    };

    auto set3f = [](GLint loc, GLfloat x, GLfloat y, GLfloat z)
    {
        if (loc >= 0)
            glUniform3f(loc, x, y, z);
    };

    set1i(_ppUSceneTex, 0);
    set1i(_ppUBloomTex, 1);
    set1i(_ppUGrainTex, 2);

    GLint lightComp = glGetUniformLocation(_ppProgId, "u_lightComparison");
    set1f(lightComp, SI_BYPASS_POSTPROCESS ? 1.0f : 0.0f);

    float resW = _ppWidth > 0.0f ? _ppWidth : _viewPortCx;
    float resH = _ppHeight > 0.0f ? _ppHeight : _viewPortCy;

    if (_ppUResolution >= 0)
        glUniform2f(_ppUResolution, resW, resH);

    set1f(_ppUTime, getLoopedTime());
    set1f(_ppUVignetteIntensity, _ppSettings.vignetteIntensity);
    set1f(_ppUVignetteRadius, _ppSettings.vignetteRadius);
    set1f(_ppUVignetteSmoothness, _ppSettings.vignetteSmoothness);
    set3f(_ppUTintColor, _ppSettings.tintR, _ppSettings.tintG, _ppSettings.tintB);
    set1f(_ppUSaturation, _ppSettings.saturation);
    set1f(_ppUContrast, _ppSettings.contrast);
    set1f(_ppUBrightness, _ppSettings.brightness);

    if (_ptrTexGrain && _ptrTexGrain->_handle)
        set1f(_ppUGrain, _ppSettings.grainIntensity);
    else
        set1f(_ppUGrain, 0.0f);

    set1f(_ppUAberration, _ppSettings.aberration);

    if (_bloomInitialized && _ppSettings.bloom.enabled)
        set1f(_ppUBloomIntensity, _ppSettings.bloom.intensity);
    else
        set1f(_ppUBloomIntensity, 0.0f);

    bool bPotato = isPotato();
    set1f(_ppUFisheyeIntensity,    bPotato ? 0.0f : _ppSettings.fisheyeIntensity);
    set1f(_ppUDistortionIntensity, bPotato ? 0.0f : _ppSettings.distortionIntensity);
    set1f(_ppURainIntensity,       bPotato ? 0.0f : _ppSettings.fRainIntensity);
    set1f(_ppUBloodDrops,          bPotato ? 0.0f : _ppSettings.fBloodDrops);

    glDisable(GL_BLEND);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glUseProgram(prevProgram);
    glBindBuffer(GL_ARRAY_BUFFER, prevVBO);

    if (blendEnabled)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);

    if (scissorEnabled)
        glEnable(GL_SCISSOR_TEST);

    restoreVertexFormat();
    updateActiveTextures();
}
bool CGfx::setupBloomShaders()
{
#ifdef TARGET_EMSCRIPTEN
    const char* preamble =
        "#version 300 es\n"
        "precision mediump float;\n"
        "precision mediump sampler2D;\n"
        "precision mediump int;\n";
#else
    const char* preamble = "#version 330 core\n";
#endif

    const char* vBody = R"(
layout(location = 0) in vec2 a_pos;
layout(location = 1) in vec2 a_uv;

out vec2 v_uv;

void main()
{
    gl_Position = vec4(a_pos, 0.0, 1.0);
    v_uv = a_uv;
}
)";

    const char* pass1Body = R"(
in vec2 v_uv;
out vec4 fragColor;

uniform sampler2D u_sceneTex;
uniform vec2 u_resolution;
uniform float u_threshold;

void main()
{
    vec2 off = 1.0 / u_resolution;

    vec4 sum = texture(u_sceneTex, v_uv) * 4.0;

    sum += texture(u_sceneTex, v_uv + vec2(-off.x, -off.y));
    sum += texture(u_sceneTex, v_uv + vec2( off.x, -off.y));
    sum += texture(u_sceneTex, v_uv + vec2(-off.x,  off.y));
    sum += texture(u_sceneTex, v_uv + vec2( off.x,  off.y));

    sum /= 8.0;

    float brightness = dot(sum.rgb, vec3(0.299, 0.587, 0.114));

    if (brightness > u_threshold)
        fragColor = sum;
    else
        fragColor = vec4(0.0, 0.0, 0.0, 1.0);
}
)";

    const char* pass2Body = R"(
in vec2 v_uv;
out vec4 fragColor;

uniform sampler2D u_tex;
uniform vec2 u_resolution;

void main()
{
    vec2 off = 1.0 / u_resolution;

    vec4 sum = texture(u_tex, v_uv + vec2(-off.x, -off.y) * 2.0);
    sum += texture(u_tex, v_uv + vec2( 0.0,    -off.y) * 2.0) * 2.0;
    sum += texture(u_tex, v_uv + vec2( off.x,  -off.y) * 2.0);
    sum += texture(u_tex, v_uv + vec2(-off.x,   0.0)   * 2.0) * 2.0;
    sum += texture(u_tex, v_uv) * 4.0;
    sum += texture(u_tex, v_uv + vec2( off.x,   0.0)   * 2.0) * 2.0;
    sum += texture(u_tex, v_uv + vec2(-off.x,   off.y) * 2.0);
    sum += texture(u_tex, v_uv + vec2( 0.0,     off.y) * 2.0) * 2.0;
    sum += texture(u_tex, v_uv + vec2( off.x,   off.y) * 2.0);

    fragColor = sum / 16.0;
}
)";

    std::string vSrc  = std::string(preamble) + vBody;
    std::string f1Src = std::string(preamble) + pass1Body;
    std::string f2Src = std::string(preamble) + pass2Body;

#ifdef TARGET_EMSCRIPTEN
    _bloomProgId[0] = jsSetupShaders(vSrc.c_str(), f1Src.c_str());
    _bloomProgId[1] = jsSetupShaders(vSrc.c_str(), f2Src.c_str());

    return _bloomProgId[0] != 0 && _bloomProgId[1] != 0;
#else
    auto compileShader = [this](GLenum type, const std::string& src) -> GLuint
    {
        GLuint sh = glCreateShader(type);
        const char* p = src.c_str();

        glShaderSource(sh, 1, &p, nullptr);
        glCompileShader(sh);

#ifdef TARGET_WIN
        GLint ok = GL_FALSE;
        glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
        if (ok != GL_TRUE)
            printShaderLog(sh);
#endif

        return sh;
    };

    auto linkProgram = [this](GLuint vs, GLuint fs) -> GLuint
    {
        GLuint prog = glCreateProgram();

        glAttachShader(prog, vs);
        glAttachShader(prog, fs);
        glLinkProgram(prog);

#ifdef TARGET_WIN
        GLint linked = GL_TRUE;
        glGetProgramiv(prog, GL_LINK_STATUS, &linked);

        if (linked != GL_TRUE)
        {
            printProgramLog(prog);
            glDeleteProgram(prog);
            return 0;
        }
#endif

        return prog;
    };

    GLuint vs = compileShader(GL_VERTEX_SHADER, vSrc);
    GLuint fs1 = compileShader(GL_FRAGMENT_SHADER, f1Src);
    GLuint fs2 = compileShader(GL_FRAGMENT_SHADER, f2Src);

    if (!vs || !fs1 || !fs2)
    {
        if (vs)   glDeleteShader(vs);
        if (fs1)  glDeleteShader(fs1);
        if (fs2)  glDeleteShader(fs2);
        return false;
    }

    _bloomProgId[0] = linkProgram(vs, fs1);
    _bloomProgId[1] = linkProgram(vs, fs2);

    glDeleteShader(vs);
    glDeleteShader(fs1);
    glDeleteShader(fs2);

    return _bloomProgId[0] != 0 && _bloomProgId[1] != 0;
#endif
}

void CGfx::lookupBloomUniforms()
{
    for (int i = 0; i < 2; ++i)
    {
        if (!_bloomProgId[i])
            continue;

        if (i == 0)
            _bloomUTex[i] = glGetUniformLocation(_bloomProgId[i], "u_sceneTex");
        else
            _bloomUTex[i] = glGetUniformLocation(_bloomProgId[i], "u_tex");

        _bloomUResolution[i] = glGetUniformLocation(_bloomProgId[i], "u_resolution");
        _bloomAPos[i]        = glGetAttribLocation(_bloomProgId[i], "a_pos");
        _bloomAUV[i]         = glGetAttribLocation(_bloomProgId[i], "a_uv");
    }

    if (_bloomProgId[0])
        _bloomUThreshold = glGetUniformLocation(_bloomProgId[0], "u_threshold");
}

bool CGfx::initBloom()
{
    if (!isBloomAvailable())
        return false;
#ifdef TARGET_EMSCRIPTEN
    if (!_isWebGL2)
        return false;
#endif

    if (!_ppInitialized)
        return false;

    if (!isBloomAvailable())
        return false;

    if (_bloomInitialized)
        return true;

    float w = _ppWidth > 0.0f ? _ppWidth : _viewPortCx;
    float h = _ppHeight > 0.0f ? _ppHeight : _viewPortCy;

    if (w <= 0.0f) w = _fOrigCx;
    if (h <= 0.0f) h = _fOrigCy;

    _bloomWidth = std::max(1.0f, w * 0.5f);
    _bloomHeight = std::max(1.0f, h * 0.5f);

    glGenFramebuffers(2, _bloomFBO);
    glGenTextures(2, _bloomTexture);

    for (int i = 0; i < 2; ++i)
    {
        glBindTexture(GL_TEXTURE_2D, _bloomTexture[i]);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA,
            (GLsizei)_bloomWidth,
            (GLsizei)_bloomHeight,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            NULL
        );

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindFramebuffer(GL_FRAMEBUFFER, _bloomFBO[i]);
        glFramebufferTexture2D(
            GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D,
            _bloomTexture[i],
            0
        );

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            destroyBloom();
            return false;
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (!setupBloomShaders())
    {
        destroyBloom();
        return false;
    }

    lookupBloomUniforms();

    _bloomInitialized = true;

    return true;
}

void CGfx::destroyBloom()
{
    for (int i = 0; i < 2; ++i)
    {
        if (_bloomProgId[i])
        {
            glDeleteProgram(_bloomProgId[i]);
            _bloomProgId[i] = 0;
        }
    }

    for (int i = 0; i < 2; ++i)
    {
        if (_bloomFBO[i])
        {
            glDeleteFramebuffers(1, &_bloomFBO[i]);
            _bloomFBO[i] = 0;
        }

        if (_bloomTexture[i])
        {
            glDeleteTextures(1, &_bloomTexture[i]);
            _bloomTexture[i] = 0;
        }
    }

    _bloomWidth = 0.0f;
    _bloomHeight = 0.0f;
    _bloomInitialized = false;
}

void CGfx::resizeBloom(float w, float h)
{
    if (!_bloomInitialized)
        return;

    if (w <= 0.0f)
        w = _viewPortCx;

    if (h <= 0.0f)
        h = _viewPortCy;

    _bloomWidth = std::max(1.0f, w * 0.5f);
    _bloomHeight = std::max(1.0f, h * 0.5f);

    for (int i = 0; i < 2; ++i)
    {
        glBindTexture(GL_TEXTURE_2D, _bloomTexture[i]);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA,
            (GLsizei)_bloomWidth,
            (GLsizei)_bloomHeight,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            NULL
        );
    }
}

void CGfx::renderBloom()
{
    if (SI_BYPASS_POSTPROCESS || !_bloomInitialized || !_ppSettings.bloom.enabled)
        return;

    if (!_ppInitialized || !_ppTexture || !_ppVAO)
        return;

    GLint prevProgram = 0;
    GLint prevFBO = 0;
    GLint prevVBO = 0;

    GLint prevViewport[4];

    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prevVBO);
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    GLboolean scissorEnabled = glIsEnabled(GL_SCISSOR_TEST);

    if (scissorEnabled)
        glDisable(GL_SCISSOR_TEST);

    glBindVertexArray(_ppVAO);

    glBindFramebuffer(GL_FRAMEBUFFER, _bloomFBO[0]);
    glViewport(0, 0, (GLsizei)_bloomWidth, (GLsizei)_bloomHeight);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(_bloomProgId[0]);
    glDisable(GL_BLEND);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _ppTexture);

    if (_bloomUTex[0] >= 0)
        glUniform1i(_bloomUTex[0], 0);

    if (_bloomUResolution[0] >= 0)
    {
        float srcW = _ppWidth > 0.0f ? _ppWidth : _viewPortCx;
        float srcH = _ppHeight > 0.0f ? _ppHeight : _viewPortCy;
        glUniform2f(_bloomUResolution[0], srcW, srcH);
    }

    if (_bloomUThreshold >= 0)
        glUniform1f(_bloomUThreshold, _ppSettings.bloom.threshold);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindFramebuffer(GL_FRAMEBUFFER, _bloomFBO[1]);
    glViewport(0, 0, (GLsizei)_bloomWidth, (GLsizei)_bloomHeight);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(_bloomProgId[1]);
    glDisable(GL_BLEND);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _bloomTexture[0]);

    if (_bloomUTex[1] >= 0)
        glUniform1i(_bloomUTex[1], 0);

    if (_bloomUResolution[1] >= 0)
        glUniform2f(_bloomUResolution[1], _bloomWidth, _bloomHeight);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glUseProgram(prevProgram);
    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)prevFBO);
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
    glBindBuffer(GL_ARRAY_BUFFER, prevVBO);

    if (blendEnabled)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);

    if (scissorEnabled)
        glEnable(GL_SCISSOR_TEST);

    restoreVertexFormat();
    updateActiveTextures();
}
_G2D_NAMESPACE_END_
