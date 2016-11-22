#pragma once

#include <GL/glew.h>
#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/material.h>
#include <vector>
#include <string>
#include "../glutils/ShaderProgram.h"
#include "Mesh.h"
#include "../glutils/TextureData.h"

namespace graphics {

    class Model {
    public:
        Model(std::string path);
        void Draw(ShaderProgram& shader);
    private:
        std::vector<Mesh> _meshes;
        std::string _directory;
        std::vector<TextureData> _loadedTextures;

        void loadModel(std::string path);
        void processNode(aiNode* node, const aiScene* scene);
        Mesh processMesh(aiMesh* mesh, const aiScene* scene);
        std::vector<TextureData> loadMaterialTextures(aiMaterial* material,
                                                      aiTextureType type,
                                                      std::string typeName);
        GLuint TextureFromFile(const char* path, std::string directory);
    };

}