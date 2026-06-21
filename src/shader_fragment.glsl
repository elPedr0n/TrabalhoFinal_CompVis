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
uniform float hud_bar2_ratio;
uniform float hud_bar3_ratio;
uniform sampler2D TextureImage9;
uniform sampler2D TextureImage10;
uniform sampler2D TextureImage11;
uniform sampler2D TextureImage12;
uniform sampler2D TextureImage13;
uniform sampler2D TextureImage14;
uniform sampler2D TextureImage15;
uniform sampler2D TextureImage16;
uniform sampler2D TextureImage17;
uniform sampler2D TextureImage18;
uniform sampler2D TextureImage19;
uniform sampler2D TextureImage20;
uniform sampler2D TextureImage21;
uniform sampler2D TextureImage22;
uniform sampler2D TextureImage23;
uniform sampler2D TextureImage24;
uniform sampler2D TextureImage25;
uniform sampler2D TextureImage26;
uniform sampler2D TextureImage27;
uniform float hud_omnitrix_frame;
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
    } else if ( object_id == 20 ) // HUD_BAR_BG
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
    } else if ( object_id == 21 ) // HUD_BAR_FG
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
    } else if ( object_id == 22 ) // HUD_BAR_GREEN
    {
        float fg_u_min = 0.885;
        float fg_u_max = 0.967;
        float v_min = 0.456;
        float v_max = 0.824;

        float scaled_y = texcoords.y * hud_bar2_ratio;
        U = texcoords.x * (fg_u_max - fg_u_min) + fg_u_min;
        V = scaled_y * (v_max - v_min) + v_min;
        vec4 tex_color = texture(TextureImage8, vec2(U,V));
        if (tex_color.a < 0.5) discard;
        Kd0 = tex_color.rgb;
    } else if ( object_id == 23 ) // HUD_BAR_YELLOW
    {
        float fg_u_min = 0.885;
        float fg_u_max = 0.967;
        float v_min = 0.055;
        float v_max = 0.418;

        float scaled_y = texcoords.y * hud_bar3_ratio;
        U = texcoords.x * (fg_u_max - fg_u_min) + fg_u_min;
        V = scaled_y * (v_max - v_min) + v_min;
        vec4 tex_color = texture(TextureImage8, vec2(U,V));
        if (tex_color.a < 0.5) discard;
        Kd0 = tex_color.rgb;
    } else if ( object_id == 24 ) // HUD_BAR_CONTAINER2
    {
        float fg_u_min = 0.688;
        float fg_u_max = 0.795;
        float v_min = 0.071;
        float v_max = 0.819;

        U = texcoords.x * (fg_u_max - fg_u_min) + fg_u_min;
        V = texcoords.y * (v_max - v_min) + v_min;
        vec4 tex_color = texture(TextureImage8, vec2(U,V));
        if (tex_color.a < 0.5) discard;
        Kd0 = tex_color.rgb;
    } else if ( object_id == 25 ) // HUD_BAR_CAP
    {
        float fg_u_min = 0.672;
        float fg_u_max = 0.811;
        float v_min = 0.851;
        float v_max = 0.972;

        U = texcoords.x * (fg_u_max - fg_u_min) + fg_u_min;
        V = texcoords.y * (v_max - v_min) + v_min;
        vec4 tex_color = texture(TextureImage8, vec2(U,V));
        if (tex_color.a < 0.5) discard;
        Kd0 = tex_color.rgb;
    } else if ( object_id == 26 ) // HUD_OMNITRIX
    {
        int cols = 4;
        int rows = 4;
        int frame = int(hud_omnitrix_frame);
        int frame_x = frame % cols;
        int frame_y = frame / cols;
        
        U = (texcoords.x + float(frame_x)) / float(cols);
        V = (texcoords.y + float(3 - frame_y)) / float(rows);
        
        vec4 tex_color = texture(TextureImage13, vec2(U,V));
        if (tex_color.a < 0.5) discard;
        Kd0 = tex_color.rgb;
    } else if ( object_id == 27 ) // HUD_HOLOGRAM_LIGHT
    {
        U = texcoords.x;
        V = texcoords.y;
        vec4 tex_color = texture(TextureImage17, vec2(U,V));
        if (tex_color.a < 0.05) discard;
        Kd0 = tex_color.rgb;
    } else if ( object_id == 28 ) // HUD_HOLOGRAM_BIGCHILL
    {
        U = texcoords.x;
        V = texcoords.y;
        vec4 tex_color = texture(TextureImage18, vec2(U,V));
        if (tex_color.a < 0.05) discard;
        Kd0 = tex_color.rgb;
    } else if ( object_id == 29 ) // HUD_HOLOGRAM_SWAMPFIRE
    {
        U = texcoords.x;
        V = texcoords.y;
        vec4 tex_color = texture(TextureImage19, vec2(U,V));
        if (tex_color.a < 0.05) discard;
        Kd0 = tex_color.rgb;
    } else if ( object_id == 30 ) // TITLE_SCREEN
    {
        U = texcoords.x;
        V = texcoords.y; // removed 1.0 - texcoords.y
        Kd0 = texture(TextureImage15, vec2(U,V)).rgb;
    } else if ( object_id == 31 ) // SAVE_ICON (3D)
    {
        U = texcoords.x;
        V = texcoords.y;
        vec4 tex_color = texture(TextureImage16, vec2(U,V));
        if (tex_color.a < 0.5) discard;
        Kd0 = tex_color.rgb;
    } else if ( object_id == 40 ) // UI_WINDOW_BG
    {
        Kd0 = vec3(0.05, 0.15, 0.05); // Dark Green
    } else if ( object_id == 41 ) // UI_WINDOW_BORDER
    {
        Kd0 = vec3(0.6, 1.0, 0.6); // Light Green
    } else if (object_id == 50) { // BARRACA
        U = texcoords.x;
        V = 1.0 - texcoords.y; // Flip V to fix upside down posters
        
        if (material_id > 48.5) {
            Kd0 = vec3(0.030713, 0.208633, 1.000000); // 049
        } else if (material_id > 47.5) {
            Kd0 = vec3(1.000000, 0.799107, 0.099897); // 048
        } else if (material_id > 46.5) {
            Kd0 = vec3(1.000000, 0.041879, 0.068113); // 047
        } else if (material_id > 44.5) {
            Kd0 = vec3(1.0, 1.0, 1.0); // 045, 046
        } else if (material_id > 43.5) {
            Kd0 = vec3(0.156194, 0.389759, 0.800000); // 044
        } else if (material_id > 42.5) {
            Kd0 = vec3(1.000000, 0.799107, 0.099897); // 043
        } else if (material_id > 39.5) {
            Kd0 = vec3(0.074214, 0.417885, 0.630757); // 040
        } else if (material_id > 38.5) {
            Kd0 = vec3(0.8, 0.8, 0.8); // 039
        } else if (material_id > 37.5) {
            Kd0 = vec3(0.800000, 0.142354, 0.028348); // 038
        } else if (material_id > 36.5) {
            Kd0 = vec3(0.8, 0.8, 0.8); // 037
        } else if (material_id > 35.5) {
            Kd0 = vec3(0.101023, 0.657837, 0.800000); // 036
        } else if (material_id > 34.5) {
            Kd0 = vec3(1.000000, 0.041879, 0.068113); // 035
        } else if (material_id > 30.5) {
            Kd0 = vec3(0.8, 0.8, 0.8); // 031
        } else if (material_id > 23.5) {
            Kd0 = texture(TextureImage24, vec2(U,V)).rgb; // 24 Roof
        } else if (material_id > 22.5) {
            Kd0 = texture(TextureImage23, vec2(U,V)).rgb; // 23
        } else if (material_id > 21.5) {
            Kd0 = texture(TextureImage22, vec2(U,V)).rgb; // 22
        } else if (material_id > 20.5) {
            Kd0 = texture(TextureImage21, vec2(U,V)).rgb; // 21
        } else if (material_id > 19.5) {
            Kd0 = texture(TextureImage20, vec2(U,V)).rgb; // 20 Machine
        } else {
            Kd0 = vec3(0.85, 0.8, 0.75); // Default
        }
    } else if (object_id == 51) { // BARRACA MACARRAO
        U = texcoords.x;
        V = texcoords.y; // Removed flip since it might be pre-flipped
        if (material_id > 25.5) {
            Kd0 = texture(TextureImage26, vec2(U,V)).rgb; // lambert1
        } else if (material_id > 24.5) {
            Kd0 = texture(TextureImage25, vec2(U,V)).rgb; // Banner1
        } else {
            Kd0 = vec3(0.8, 0.8, 0.8);
        }
    } else if (object_id == 52) { // BARRACA BANANA
        U = texcoords.x;
        V = 1.0 - texcoords.y; // Corrected flip for the banana stall
        if (material_id > 136.5) { Kd0 = vec3(0.000000, 0.800489, 0.002963); } // 137 Green
        else if (material_id > 135.5) { Kd0 = vec3(0.800536, 0.586257, 0.000000); } // 136 Orange
        else if (material_id > 134.5) { Kd0 = vec3(0.703898, 0.014190, 0.601211); } // 135 Purple/Pink
        else if (material_id > 131.5) { Kd0 = vec3(0.8, 0.8, 0.8); } // 132
        else if (material_id > 126.5) { Kd0 = vec3(0.800000, 0.652655, 0.109448); } // 127
        else if (material_id > 125.5) { Kd0 = vec3(0.276299, 0.800000, 0.493059); } // 126 Mint
        else if (material_id > 124.5) { Kd0 = vec3(0.800000, 0.352613, 0.690372); } // 125 Pink
        else if (material_id > 123.5) { Kd0 = vec3(0.800000, 0.548766, 0.224314); } // 124 Orange
        else if (material_id > 121.5) { Kd0 = vec3(0.363019, 0.326505, 0.151221); } // 122 Brown
        else if (material_id > 120.5) { Kd0 = vec3(0.8, 0.8, 0.8); } // 121
        else if (material_id > 119.5) { Kd0 = vec3(0.911397, 0.845363, 0.372078); } // 120 Yellow
        else if (material_id > 118.5) { Kd0 = vec3(0.029059, 0.006006, 0.002365); } // 119 Dark Brown
        else if (material_id > 117.5) { Kd0 = vec3(0.933090, 0.922529, 0.008153); } // 118 Yellow
        else if (material_id > 108.5) { Kd0 = vec3(0.8, 0.8, 0.8); } // 109 White/Gray
        else if (material_id > 103.5) { Kd0 = vec3(0.800000, 0.142353, 0.028348); } // 104 Red
        else if (material_id > 26.5) { Kd0 = texture(TextureImage27, vec2(U,V)).rgb; } // 27 cb_0.png
        else if (material_id > 19.5) { Kd0 = texture(TextureImage20, vec2(U,V)).rgb; } // 20 Machine
        else { Kd0 = vec3(0.8, 0.8, 0.8); }
    } else if (object_id == GROUND) {
        U = texcoords.x;
        V = texcoords.y;
        
        if (material_id > 18.5) {
            Kd0 = vec3(0.9, 0.9, 0.9); // 19.0 - White
        } else if (material_id > 17.5) {
            Kd0 = vec3(0.1, 0.1, 0.1); // 18.0 - Dark/Black
        } else if (material_id > 16.5) {
            Kd0 = vec3(0.5, 0.5, 0.55); // 17.0 - Metal Gray for Barricades
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

    if ((object_id >= 20 && object_id <= 29) || object_id == 30 || object_id == 31 || object_id == 40 || object_id == 41 || UseOverrideKd == 1) {
        color.rgb = Kd0;
    } else {
        // Equação de Iluminação
        float lambert = max(0,dot(n,l));
        color.rgb = Kd0 * (lambert + 0.01);
        
        // Simular brilho metálico (Specular) para as barricadas (material_id 17.0)
        if (object_id == GROUND && material_id > 16.5 && material_id <= 17.5) {
            vec4 h = normalize(v + l); // Half-vector
            float specular = pow(max(0.0, dot(n, h)), 64.0); // Shininess alto para metal
            color.rgb += vec3(0.7, 0.7, 0.7) * specular;
        }
        
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
    } else if (object_id >= 27 && object_id <= 29) {
        color.a = 0.75; // Semi-transparent hologram
    } else if (object_id == 40) {
        color.a = 0.85; // Semi-transparent for UI background
    } else {
        color.a = 1.0;
    }

    // Cor final com correção gamma, considerando monitor sRGB.
    // Veja https://en.wikipedia.org/w/index.php?title=Gamma_correction&oldid=751281772#Windows.2C_Mac.2C_sRGB_and_TV.2Fvideo_standard_gammas
    color.rgb = pow(color.rgb, vec3(1.0,1.0,1.0)/2.2);
} 
