#version 330 core

// Atributos de fragmentos recebidos como entrada ("in") pelo Fragment Shader.
// Neste exemplo, este atributo foi gerado pelo rasterizador como a
// interpolação da posição global e a normal de cada vértice, definidas em
// "shader_vertex.glsl" e "main.cpp".
in vec4 position_world;
in vec4 normal;

// Posição do vértice atual no sistema de coordenadas local do modelo.
in vec4 position_model;

// Coordenadas de textura obtidas do arquivo OBJ (se existirem!)
in vec2 texcoords;
in float material_id;
in vec3 vert_color;

// Matrizes computadas no código C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Identificador que define qual objeto está sendo desenhado no momento
#define SPHERE 0
#define BUNNY  1
#define PLANE  2
#define CHILL  3
#define SWAMPFIRE 4
#define BLOCO 5
uniform int object_id;
#define GROUND 13

// Parâmetros da axis-aligned bounding box (AABB) do modelo
uniform vec4 aabb_min;
uniform vec4 aabb_max;

// Variáveis para acesso das imagens de textura
uniform sampler2D TextureImage0;
uniform sampler2D TextureImage1;
uniform sampler2D TextureImage2;
uniform sampler2D TextureImage3;
uniform sampler2D TextureImage4;
uniform sampler2D TextureImage5;
uniform sampler2D TextureImage6;
uniform sampler2D TextureImage7;
uniform sampler2D TextureImage8;
uniform float hud_health_ratio;
uniform sampler2D TextureImage9;
uniform sampler2D TextureImage10;
uniform sampler2D TextureImage11;
uniform sampler2D TextureImage12;
uniform sampler2D TextureImage14;
    // Optional override color for procedural particles
    uniform vec3 OverrideKd;
    uniform int UseOverrideKd;
uniform float current_time;
uniform int is_frozen;

// O valor de saída ("out") de um Fragment Shader é a cor final do fragmento.
out vec4 color;

// Constantes
#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923

void main()
{
    // Obtemos a posição da câmera utilizando a inversa da matriz que define o
    // sistema de coordenadas da câmera.
    vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 camera_position = inverse(view) * origin;

    // O fragmento atual é coberto por um ponto que percente à superfície de um
    // dos objetos virtuais da cena. Este ponto, p, possui uma posição no
    // sistema de coordenadas global (World coordinates). Esta posição é obtida
    // através da interpolação, feita pelo rasterizador, da posição de cada
    // vértice.
    vec4 p = position_world;

    // Normal do fragmento atual, interpolada pelo rasterizador a partir das
    // normais de cada vértice.
    vec4 n = normalize(normal);

    // Vetor que define o sentido da fonte de luz em relação ao ponto atual.
    vec4 l = normalize(vec4(1.0,1.0,0.0,0.0));

    // Vetor que define o sentido da câmera em relação ao ponto atual.
    vec4 v = normalize(camera_position - p);

    // Coordenadas de textura U e V
    float U = 0.0;
    float V = 0.0;

	// Coeficiente de refletância difusa
	vec3 Kd0 = vec3(1.0, 0.0, 1.0);

    if ( object_id == SPHERE )
    {
        // PREENCHA AQUI as coordenadas de textura da esfera, computadas com
        // projeção esférica EM COORDENADAS DO MODELO. Utilize como referência
        // o slides 134-150 do documento Aula_20_Mapeamento_de_Texturas.pdf.
        // A esfera que define a projeção deve estar centrada na posição
        // "aabb_center" definida abaixo.

        // Você deve utilizar:
        //   função 'length( )' : comprimento Euclidiano de um vetor
        //   função 'atan( , )' : arcotangente. Veja https://en.wikipedia.org/wiki/Atan2.
        //   função 'asin( )'   : seno inverso.
        //   constante M_PI
        //   variável position_model

        vec4 aabb_center = (aabb_min + aabb_max) / 2.0;
        vec4 d = position_model - aabb_center;

        float rho   = length(d);
        float theta = atan(d.x,d.z);
        float phi   = asin(d.y / rho);

        U = (theta + M_PI) / 2.0 / M_PI;
        V = (phi + M_PI_2) / M_PI;

		// Obtemos a refletância difusa a partir da leitura da imagem TextureImage0
		Kd0 = texture(TextureImage0, vec2(U,V)).rgb;
    }
    else if ( object_id == BUNNY )
    {
        // PREENCHA AQUI as coordenadas de textura do coelho, computadas com
        // projeção planar XY em COORDENADAS DO MODELO. Utilize como referência
        // o slides 99-104 do documento Aula_20_Mapeamento_de_Texturas.pdf,
        // e também use as variáveis min*/max* definidas abaixo para normalizar
        // as coordenadas de textura U e V dentro do intervalo [0,1]. Para
        // tanto, veja por exemplo o mapeamento da variável 'p_v' utilizando
        // 'h' no slides 158-160 do documento Aula_20_Mapeamento_de_Texturas.pdf.
        // Veja também a Questão 4 do Questionário 4 no Moodle.

        float minx = aabb_min.x;
        float maxx = aabb_max.x;

        float miny = aabb_min.y;
        float maxy = aabb_max.y;

        float minz = aabb_min.z;
        float maxz = aabb_max.z;

        U = (position_model.x - minx) / (maxx - minx);
        V = (position_model.y - miny) / (maxy - miny);

		// Obtemos a refletância difusa a partir da leitura da imagem TextureImage0
		Kd0 = texture(TextureImage0, vec2(U,V)).rgb;
    }
    else if ( object_id == PLANE )
    {
        // Coordenadas de textura do plano, obtidas do arquivo OBJ.
        U = texcoords.x * 20;
        V = texcoords.y * 20;

		// Obtemos a refletância difusa a partir da leitura da imagem TextureImage1
		Kd0 = texture(TextureImage1, vec2(U,V)).rgb;
    }
    else if ( object_id == CHILL )
    {
        U = texcoords.x;
        V = texcoords.y;
        Kd0 = texture(TextureImage2, vec2(U,V)).rgb;
    }
    else if ( object_id == 15 ) // UAF_CHILL
    {
        U = texcoords.x;
        V = 1.0 - texcoords.y;  // Flip V for UAF Sketchfab model
        Kd0 = texture(TextureImage2, vec2(U,V)).rgb;
    }
    else if ( object_id == SWAMPFIRE )
    {
        U = texcoords.x;
        V = 1.0 - texcoords.y;  // flip V for glTF
        Kd0 = texture(TextureImage5, vec2(U,V)).rgb;
    }
    else if ( object_id == 8 ) // BENTENNYSON
    {
        U = texcoords.x;
        V = 1.0 - texcoords.y;  // flip V for glTF
        Kd0 = texture(TextureImage6, vec2(U,V)).rgb;

    }
    else if ( object_id == 9 ) // FOREVERKNIGHT
    {
        U = texcoords.x;
        V = 1.0 - texcoords.y;  // flip V for glTF
        Kd0 = texture(TextureImage7, vec2(U,V)).rgb;
    }
    else if ( object_id == 11 ) // CASTLE
    {
        U = texcoords.x;
        V = 1.0 - texcoords.y;  // flip V for glTF
        Kd0 = texture(TextureImage8, vec2(U,V)).rgb;
    } else if ( object_id == 6 ) // FIREBALL
    {
        // Solid yellow/orange for the fireball and its particles (emissive)
        Kd0 = vec3(1.0, 0.75, 0.12) * 2.0;
    } else if ( object_id == 7 ) // GREEN TRANSFORM PARTICLES
    {
        // Bright green emissive for transform particles
        Kd0 = vec3(0.25, 1.0, 0.35) * 1.8;
    } else if (object_id == BLOCO) 
    {
        U = texcoords.x;
        V = texcoords.y;
        Kd0 = texture(TextureImage4, vec2(U,V)).rgb; 
    } else if ( object_id == 10 ) // COLLECT_OBJ
    {
        float blink = (sin(current_time * 15.0) + 1.0) * 0.5;
        Kd0 = mix(vec3(1.0, 0.0, 0.0), vec3(2.0, 2.0, 2.0), blink);
    } else if ( object_id == 13 ) // HUD_BAR_BG
    {
        float bg_u_min = 0.047;
        float bg_u_max = 0.275;
        float v_min = 0.025;
        float v_max = 0.96;
        
        U = texcoords.x * (bg_u_max - bg_u_min) + bg_u_min;
        V = texcoords.y * (v_max - v_min) + v_min;
        vec4 tex_color = texture(TextureImage8, vec2(U,V));
        if (tex_color.a < 0.5) discard;
        Kd0 = tex_color.rgb;
    } else if ( object_id == 14 ) // HUD_BAR_FG
    {
        float fg_u_min = 0.342;
        float fg_u_max = 0.57;
        float v_min = 0.025;
        float v_max = 0.96;

        float scaled_y = texcoords.y * hud_health_ratio;
        U = texcoords.x * (fg_u_max - fg_u_min) + fg_u_min;
        V = scaled_y * (v_max - v_min) + v_min;
        vec4 tex_color = texture(TextureImage8, vec2(U,V));
        if (tex_color.a < 0.5) discard;
        Kd0 = tex_color.rgb;
    } else if (object_id == GROUND) {
        U = texcoords.x;
        V = texcoords.y;
        
        if (material_id > 18.5) {
            Kd0 = vec3(0.9, 0.9, 0.9); // 19.0 - White
        } else if (material_id > 17.5) {
            Kd0 = vec3(0.1, 0.1, 0.1); // 18.0 - Dark/Black
        } else if (material_id > 16.5) {
            Kd0 = vec3(0.9, 0.6, 0.03); // 17.0 - Orange
        } else if (material_id > 15.5) {
            Kd0 = vec3(1.0, 0.1, 0.1); // 16.0 - Red
        } else if (material_id > 14.5) {
            Kd0 = vec3(0.1, 0.2, 0.9); // 15.0 - Blue
        } else if (material_id > 13.5) {
            Kd0 = texture(TextureImage14, vec2(U,V)).rgb;
        } else if (material_id > 11.5) {
            Kd0 = texture(TextureImage12, vec2(U,V)).rgb;
        } else if (material_id > 10.5) {
            Kd0 = texture(TextureImage11, vec2(U,V)).rgb;
        } else if (material_id > 9.5) {
            Kd0 = texture(TextureImage10, vec2(U,V)).rgb;
        } else {
            Kd0 = texture(TextureImage9, vec2(U,V)).rgb;
        }                                                                                                                                   

    }
    
        // Override color when requested (per-particle color via uniform)
        if (UseOverrideKd == 1) {
            Kd0 = OverrideKd;
        }

    // Debug axes rendering: when object_id==100, use per-vertex color directly
    if (object_id == 100) {
        color = vec4(vert_color, 1.0);
        return;
    }

    // Debug bounding boxes rendering: when object_id==101, draw solid yellow lines
    if (object_id == 101) {
        color = vec4(1.0, 1.0, 0.0, 1.0);
        return;
    }

    if (object_id == 13 || object_id == 14 || UseOverrideKd == 1) {
        color.rgb = Kd0;
    } else {
        // Equação de Iluminação
        float lambert = max(0,dot(n,l));
        color.rgb = Kd0 * (lambert + 0.01);
        
        // Ice Breath Freeze Tint
        if (is_frozen == 1) {
            color.rgb = mix(color.rgb, vec3(0.4, 0.8, 1.0), 0.25);
        }
    }

    // NOTE: Se você quiser fazer o rendering de objetos transparentes, é
    // necessário:
    // 1) Habilitar a operação de "blending" de OpenGL logo antes de realizar o
    //    desenho dos objetos transparentes, com os comandos abaixo no código C++:
    //      glEnable(GL_BLEND);
    //      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // 2) Realizar o desenho de todos objetos transparentes *após* ter desenhado
    //    todos os objetos opacos; e
    // 3) Realizar o desenho de objetos transparentes ordenados de acordo com
    //    suas distâncias para a câmera (desenhando primeiro objetos
    //    transparentes que estão mais longe da câmera).
    // Use lower alpha for additive particles so blending uses source alpha
    if (object_id == 6) {
        // Procedural soft radial gradient for particles (no solid circle)
        float edge_fade = max(0.0, dot(n, v));
        color.a = 0.85 * pow(edge_fade, 1.5); // Soft falloff
    } else if (object_id == 10) {
        color.a = 0.3; // Mais transparente
    } else {
        color.a = 1.0;
    }

    // Cor final com correção gamma, considerando monitor sRGB.
    // Veja https://en.wikipedia.org/w/index.php?title=Gamma_correction&oldid=751281772#Windows.2C_Mac.2C_sRGB_and_TV.2Fvideo_standard_gammas
    color.rgb = pow(color.rgb, vec3(1.0,1.0,1.0)/2.2);
} 
