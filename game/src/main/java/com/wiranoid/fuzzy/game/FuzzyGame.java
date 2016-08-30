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

        window = new Window(1600, 900, "Fuzzy", false, true);
    }

    private static Camera camera = new Camera(
            // position
            new Vector3f(0.0f, 3.0f, -10.0f),
            // direction
            new Vector3f(0.0f, 0.0f, 0.0f),
            // world up
            new Vector3f(0.0f, 1.0f, 0.0f),
            // yaw and pitch angles
            90.0f, 0.0f,
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
        glEnable(GL_STENCIL_TEST);

        ShaderProgram shader = new ShaderProgram(
                Shader.load(Shader.Type.VERTEX, "assets/shaders/shader.vert"),
                Shader.load(Shader.Type.FRAGMENT, "assets/shaders/shader.frag")
        );

        Mesh floor = ObjLoader.loadMesh("assets/models/floor/floor.obj");
        floor.bind(shader);

        Mesh box = ObjLoader.loadMesh("assets/models/box/box.obj");
        box.bind(shader);

        Mesh stall = ObjLoader.loadMesh("assets/models/stall/stall.obj");
        stall.bind(shader);

        Mesh nanosuit = ObjLoader.loadMesh("assets/models/nanosuit/nanosuit.obj");
        nanosuit.bind(shader);

        Mesh bunny = ObjLoader.loadMesh("assets/models/bunny/bunny.obj");
        bunny.bind(shader);

        Texture grassTexture = Texture.load("assets/textures/grass.png");
        Texture boxTexture = Texture.load("assets/textures/box.jpg");
        Texture stallTexture = Texture.load("assets/textures/stall.png");

        // Game loop
        while (!window.isClosing()) {
            // Calculate deltatime of current frame
            float currentFrame = (float) glfwGetTime();
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;

            glfwPollEvents();
            doMovement();

            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

            shader.use();

            shader.setUniform("view", camera.getViewMatrix());

            Matrix4f projection = new Matrix4f().perspective(
                    (float) Math.toRadians(camera.getZoom()), WIDTH / HEIGHT, 0.1f, 100.0f);
            shader.setUniform("projection", projection);

            Matrix4f model = new Matrix4f();
            shader.setUniform("model", model);

            shader.setUniform("useTexture", true);

            grassTexture.bind();
            floor.render();

            shader.setUniform("model", new Matrix4f().scale(0.5f));

            stallTexture.bind();
            stall.render();

            boxTexture.bind();
            for (int i = 1; i < 10; i += 2) {
                for (int j = 7; j < 30; j += 3) {
                    for (int k = -18; k < 19; k += 3) {
                        shader.setUniform("model", new Matrix4f()
                                .scale(0.5f).translate(k, i, j));
                        box.render();
                    }
                }
            }

            shader.setUniform("useTexture", false);

            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

            shader.setUniform("model", new Matrix4f()
                    .scale(0.13f)
                    .rotateY((float) Math.toRadians(180.0f))
                    .translate(0.0f, 0.0f, -9.0f));

            nanosuit.render();

            shader.setUniform("model", new Matrix4f()
                    .scale(0.4f)
                    .translate(-9.0f, 0.0f, -9.0f)
                    .rotateY((float) Math.toRadians(120.0f)));

            bunny.render();

            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

            window.update();
        }

        floor.dispose();
        box.dispose();
        stall.dispose();
        nanosuit.dispose();

        shader.dispose();

        grassTexture.dispose();
        boxTexture.dispose();
        stallTexture.dispose();

        window.dispose();

        // Terminate GLFW, clearing any resources allocated by GLFW
        glfwTerminate();
        errorCallback.free();
    }

}
