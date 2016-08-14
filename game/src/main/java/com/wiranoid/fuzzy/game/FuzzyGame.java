package com.wiranoid.fuzzy.game;

import com.wiranoid.fuzzy.graphics.Camera;
import com.wiranoid.fuzzy.graphics.Window;
import com.wiranoid.fuzzy.graphics.Texture;
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

    private static Window window;

    private static int WIDTH;
    private static int HEIGHT;

    static {
        glfwSetErrorCallback(errorCallback);

        if (!glfwInit()) {
            throw new IllegalStateException("Unable to initialize GLFW");
        }

        GLFWVidMode vidMode;
        vidMode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        WIDTH = vidMode.width();
        HEIGHT = vidMode.height();

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

    // Deltatime
    // Time between current frame and last frame
    private static float deltaTime = 0.0f;
    // Time of last frame
    private static float lastFrame = 0.0f;

    private static boolean[] pressedKeys = new boolean[1024];
    private static boolean[] toggleKeys = new boolean[1024];


    // Is called whenever a key is pressed/released via GLFW
    private static GLFWKeyCallback keyCallback = new GLFWKeyCallback() {
        @Override
        public void invoke(long window, int key, int scancode, int action, int mods) {
            if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
                FuzzyGame.window.setIsShouldClose(true);
            }

            if (0 <= key && key < 1024) {
                if (action == GLFW_PRESS) {
                    pressedKeys[key] = true;
                } else if (action == GLFW_RELEASE) {
                    pressedKeys[key] = false;
                }
            }

            if (0 <= key && key < 1024) {
                if (action == GLFW_PRESS) {
                    toggleKeys[key] = !toggleKeys[key];
                }
            }
        }
    };

    private static void doMovement() {
        if (pressedKeys[GLFW_KEY_W]) {
            camera.processKeyboard(Camera.Movement.FORWARD, deltaTime);
        }
        if (pressedKeys[GLFW_KEY_S]) {
            camera.processKeyboard(Camera.Movement.BACKWARD, deltaTime);
        }
        if (pressedKeys[GLFW_KEY_A]) {
            camera.processKeyboard(Camera.Movement.LEFT, deltaTime);
        }
        if (pressedKeys[GLFW_KEY_D]) {
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

        // Positions of all containers
        Vector3f[] cubePositions = {
            new Vector3f(0.0f, 0.0f, 0.0f),
            new Vector3f(2.0f, 5.0f, -15.0f),
            new Vector3f(-1.5f, -2.2f, -2.5f),
            new Vector3f(-3.8f, -2.0f, -12.3f),
            new Vector3f(2.4f, -0.4f, -3.5f),
            new Vector3f(-1.7f, 3.0f, -7.5f),
            new Vector3f(1.3f, -2.0f, -2.5f),
            new Vector3f(1.5f, 2.0f, -2.5f),
            new Vector3f(1.5f, 0.2f, -1.5f),
            new Vector3f(-1.3f, 1.0f, -1.5f)
        };

        // Positions of the point lights
        Vector3f[] pointLightPositions = {
            new Vector3f(0.7f, 0.2f, 2.0f),
            new Vector3f(2.3f, -3.3f, -4.0f),
            new Vector3f(-4.0f, 2.0f, -12.0f),
            new Vector3f(0.0f, 0.0f, -3.0f)
        };

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

        lightingShader.setUniform("material.shininess", 32.0f);

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

            // Use corresponding shader when setting uniforms/drawing objects
            lightingShader.use();

            lightingShader.setUniform("viewPosition", camera.getPosition());

            // Set lights properties
            // Directional light
            lightingShader.setUniform("directionalLight.direction", new Vector3f(-0.2f, -1.0f, -0.3f));
            lightingShader.setUniform("directionalLight.ambient", new Vector3f(0.05f));
            lightingShader.setUniform("directionalLight.diffuse", new Vector3f(0.4f));
            lightingShader.setUniform("directionalLight.specular", new Vector3f(0.5f));
            // Point light 1
            lightingShader.setUniform("pointLights[0].position", pointLightPositions[0]);
            lightingShader.setUniform("pointLights[0].ambient", new Vector3f(0.05f));
            lightingShader.setUniform("pointLights[0].diffuse", new Vector3f(0.8f));
            lightingShader.setUniform("pointLights[0].specular", new Vector3f(1.0f));
            lightingShader.setUniform("pointLights[0].constant", 1.0f);
            lightingShader.setUniform("pointLights[0].linear", 0.09f);
            lightingShader.setUniform("pointLights[0].quadratic", 0.032f);
            // Point light 2
            lightingShader.setUniform("pointLights[1].position", pointLightPositions[1]);
            lightingShader.setUniform("pointLights[1].ambient", new Vector3f(0.05f));
            lightingShader.setUniform("pointLights[1].diffuse", new Vector3f(0.8f));
            lightingShader.setUniform("pointLights[1].specular", new Vector3f(1.0f));
            lightingShader.setUniform("pointLights[1].constant", 1.0f);
            lightingShader.setUniform("pointLights[1].linear", 0.09f);
            lightingShader.setUniform("pointLights[1].quadratic", 0.032f);
            // Point light 3
            lightingShader.setUniform("pointLights[2].position", pointLightPositions[2]);
            lightingShader.setUniform("pointLights[2].ambient", new Vector3f(0.05f));
            lightingShader.setUniform("pointLights[2].diffuse", new Vector3f(0.8f));
            lightingShader.setUniform("pointLights[2].specular", new Vector3f(1.0f));
            lightingShader.setUniform("pointLights[2].constant", 1.0f);
            lightingShader.setUniform("pointLights[2].linear", 0.09f);
            lightingShader.setUniform("pointLights[2].quadratic", 0.032f);
            // Point light 4
            lightingShader.setUniform("pointLights[3].position", pointLightPositions[3]);
            lightingShader.setUniform("pointLights[3].ambient", new Vector3f(0.05f));
            lightingShader.setUniform("pointLights[3].diffuse", new Vector3f(0.8f));
            lightingShader.setUniform("pointLights[3].specular", new Vector3f(1.0f));
            lightingShader.setUniform("pointLights[3].constant", 1.0f);
            lightingShader.setUniform("pointLights[3].linear", 0.09f);
            lightingShader.setUniform("pointLights[3].quadratic", 0.032f);
            // SpotLight
            lightingShader.setUniform("spotLight.position", camera.getPosition());
            lightingShader.setUniform("spotLight.direction", camera.getDirection());
            lightingShader.setUniform("spotLight.ambient", new Vector3f(0.0f));
            lightingShader.setUniform("spotLight.diffuse", new Vector3f(1.0f));
            lightingShader.setUniform("spotLight.specular", new Vector3f(1.0f));
            lightingShader.setUniform("spotLight.constant", 1.0f);
            lightingShader.setUniform("spotLight.linear", 0.09f);
            lightingShader.setUniform("spotLight.quadratic", 0.032f);
            lightingShader.setUniform("spotLight.innerCutOff", (float) Math.cos(Math.toRadians(12.5f)));
            lightingShader.setUniform("spotLight.outerCutOff", (float) Math.cos(Math.toRadians(15.0f)));
            if (toggleKeys[GLFW.GLFW_KEY_SPACE]) {
                lightingShader.setUniform("spotLight.enabled", true);
            } else {
                lightingShader.setUniform("spotLight.enabled", false);
            }

            // Create camera transformations
            lightingShader.setUniform("view", camera.getViewMatrix());

            Matrix4f projection = new Matrix4f().perspective(
                    (float) Math.toRadians(camera.getZoom()), WIDTH / HEIGHT, 0.1f, 100.0f);
            lightingShader.setUniform("projection", projection);

            diffuseMap.bind(0);
            specularMap.bind(1);

            glBindVertexArray(containerVAO);
            // Draw 10 containers with the same VAO and VBO information;
            // only their world space coordinates differ
            for (int i = 0; i < cubePositions.length; i++) {
                Matrix4f model = new Matrix4f();
                model.translate(cubePositions[i]);
                float angle = 20.0f * i;
                model.rotateXYZ(angle, 0.3f * angle, 0.7f * angle);
                lightingShader.setUniform("model", model);

                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
            glBindVertexArray(0);

            // Also draw the lamp object, again binding the appropriate shader
            lampShader.use();

            lampShader.setUniform("view", camera.getViewMatrix());

            projection = new Matrix4f().perspective(
                    (float) Math.toRadians(camera.getZoom()), WIDTH / HEIGHT, 0.1f, 100.0f);
            lampShader.setUniform("projection", projection);

            // We now draw as many light bulbs as we have point lights
            glBindVertexArray(lightVAO);
            for (int i = 0; i < pointLightPositions.length; i++) {
                Matrix4f model = new Matrix4f();
                model.translate(pointLightPositions[i]);
                model.scale(0.2f);
                lampShader.setUniform("model", model);

                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
            glBindVertexArray(0);

            window.update();
        }

        // Properly de-allocate all resources once they've outlived their purpose
        glDeleteVertexArrays(containerVAO);
        glDeleteVertexArrays(lightVAO);
        glDeleteBuffers(VBO);

        diffuseMap.dispose();
        specularMap.dispose();

        lightingShader.dispose();
        lampShader.dispose();

        window.dispose();

        // Terminate GLFW, clearing any resources allocated by GLFW
        glfwTerminate();
        errorCallback.free();
    }

}