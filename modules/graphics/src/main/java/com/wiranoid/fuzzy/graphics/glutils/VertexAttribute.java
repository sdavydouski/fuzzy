package com.wiranoid.fuzzy.graphics.glutils;

public class VertexAttribute {
    public enum Usage {
        Position, Normal, Color, TextureCoordinates
    }

    private VertexAttribute.Usage usage;
    private float[] data;
    private int offset = -1;
    private int location = -1;

    public Usage getUsage() {
        return usage;
    }

    public float[] getData() {
        return data;
    }

    public int getOffset() {
        return offset;
    }

    public void setOffset(int offset) {
        this.offset = offset;
    }

    public int getLocation() {
        return location;
    }

    public void setLocation(int location) {
        this.location = location;
    }

    public VertexAttribute(VertexAttribute.Usage usage, float... data) {
        this.usage = usage;

        switch (usage) {
            case Position:
            case Normal:
            case Color:
                if (data.length == 3) {
                    this.data = data;
                }
                break;
            case TextureCoordinates:
                if (data.length == 2) {
                    this.data = data;
                }
                break;
            default:
                throw new IllegalArgumentException("Unsupported vertex attribute type! " +
                        "Only position, normal, color and texture coordinates " +
                        "types are currently supported.");
        }

        if (this.data == null) {
            throw new IllegalArgumentException("Invalid number of items for " + usage + " type attribute!");
        }
    }

    public int size() {
        return this.data.length;
    }

    public int sizeInBytes() {
        return this.data.length * Float.BYTES;
    }
}
