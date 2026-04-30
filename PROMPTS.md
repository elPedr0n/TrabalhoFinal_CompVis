### Commit com auxílio de IA: Carregamento de um dos modelos dos alienígenas para testes (5fe8cafcf566dbf1ee734b49811dcac3ad6e757d)

> Esse primeiro commit com IA acabei esquecendo de colocar o prompt, segue o código editado pela IA assim como seu prompt.

"Na pasta de data coloquei um novo modelo de um dos alienigenas do jogo com 2 texturas. Quero que você modifique a main e os arquivos .glsl para poder renderizar de forma correta esse novo modelo."

na `main.cpp`
```c
56 std::map<std::string, std::vector<float>> face_texture_selector_by_shape;
...
111 void ParseFaceTextureSelectorsFromObj(const char* filename)
    {
        std::ifstream file(filename);
        if (!file.good())
            return;

        std::string line;
        std::string current_shape;
        float current_selector = 2.0f; // bcck1.png => TextureImage2

        while (std::getline(file, line))
        {
            if (line.rfind("g ", 0) == 0 || line.rfind("o ", 0) == 0)
            {
                current_shape = line.substr(2);
                continue;
            }

            if (line.rfind("usemtl ", 0) == 0)
            {
                std::string material_name = line.substr(7);
                if (material_name.find("bcck2") != std::string::npos)
                    current_selector = 3.0f; // bcck2.png => TextureImage3
                else if (material_name.find("bcck1") != std::string::npos)
                    current_selector = 2.0f;
                continue;
            }

            if (line.rfind("f ", 0) == 0 && !current_shape.empty())
            {
                face_texture_selector_by_shape[current_shape].push_back(current_selector);
            }
        }
144    }
...
341 LoadTextureImage("../../data/bcck1.png"); // TextureImage2
342 LoadTextureImage("../../data/bcck2.png"); // TextureImage3
...
357 ObjModel bigchillmodel("../../data/big_chill_cloaked.obj")
358 ComputeNormals(&bigchillmodel);
359 BuildTrianglesAndAddToVirtualScene(&bigchillmodel);
...
828 int material_id = -1;
    if (triangle < model->shapes[shape].mesh.material_ids.size())
    {
        material_id = model->shapes[shape].mesh.material_ids[triangle];
    }

    float texture_selector = 2.0f; // bcck1.png => TextureImage2
    auto face_selector_it = model->face_texture_selector_by_shape.find(model->shapes[shape].name);
    if (face_selector_it != model->face_texture_selector_by_shape.end() &&
        triangle < face_selector_it->second.size())
    {
        texture_selector = face_selector_it->second[triangle];
    }
    else if (material_id >= 0 && material_id < (int)model->materials.size())
    {
        const auto& material = model->materials[material_id];
        const bool uses_bcck2 =
            material.name.find("bcck2") != std::string::npos ||
            material.diffuse_texname.find("bcck2") != std::string::npos;
        const bool uses_bcck1 =
            material.name.find("bcck1") != std::string::npos ||
            material.diffuse_texname.find("bcck1") != std::string::npos;

        if (uses_bcck2)
        {
            texture_selector = 3.0f;
        }
        else if (!uses_bcck1)
        {
            auto it = fallback_texture_unit_by_material_id.find(material_id);
            if (it == fallback_texture_unit_by_material_id.end())
            {
                fallback_texture_unit_by_material_id[material_id] = next_fallback_texture_unit;
                texture_selector = next_fallback_texture_unit;
                next_fallback_texture_unit = (next_fallback_texture_unit == 2.0f) ? 3.0f : 2.0f;
            }
            else
            {
                texture_selector = it->second;
            }
        }
    }
    else if (material_id >= 0)
    {
        auto it = fallback_texture_unit_by_material_id.find(material_id);
        if (it == fallback_texture_unit_by_material_id.end())
        {
            fallback_texture_unit_by_material_id[material_id] = next_fallback_texture_unit;
            texture_selector = next_fallback_texture_unit;
            next_fallback_texture_unit = (next_fallback_texture_unit == 2.0f) ? 3.0f : 2.0f;
        }
        else
        {
            texture_selector = it->second;
        }
883 }
...
931 texture_selector_coefficients.push_back(texture_selector);
...
989 if ( !texture_selector_coefficients.empty() )
    {
        GLuint VBO_texture_selector_coefficients_id;
        glGenBuffers(1, &VBO_texture_selector_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_selector_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_selector_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_selector_coefficients.size() * sizeof(float), texture_selector_coefficients.data());
        location = 3; // "(location = 3)" em "shader_vertex.glsl"
        number_of_dimensions = 1; // float em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
1001 }
```

no `shader_fragment.glsl`
```
15 in float material_id;
...
26 #define CHILL  3
...
37 uniform sampler2D TextureImage3;
...
75 vec3 Kd0 = vec3(1.0, 0.0, 1.0);
...
140 else if ( object_id == CHILL )
    {
        // Coordenadas de textura do Big Chill, obtidas do arquivo OBJ.
        U = texcoords.x;
        V = texcoords.y;

        // "material_id" aqui recebe diretamente a unidade de textura (2 ou 3).
        Kd0 = (material_id > 2.5)
            ? texture(TextureImage3, vec2(U,V)).rgb
            : texture(TextureImage2, vec2(U,V)).rgb;
150 }
```
no `shader_vertex.glsl`
```
8 layout (location = 3) in float material_coefficients;
...
23 out float material_id;
...
68 material_id = material_coefficients;
```