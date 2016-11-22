#pragma once

#include <GL/glew.h>
#include <vector>
#include "../glutils/VertexData.h"
#include "../glutils/TextureData.h"
#include "../glutils/ShaderProgram.h"

namespace graphics {

    class Mesh {
    public:
        /*  Mesh Data  */
        std::vector<VertexData> vertices;
        std::vector<GLuint> indices;
        std::vector<TextureData> textures;

        Mesh(const std::vector<VertexData>& vertices,
             const std::vector<GLuint>& indices,
             const std::vector<TextureData>& textures);
        void Draw(ShaderProgram& shader);
    private:
        GLuint _VAO;
        GLuint _VBO;
        GLuint _EBO;

        void setupMesh();
    };

}
