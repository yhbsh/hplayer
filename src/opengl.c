#include "opengl.h"

#include <stdio.h>

const char *vert_src = "#version 410\n"
                       "layout (location = 0) in vec2 position;\n"
                       "layout (location = 1) in vec2 texCoord;\n"
                       "out vec2 texCoordVarying;\n"
                       "void main() {\n"
                       "    gl_Position = vec4(position, 0.0, 1.0);\n"
                       "    texCoordVarying = texCoord;\n"
                       "}\n";

const char *frag_src = "#version 410\n"
                       "in vec2 texCoordVarying;\n"
                       "uniform sampler2D textureY;\n"
                       "uniform sampler2D textureU;\n"
                       "uniform sampler2D textureV;\n"
                       "out vec4 fragColor;\n"
                       "void main() {\n"
                       "    vec3 yuv;\n"
                       "    vec3 rgb;\n"
                       "    yuv.x = texture(textureY, texCoordVarying).r;\n"
                       "    yuv.y = texture(textureU, texCoordVarying).r - 0.5;\n"
                       "    yuv.z = texture(textureV, texCoordVarying).r - 0.5;\n"
                       "    rgb = mat3(1.0, 1.0, 1.0, 0.0, -0.344, 1.772, 1.402, -0.714, 0.0) * yuv;\n"
                       "    fragColor = vec4(rgb, 1.0);\n"
                       "}\n";

int init_opengl(GLuint *prog, GLuint *vao, GLuint *vbo, GLuint *textures) {
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *version     = glGetString(GL_VERSION);
    const GLubyte *glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &vert_src, NULL);
    glCompileShader(vert);

    GLint status;
    glGetShaderiv(vert, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        GLchar infoLog[512];
        glGetShaderInfoLog(vert, 512, NULL, infoLog);
        printf("Vertex Shader Compilation Failed: %s\n", infoLog);
        return -1;
    }

    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &frag_src, NULL);
    glCompileShader(frag);

    glGetShaderiv(frag, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        GLchar infoLog[512];
        glGetShaderInfoLog(frag, 512, NULL, infoLog);
        printf("Fragment Shader Compilation Failed: %s\n", infoLog);
        return -1;
    }

    *prog = glCreateProgram();
    glAttachShader(*prog, vert);
    glAttachShader(*prog, frag);
    glLinkProgram(*prog);

    glGetProgramiv(*prog, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        GLchar infoLog[512];
        glGetProgramInfoLog(*prog, 512, NULL, infoLog);
        printf("Shader Program Linking Failed: %s\n", infoLog);
        return -1;
    }

    glUseProgram(*prog);

    glGenTextures(3, textures);

    // clang-format off
    GLfloat vertices[] = {
        -1.0f, +1.0f, +0.0f, +0.0f,
        -1.0f, -1.0f, +0.0f, +1.0f,
        +1.0f, +1.0f, +1.0f, +0.0f,
        +1.0f, -1.0f, +1.0f, +1.0f,
    };
    // clang-format on

    glGenVertexArrays(1, vao);
    glBindVertexArray(*vao);

    glGenBuffers(1, vbo);
    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    GLuint positionAttrib = glGetAttribLocation(*prog, "position");
    glEnableVertexAttribArray(positionAttrib);
    glVertexAttribPointer(positionAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void *) 0);

    GLuint texCoordAttrib = glGetAttribLocation(*prog, "texCoord");
    glEnableVertexAttribArray(texCoordAttrib);
    glVertexAttribPointer(texCoordAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid *) (2 * sizeof(GLfloat)));

    glUniform1i(glGetUniformLocation(*prog, "textureY"), 0);
    glUniform1i(glGetUniformLocation(*prog, "textureU"), 1);
    glUniform1i(glGetUniformLocation(*prog, "textureV"), 2);

    // Clean up shaders
    glDetachShader(*prog, vert);
    glDetachShader(*prog, frag);
    glDeleteShader(vert);
    glDeleteShader(frag);

    return 0;
}

void deinit_opengl(GLuint prog, GLuint vao, GLuint vbo, GLuint *textures) {
    glDeleteProgram(prog);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteTextures(3, textures);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
}
