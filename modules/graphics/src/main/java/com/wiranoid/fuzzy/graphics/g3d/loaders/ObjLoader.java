package com.wiranoid.fuzzy.graphics.g3d.loaders;

import com.wiranoid.fuzzy.graphics.Mesh;
import com.wiranoid.fuzzy.graphics.glutils.Vertex;
import org.joml.Vector2f;
import org.joml.Vector3f;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

public class ObjLoader {

    private final static String DELIMITER = "\\s+";

    public static Mesh loadMesh(String fileName) {
        List<Vector3f> positions = new ArrayList<>();
        List<Vector3f> normals = new ArrayList<>();
        List<Vector2f> uvs = new ArrayList<>();
        List<Face> faces = new ArrayList<>();

        parseFile(fileName, positions, normals, uvs, faces);

        Vector3f[] orderedNormals = new Vector3f[positions.size()];
        Vector2f[] orderedUVs = new Vector2f[positions.size()];
        List<Integer> indicesList = new ArrayList<>();

        boolean hasTextures = uvs.size() > 0;

        int index;
        for (Face face: faces) {
            for (Face.IndexGroup group: face.groups) {
                index = group.position;

                orderedNormals[index] = normals.get(group.normal);

                if (hasTextures) {
                    orderedUVs[index] = uvs.get(group.uv);
                }

                indicesList.add(index);
            }
        }

        Vertex[] vertices = new Vertex[positions.size()];
        if (hasTextures) {
            for (int i = 0; i < vertices.length; i++) {
                vertices[i] = new Vertex(
                        positions.get(i),
                        orderedNormals[i],
                        orderedUVs[i]
                );
            }
        } else {
            for (int i = 0; i < vertices.length; i++) {
                vertices[i] = new Vertex(
                        positions.get(i),
                        orderedNormals[i]
                );
            }
        }

        int[] indices = indicesList.stream().mapToInt(i -> i).toArray();

        return new Mesh(vertices, indices);
    }

    private static void parseFile(String fileName,
                                  List<Vector3f> positions,
                                  List<Vector3f> normals,
                                  List<Vector2f> uvs,
                                  List<Face> faces) {
        try (BufferedReader reader = new BufferedReader(new FileReader(fileName))) {
            String line;

            while ((line = reader.readLine()) != null) {
                if (line.startsWith("v ")) {
                    String[] tokens = line.split(DELIMITER);
                    positions.add(new Vector3f(
                            Float.parseFloat(tokens[1]),
                            Float.parseFloat(tokens[2]),
                            Float.parseFloat(tokens[3])
                    ));
                } else if (line.startsWith("vn ")) {
                    String[] tokens = line.split(DELIMITER);
                    normals.add(new Vector3f(
                            Float.parseFloat(tokens[1]),
                            Float.parseFloat(tokens[2]),
                            Float.parseFloat(tokens[3])
                    ));
                } else if (line.startsWith("vt ")) {
                    String[] tokens = line.split(DELIMITER);
                    uvs.add(new Vector2f(
                            Float.parseFloat(tokens[1]),
                            Float.parseFloat(tokens[2])
                    ));
                } else if (line.startsWith("f")) {
                    faces.add(new Face(line.split(DELIMITER)));
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
            private int uv = -1;
            private int normal = -1;

            private IndexGroup(int position, int normal) {
                this.position = position;
                this.normal = normal;
            }

            private IndexGroup(int position, int uv, int normal) {
                this.position = position;
                this.uv = uv;
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
