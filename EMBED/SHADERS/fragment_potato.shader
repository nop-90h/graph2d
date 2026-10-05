precision mediump float;

// Юниформы
uniform sampler2D u_image0, u_image1, u_image2, u_image3;
uniform float u_shockTime, u_lowHPEffect, u_time, u_aberration; 
uniform vec2 u_shockCenter, u_resolution, u_tutRectSize;
uniform vec4 u_tutParams; 


// Юниформы для 5 вспышек
uniform vec3 u_hit0; uniform float u_hitI0;
uniform vec3 u_hit1; uniform float u_hitI1;
uniform vec3 u_hit2; uniform float u_hitI2;
uniform vec3 u_hit3; uniform float u_hitI3;
uniform vec3 u_hit4; uniform float u_hitI4;
uniform vec3 u_hitColor;

// Упакованные Varying
varying vec4 v_v1; // [texCoord.x, texCoord.y, alphaBlend, sparkLife]
varying vec4 v_v2; // tintColor
varying vec4 v_v3; // gsColor
varying vec4 v_v4; // addRGB
varying vec4 v_v5; // texMuls (0, 1, 2, 3)
varying vec2 v_v6; // [effectType, tutIntensity]

void main() {
    // --- 1. РАСПАКОВКА ---
    vec2  uv_main      = v_v1.xy;
    float alphaBlend   = v_v1.z;
    float sparkLife    = v_v1.w;
    
    vec4  tintColor    = v_v2;
    vec4  gsColor      = v_v3;
    vec4  addRGB       = v_v4;
    vec4  texMuls      = v_v5;
    
    float effectType   = v_v6.x;
    float tutIntensity = v_v6.y;

    vec2 uv_distorted = uv_main;

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

    // Применяем маску к финальному цвету
    //color_out *= mask;


    if (gsColor.a > 0.0) {
        float grey = dot(color_out.rgb, vec3(0.299, 0.587, 0.114));
        color_out.rgb = mix(color_out.rgb, vec3(grey), gsColor.a);
    }

    color_out *= tintColor;
    color_out.rgb += addRGB.rgb;

// --- 4. ПОСТ-ЭФФЕКТЫ ---
    color_out.rgb *= color_out.a;
    color_out.a *= alphaBlend;

    if (u_lowHPEffect > 0.0) {
        float hp_dist = distance(uv_main, vec2(0.5));
        float pulse = smoothstep(0.3, 0.8, hp_dist) * ((sin(u_time * 5.0) * 0.5 + 0.5) * u_lowHPEffect) * color_out.a;
        color_out.rgb = mix(color_out.rgb, vec3(0.8, 0.0, 0.0) * color_out.a, pulse * 0.7);
    }

    // --- 5. ТУТОРИАЛ (SDF логика выполняется только при наличии v_tutIntensity) ---
    if (tutIntensity > 0.0) {
        vec2 aspect_vec = vec2(u_resolution.x / (u_resolution.y + 0.001), 1.0);
        vec2 p_tut = (gl_FragCoord.xy / u_resolution - u_tutParams.xy) * aspect_vec;
        vec2 d_tut = abs(p_tut) - (u_tutRectSize * aspect_vec) + u_tutParams.z;
        float tut_dist = length(max(d_tut, 0.0)) + min(max(d_tut.x, d_tut.y), 0.0) - u_tutParams.z;
        
        float mask = smoothstep(0.0, 0.005, tut_dist);
        color_out.rgb *= mix(1.0, 1.0 - tutIntensity, mask);
        
        // Эффект мерцания рамки
        float pulse_tut = sin(u_time * 4.0) * 0.1 + 1.0;
        color_out.rgb *= mix(pulse_tut, 1.0, mask);
        
        // Бегущая полоса по рамке
        float edge_glow = smoothstep(0.004, 0.0, abs(tut_dist)) * smoothstep(0.4, 0.5, sin(p_tut.y * 15.0 - u_time * 8.0)) * 0.5 * tutIntensity;
        color_out.rgb += edge_glow;
    }

    gl_FragColor = color_out;
}