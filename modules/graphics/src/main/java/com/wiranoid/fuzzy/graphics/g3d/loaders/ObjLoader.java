package com.wiranoid.fuzzy.graphics.g3d.loaders;

import com.wiranoid.fuzzy.graphics.Mesh;
import com.wiranoid.fuzzy.graphics.glutils.Vertex;
import com.wiranoid.fuzzy.graphics.glutils.VertexAttribute;
import org.joml.Vector2f;
import org.joml.Vector3f;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

//todo: mtl loader
public class ObjLoader {

    private final static String DELIMITER = "\\s+";

    public static Mesh loadMesh(String fileName) {
        List<Vector3f> positionsList = new ArrayList<>();
        List<Vector2f> textureCoordinatesList = new ArrayList<>();
        List<Vector3f> normalsList = new ArrayList<>();
        List<Face> facesList = new ArrayList<>();

        parseFile(fileName, positionsList, textureCoordinatesList, normalsList, facesList);

        float[] positions = new float[positionsList.size() * 3];
        for (int i = 0; i < positionsList.size(); i++) {
            positions[i * 3] = positionsList.get(i).x;
            positions[i * 3 + 1] = positionsList.get(i).y;
            positions[i * 3 + 2] = positionsList.get(i).z;
        }

        boolean hasTextures = textureCoordinatesList.size() > 0;

        float[] textureCoordinates = new float[positionsList.size() * 2];
        float[] normals = new float[positionsList.size() * 3];
        List<Integer> indicesList = new ArrayList<>();

        int index;
        for (Face face: facesList) {
            for (Face.IndexGroup group: face.groups) {
                index = group.position;

                indicesList.add(index);

                if (hasTextures) {
                    textureCoordinates[index * 2] = textureCoordinatesList.get(group.textureCoordinate).x;
                    textureCoordinates[index * 2 + 1] = 1 - textureCoordinatesList.get(group.textureCoordinate).y;
                }

                normals[index * 3] = normalsList.get(group.normal).x;
                normals[index * 3 + 1] = normalsList.get(group.normal).y;
                normals[index * 3 + 2] = normalsList.get(group.normal).z;
            }
        }

        int[] indices = indicesList.stream().mapToInt(i -> i).toArray();

        Vertex[] vertices = new Vertex[positionsList.size()];
        if (hasTextures) {
            for (int i = 0; i < vertices.length; i++) {
                vertices[i] = new Vertex(
                        new VertexAttribute(VertexAttribute.Usage.Position,
                                positions[i * 3], positions[i * 3 + 1], positions[i * 3 + 2]),
                        new VertexAttribute(VertexAttribute.Usage.TextureCoordinates,
                                textureCoordinates[i * 2], textureCoordinates[i * 2 + 1]),
                        new VertexAttribute(VertexAttribute.Usage.Normal,
                                normals[i * 3], normals[i * 3 + 1], normals[i * 3 + 2])
                );
            }
        } else {
            for (int i = 0; i < vertices.length; i++) {
                vertices[i] = new Vertex(
                        new VertexAttribute(VertexAttribute.Usage.Position,
                                positions[i * 3], positions[i * 3 + 1], positions[i * 3 + 2]),
                        new VertexAttribute(VertexAttribute.Usage.Normal,
                                normals[i * 3], normals[i * 3 + 1], normals[i * 3 + 2])
                );
            }
        }

        return new Mesh(vertices, indices);
    }

    private static void parseFile(String fileName,
                                  List<Vector3f> positionsList,
                                  List<Vector2f> textureCoordinatesList,
                                  List<Vector3f> normalsList,
                                  List<Face> facesList) {
        try (BufferedReader reader = new BufferedReader(new FileReader(fileName))) {
            String line;

            while ((line = reader.readLine()) != null) {
                if (line.startsWith("v ")) {
                    String[] tokens = line.split(DELIMITER);
                    positionsList.add(new Vector3f(
                            Float.parseFloat(tokens[1]),
                            Float.parseFloat(tokens[2]),
                            Float.parseFloat(tokens[3])
                    ));
                } else if (line.startsWith("vt ")) {
                    String[] tokens = line.split(DELIMITER);
                    textureCoordinatesList.add(new Vector2f(
                            Float.parseFloat(tokens[1]),
                            Float.parseFloat(tokens[2])
                    ));
                } else if (line.startsWith("vn ")) {
                    String[] tokens = line.split(DELIMITER);
                    normalsList.add(new Vector3f(
                            Float.parseFloat(tokens[1]),
                            Float.parseFloat(tokens[2]),
                            Float.parseFloat(tokens[3])
                    ));
                } else if (line.startsWith("f")) {
                    facesList.add(new Face(line.split(DELIMITER)));
                }
            }
        }
        catch(IOException ex) {
            throw new RuntimeException("Failed to load a model file!"
                    + System.lineSeparator() + ex.getMessage());
        }
    }

    private static class Face {
        private class IndexGroup {
            private int position = -1;
            private int textureCoordinate = -1;
            private int normal = -1;

            private IndexGroup(int position, int textureCoordinate, int normal) {
                this.position = position;
                this.textureCoordinate = textureCoordinate;
                this.normal = normal;
            }

            private IndexGroup(int position, int normal) {
                this.position = position;
                this.normal = normal;
            }
        }

        private IndexGroup[] groups;

        private Face(String[] tokens) {
            this.groups = new IndexGroup[3];
            for (int i = 1; i < tokens.length; i++) {
                String[] indices = tokens[i].split("/");

                if (indices[1].isEmpty()) {
                    this.groups[i - 1] = new IndexGroup(
                            Integer.parseInt(indices[0]) - 1,
                            Integer.parseInt(indices[2]) - 1
                    );
                } else {
                    this.groups[i - 1] = new IndexGroup(
                            Integer.parseInt(indices[0]) - 1,
                            Integer.parseInt(indices[1]) - 1,
                            Integer.parseInt(indices[2]) - 1
                    );
                }

            }
        }

    }

}
