package com.wiranoid.fuzzy.game;

import com.wiranoid.fuzzy.graphics.Camera;
import com.wiranoid.fuzzy.graphics.Window;
import com.wiranoid.fuzzy.graphics.g2d.Texture;
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
import static org.lwjgl.opengl.GL13.*;
import static org.lwjgl.opengl.GL15.*;
import static org.lwjgl.opengl.GL30.*;


public class FuzzyGame {
    private static GLFWErrorCallback errorCallback
            = GLFWErrorCallback.createPrint(System.err);

    private static Window window;

    static {
        glfwSetErrorCallback(errorCallback);

        if (!glfwInit()) {
            throw new IllegalStateException("Unable to initialize GLFW");
        }

        window = new Window(1200, 800, "Fuzzy", false, true);
    }

    private static Camera camera = new Camera(
            // position
            new Vector3f(-1.0f, 2.0f, 7.0f),
            // direction
            new Vector3f(0.0f, 0.0f, 0.0f),
            // world up
            new Vector3f(0.0f, 1.0f, 0.0f),
            // yaw and pitch angles
            -80.0f, -10.0f,
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
                FuzzyGame.window.setIsShouldClose(true);
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

    public static void main(String[] args) {
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
                // Positions           // Normals           // Texture Coords
                -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
                0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
                0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
                0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
                -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
                -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

                -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,
                0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 0.0f,
                0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
                0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
                -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 1.0f,
                -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,

                -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
                -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
                -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
                -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
                -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
                -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

                0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
                0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
                0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
                0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
                0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
                0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

                -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
                0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
                0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
                0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
                -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
                -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

                -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
                0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
                0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
                0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
                -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
                -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
        };

        FloatBuffer verticesBuffer = BufferUtils.createFloatBuffer(36 * 8);

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

        // Position attribute
        lightingShader.setVertexAttribute(0, 3, GL_FLOAT, false, 8 * Float.BYTES, 0);
        // Normal attribute
        lightingShader.setVertexAttribute(1, 3, GL_FLOAT, false, 8 * Float.BYTES, 3 * Float.BYTES);
        // Texture coords
        lightingShader.setVertexAttribute(2, 2, GL_FLOAT, false, 8 * Float.BYTES, 6 * Float.BYTES);
        glBindVertexArray(0);

        // Then, we set the light's VAO (VBO stays the same.
        // After all, the vertices are the same for the light object (also a 3D cube))
        int lightVAO = glGenVertexArrays();
        glBindVertexArray(lightVAO);

        // We only need to bind to the VBO (to link it with glVertexAttribPointer),
        // no need to fill it; the VBO's data already contains all we need.
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        // Set the vertex attributes (only position data for the lamp))
        lightingShader.setVertexAttribute(0, 3, GL_FLOAT, false, 8 * Float.BYTES, 0);
        glBindVertexArray(0);

        lightingShader.use();
        Texture diffuseMap = Texture.load("assets/textures/container2.png");
        lightingShader.setUniform("material.diffuse", 0);

        Texture specularMap = Texture.load("assets/textures/container2_specular.png");
        lightingShader.setUniform("material.specular", 1);

        Texture emissionMap = Texture.load("assets/textures/matrix.jpg");
        lightingShader.setUniform("material.emission", 2);

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

            // Change the light's position values over time (can be done anywhere
            // in the game loop actually, but try to do it at least before using the light source positions)
//            lightPos.x = (float) Math.cos(glfwGetTime()) * 2.0f;
//            lightPos.z = (float) Math.sin(glfwGetTime()) * 2.0f;

            // Use corresponding shader when setting uniforms/drawing objects
            lightingShader.use();

            // Set material properties
            lightingShader.setUniform("material.specular", new Vector3f(0.5f, 0.5f, 0.5f));
            lightingShader.setUniform("material.shininess", 64.0f);

            // Set lights properties
            Vector3f lightColor = new Vector3f(1.0f);
            // Decrease the influence
            Vector3f diffuseColor = lightColor.mul(new Vector3f(0.5f), new Vector3f());
            // Low influence
            Vector3f ambientColor = diffuseColor.mul(new Vector3f(0.2f), new Vector3f());

            lightingShader.setUniform("light.ambient", ambientColor);
            // Let's darken the light a bit to fit the scene
            lightingShader.setUniform("light.diffuse", diffuseColor);
            lightingShader.setUniform("light.specular", new Vector3f(1.0f, 1.0f, 1.0f));
            lightingShader.setUniform("light.position", lightPos);

            lightingShader.setUniform("viewPos", camera.getPosition());

            // Create camera transformations
            lightingShader.setUniform("view", camera.getViewMatrix());

            Matrix4f projection = new Matrix4f().perspective(
                    (float) Math.toRadians(camera.getZoom()), WIDTH / HEIGHT, 0.1f, 100.0f);
            lightingShader.setUniform("projection", projection);

            // Draw the container (using container's vertex attributes)
            lightingShader.setUniform("model", new Matrix4f());

            // Bind diffuse map
            glActiveTexture(GL_TEXTURE0);
            diffuseMap.bind();
            // Bind specular map
            glActiveTexture(GL_TEXTURE1);
            specularMap.bind();
            // Bind emission map
            glActiveTexture(GL_TEXTURE2);
            emissionMap.bind();

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
