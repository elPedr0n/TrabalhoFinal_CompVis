// FONTE: Biblioteca tinygltf para carregamento de modelos no formato glTF 2.0.
// Repositório: https://github.com/syoyo/tinygltf
// Licença: MIT License
#define TINYGLTF_IMPLEMENTATION

// FONTE: Biblioteca stb_image para carregamento de imagens/texturas (PNG, JPG, etc.).
// Repositório: https://github.com/nothings/stb (arquivo stb_image.h)
// Licença: MIT License / Public Domain
#define STB_IMAGE_IMPLEMENTATION

// FONTE: Biblioteca stb_image_write para escrita de imagens.
// Repositório: https://github.com/nothings/stb (arquivo stb_image_write.h)
// Licença: MIT License / Public Domain
#define STB_IMAGE_WRITE_IMPLEMENTATION

#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE
#include "stb_image.h"
#include "stb_image_write.h"
#include "tiny_gltf.h"