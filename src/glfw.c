#include <stdio.h>

#include "glfw.h"

void error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if ((key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q) && action == GLFW_PRESS) { glfwSetWindowShouldClose(window, GLFW_TRUE); }
    if (key == GLFW_KEY_F && action == GLFW_PRESS) { toggle_fullscreen(window); }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    int window_width, window_height;
    glfwGetWindowSize(window, &window_width, &window_height);

    float aspect_ratio = 16.0f / 9.0f;
    int new_width = width;
    int new_height = height;

    if (width / aspect_ratio > height) {
        new_width = height * aspect_ratio;
    } else {
        new_height = width / aspect_ratio;
    }

    int viewport_x = (width - new_width) / 2;
    int viewport_y = (height - new_height) / 2;

    glViewport(viewport_x, viewport_y, new_width, new_height);
}

void toggle_fullscreen(GLFWwindow* window) {
    static int windowed_xpos, windowed_ypos, windowed_width, windowed_height;
    if (glfwGetWindowMonitor(window)) {
        glfwSetWindowMonitor(window, NULL, windowed_xpos, windowed_ypos, windowed_width, windowed_height, 0);
    } else {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwGetWindowPos(window, &windowed_xpos, &windowed_ypos);
        glfwGetWindowSize(window, &windowed_width, &windowed_height);
        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    }
}

int init_glfw(GLFWwindow **window) {
    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    int factor = 100;
    int width = 16 * factor;
    int height = 9 * factor;

    *window = glfwCreateWindow(width, height, "Video Player", NULL, NULL);
    if (*window == NULL) {
        glfwTerminate();
        return -1;
    }

    glfwSetKeyCallback(*window, key_callback);
    glfwSetFramebufferSizeCallback(*window, framebuffer_size_callback);

    glfwMakeContextCurrent(*window);
    glfwSwapInterval(0);

    glfwSetWindowPos(*window, (mode->width - width) / 2, (mode->height - height) / 2);

    return 0;
}

void deinit_glfw(GLFWwindow *window) {
    if (window) glfwDestroyWindow(window);
    glfwTerminate();
}
