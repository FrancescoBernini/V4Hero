#ifndef ENDFLAG_H
#define ENDFLAG_H

#include "../../Entity.h"
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>

class EndFlag : public Entity
{
public:
    sf::Clock endMissionClock;
    sf::Clock fakeLoadingTimer;
    bool missionEnd = false;
    sf::SoundBuffer end_sound;
    sf::Sound s_end;

    EndFlag();
    void LoadConfig();
    void Draw();
    void OnCollide(CollidableObject* otherObject, int collidedWith = -1, vector<string> collisionData = {});
};

#endif // ENDFLAG_H
