package com.wiranoid.fuzzy.graphics;

import com.wiranoid.fuzzy.core.utils.Disposable;
import com.wiranoid.fuzzy.graphics.glutils.ShaderProgram;
import com.wiranoid.fuzzy.graphics.glutils.Vertex;
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

    public Mesh(Vertex[] vertices) {
        this.vertices = vertices;
        this.vaoId = glGenVertexArrays();
        this.vboId = glGenBuffers();
        this.eboId = -1;
    }

    public Mesh(Vertex[] vertices, int[] indices) {
        this.vertices = vertices;
        this.vaoId = glGenVertexArrays();
        this.vboId = glGenBuffers();
        this.indices = convertToBuffer(indices);
        this.eboId = glGenBuffers();
    }

    public void bind(ShaderProgram shaderProgram) {
        glBindVertexArray(this.vaoId);

        glBindBuffer(GL_ARRAY_BUFFER, this.vboId);
        glBufferData(GL_ARRAY_BUFFER, convertToBuffer(vertices), GL_STATIC_DRAW);

        Vertex vertex = this.vertices[0];

        for (int i = 0; i < vertex.getNumberOfAttributes(); i++) {
            shaderProgram.setVertexAttribute(
                    vertex.getLocations()[i],
                    vertex.getSizes()[i],
                    GL_FLOAT,
                    false,
                    vertex.getSizeInBytes(),
                    vertex.getOffsets()[i]
            );
        }

        if (this.eboId != -1) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this.eboId);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices, GL_STATIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);
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

    private FloatBuffer convertToBuffer(Vertex[] vertices) {
        int capacity = vertices.length * vertices[0].getSize();

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
