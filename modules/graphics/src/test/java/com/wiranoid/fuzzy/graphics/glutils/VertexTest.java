package com.wiranoid.fuzzy.graphics.glutils;

import org.junit.Test;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;

public class VertexTest {

    private Vertex vertex = new Vertex(
            new VertexAttribute(VertexAttribute.Usage.Position, 1.0f, -1.0f, 0.5f),
            new VertexAttribute(VertexAttribute.Usage.Normal, 1.0f, -1.0f, 0.5f),
            new VertexAttribute(VertexAttribute.Usage.TextureCoordinates, 1.0f, -1.0f)
    );

    @Test
    public void testData() {
        float[] expected = { 1.0f, -1.0f, 0.5f, 1.0f, -1.0f, 0.5f, 1.0f, -1.0f };
        assertArrayEquals(expected, vertex.getData(), 0.00000001f);
    }

    @Test
    public void testNumberOfComponents() {
        assertEquals(8, vertex.getNumberOfComponents());
    }

    @Test
    public void testSizeInBytes() {
        assertEquals(8 * Float.BYTES, vertex.sizeInBytes());
    }

    @Test
    public void testOffsets() {
        VertexAttribute position = vertex.findByUsage(VertexAttribute.Usage.Position);
        assertEquals(0, position.getOffset());

        VertexAttribute normal = vertex.findByUsage(VertexAttribute.Usage.Normal);
        assertEquals(3 * Float.BYTES, normal.getOffset());

        VertexAttribute textureCoordinates = vertex.findByUsage(VertexAttribute.Usage.TextureCoordinates);
        assertEquals(6 * Float.BYTES, textureCoordinates.getOffset());
    }

    @Test
    public void testLocations() {
        VertexAttribute position = vertex.findByUsage(VertexAttribute.Usage.Position);
        assertEquals(0, position.getLocation());

        VertexAttribute normal = vertex.findByUsage(VertexAttribute.Usage.Normal);
        assertEquals(1, normal.getLocation());

        VertexAttribute textureCoordinates = vertex.findByUsage(VertexAttribute.Usage.TextureCoordinates);
        assertEquals(2, textureCoordinates.getLocation());
    }
}
