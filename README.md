# Relatório Final - Computação Gráfica e Visualização (INF01047)

**Nomes:** Pedro Henrique Moreira de Andrade Jacinto e João Pedro Simon Figueiró

>Este relatório tem como finalidade descrever o processo de desenvolvimento do nosso jogo, **Ben 10 Ben 10 Força Alienígena Ultra Remaster (OpenGL version)**.

## Descrição da Aplicação
O *Ben 10 Alien Force* é um jogo estilo *beat 'em up* de Playstation 2 em que podemos controlar o jovem Ben Tennyson. Graças ao Omnitrix, ele possui a capacidade de se transformar em diferentes aliens, derrotar vários inimigos e proteger a Terra. Nossa aplicação procurou adaptar uma porção da primeira fase do jogo original: *Knight-mare at the Pier*. 

Temos uma ambientação adaptada, mas mantendo a essência da fonte. Elementos como uma roda gigante e barraquinhas são dispostos ao longo da fase, levando a uma seção de plataforma na qual o jogador deve pular entre troncos e um barco para alcançar o castelo. Abaixo do mapa, encontra-se um mar tóxico que mata instantaneamente qualquer um que cair nele.

O jogador possui uma quantidade de pontos de vida representada pela barra vermelha. Se ela acabar, Ben morrerá e o jogador terá que reiniciar a fase. 

Nessa fase, o jogador pode controlar Ben e duas de suas transformações alienígenas: *Swampfire* e *Big Chill*. Para se transformar, o jogador deve estar com sua barra de transformação cheia. Essa barra diminui enquanto Ben está transformado, e pode forçar a destransformação caso se esgote.

Cada uma das formas jogáveis possui um ataque básico e um ataque especial que consome energia especial representada pela barra amarela, além de diferentes habilidades:
- **Ben**: possui baixa resistência e ataques fracos. Seu ataque básico é um soco e seu ataque especial é um grande tapa, mas pode se transformar em um dos aliens para ter acesso a ataques mais poderosos;
- **Swampfire**: possui alta resistência. Seu ataque básico consiste em uma sequência de golpes e seu ataque especial permite lançar uma bola de fogo, cujo tamanho e dano aumentam conforme o jogador carrega o ataque;
- **Big Chill**: possui resistência moderada. Seu ataque básico consiste em uma sequência de socos de curto alcance e seu ataque especial permite liberar um sopro gelado contínuo que dá dano leve a inimigos e os congela, deixando-os mais lentos. Também possui a habilidade de pulo duplo no ar.

Ao longo do trajeto, Ben encontra objetos quebráveis - bancos, caixas e ursinhos de pelúcia - que podem ser destruídos para liberar orbes que regeneram sua barra de vida, transformação ou energia especial. 

Além disso, inimigos serão encontrados pelo caminho: os Cavaleiros Eternos. Ao serem eliminados, derrubam orbes que regeneram o jogador. Eles vêm em duas variedades:
- **Cavaleiro básico**: inimigo com ataques de curto alcance. É mais resistente;
- **Cavaleiro *ranged***: inimigo que ataca à distância, lançando projéteis em arco. É mais lento e menos resistente.

O objetivo do jogo é percorrer o mapa, derrotar inimigos e chegar ao final da fase, onde Ben enfrenta um desafio final em frente ao castelo dos Cavaleiros Eternos. Ao derrotar todos os inimigos próximos ao castelo, Ben consegue completar a fase e vencer o jogo.

## Manual do usuário

|              Ação              | Teclado |                           Gamepad                           |
| :----------------------------: | :-----: | :---------------------------------------------------------: |
|          Movimentação          |   WASD  |                      Analógico esquerdo                     |
| Controle da câmera (3ª pessoa) |  Mouse  |                      Analógico direito                      |
|              Pulo              |  Espaço |                  A (Xbox) / X (PlayStation)                 |
|          Ataque básico         |    E    |                  X (Xbox) / □ (PlayStation)                 |
|         Ataque especial        |    Q    |                  Y (Xbox) / △ (PlayStation)                 |
|  Transformar / Destransformar  |    Z    |                 RB (Xbox) / R1 (PlayStation)                |
|  Trocar alienígena selecionado |    X    | D-Pad esquerdo/direito / LB-RB (Xbox) / L1-R1 (PlayStation) |
|           Emote (Ben)          |    G    |                 RS (Xbox) / R3 (PlayStation)                |
|          Trocar câmera         |    C    |             Select (Xbox) / Share (PlayStation)             |
|            Confirmar           |  Enter  |                            Start                            |



## Contribuição de cada desenvolvedor no projeto
Embora ambos os desenvolvedores tenham contribuído para o projeto, cada um teve uma função mais destacada em algumas partes do desenvolvimento. Segue abaixo as principais contribuições de cada desenvolvedor:

#### Pedro Henrique Moreira de Andrade Jacinto:
- Implementação do sistema de colisão e física do jogo;
- Implementação do sistema de câmeras, incluindo o posicionamento das câmeras fixas e a transição entre elas;
- Implementação do sistema de iluminação, incluindo a iluminação global e a iluminação local dependente do alien selecionado;
- Design e implementação do mapa do jogo, incluindo:
  - Colisões com os elementos do mapa e design de fase;
  - Implementação dos modelos fixos como a roda gigante, o castelo, o barco, as barracas, os troncos e as cercas;
  - Plano de fundo e mar tóxico;
- Implementação inicial de:
  - Movimentação do jogador e inimigos;
  - Ataques e hitboxes do jogador;
- Resolução de merges em casos de conflitos complexos.

#### João Pedro Simon Figueiró:
- Refinamento dos modelos e animações do jogador e dos inimigos;
- Refinamento do sistema de ataques e habilidades do jogador, incluindo:
  - Ataques básicos e especiais, com barra dedicada;
  - Sistema de transformação entre os aliens, com barra dedicada;
  - Sistema de vida, dano e morte, com barra dedicada.
- Refinamento do sistema de combate e inimigos, incluindo:
  - Inimigo básico, com ataques de curto alcance;
  - Inimigo *ranged*, com ataques de longo alcance usando curva de Bezier;
- Implementação do sistema de partículas e fragmentos;
- Implementação e posicionamento de objetos quebráveis;
- Implementação de coletáveis;
- Implementação do sistema de menus e HUD;
- Implementação e posicionamento dos *spawners* de inimigos e a lógica de vitória associada a eles;
- Implementação do sistema de áudio e configuração de sons e músicas;
- Implementação do mapeamento de inputs para gamepad;
- Implementação do botão de *emote* para o jogador.

## Utilização de IA no desenvolvimento

## Imagens da aplicação
