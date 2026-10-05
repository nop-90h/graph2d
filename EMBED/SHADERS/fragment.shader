#ifdef GL_ES
precision mediump float;
#else
#define mediump
#define lowp
#define highp
#endif

// ============================================================================
// ЮНИФОРМЫ
// ============================================================================

// Спрайты
uniform sampler2D u_image0, u_image1, u_image2, u_image3;
uniform float u_shockTime, u_lowHPEffect, u_time, u_aberration; 
uniform vec2 u_shockCenter, u_resolution, u_tutRectSize;
uniform vec4 u_tutParams; 

// 5 вспышек
uniform vec3 u_hit0; uniform float u_hitI0;
uniform vec3 u_hit1; uniform float u_hitI1;
uniform vec3 u_hit2; uniform float u_hitI2;
uniform vec3 u_hit3; uniform float u_hitI3;
uniform vec3 u_hit4; uniform float u_hitI4;
uniform vec3 u_hitColor;

// Reflection / Mask
uniform float u_isMaskPass;
uniform sampler2D u_maskTexture;
uniform sampler2D u_previousMaskTexture; // оставлен для совместимости, сейчас не используется
uniform sampler2D u_reflectionTexture;   // <<< полноцветный проход CAN_BE_REFLECTED (юнит 6)
uniform float u_reflIntensity;
uniform float u_reflWaterDarken;
uniform vec3 u_reflTintColor;
uniform float u_reflBrightnessMul;
uniform float u_reflEdgeFade;
uniform float u_reflDistortion;
uniform float u_reflWaveSpeed;
uniform float u_reflWaveFreq;
uniform float u_reflWaveAmp;
uniform float u_reflYOffset;           // <<< смещение UV отражения по Y (чтобы подогнать под лужу)

// ============================================================================
// VARYING
// ============================================================================

varying vec4 v_v1; // [texCoord.x, texCoord.y, alphaBlend, sparkLife]
varying vec4 v_v2; // tintColor
varying vec4 v_v3; // reflectionData: x = eReflectionType (0=NONE,1=MIRROR,2=CAN_BE_REFLECTED)
varying vec4 v_v4; // addRGB
varying vec4 v_v5; // texMuls (0, 1, 2, 3)
varying vec2 v_v6; // [effectType, tutIntensity]

// ============================================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
// ============================================================================

float sampleAlpha(vec2 uv) {
    if (v_v5.x > 0.5) return texture2D(u_image0, uv).a;
    if (v_v5.y > 0.5) return texture2D(u_image1, uv).a;
    if (v_v5.z > 0.5) return texture2D(u_image2, uv).a;
    return texture2D(u_image3, uv).a;
}

vec4 sampleColor(vec2 uv) {
    if (v_v5.x > 0.5) return texture2D(u_image0, uv);
    if (v_v5.y > 0.5) return texture2D(u_image1, uv);
    if (v_v5.z > 0.5) return texture2D(u_image2, uv);
    return texture2D(u_image3, uv);
}

vec4 cleanSprite(vec2 uv) {
    vec4 c;
    if (v_v5.x > 0.5) c = texture2D(u_image0, uv);
    else if (v_v5.y > 0.5) c = texture2D(u_image1, uv);
    else if (v_v5.z > 0.5) c = texture2D(u_image2, uv);
    else c = texture2D(u_image3, uv);
    
    if (c.a < 0.01) return vec4(0.0);
    
    vec2 off = vec2(0.003, 0.0);
    float aN = sampleAlpha(uv + off.xy);
    float aS = sampleAlpha(uv - off.xy);
    float aE = sampleAlpha(uv + off.yx);
    float aW = sampleAlpha(uv - off.yx);
    float maxNeighbor = max(max(aN, aS), max(aE, aW));
    
    if (c.a < 0.25 && maxNeighbor < 0.2) {
        return vec4(0.0);
    }
    return c;
}

const float MIN_ALPHA = 0.01; 

float filterAlpha(vec2 uv) {
    float a = sampleAlpha(uv);
    return a < MIN_ALPHA ? 0.0 : a; 
}

float detectEdge(vec2 uv, float centerAlpha) {
    if (centerAlpha < MIN_ALPHA) {
        return 0.0;
    }
    vec2 off = vec2(0.1, 0.0);
    float aN = filterAlpha(uv + off.xy);
    float aS = filterAlpha(uv - off.xy);
    float aE = filterAlpha(uv + off.yx);
    float aW = filterAlpha(uv - off.yx);
    float grad = abs(aN - centerAlpha) + abs(aS - centerAlpha) + abs(aE - centerAlpha) + abs(aW - centerAlpha);
    float aNE = filterAlpha(uv + off.xy + off.yx);
    float aSW = filterAlpha(uv - off.xy - off.yx);
    float aSE = filterAlpha(uv + off.xy - off.yx);
    float aNW = filterAlpha(uv - off.xy + off.yx);
    grad += (abs(aNE - centerAlpha) + abs(aSW - centerAlpha) + abs(aSE - centerAlpha) + abs(aNW - centerAlpha)) * 0.5;
    return smoothstep(0.05, 0.5, grad);
}

vec2 sobelAlpha(vec2 uv, vec2 texel) {
    float tl = sampleAlpha(uv + vec2(-texel.x, -texel.y));
    float t  = sampleAlpha(uv + vec2( 0.0,    -texel.y));
    float tr = sampleAlpha(uv + vec2( texel.x, -texel.y));
    float l  = sampleAlpha(uv + vec2(-texel.x,  0.0));
    float r  = sampleAlpha(uv + vec2( texel.x,  0.0));
    float bl = sampleAlpha(uv + vec2(-texel.x,  texel.y));
    float b  = sampleAlpha(uv + vec2( 0.0,     texel.y));
    float br = sampleAlpha(uv + vec2( texel.x,  texel.y));
    float dx = -tl - 2.0*l - bl + tr + 2.0*r + br;
    float dy = -tl - 2.0*t - tr + bl + 2.0*b + br;
    return vec2(dx, dy);
}

float metaball(vec2 p, vec2 c, float r) {
    return pow(max(0.0, 1.0 - length(p - c) / r), 2.0);
}

float goldParticle(vec2 p, vec2 c, float phase, float t) {
    float life = fract(t + phase);
    vec2 pos = c + vec2(sin(phase * 6.283) * 0.08, -life * 0.9);
    return smoothstep(0.05, 0.0, length(p - pos)) * (1.0 - life);
}

// ============================================================================
// MAIN
// ============================================================================

void main() {
    // --- 1. РАСПАКОВКА ---
    vec2  uv_main      = v_v1.xy;
    float alphaBlend   = v_v1.z;
    float sparkLife    = v_v1.w;
    
    vec4  tintColor    = v_v2;
    float reflectionType = v_v3.x;
    
    vec4  addRGB       = v_v4;
    vec4  texMuls      = v_v5;
    
    float effectType   = v_v6.x;
    float tutIntensity = v_v6.y;

    vec2 uv_distorted = uv_main;

    // --- 2. ИСКАЖЕНИЯ ---
    if (effectType > 6.5 && effectType < 7.5) {
        float p_jitter = fract(sin(dot(uv_distorted, vec2(12.9, 78.2))) * 437.5);
        uv_distorted += (p_jitter - 0.5) * 0.003 * sparkLife;
    }

    if (u_shockTime > 0.0) {
        if (u_shockTime < 0.2) {
            uv_distorted.x += sin(uv_distorted.y * 40.0) * u_aberration * 0.1;
        }
        vec2 shock_dir = uv_distorted - u_shockCenter;
        float dist = length(shock_dir);
        if (dist < u_shockTime + 0.1) {
            float mask_shock = smoothstep(u_shockTime - 0.1, u_shockTime, dist) * 
                         (1.0 - smoothstep(u_shockTime, u_shockTime + 0.1, dist));
            uv_distorted += normalize(shock_dir + 0.0001) * (mask_shock * 0.05 * (1.0 - u_shockTime));
        }
    }

    // --- 3. ВЫБОРКА ---
    vec4 color_out = vec4(0.0);
    
    if (v_v5.x > 0.5) {
        color_out = texture2D(u_image0, uv_main);
    } else if (v_v5.y > 0.5) {
        color_out = texture2D(u_image1, uv_main);
    } else if (v_v5.z > 0.5) {
        color_out = texture2D(u_image2, uv_main);
    } else if (v_v5.w > 0.5) {
        color_out = texture2D(u_image3, uv_main);
    }

    if (effectType > 26.5 && effectType < 30.5) {
        color_out = cleanSprite(uv_main);
    }

    // =========================================================================
    // REFLECTION: МАСК-ПРОХОД — только MIRROR пишет маску лужи
    // =========================================================================
    if (u_isMaskPass > 0.5 && u_isMaskPass < 1.5) {
        if (reflectionType > 0.5 && reflectionType < 1.5) { // MIRROR
            if (color_out.a < 0.01) discard;
            // alpha в R и в A, чтобы блендинг GL_ONE/GL_ONE_MINUS_SRC_ALPHA 
            // корректно накапливал/заменял маски без жёсткого стирания
            gl_FragColor = vec4(color_out.a, 0.0, 0.0, color_out.a);
        } else {
            discard; // CAN_BE_REFLECTED и прочие не пишут в маску
        }
        return;
    }

// =========================================================================
// REFLECTION: ПОЛНОЦВЕТНЫЙ ПРОХОД — CAN_BE_REFLECTED рендерится в _reflTexture
// =========================================================================
if (u_isMaskPass > 1.5 && u_isMaskPass < 2.5) {
    if (reflectionType > 1.5 && reflectionType < 2.5) { // CAN_BE_REFLECTED
        if (color_out.a < 0.01) discard;
        // ПРАВИЛЬНЫЙ premultiplied — иначе при перекрытии спрайтов и чтении
        // получится неправильный цвет (бирюза / темнота)
        color_out.rgb *= color_out.a;
        gl_FragColor = color_out;
    } else {
        discard;
    }
    return;
}

// =========================================================================
// REFLECTION: ГЛАВНЫЙ ПРОХОД — полноцветные отражения для MIRROR
// =========================================================================
if (reflectionType > 0.5 && reflectionType < 1.5) {
    vec2 screenUV = gl_FragCoord.xy / u_resolution;
    float maskR = texture2D(u_maskTexture, screenUV).r;
    if (maskR < 0.01) discard;
    
    vec2 reflectedUV = vec2(screenUV.x, 1.0 - screenUV.y - u_reflYOffset);
    
    if (u_reflDistortion > 0.0) {
        float wave = sin(reflectedUV.y * u_reflWaveFreq + u_time * u_reflWaveSpeed) * u_reflWaveAmp * u_reflDistortion;
        reflectedUV.x += wave;
    }
    
    vec4 reflColor = texture2D(u_reflectionTexture, reflectedUV);
    float charAlpha = reflColor.a;
    
    vec3 baseColor = color_out.rgb * (1.0 - u_reflWaterDarken);
    
    // Восстанавливаем полноцветный (unpremultiplied) цвет отражения
    vec3 reflectionColor = reflColor.a > 0.001 
        ? (reflColor.rgb / reflColor.a) 
        : vec3(0.0);
    reflectionColor *= u_reflTintColor * u_reflBrightnessMul;
    
    // Полноцветное отражение: при charAlpha=1 полностью заменяем цвет воды,
    // без остаточного «наложения» baseColor
    // 1. Считаем mixFactor строго в диапазоне [0.0, 1.0]
    float mixFactor = clamp(charAlpha * u_reflIntensity, 0.0, 1.0);
    
    // 2. Смешиваем: если mixFactor = 0, останется чистый baseColor (без бирюзового оверлея)
    vec3 finalRGB = mix(baseColor, reflectionColor, mixFactor);
    
    // 3. Выводим БЕЗ edgeMask для теста
    gl_FragColor = vec4(finalRGB, color_out.a);
    return;
}
    // =========================================================================
    // СВЕТ ОТ УДАРОВ (общий для всех)
    // =========================================================================
    vec2 p = gl_FragCoord.xy / u_resolution;
    float aspect = u_resolution.x / u_resolution.y;
    vec2 p_asp = p * vec2(aspect, 1.0);
    float glow = 0.0;
    
    if (u_hitI0 > 0.001) {
        float m = smoothstep(u_hit0.z, 0.0, distance(p_asp, u_hit0.xy * vec2(aspect, 1.0)));
        glow += (m * m * m * m) * u_hitI0;
    }
    if (u_hitI1 > 0.001) {
        float m = smoothstep(u_hit1.z, 0.0, distance(p_asp, u_hit1.xy * vec2(aspect, 1.0)));
        glow += (m * m * m * m) * u_hitI1;
    }
    if (u_hitI2 > 0.001) {
        float m = smoothstep(u_hit2.z, 0.0, distance(p_asp, u_hit2.xy * vec2(aspect, 1.0)));
        glow += (m * m * m * m) * u_hitI2;
    }
    if (u_hitI3 > 0.001) {
        float m = smoothstep(u_hit3.z, 0.0, distance(p_asp, u_hit3.xy * vec2(aspect, 1.0)));
        glow += (m * m * m * m) * u_hitI3;
    }
    if (u_hitI4 > 0.001) {
        float m = smoothstep(u_hit4.z, 0.0, distance(p_asp, u_hit4.xy * vec2(aspect, 1.0)));
        glow += (m * m * m * m) * u_hitI4;
    }

    vec3 final_glow = u_hitColor * glow * 1.5;
    color_out.rgb = 1.0 - (1.0 - color_out.rgb) * (1.0 - final_glow);

    color_out *= tintColor;
    color_out.rgb += addRGB.rgb;

    // --- ПОСТ-ЭФФЕКТЫ ---
    color_out.rgb *= color_out.a;
    color_out.a *= alphaBlend;

    if (u_lowHPEffect > 0.0) {
        float hp_dist = distance(uv_main, vec2(0.5));
        float pulse = smoothstep(0.3, 0.8, hp_dist) * ((sin(u_time * 5.0) * 0.5 + 0.5) * u_lowHPEffect) * color_out.a;
        color_out.rgb = mix(color_out.rgb, vec3(0.8, 0.0, 0.0) * color_out.a, pulse * 0.7);
    }

    // --- ТУТОРИАЛ ---
    if (tutIntensity > 0.0) {
        vec2 aspect_vec = vec2(u_resolution.x / (u_resolution.y + 0.001), 1.0);
        vec2 p_tut = (gl_FragCoord.xy / u_resolution - u_tutParams.xy) * aspect_vec;
        vec2 d_tut = abs(p_tut) - (u_tutRectSize * aspect_vec) + u_tutParams.z;
        float tut_dist = length(max(d_tut, 0.0)) + min(max(d_tut.x, d_tut.y), 0.0) - u_tutParams.z;
        
        float mask = smoothstep(0.0, 0.005, tut_dist);
        color_out.rgb *= mix(1.0, 1.0 - tutIntensity, mask);
        
        float pulse_tut = sin(u_time * 4.0) * 0.1 + 1.0;
        color_out.rgb *= mix(pulse_tut, 1.0, mask);
        
        float edge_glow = smoothstep(0.004, 0.0, abs(tut_dist)) * smoothstep(0.4, 0.5, sin(p_tut.y * 15.0 - u_time * 8.0)) * 0.5 * tutIntensity;
        color_out.rgb += edge_glow;
    }

    gl_FragColor = color_out;
}