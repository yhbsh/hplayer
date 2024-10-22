#version 410 core

in vec2 TexCoord;
uniform sampler2D textureY;
uniform sampler2D textureU;
uniform sampler2D textureV;

out vec4 fragColor;

void main() {
    float r, g, b, y, u, v;
    vec2 uv = TexCoord;

    y = texture(textureY, uv).r;
    u = texture(textureU, uv).r - 0.5;
    v = texture(textureV, uv).r - 0.5;

    y = 1.1643 * (y - 0.0625);

    r = y + 1.403 * v;
    g = y - 0.344 * u - 0.714 * v;
    b = y + 1.770 * u;

    fragColor = vec4(clamp(r, 0.0, 1.0), clamp(g, 0.0, 1.0), clamp(b, 0.0, 1.0), 1.0);
}
