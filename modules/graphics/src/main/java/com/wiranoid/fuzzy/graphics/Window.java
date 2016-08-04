package com.wiranoid.fuzzy.graphics;

import org.lwjgl.glfw.*;
import org.lwjgl.opengl.GL;

import static org.lwjgl.glfw.GLFW.*;
import static org.lwjgl.opengl.GL11.GL_FALSE;
import static org.lwjgl.system.MemoryUtil.NULL;

public class Window {
    private final long id;

    private GLFWKeyCallback keyCallback;
    private GLFWCursorPosCallback mouseCallback;
    private GLFWScrollCallback scrollCallback;

    private boolean vsync;


    public long getId() {
        return id;
    }

    public void setKeyCallback(GLFWKeyCallback keyCallback) {
        this.keyCallback = keyCallback;
        glfwSetKeyCallback(id, keyCallback);
    }

    public void setMouseCallback(GLFWCursorPosCallback mouseCallback) {
        this.mouseCallback = mouseCallback;
        glfwSetCursorPosCallback(id, mouseCallback);
    }

    public void setScrollCallback(GLFWScrollCallback scrollCallback) {
        this.scrollCallback = scrollCallback;
        glfwSetScrollCallback(id, scrollCallback);
    }

    public boolean isVSyncEnabled() {
        return vsync;
    }

    public void setVSync(boolean vsync) {
        this.vsync = vsync;
        if (vsync) {
            glfwSwapInterval(1);
        } else {
            glfwSwapInterval(0);
        }
    }

    public Window(int width, int height, String title, boolean isFullScreen, boolean vsync) {
        this.vsync = vsync;

        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

        if (isFullScreen) {
            id = glfwCreateWindow(width, height, title, glfwGetPrimaryMonitor(), NULL);
        } else {
            id = glfwCreateWindow(width, height, title, NULL, NULL);

            /* Center window on screen */
            GLFWVidMode vidMode = glfwGetVideoMode(glfwGetPrimaryMonitor());
            glfwSetWindowPos(id,
                    (vidMode.width() - width) / 2,
                    (vidMode.height() - height) / 2);
        }

        if (id == NULL) {
            glfwTerminate();
            throw new RuntimeException("Failed to create GLFW window");
        }

        /* Create OpenGL context */
        glfwMakeContextCurrent(id);
        GL.createCapabilities();

        /* Enable v-sync */
        if (vsync) {
            glfwSwapInterval(1);
        }
    }

    public void setTitle(String title) {
        glfwSetWindowTitle(id, title);
    }

    public boolean isClosing() {
        return glfwWindowShouldClose(id);
    }

    public void update() {
        glfwSwapBuffers(id);
    }

    public void delete() {
        glfwDestroyWindow(id);
        keyCallback.free();
        mouseCallback.free();
        scrollCallback.free();
    }

}
