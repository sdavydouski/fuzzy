package com.wiranoid.fuzzy.graphics;

import com.wiranoid.fuzzy.graphics.shaders.Shader;
import com.wiranoid.fuzzy.graphics.shaders.ShaderProgram;
import org.joml.Matrix4f;
import org.joml.Vector3f;
import org.lwjgl.BufferUtils;
import org.lwjgl.glfw.*;
import org.lwjgl.opengl.GL;

import java.nio.ByteBuffer;
import java.nio.FloatBuffer;
import java.nio.IntBuffer;

import static org.lwjgl.glfw.GLFW.*;
import static org.lwjgl.opengl.GL11.*;
import static org.lwjgl.opengl.GL13.*;
import static org.lwjgl.opengl.GL15.*;
import static org.lwjgl.opengl.GL20.*;
import static org.lwjgl.opengl.GL30.*;
import static org.lwjgl.stb.STBImage.*;
import static org.lwjgl.system.MemoryUtil.NULL;


public class FuzzyGame {

    private static int WIDTH;
    private static int HEIGHT;

    private static GLFWErrorCallback errorCallback
            = GLFWErrorCallback.createPrint(System.err);

    // Camera
    private static Vector3f cameraPos = new Vector3f(0.0f, 0.0f, 3.0f);
    private static Vector3f cameraFront = new Vector3f(0.0f, 0.0f, -1.0f);
    private static Vector3f cameraUp = new Vector3f(0.0f, 1.0f, 0.0f);

    // Yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction
    // vector pointing to the right (due to how Eular angles work) so we initially
    // rotate a bit to the left.
    private static float yaw = -90.0f;
    private static float pitch = 0.0f;
    private static float lastX = WIDTH / 2.0f;
    private static float lastY = HEIGHT / 2.0f;

    private static float fov = 45.0f;

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
        // Camera controls
        float cameraSpeed = 5.0f * deltaTime;
        if (keys[GLFW_KEY_W]) {
            cameraPos.add(cameraFront.mul(cameraSpeed, new Vector3f()));
        }
        if (keys[GLFW_KEY_S]) {
            cameraPos.sub(cameraFront.mul(cameraSpeed, new Vector3f()));
        }
        if (keys[GLFW_KEY_A]) {
            cameraPos.sub(cameraFront.cross(cameraUp, new Vector3f()).normalize().mul(cameraSpeed));
        }
        if (keys[GLFW_KEY_D]) {
            cameraPos.add(cameraFront.cross(cameraUp, new Vector3f()).normalize().mul(cameraSpeed));
        }
    }

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

            // Arbitrary value
            float sensitivity = 0.03f;
            xOffset *= sensitivity;
            yOffset *= sensitivity;

            yaw += xOffset;
            pitch += yOffset;

            // Make sure that when pitch is out of bounds, screen doesn't get flipped
            if (pitch > 89.0f) {
                pitch = 89.0f;
            }
            if (pitch < -89.0f) {
                pitch = -89.0f;
            }

            Vector3f front = new Vector3f();
            front.x = (float) (Math.cos(Math.toRadians(yaw)) * Math.cos(Math.toRadians(pitch)));
            front.y = (float) Math.sin(Math.toRadians(pitch));
            front.z = (float) (Math.sin(Math.toRadians(yaw)) * Math.cos(Math.toRadians(pitch)));
            cameraFront = front.normalize();
        }
    };

    private static GLFWScrollCallback scrollCallback = new GLFWScrollCallback() {
        @Override
        public void invoke(long window, double xoffset, double yoffset) {
            if (1.0f <= fov && fov <= 80.0f) {
                fov -= yoffset * 3.0f;
            }
            if (fov <= 1.0f) {
                fov = 1.0f;
            }
            if (fov >= 80.0f) {
                fov = 80.0f;
            }
        }
    };

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

        GLFWVidMode vidMode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        WIDTH = vidMode.width();
        HEIGHT = vidMode.height();
//        glfwSetWindowPos(window,
//                (vidMode.width() - WIDTH) / 2,
//                (vidMode.height() - HEIGHT) / 2);

        // Fullscreen
        long window = glfwCreateWindow(WIDTH, HEIGHT, "Fuzzy", glfwGetPrimaryMonitor(), NULL);
        if (window == NULL) {
            glfwTerminate();
            throw new RuntimeException("Failed to create GLFW window");
        }

        glfwSetKeyCallback(window, keyCallback);
        glfwSetCursorPosCallback(window, mouseCallback);
        glfwSetScrollCallback(window, scrollCallback);

        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        glfwMakeContextCurrent(window);
        // Enable V-Sync
        glfwSwapInterval(1);
        GL.createCapabilities();

        // Define the viewport dimensions
        IntBuffer width = BufferUtils.createIntBuffer(1);
        IntBuffer height = BufferUtils.createIntBuffer(1);
        glfwGetFramebufferSize(window, width, height);
        glViewport(0, 0, width.get(), height.get());

        // Shaders
        Shader vertexShader = Shader.load(GL_VERTEX_SHADER, "assets/shaders/vertex/vertex.vert");
        Shader fragmentShader = Shader.load(GL_FRAGMENT_SHADER, "assets/shaders/fragment/fragment.frag");

        // Link shaders
        ShaderProgram shaderProgram = new ShaderProgram();
        shaderProgram.attachShader(vertexShader);
        shaderProgram.attachShader(fragmentShader);
        shaderProgram.link();

        vertexShader.delete();
        fragmentShader.delete();

        // Setup OpenGL options
        glEnable(GL_DEPTH_TEST);

        // Bind the Vertex Array Object first, then bind and set vertex buffer(s) and attribute pointer(s).
        int VAO = glGenVertexArrays();
        glBindVertexArray(VAO);

        float[] vertices = {
                -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
                0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
                0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
                0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
                -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
                -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

                -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
                0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
                0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
                0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
                -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
                -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

                -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
                -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
                -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
                -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
                -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
                -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

                0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
                0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
                0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
                0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
                0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
                0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

                -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
                0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
                0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
                0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
                -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
                -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

                -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
                0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
                0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
                0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
                -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
                -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
        };

        FloatBuffer verticesBuffer = BufferUtils.createFloatBuffer(36 * 6);

        for (float vertex : vertices) {
            verticesBuffer.put(vertex);
        }

        // World space positions of our cubes
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

        // Passing the buffer without flipping will crash JVM because of a EXCEPTION_ACCESS_VIOLATION
        verticesBuffer.flip();

        int VBO = glGenBuffers();
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, verticesBuffer, GL_STATIC_DRAW);

        // We need to specify the input to our vertex shader
        shaderProgram.setVertexAttributePointer(0, 3, GL_FLOAT, false, 5 * Float.BYTES, 0);
        shaderProgram.setVertexAttributePointer(2, 2, GL_FLOAT, false, 5 * Float.BYTES, 3 * Float.BYTES);

        // This is allowed, the call to glVertexAttribPointer registered VBO as the currently bound
        // vertex buffer object so afterwards we can safely unbind
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Unbind VAO (it's always a good thing to unbind any buffer/array to prevent strange bugs),
        glBindVertexArray(0);

        // Load and create a texture
        // ====================
        // Texture 1
        // ====================
        int texture1 = glGenTextures();
        // All upcoming GL_TEXTURE_2D operations now have effect on this texture object
        glBindTexture(GL_TEXTURE_2D, texture1);

        // Set the texture wrapping parameters
        // Set texture wrapping to GL_REPEAT (usually basic wrapping method)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // Set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Load image, create texture and generate mipmaps
        IntBuffer texture1Width = BufferUtils.createIntBuffer(1);
        IntBuffer texture1Height = BufferUtils.createIntBuffer(1);
        IntBuffer texture1Comp = BufferUtils.createIntBuffer(1);

        stbi_set_flip_vertically_on_load(1);
        ByteBuffer texture1Image = stbi_load(
                "assets/textures/container.jpg", texture1Width, texture1Height, texture1Comp, 3
        );

        if (texture1Image == null) {
            throw new RuntimeException("Failed to load a texture file!"
                    + System.lineSeparator() + stbi_failure_reason());
        }

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                texture1Width.get(), texture1Height.get(), 0, GL_RGB, GL_UNSIGNED_BYTE, texture1Image);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(texture1Image);
        // Unbind texture when done, so we won't accidentally mess up our texture.
        glBindTexture(GL_TEXTURE_2D, 0);

        // ====================
        // Texture 2
        // ====================
        int texture2 = glGenTextures();
        // All upcoming GL_TEXTURE_2D operations now have effect on this texture object
        glBindTexture(GL_TEXTURE_2D, texture2);

        // Set the texture wrapping parameters
        // Set texture wrapping to GL_REPEAT (usually basic wrapping method)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // Set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Load image, create texture and generate mipmaps
        IntBuffer texture2Width = BufferUtils.createIntBuffer(1);
        IntBuffer texture2Height = BufferUtils.createIntBuffer(1);
        IntBuffer texture2Comp = BufferUtils.createIntBuffer(1);

        stbi_set_flip_vertically_on_load(1);
        ByteBuffer texture2Image = stbi_load(
                "assets/textures/awesomeface.png", texture2Width, texture2Height, texture2Comp, 3
        );

        if (texture2Image == null) {
            throw new RuntimeException("Failed to load a texture file!"
                    + System.lineSeparator() + stbi_failure_reason());
        }

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                texture2Width.get(), texture2Height.get(), 0, GL_RGB, GL_UNSIGNED_BYTE, texture2Image);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(texture2Image);
        // Unbind texture when done, so we won't accidentally mess up our texture.
        glBindTexture(GL_TEXTURE_2D, 0);

        // Enable wireframe polygons
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        // Game loop
        while (!glfwWindowShouldClose(window)) {
            // Calculate deltatime of current frame
            float currentFrame = (float) glfwGetTime();
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;

            // Check if any events have been activated (key pressed, mouse moved etc.)
            // and call corresponding response functions
            glfwPollEvents();
            doMovement();

            // Render
            // Clear the colorbuffer
            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Bind Textures using texture units
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture1);
            shaderProgram.setUniform("ourTexture1", 0);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, texture2);
            shaderProgram.setUniform("ourTexture2", 1);

            // Activate the shader
            shaderProgram.use();

            // Camera/View transformation
            Matrix4f view = new Matrix4f();
            view.lookAt(cameraPos, cameraPos.add(cameraFront, new Vector3f()), cameraUp);

            shaderProgram.setUniform("view", view);

            // Projection
            Matrix4f projection = new Matrix4f().perspective((float) Math.toRadians(fov), WIDTH / HEIGHT, 0.1f, 100.0f);
            shaderProgram.setUniform("projection", projection);

            glBindVertexArray(VAO);

            for (int i = 0; i < cubePositions.length; i++) {
                // Calculate the model matrix for each object and pass it to shader before drawing
                Matrix4f model = new Matrix4f().translate(cubePositions[i]);
                float time= (float) glfwGetTime();
                model.rotateXYZ(time * (i + 1) / 3.0f, time, time / (i + 1));
                shaderProgram.setUniform("model", model);

                glDrawArrays(GL_TRIANGLES, 0, 36);
            }

            glBindVertexArray(0);

            // Swap the screen buffers
            glfwSwapBuffers(window);
        }

        // Properly de-allocate all resources once they've outlived their purpose
        glDeleteVertexArrays(VAO);
        glDeleteBuffers(VBO);
        shaderProgram.delete();

        glfwDestroyWindow(window);
        keyCallback.free();

        // Terminate GLFW, clearing any resources allocated by GLFW
        glfwTerminate();
        errorCallback.free();
    }

}
