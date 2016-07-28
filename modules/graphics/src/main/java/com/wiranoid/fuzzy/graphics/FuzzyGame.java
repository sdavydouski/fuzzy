package com.wiranoid.fuzzy.graphics;

import com.wiranoid.fuzzy.graphics.shaders.Shader;
import com.wiranoid.fuzzy.graphics.shaders.ShaderProgram;
import org.joml.Matrix4f;
import org.joml.Vector3f;
import org.lwjgl.BufferUtils;
import org.lwjgl.glfw.GLFWErrorCallback;
import org.lwjgl.glfw.GLFWKeyCallback;
import org.lwjgl.glfw.GLFWVidMode;
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

        // Bind the Vertex Array Object first, then bind and set vertex buffer(s) and attribute pointer(s).
        int VAO = glGenVertexArrays();
        glBindVertexArray(VAO);

        FloatBuffer vertices = BufferUtils.createFloatBuffer(4 * 8);
        vertices
                // Top Right
                .put(0.5f).put(0.5f).put(0.0f)      // Position
                .put(1.0f).put(0.0f).put(0.0f)      // Color
                .put(1.0f).put(1.0f);               // Texture coord
        vertices
                // Bottom Right
                .put(0.5f).put(-0.5f).put(0.0f)     // Position
                .put(0.0f).put(1.0f).put(0.0f)      // Color
                .put(1.0f).put(0.0f);               // Texture coord
        vertices
                // Bottom Left
                .put(-0.5f).put(-0.5f).put(0.0f)    // Position
                .put(0.0f).put(0.0f).put(1.0f)      // Color
                .put(0.0f).put(0.0f);               // Texture coord
        vertices
                // Top Left
                .put(-0.5f).put(0.5f).put(0.0f)     // Position
                .put(1.0f).put(1.0f).put(0.0f)      // Color
                .put(0.0f).put(1.0f);               // Texture coord
        // Passing the buffer without flipping will crash JVM because of a EXCEPTION_ACCESS_VIOLATION
        vertices.flip();

        int VBO = glGenBuffers();
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices, GL_STATIC_DRAW);


        IntBuffer indices = BufferUtils.createIntBuffer(2 * 3);
        // First Triangle
        indices.put(0).put(1).put(3);
        // Second Triangle
        indices.put(1).put(2).put(3);
        // Passing the buffer without flipping will crash JVM because of a EXCEPTION_ACCESS_VIOLATION
        indices.flip();

        int EBO = glGenBuffers();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices, GL_STATIC_DRAW);

        // We need to specify the input to our vertex shader
        shaderProgram.setVertexAttributePointer(0, 3, GL_FLOAT, false, 8 * Float.BYTES, 0);
        shaderProgram.setVertexAttributePointer(1, 3, GL_FLOAT, false, 8 * Float.BYTES, 3 * Float.BYTES);
        shaderProgram.setVertexAttributePointer(2, 2, GL_FLOAT, false, 8 * Float.BYTES, 6 * Float.BYTES);

        // This is allowed, the call to glVertexAttribPointer registered VBO as the currently bound
        // vertex buffer object so afterwards we can safely unbind
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Unbind VAO (it's always a good thing to unbind any buffer/array to prevent strange bugs),
        // remember: do NOT unbind the EBO, keep it bound to this VAO
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
            // Check if any events have been activated (key pressed, mouse moved etc.)
            // and call corresponding response functions
            glfwPollEvents();

            // Render
            // Clear the colorbuffer
            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            // Activate the shader
            shaderProgram.use();

            Matrix4f model = new Matrix4f().rotate((float) Math.toRadians(-55), new Vector3f(1.0f, 0.0f, 0.0f));
            shaderProgram.setUniform("model", model);

            Matrix4f view = new Matrix4f().translate(new Vector3f(0.0f, 0.0f, -3.0f));
            shaderProgram.setUniform("view", view);

            // Note: currently we set the projection matrix each frame,
            // but since the projection matrix rarely changes it's often
            // best practice to set it outside the main loop only once.
            Matrix4f projection = new Matrix4f().perspective((float) Math.toRadians(45), WIDTH / HEIGHT, 0.1f, 100.0f);
            shaderProgram.setUniform("projection", projection);

            // Bind Textures using texture units
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture1);
            shaderProgram.setUniform("ourTexture1", 0);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, texture2);
            shaderProgram.setUniform("ourTexture2", 1);

            // Draw container
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);

            // Swap the screen buffers
            glfwSwapBuffers(window);
        }

        // Properly de-allocate all resources once they've outlived their purpose
        glDeleteVertexArrays(VAO);
        glDeleteBuffers(VBO);
        glDeleteBuffers(EBO);
        shaderProgram.delete();

        glfwDestroyWindow(window);
        keyCallback.free();

        // Terminate GLFW, clearing any resources allocated by GLFW
        glfwTerminate();
        errorCallback.free();
    }

}
