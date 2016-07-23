package com.wiranoid.fuzzy.graphics;

import org.lwjgl.BufferUtils;
import org.lwjgl.glfw.GLFWErrorCallback;
import org.lwjgl.glfw.GLFWKeyCallback;
import org.lwjgl.glfw.GLFWVidMode;
import org.lwjgl.opengl.GL;

import java.io.IOException;
import java.nio.FloatBuffer;
import java.nio.IntBuffer;
import java.nio.file.Files;
import java.nio.file.Paths;

import static org.lwjgl.glfw.GLFW.*;
import static org.lwjgl.opengl.GL11.*;
import static org.lwjgl.opengl.GL15.*;
import static org.lwjgl.opengl.GL20.*;
import static org.lwjgl.opengl.GL30.*;
import static org.lwjgl.system.MemoryUtil.NULL;


public class FuzzyGame {

    private static GLFWErrorCallback errorCallback
            = GLFWErrorCallback.createPrint(System.err);

    // Is called whenever a key is pressed/released via GLFW
    private static GLFWKeyCallback keyCallback = new GLFWKeyCallback() {
        @Override
        public void invoke(long window, int key, int scancode, int action, int mods) {
            if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
                glfwSetWindowShouldClose(window, true);
            }
        }
    };

    private static int WIDTH = 640;
    private static int HEIGHT = 480;

    public static void run() {
        glfwSetErrorCallback(errorCallback);

        if (!glfwInit()) {
            throw new IllegalStateException("Unable to initialize GLFW");
        }

        // Set all the required options for GLFW
        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

        long window = glfwCreateWindow(WIDTH, HEIGHT, "Fuzzy", NULL, NULL);
        if (window == NULL) {
            glfwTerminate();
            throw new RuntimeException("Failed to create GLFW window");
        }

        GLFWVidMode vidMode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        glfwSetWindowPos(window,
                (vidMode.width() - WIDTH) / 2,
                (vidMode.height() - HEIGHT) / 2);

        glfwSetKeyCallback(window, keyCallback);

        glfwMakeContextCurrent(window);
        GL.createCapabilities();

        // Define the viewport dimensions
        IntBuffer width = BufferUtils.createIntBuffer(1);
        IntBuffer height = BufferUtils.createIntBuffer(1);
        glfwGetFramebufferSize(window, width, height);
        glViewport(0, 0, width.get(), height.get());

        //read shaders from files
        String vertexShaderSource = "";
        String fragmentShaderSource = "";
        try {
            vertexShaderSource = new String(Files.readAllBytes(Paths.get("shaders/vertex/vertex.vert")));
            fragmentShaderSource = new String(Files.readAllBytes(Paths.get("shaders/fragment/fragment.frag")));
        }
        catch (IOException ex) {
            System.err.println(ex.getMessage());
        }

        // Build and compile our shader program
        // Vertex shader
        int vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, vertexShaderSource);
        glCompileShader(vertexShader);

        // Check for compile time errors
        int status = glGetShaderi(vertexShader, GL_COMPILE_STATUS);
        if (status != GL_TRUE) {
            throw new RuntimeException(glGetShaderInfoLog(vertexShader));
        }

        // Fragment shader
        int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, fragmentShaderSource);
        glCompileShader(fragmentShader);

        // Check for compile time errors
        status = glGetShaderi(fragmentShader, GL_COMPILE_STATUS);
        if (status != GL_TRUE) {
            throw new RuntimeException(glGetShaderInfoLog(fragmentShader));
        }

        // Link shaders
        int shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        // Check for linking errors
        status = glGetProgrami(shaderProgram, GL_LINK_STATUS);
        if (status != GL_TRUE) {
            throw new RuntimeException(glGetProgramInfoLog(shaderProgram));
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        // Bind the Vertex Array Object first, then bind and set vertex buffer(s) and attribute pointer(s).
        int VAO = glGenVertexArrays();
        glBindVertexArray(VAO);

        FloatBuffer vertices = BufferUtils.createFloatBuffer(3 * 6);
        vertices.put(0.5f).put(-0.5f).put(0.0f).put(1.0f).put(0.0f).put(0.0f);
        vertices.put(-0.5f).put(-0.5f).put(0.0f).put(0.0f).put(1.0f).put(0.0f);
        vertices.put(0.0f).put(0.5f).put(0.0f).put(0.0f).put(0.0f).put(1.0f);
        // Passing the buffer without flipping will crash JVM because of a EXCEPTION_ACCESS_VIOLATION
        vertices.flip();

        int VBO = glGenBuffers();
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices, GL_STATIC_DRAW);

        // We need to specify the input to our vertex shader
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, false, 6 * Float.BYTES, 0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, false, 6 * Float.BYTES, 3 * Float.BYTES);

        // This is allowed, the call to glVertexAttribPointer registered VBO as the currently bound
        // vertex buffer object so afterwards we can safely unbind
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Unbind VAO (it's always a good thing to unbind any buffer/array to prevent strange bugs),
        // remember: do NOT unbind the EBO, keep it bound to this VAO
        glBindVertexArray(0);

        // Enable wireframe polygons
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        // Game loop
        while (!glfwWindowShouldClose(window)) {
            // Check if any events have been activated (key pressed, mouse moved etc.)
            // and call corresponding response functions
            glfwPollEvents();

            // Render
            // Clear the colorbuffer
            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            // Activate the shader
            glUseProgram(shaderProgram);

            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindVertexArray(0);

            // Swap the screen buffers
            glfwSwapBuffers(window);
        }

        // Properly de-allocate all resources once they've outlived their purpose
        glDeleteVertexArrays(VAO);
        glDeleteBuffers(VBO);
        glDeleteProgram(shaderProgram);

        glfwDestroyWindow(window);
        keyCallback.free();

        // Terminate GLFW, clearing any resources allocated by GLFW
        glfwTerminate();
        errorCallback.free();
    }

}
