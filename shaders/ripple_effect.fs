#ifdef GL_ES
precision highp float;
#endif

uniform float uTime;
uniform float uHeight;
uniform vec2 iResolution;
uniform vec2 targetTexturePos;
uniform sampler2D targetTexture;

layout(origin_upper_left) in vec4 gl_FragCoord;

void main(void) {
    vec2 cPos = -1.0 + 2.0 * (gl_FragCoord.xy - targetTexturePos) / iResolution;
    float cLength = length(cPos);

    vec2 uv = (gl_FragCoord.xy - targetTexturePos)/iResolution+(cPos/cLength)*cos(cLength*12.0-uTime*4.0)*uHeight;
    vec3 col = texture2D(targetTexture, vec2(uv.x, uv.y)).xyz;

    gl_FragColor = vec4(col, 1.0);
}