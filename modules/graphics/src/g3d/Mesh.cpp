#include "../../include/g3d/Mesh.h"
#include "../../include/glutils/ShaderProgram.h"
#include <GL/glew.h>
#include <vector>
#include <sstream>

using namespace graphics;

Mesh::Mesh(const std::vector<VertexData>& vertices,
           const std::vector<GLuint>& indices,
           const std::vector<TextureData>& textures) :
    vertices(vertices),
    indices(indices),
    textures(textures) {
    this->setupMesh();
}

void Mesh::Draw(ShaderProgram& shader) {
    GLuint diffuseNumber = 1;
    GLuint specularNumber = 1;

    for (GLuint i = 0; i < this->textures.size(); i++) {
        // Activate proper texture unit before binding
        glActiveTexture(GL_TEXTURE0 + i);
        // Retrieve texture number (the N in diffuse_textureN)
        std::string type = this->textures[i].type;
        std::string number = (type == "texture_diffuse") ?
                             std::to_string(diffuseNumber++) :
                             // texture_specular
                             std::to_string(specularNumber++);

        // Now set the sampler to the correct texture unit
        shader.setUniform(type + number, i);
        // And finally bind the texture
        glBindTexture(GL_TEXTURE_2D, this->textures[i].id);
    }

    glActiveTexture(GL_TEXTURE0);

    // Draw mesh
    glBindVertexArray(this->_VAO);
    glDrawElements(GL_TRIANGLES, this->indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

// Initializes all the buffer objects/arrays
void Mesh::setupMesh() {
    glGenVertexArrays(1, &this->_VAO);
    glGenBuffers(1, &this->_VBO);
    glGenBuffers(1, &this->_EBO);

    glBindVertexArray(this->_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, this->_VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 this->vertices.size() * sizeof(VertexData),
                 &this->vertices[0],
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 this->indices.size() * sizeof(GLuint),
                 &this->indices[0],
                 GL_STATIC_DRAW);

    // Vertex Positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (GLvoid*) 0);

    // Vertex Normals
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (GLvoid*) offsetof(VertexData, Normal));

    // Vertex Texture Coordinates
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), (GLvoid*) offsetof(VertexData, UV));

    glBindVertexArray(0);
}
