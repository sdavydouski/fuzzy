package com.wiranoid.fuzzy.graphics;

import com.wiranoid.fuzzy.core.utils.Disposable;
import com.wiranoid.fuzzy.graphics.glutils.ShaderProgram;
import com.wiranoid.fuzzy.graphics.glutils.Vertex;
import com.wiranoid.fuzzy.graphics.glutils.VertexAttribute;
import org.lwjgl.BufferUtils;

import java.nio.FloatBuffer;
import java.util.List;

import static org.lwjgl.opengl.GL11.GL_FLOAT;
import static org.lwjgl.opengl.GL11.GL_TRIANGLES;
import static org.lwjgl.opengl.GL11.glDrawArrays;
import static org.lwjgl.opengl.GL15.*;
import static org.lwjgl.opengl.GL30.*;

public class Mesh implements Disposable {
    private List<Vertex> vertices;

    private final int vaoId;
    private final int vboId;

    public Mesh(List<Vertex> vertices, ShaderProgram shaderProgram) {
        this.vertices = vertices;

        this.vaoId = glGenVertexArrays();
        glBindVertexArray(vaoId);

        this.vboId = glGenBuffers();
        glBindBuffer(GL_ARRAY_BUFFER, this.vboId);
        glBufferData(GL_ARRAY_BUFFER, convertToBuffer(vertices), GL_STATIC_DRAW);

        Vertex vertex = vertices.get(0);

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

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    public void render() {
        glBindVertexArray(this.vaoId);
        glDrawArrays(GL_TRIANGLES, 0, this.vertices.size());
        glBindVertexArray(0);
    }

    public void dispose() {
        glDeleteVertexArrays(this.vaoId);
        glDeleteBuffers(this.vboId);
    }

    private FloatBuffer convertToBuffer(List<Vertex> vertices) {
        int capacity = vertices.size() * vertices.get(0).getNumberOfComponents();

        FloatBuffer buffer = BufferUtils.createFloatBuffer(capacity);
        for (Vertex vertex: vertices) {
            buffer.put(vertex.getData());
        }

        buffer.flip();

        return buffer;
    }
}
