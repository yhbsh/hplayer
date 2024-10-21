#version 410 core

in vec2 TexCoord;
uniform sampler2D textureY;
uniform sampler2D textureU;
uniform sampler2D textureV;
uniform int pixel_format;

out vec4 fragColor;

void main() {
    float r, g, b, y, u, v;
    vec2 uv = TexCoord;

    if (pixel_format == 25) {
        vec4 uv_sample = texture(textureU, uv);
        u = uv_sample.r - 0.5;
        v = uv_sample.g - 0.5;
    } else {
        y = texture(textureY, uv).r;
        u = texture(textureU, uv).r - 0.5;
        v = texture(textureV, uv).r - 0.5;
    }

    if (pixel_format == 0 || pixel_format == 25 || pixel_format == 42) {
        y = 1.1643 * (y - 0.0625);
    }

    r = y + 1.5958 * v;
    g = y - 0.39173 * u - 0.81290 * v;
    b = y + 2.017 * u;

    fragColor = vec4(r, g, b, 1.0);
}
