#include "globals.h"

// Inicialização das plataformas
Platform g_platforms[MAX_PLATFORMS] = {
    { glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(1.8f, 1.8f, 1.8f) }, 
    { glm::vec3(2.0f, -1.0f, 2.0f), glm::vec3(1.8f, 1.8f, 1.8f) }, 
    { glm::vec3(4.0f, -1.0f, 4.0f), glm::vec3(1.8f, 1.8f, 1.8f) }  
};

bool CheckCollisionAABB(glm::vec3 posA, glm::vec3 scaleA, glm::vec3 posB, glm::vec3 scaleB) {
    // Atenção: Assumindo que posA (jogador) tem a origem NOS PÉS (Y mínimo)
    // E posB (plataforma) tem a origem NO CENTRO.
    
    bool collisionX = (posA.x - scaleA.x/2.0f <= posB.x + scaleB.x/2.0f) && 
                      (posA.x + scaleA.x/2.0f >= posB.x - scaleB.x/2.0f);
                      
    bool collisionY = (posA.y <= posB.y + scaleB.y) && 
                      (posA.y + scaleA.y >= posB.y);
                      
    bool collisionZ = (posA.z - scaleA.z/2.0f <= posB.z + scaleB.z/2.0f) && 
                      (posA.z + scaleA.z/2.0f >= posB.z - scaleB.z/2.0f);

    return collisionX && collisionY && collisionZ;
}