#version 410 core

in vec2 texCoordVarying;
uniform sampler2D textureY;
uniform sampler2D textureU;
uniform sampler2D textureV;
uniform int pixel_format;

out vec4 fragColor;

void main() {
    float r, g, b, y, u, v;
    vec2 uv = texCoordVarying;

    // Sample Y, U, and V based on the pixel format
    if (pixel_format == 25) {
        // AV_PIX_FMT_NV12: interleaved UV plane
        vec4 uv_sample = texture(textureU, uv);
        u = uv_sample.r - 0.5;
        v = uv_sample.g - 0.5;
    } else {
        // AV_PIX_FMT_YUV420P and AV_PIX_FMT_YUVJ420P: separate U and V planes
        y = texture(textureY, uv).r;
        u = texture(textureU, uv).r - 0.5;
        v = texture(textureV, uv).r - 0.5;
    }

    // Adjust Y based on pixel format
    if (pixel_format == 12) {
        // AV_PIX_FMT_YUVJ420P: full range
        // No adjustment needed for full range
    } else if (pixel_format == 0 || pixel_format == 25 || pixel_format == 42) {
        // AV_PIX_FMT_YUV420P, AV_PIX_FMT_NV12, AV_PIX_FMT_YUV420P10LE: limited range
        y = 1.1643 * (y - 0.0625);
    }

    // YUV to RGB conversion
    r = y + 1.5958 * v;
    g = y - 0.39173 * u - 0.81290 * v;
    b = y + 2.017 * u;

    // Output the final color
    fragColor = vec4(r, g, b, 1.0);
}
