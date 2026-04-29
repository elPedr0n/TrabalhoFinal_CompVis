### Ben  10 de play2 remasterizado ultra master
[link gameplay](https://www.youtube.com/watch?v=NFzR4qwJwrA)

A ideia eh implementar ao menos a primeira fase do game, o pier


- **Malhas poligonais complexas** Implementaremos vários inimigos e cenarios, então terá varios modelos geométricos

- **Transformações geométricas controladas pelo usuário** Além da movimentação do personagem, teremos mudanças de ângulos com o mouse, fazendo com que tanto o calculo da camera como movimentação ao longo do espaço seja feito por transformações

-  **Diferentes tipos de câmeras** Pensamos em usar a camera em 3a pessoa como mostra a gameplay, mas tambem implementar uma versão fixada da mesma, semelhante ao primeiro jogo de resident evil, ou ate mesmo uma camera isométrica 

- **Instâncias de objetos** Teremos vários inimigos que o jogador enfrenta, então teremos várias instâncias dos objetos do jogo

- **Testes de intersecção** Principalmente a colisão com o mundo, os desafios de plataforma e os ataques atingindo inimigos serão computados com a interecção de objetos

- **Modelos de Iluminação em todos os objetos** Teremos a iluminação global, mas também temos a iluminação dos efeitos dos personagens, que interferem tanto globalmente quanto aos objetos

- **Mapeamento de texturas em todos os objetos** Mapearemos as texturas de tudo usando imagens como solicitado no enunciado

- **Movimentação com curva Bézier cúbica** Implementaremos uma parte do jogo em que acontecerá uma wave de inimigos sobre um objeto, que se movimenta sobre o ambiente usando uma curva de bezier cúbica

- **Animações baseadas no tempo ($\Delta t$)** Tudo será calculado usando o $\Delta t$, a fim de n termos diferenças de velocidades em diferentes ambientes.


### 375 INFINITO SIMULATION

Nossa ideia original, n temos um vídeo de demonstração de referência. MAs a ideia central é a seguinte, temos um gurizao que está esperando o ônibus na parada, um 375 para chegar ao campus do vale. Como é de conhecimento geral, esse onibus é um ser sobrenatural, volta e meia aparece em horários corretos, ou só não aparece. Simularemos esse gurizao esperando o onibus, com mecanicas que englobam a vida cotidiana, como *piscar*, puxar o telefone para conferir os horários ou ver reels, conversar com outros seres na parada ou até mesmo mudar sua visão para o seu passaro de estimação, que permite uma visão global da rua. O objetivo principal é justamente pegar o ônibus, que terá variações de comportamento baseado em um script. 


- **Malhas poligonais complexas** Implementaremos um cenário vivo com varios objetos 

- **Transformações geométricas controladas pelo usuário** Temos a movimentação do proprio jogador, assim como interações em seu ambiente e celular

-  **Diferentes tipos de câmeras** Temos uma camera fixa em 3a pessoa no personagem, mas podemos ter também uma fixa na parada do onibus, assim como uma fixa no passaro de estimação para averiguar o ambiente

- **Instâncias de objetos** Temos o proprio jogador, assim como seres na parada de onibus e os proprios ônibus

- **Testes de intersecção** Teremos principalmente testes envolvendo colisão com objetos do ambiente

- **Modelos de Iluminação em todos os objetos** Temos a iluminação global do mundo, assim como uma iluminação gerada pelo celular e luzes dos ônibus

- **Mapeamento de texturas em todos os objetos** Mapearemos as texturas de tudo usando imagens como solicitado no enunciado

- **Movimentação com curva Bézier cúbica** Tanto a movimentação dos ônibus quanto a movimentação do passaro serao modeladas usando uma curva de Bézier cúbica

- **Animações baseadas no tempo ($\Delta t$)** Tudo será calculado usando o $\Delta t$, a fim de n termos diferenças de velocidades em diferentes ambientes.


### TERMO EM PRIMEIRA PESSOA COM ARMAS

Novamente, é uma ideia original então n temos um vídeo.
Mas a ideia é bem simples. Temos o jogo [Termo](https://term.ooo/), jogo de adivinhação de palavras. Da mesma forma que funcionna o jogo, tentaremos adivinhar a palavra, so que a medida que erramos as letras (letras em cinza) elas virão nos atacar. Precisaremos eliminar elas para poder fazer mais uma adivinhação. Imaginamos 2 modos principais, um em primeira pessoa, em que ao fundo temos o teclado com as letras, e devemos atirar nelas quando vierem em nossa direção. Outra abordagem seria um gameplay semelhante a space invaders. O usuário insere a palavra de alguma forma, e as letras virão na direção dele também, mas mais caótica, como um bullet hell da vida.


- **Malhas poligonais complexas** Temos a implementação de ou um modelo de nave ou armas em primeira pessoa, assim como o proprio termo que jogaremos

- **Transformações geométricas controladas pelo usuário** A movimentação do personagem e da camera em primeira pessoa eh controlada pelo usuario. 

-  **Diferentes tipos de câmeras** Temos uma camera em primeira pessoa, assim como uma fixa top down para o caso da espaçonave.

- **Instâncias de objetos** Temos o proprio jogador, assim como seres na parada de onibus e os proprios ônibus

- **Testes de intersecção** Teremos muitos testes envolvendo projeteis com objetos ao mundo

- **Modelos de Iluminação em todos os objetos** Temos a iluminação global do ambiente, assim como o brilho dos projeteis dos inimigos e das letras corretas brilhando ao fundo.

- **Mapeamento de texturas em todos os objetos** Mapearemos as texturas de tudo usando imagens como solicitado no enunciado

- **Movimentação com curva Bézier cúbica** A movimentação das letras na direção do player irá seguir uma curva de Bézier cubica, principalmente na versão da espaçonave

- **Animações baseadas no tempo ($\Delta t$)** Tudo será calculado usando o $\Delta t$, a fim de n termos diferenças de velocidades em diferentes ambientes.

### ESCALADA CINEMÁTICA
[link gameplay](https://www.youtube.com/watch?v=9iuTUWnKqaM)
A ideia eh montar um simulador de escalada semelhane a peak, um jogo em primeira pessoa que conseguimos escalar em um mundo elegante


- **Malhas poligonais complexas** Temos a implementação de objetos assim como das mãos e do mundo, tudo em malhas de polígonos

- **Transformações geométricas controladas pelo usuário** A movimentação do personagem e da camera em primeira pessoa será emstrada pelo usuário 

-  **Diferentes tipos de câmeras** Teremos 2 modos de jogo, diferentemente do original. Temos a versão normal, em primeira pessoa, mas também um modo de boulder indoor, que tem a camera fixa e basta coordenação dos dos braços da forma correta para concluir

- **Instâncias de objetos** Temos o jogador, os holds (onde o jogador se segura) e objetos do mundo.

- **Testes de intersecção** Teremos testes de intersecção envolvendo principalmente colisão das maos do personagem com os holds

- **Modelos de Iluminação em todos os objetos** Temos a iluminação global do ambiente. Podemos criar um modo noturno onde colocamos iluminação de lanternas.

- **Mapeamento de texturas em todos os objetos** Mapearemos as texturas de tudo usando imagens como solicitado no enunciado

- **Movimentação com curva Bézier cúbica** A movimentação de objetos do cenário seguirão uma curva de Bézier.

- **Animações baseadas no tempo ($\Delta t$)** Tudo será calculado usando o $\Delta t$, a fim de n termos diferenças de velocidades em diferentes ambientes.
