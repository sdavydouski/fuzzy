package com.wiranoid.fuzzy.graphics.glutils;

import org.junit.Test;

import static org.junit.Assert.assertEquals;

public class VertexAttributeTest {

    private VertexAttribute attribute =
            new VertexAttribute(VertexAttribute.Usage.Position, 0.5f, -1.0f, 0.0f);

    @Test(expected = IllegalArgumentException.class)
    public void testConstructorWithPositionUsage() {
        new VertexAttribute(VertexAttribute.Usage.Position, 0.5f);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testConstructorWithNormalUsage() {
        new VertexAttribute(VertexAttribute.Usage.Normal, 0.5f, 3.4f);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testConstructorWithColorUsage() {
        new VertexAttribute(VertexAttribute.Usage.Color, 0.5f, 3.4f, 4.5f, 3.0f);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testConstructorWithTextureCoordinatesUsage() {
        new VertexAttribute(VertexAttribute.Usage.TextureCoordinates, 0.5f, 3.4f, 0.12f);
    }

    @Test
    public void testOffset() {
        assertEquals(-1, attribute.getOffset());
        attribute.setOffset(3);
        assertEquals(3, attribute.getOffset());
    }

    @Test
    public void testLocation() {
        assertEquals(-1, attribute.getLocation());
        attribute.setLocation(0);
        assertEquals(0, attribute.getLocation());
    }

    @Test
    public void testSizeInBytes() {
        assertEquals(3 * Float.BYTES, attribute.sizeInBytes());
    }
}
