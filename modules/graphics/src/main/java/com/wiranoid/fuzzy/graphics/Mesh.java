package com.wiranoid.fuzzy.graphics;

import com.wiranoid.fuzzy.core.utils.Disposable;
import com.wiranoid.fuzzy.graphics.glutils.ShaderProgram;
import com.wiranoid.fuzzy.graphics.glutils.Vertex;
import com.wiranoid.fuzzy.graphics.glutils.VertexAttribute;
import org.lwjgl.BufferUtils;

import java.nio.FloatBuffer;
import java.nio.IntBuffer;

import static org.lwjgl.opengl.GL11.*;
import static org.lwjgl.opengl.GL15.*;
import static org.lwjgl.opengl.GL30.*;

public class Mesh implements Disposable {
    private Vertex[] vertices;
    private IntBuffer indices;

    private final int vaoId;
    private final int vboId;
    private final int eboId;

    public Mesh(Vertex[] vertices, ShaderProgram shaderProgram) {
        this.vertices = vertices;

        this.vaoId = glGenVertexArrays();
        glBindVertexArray(vaoId);

        this.vboId = glGenBuffers();
        fillVBO(shaderProgram);

        this.eboId = -1;

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    public Mesh(Vertex[] vertices, int[] indices, ShaderProgram shaderProgram) {
        this.vertices = vertices;

        this.vaoId = glGenVertexArrays();
        glBindVertexArray(vaoId);

        this.vboId = glGenBuffers();
        fillVBO(shaderProgram);

        this.indices = convertToBuffer(indices);

        this.eboId = glGenBuffers();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this.eboId);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices, GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    public void render() {
        glBindVertexArray(this.vaoId);

        if (this.eboId == -1) {
            glDrawArrays(GL_TRIANGLES, 0, this.vertices.length);
        } else {
            glDrawElements(GL_TRIANGLES, this.indices);
        }

        glBindVertexArray(0);
    }

    public void dispose() {
        glDeleteVertexArrays(this.vaoId);
        glDeleteBuffers(this.vboId);
    }

    private void fillVBO(ShaderProgram shaderProgram) {
        glBindBuffer(GL_ARRAY_BUFFER, this.vboId);
        glBufferData(GL_ARRAY_BUFFER, convertToBuffer(vertices), GL_STATIC_DRAW);

        Vertex vertex = vertices[0];

        for (VertexAttribute attribute : vertex) {
            shaderProgram.setVertexAttribute(
                    attribute.getLocation(),
                    attribute.size(),
                    GL_FLOAT,
                    false,
                    vertex.sizeInBytes(),
                    attribute.getOffset()
            );
        }
    }

    private FloatBuffer convertToBuffer(Vertex[] vertices) {
        int capacity = vertices.length * vertices[0].getNumberOfComponents();

        FloatBuffer buffer = BufferUtils.createFloatBuffer(capacity);
        for (Vertex vertex: vertices) {
            buffer.put(vertex.getData());
        }

        buffer.flip();

        return buffer;
    }

    private IntBuffer convertToBuffer(int[] indices) {
        IntBuffer buffer = BufferUtils.createIntBuffer(indices.length);
        buffer.put(indices);
        buffer.flip();

        return buffer;
    }
}
