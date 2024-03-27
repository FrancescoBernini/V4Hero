#ifndef FEVERWORM_H
#define FEVERWORM_H

#include "../../../../Graphics/PSprite.h"
#include "../../Entity.h"
#include <SFML/Graphics.hpp>

class FeverWorm : public Entity
{
public:
    sf::Texture tex_c, tex_o1, tex_m, tex_b, tex_o2, tex_exc;
    sf::Texture tex_number[10];

    PSprite let_c, let_o1, let_m, let_b, let_o2, let_exc;
    PSprite number;

    float next_x = -350;
    float speed = 120;

    int f_combo = 0;
    int old_combo = 0;

    bool fever_achieved = false;

    FeverWorm();
    void LoadConfig();
    void doRhythm(std::string current_song, std::string current_drum, int combo, int realcombo, bool advanced_prefever, float beatBounce, float satisfaction);
    void Draw();
};

#endif // FEVERWORM_H
