package com.wiranoid.fuzzy.graphics.glutils;

import java.util.Arrays;
import java.util.Iterator;
import java.util.Optional;

public class Vertex implements Iterable<VertexAttribute> {

    private VertexAttribute[] attributes;
    private int numberOfComponents = -1;

    public int getNumberOfComponents() {
        return this.numberOfComponents;
    }

    //max 16 attributes
    public Vertex(VertexAttribute... attributes) {
        this.attributes = attributes;
        this.numberOfComponents = countNumberOfComponents();
        calculateOffsets();
        setLocations();
    }

    public int sizeInBytes() {
        return Arrays.stream(this.attributes)
                .mapToInt(VertexAttribute::sizeInBytes)
                .sum();
    }

    public VertexAttribute findByUsage(VertexAttribute.Usage usage) {
        Optional<VertexAttribute> optional = Arrays.stream(this.attributes)
                .filter(attribute -> attribute.getUsage() == usage)
                .findFirst();

        return optional.isPresent() ?
                optional.get() :
                null;
    }

    public float[] getData() {
        float[] data = new float[this.numberOfComponents];

        int index = 0;
        for (VertexAttribute attribute : attributes) {
            for (int j = 0; j < attribute.size(); j++) {
                data[index++] = attribute.getData()[j];
            }
        }

        return data;
    }

    private int countNumberOfComponents() {
        return this.numberOfComponents = Arrays.stream(this.attributes)
                .mapToInt(VertexAttribute::size)
                .sum();
    }

    private void calculateOffsets() {
        int prevSizes = 0;

        for (VertexAttribute attribute : this.attributes) {
            attribute.setOffset(prevSizes);
            prevSizes += attribute.sizeInBytes();
        }
    }

    private void setLocations() {
        for (int i = 0; i < this.attributes.length; i++) {
            this.attributes[i].setLocation(i);
        }
    }

    @Override
    public Iterator<VertexAttribute> iterator() {
        return new Iterator<VertexAttribute>() {
            private int position;

            @Override
            public boolean hasNext() {
                return attributes.length > position;
            }

            @Override
            public VertexAttribute next() {
                return attributes[position++];
            }
        };
    }
}
