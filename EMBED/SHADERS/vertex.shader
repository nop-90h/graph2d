attribute vec2  a_position;
attribute vec2  a_texCoord;
attribute mat3  a_mat3;
attribute vec4  a_tintColor;
attribute vec4  a_gsColor;
attribute vec4  a_isNormalBlend;
attribute vec4  a_addRGB;
attribute vec4  a_texMuls;
attribute vec3  a_params;

// ”œ¿ Œ¬ ¿:
varying vec4 v_v1; // [texCoord.x, texCoord.y, alphaBlend, params.x (sparkLife)]
varying vec4 v_v2; // tintColor
varying vec4 v_v3; // gsColor
varying vec4 v_v4; // addRGB
varying vec4 v_v5; // texMuls
varying vec2 v_v6; // [params.y (effectType), params.z (tutIntensity)]

void main() {
    v_v1 = vec4(a_texCoord, a_isNormalBlend.a, a_params.x);
    v_v2 = a_tintColor;
    v_v3 = a_gsColor;
    v_v4 = a_addRGB;
    v_v5 = a_texMuls;
    v_v6 = a_params.yz;
    
    gl_Position = vec4((a_mat3 * vec3(a_position, 1.0)).xy, 0.0, 1.0);
}