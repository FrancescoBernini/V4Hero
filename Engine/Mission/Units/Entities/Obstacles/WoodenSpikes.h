#ifndef WOODENSPIKES_H
#define WOODENSPIKES_H

#include "../../Entity.h"
#include <SFML/Graphics.hpp>

class WoodenSpikes : public Entity
{
public:
    bool droppeditem = false;
    sf::Clock death_timer;

    sf::SoundBuffer s_broken;

    WoodenSpikes();
    void LoadConfig();
    void parseAdditionalData(nlohmann::json additional_data);
    void Draw();
    void OnCollide(CollidableObject* otherObject, int collidedWith = -1, vector<string> collisionData = {});
};

#endif // WOODENSPIKES_H
