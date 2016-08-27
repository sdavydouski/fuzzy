package com.wiranoid.fuzzy.game;

import com.wiranoid.fuzzy.graphics.Camera;
import com.wiranoid.fuzzy.graphics.Mesh;
import com.wiranoid.fuzzy.graphics.Texture;
import com.wiranoid.fuzzy.graphics.Window;
import com.wiranoid.fuzzy.graphics.g3d.loaders.ObjLoader;
import com.wiranoid.fuzzy.graphics.glutils.Shader;
import com.wiranoid.fuzzy.graphics.glutils.ShaderProgram;
import org.joml.Matrix4f;
import org.joml.Vector3f;
import org.lwjgl.BufferUtils;
import org.lwjgl.glfw.*;

import java.nio.IntBuffer;

import static org.lwjgl.glfw.GLFW.*;
import static org.lwjgl.opengl.GL11.*;

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

        window = new Window(1200, 700, "Fuzzy", false, true);
    }

    private static Camera camera = new Camera(
            // position
            new Vector3f(0.0f, 3.0f, 10.0f),
            // direction
            new Vector3f(0.0f, 0.0f, 0.0f),
            // world up
            new Vector3f(0.0f, 1.0f, 0.0f),
            // yaw and pitch angles
            -90.0f, 0.0f,
            // movementSpeed, mouseSensitivity, field of view (zoom)
            6.0f, 0.03f, 45.0f);

    // Deltatime
    // Time between current frame and last frame
    private static float deltaTime = 0.0f;
    // Time of last frame
    private static float lastFrame = 0.0f;

    private static boolean[] pressedKeys = new boolean[1024];

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

        ShaderProgram shader = new ShaderProgram(
                Shader.load(Shader.Type.VERTEX, "assets/shaders/models/box/box.vert"),
                Shader.load(Shader.Type.FRAGMENT, "assets/shaders/models/box/box.frag")
        );

        Mesh box = ObjLoader.loadMesh("assets/models/box/box.obj");
        box.bind(shader);
        Mesh sphere = ObjLoader.loadMesh("assets/models/sphere/sphere.obj");
        sphere.bind(shader);

        Texture boxTexture = Texture.load("assets/textures/metal.jpg");
        Texture sphereTexture = Texture.load("assets/textures/triangles.jpg");

        // Game loop
        while (!window.isClosing()) {
            // Calculate deltatime of current frame
            float currentFrame = (float) glfwGetTime();
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;

            glfwPollEvents();
            doMovement();

            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            shader.use();

            shader.setUniform("view", camera.getViewMatrix());

            Matrix4f projection = new Matrix4f().perspective(
                    (float) Math.toRadians(camera.getZoom()), WIDTH / HEIGHT, 0.1f, 100.0f);
            shader.setUniform("projection", projection);

            Matrix4f model = new Matrix4f();
            shader.setUniform("model", model);

            boxTexture.bind();
            box.render();

            shader.setUniform("model", model.translate(0.0f, 5.0f, 0.0f));

            sphereTexture.bind();
            sphere.render();

            window.update();
        }

        box.dispose();
        sphere.dispose();
        shader.dispose();
        boxTexture.dispose();
        sphereTexture.dispose();

        window.dispose();

        // Terminate GLFW, clearing any resources allocated by GLFW
        glfwTerminate();
        errorCallback.free();
    }

}
