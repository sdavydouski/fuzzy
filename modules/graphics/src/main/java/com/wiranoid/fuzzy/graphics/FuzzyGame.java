package com.wiranoid.fuzzy.graphics;

import com.wiranoid.fuzzy.graphics.glutils.Shader;
import com.wiranoid.fuzzy.graphics.glutils.ShaderProgram;
import org.joml.Matrix4f;
import org.joml.Vector3f;
import org.lwjgl.BufferUtils;
import org.lwjgl.glfw.*;

import java.nio.FloatBuffer;
import java.nio.IntBuffer;

import static org.lwjgl.glfw.GLFW.*;
import static org.lwjgl.opengl.GL11.*;
import static org.lwjgl.opengl.GL15.*;
import static org.lwjgl.opengl.GL30.*;


public class FuzzyGame {
    private static GLFWErrorCallback errorCallback
            = GLFWErrorCallback.createPrint(System.err);

    static {
        glfwSetErrorCallback(errorCallback);

        if (!glfwInit()) {
            throw new IllegalStateException("Unable to initialize GLFW");
        }
    }

    private static Camera camera = new Camera(
            // position
            new Vector3f(-1.0f, 2.0f, 5.0f),
            // direction
            new Vector3f(0.0f, 0.0f, 0.0f),
            // world up
            new Vector3f(0.0f, 1.0f, 0.0f),
            // yaw and pitch angles
            -70.0f, -20.0f,
            // movementSpeed, mouseSensitivity, field of view (zoom)
            5.0f, 0.03f, 45.0f);

    private static GLFWVidMode vidMode = glfwGetVideoMode(glfwGetPrimaryMonitor());

    private static int WIDTH = vidMode.width();
    private static int HEIGHT = vidMode.height();

    // Deltatime
    // Time between current frame and last frame
    private static float deltaTime = 0.0f;
    // Time of last frame
    private static float lastFrame = 0.0f;

    private static boolean[] keys = new boolean[1024];

    // Is called whenever a key is pressed/released via GLFW
    private static GLFWKeyCallback keyCallback = new GLFWKeyCallback() {
        @Override
        public void invoke(long window, int key, int scancode, int action, int mods) {
            if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
                glfwSetWindowShouldClose(window, true);
            }

            if (0 <= key && key < 1024) {
                if (action == GLFW_PRESS) {
                    keys[key] = true;
                } else if (action == GLFW_RELEASE) {
                    keys[key] = false;
                }
            }
        }
    };

    private static void doMovement() {
        if (keys[GLFW_KEY_W]) {
            camera.processKeyboard(Camera.Movement.FORWARD, deltaTime);
        }
        if (keys[GLFW_KEY_S]) {
            camera.processKeyboard(Camera.Movement.BACKWARD, deltaTime);
        }
        if (keys[GLFW_KEY_A]) {
            camera.processKeyboard(Camera.Movement.LEFT, deltaTime);
        }
        if (keys[GLFW_KEY_D]) {
            camera.processKeyboard(Camera.Movement.RIGHT, deltaTime);
        }
    }

    private static float lastX = WIDTH / 2.0f;
    private static float lastY = HEIGHT / 2.0f;

    private static boolean firstMouse = true;
    private static GLFWCursorPosCallback mouseCallback = new GLFWCursorPosCallback() {
        @Override
        public void invoke(long window, double xpos, double ypos) {
            if (firstMouse) {
                lastX = (float) xpos;
                lastY = (float) ypos;
                firstMouse = false;
            }

            float xOffset = (float) xpos - lastX;
            // Reversed since y-coordinates go from bottom to left
            float yOffset = (float) (lastY - ypos);

            lastX = (float) xpos;
            lastY = (float) ypos;

            camera.processMouseMovement(xOffset, yOffset, true);
        }
    };

    private static GLFWScrollCallback scrollCallback = new GLFWScrollCallback() {
        @Override
        public void invoke(long window, double xoffset, double yoffset) {
            camera.processMouseScroll((float) yoffset);
        }
    };

    public static void run() {
        Window window = new Window(1200, 800, "Fuzzy", false, true);

        window.setKeyCallback(keyCallback);
        window.setMouseCallback(mouseCallback);
        window.setScrollCallback(scrollCallback);

        window.setInputMode(GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        // Define the viewport dimensions
        IntBuffer width = BufferUtils.createIntBuffer(1);
        IntBuffer height = BufferUtils.createIntBuffer(1);
        glfwGetFramebufferSize(window.getId(), width, height);
        glViewport(0, 0, width.get(), height.get());

        // Setup OpenGL options
        glEnable(GL_DEPTH_TEST);

        ShaderProgram lightingShader = new ShaderProgram(
                Shader.load(Shader.Type.VERTEX, "assets/shaders/vertex/lighting.vert"),
                Shader.load(Shader.Type.FRAGMENT, "assets/shaders/fragment/lighting.frag")
        );
        lightingShader.link();

        ShaderProgram lampShader = new ShaderProgram(
                Shader.load(Shader.Type.VERTEX, "assets/shaders/vertex/lamp.vert"),
                Shader.load(Shader.Type.FRAGMENT, "assets/shaders/fragment/lamp.frag")
        );
        lampShader.link();

        // Light attributes
        Vector3f lightPos = new Vector3f(1.2f, 1.0f, 2.0f);

        // Set up vertex data (and buffer(s)) and attribute pointers
        float[] vertices = {
                -0.5f, -0.5f, -0.5f,
                0.5f, -0.5f, -0.5f,
                0.5f,  0.5f, -0.5f,
                0.5f,  0.5f, -0.5f,
                -0.5f,  0.5f, -0.5f,
                -0.5f, -0.5f, -0.5f,

                -0.5f, -0.5f,  0.5f,
                0.5f, -0.5f,  0.5f,
                0.5f,  0.5f,  0.5f,
                0.5f,  0.5f,  0.5f,
                -0.5f,  0.5f,  0.5f,
                -0.5f, -0.5f,  0.5f,

                -0.5f,  0.5f,  0.5f,
                -0.5f,  0.5f, -0.5f,
                -0.5f, -0.5f, -0.5f,
                -0.5f, -0.5f, -0.5f,
                -0.5f, -0.5f,  0.5f,
                -0.5f,  0.5f,  0.5f,

                0.5f,  0.5f,  0.5f,
                0.5f,  0.5f, -0.5f,
                0.5f, -0.5f, -0.5f,
                0.5f, -0.5f, -0.5f,
                0.5f, -0.5f,  0.5f,
                0.5f,  0.5f,  0.5f,

                -0.5f, -0.5f, -0.5f,
                0.5f, -0.5f, -0.5f,
                0.5f, -0.5f,  0.5f,
                0.5f, -0.5f,  0.5f,
                -0.5f, -0.5f,  0.5f,
                -0.5f, -0.5f, -0.5f,

                -0.5f,  0.5f, -0.5f,
                0.5f,  0.5f, -0.5f,
                0.5f,  0.5f,  0.5f,
                0.5f,  0.5f,  0.5f,
                -0.5f,  0.5f,  0.5f,
                -0.5f,  0.5f, -0.5f
        };

        FloatBuffer verticesBuffer = BufferUtils.createFloatBuffer(36 * 6);

        for (float vertex : vertices) {
            verticesBuffer.put(vertex);
        }
        verticesBuffer.flip();

        // First, set the container's VAO (and VBO)
        int containerVAO = glGenVertexArrays();
        glBindVertexArray(containerVAO);

        int VBO = glGenBuffers();
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, verticesBuffer, GL_STATIC_DRAW);

        lightingShader.setVertexAttribute(0, 3, GL_FLOAT, false, 3 * Float.BYTES, 0);
        glBindVertexArray(0);

        // Then, we set the light's VAO (VBO stays the same.
        // After all, the vertices are the same for the light object (also a 3D cube))
        int lightVAO = glGenVertexArrays();
        glBindVertexArray(lightVAO);

        // We only need to bind to the VBO (to link it with glVertexAttribPointer),
        // no need to fill it; the VBO's data already contains all we need.
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        // Set the vertex attributes (only position data for the lamp))
        lightingShader.setVertexAttribute(0, 3, GL_FLOAT, false, 3 * Float.BYTES, 0);
        glBindVertexArray(0);

        // Game loop
        while (!window.isClosing()) {
            // Calculate deltatime of current frame
            float currentFrame = (float) glfwGetTime();
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;

            // Check if any events have been activated (key pressed, mouse moved etc.)
            // and call corresponding response functions
            glfwPollEvents();
            doMovement();

            // Clear the colorbuffer
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Use cooresponding shader when setting uniforms/drawing objects
            lightingShader.use();

            lightingShader.setUniform("objectColor", new Vector3f(1.0f, 0.5f, 0.31f));
            lightingShader.setUniform("lightColor", new Vector3f(1.0f, 0.5f, 1.0f));

            // Create camera transformations
            lightingShader.setUniform("view", camera.getViewMatrix());

            Matrix4f projection = new Matrix4f().perspective(
                    (float) Math.toRadians(camera.getZoom()), WIDTH / HEIGHT, 0.1f, 100.0f);
            lightingShader.setUniform("projection", projection);

            // Draw the container (using container's vertex attributes)
            lightingShader.setUniform("model", new Matrix4f());

            glBindVertexArray(containerVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glBindVertexArray(0);

            // Also draw the lamp object, again binding the appropriate shader
            lampShader.use();

            lampShader.setUniform("view", camera.getViewMatrix());

            projection = new Matrix4f().perspective(
                    (float) Math.toRadians(camera.getZoom()), WIDTH / HEIGHT, 0.1f, 100.0f);
            lampShader.setUniform("projection", projection);

            // Draw the light object (using light's vertex attributes)
            lampShader.setUniform("model", new Matrix4f().translate(lightPos).scale(new Vector3f(0.2f)));

            glBindVertexArray(lightVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glBindVertexArray(0);

            window.update();
        }

        // Properly de-allocate all resources once they've outlived their purpose
        glDeleteVertexArrays(containerVAO);
        glDeleteVertexArrays(lightVAO);
        glDeleteBuffers(VBO);
        lightingShader.dispose();
        lampShader.dispose();

        window.dispose();

        // Terminate GLFW, clearing any resources allocated by GLFW
        glfwTerminate();
        errorCallback.free();
    }

}
