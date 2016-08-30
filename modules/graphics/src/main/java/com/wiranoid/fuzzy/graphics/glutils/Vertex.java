package com.wiranoid.fuzzy.graphics.glutils;

import org.joml.Vector2f;
import org.joml.Vector3f;

public class Vertex {

    private Vector3f position;
    private Vector3f normal;
    private Vector2f uv;

    private int numberOfAttributes;
    private int size;
    private int sizeInBytes;

    private float[] data;

    private int[] sizes;
    private int[] offsets;
    private int[] locations;

    public Vector3f getPosition() {
        return position;
    }

    public Vector3f getNormal() {
        return normal;
    }

    public Vector2f getUv() {
        return uv;
    }

    public int getNumberOfAttributes() {
        return numberOfAttributes;
    }

    public int getSize() {
        return size;
    }

    public int getSizeInBytes() {
        return sizeInBytes;
    }

    public float[] getData() {
        return data;
    }

    public int[] getSizes() {
        return sizes;
    }

    public int[] getOffsets() {
        return offsets;
    }

    public int[] getLocations() {
        return locations;
    }

    public Vertex(Vector3f position) {
        this.position = position;
        this.numberOfAttributes = 1;
        this.size = 3;
        this.sizeInBytes = this.size * Float.BYTES;
        this.data = new float[] { position.x, position.y, position.z };
        this.sizes = new int[] { 3 };
        this.offsets = new int[] { 0 };
        this.locations = new int[] { 0 };
    }

    public Vertex(Vector3f position, Vector3f normal) {
        this.position = position;
        this.normal = normal;
        this.numberOfAttributes = 2;
        this.size = 6;
        this.sizeInBytes = this.size * Float.BYTES;
        this.data = new float[] {
            position.x, position.y, position.z,
            normal.x, normal.y, normal.z
        };
        this.sizes = new int[] { 3, 3 };
        this.offsets = new int[] { 0, 3 * Float.BYTES };
        this.locations = new int[] { 0, 1 };
    }

    public Vertex(Vector3f position, Vector3f normal, Vector2f uv) {
        this.position = position;
        this.normal = normal;
        this.uv = uv;
        this.numberOfAttributes = 3;
        this.size = 8;
        this.sizeInBytes = this.size * Float.BYTES;
        this.data = new float[] {
            position.x, position.y, position.z,
            normal.x, normal.y, normal.z,
            uv.x, uv.y
        };
        this.sizes = new int[] { 3, 3, 2 };
        this.offsets = new int[] { 0, 3 * Float.BYTES, 6 * Float.BYTES };
        this.locations = new int[] { 0, 1, 2 };
    }
}
