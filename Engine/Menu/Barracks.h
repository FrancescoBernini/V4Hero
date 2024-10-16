#ifndef BARRACKSMENU_H
#define BARRACKSMENU_H
#include "../Config.h"
#include "../Dialog/ControlTips.h"
#include "../Dialog/DialogBox.h"
#include "../Dialog/RoundedRect.h"
#include "../Graphics/Menu.h"
#include "../Graphics/PText.h"
#include "../Input/InputController.h"
#include "../Mission/MissionController.h"
#include "../Mission/Camera.h"
#include "../Item/Item.h"
#include <SFML/Graphics.hpp>

class V4Core;
class Barracks : public Menu
{
private:
    ControlTips ctrlTips;

    std::vector<std::unique_ptr<AnimatedObject>> barracks_units;

    sf::RectangleShape rs_cover;
    sf::RectangleShape mm_bigBox;
    sf::RectangleShape mm_titleBox;
    sf::RectangleShape mm_selected_item_line;

    sf::Texture t_background;
    PSprite s_background;
    float ratio_x;
    float ratio_y;

    float res_ratio_x, res_ratio_y;
    int q = 0;

    float patapon_y;
    float floor_y;

    int quality_setting = 0;


    PText t_title;
    int current_item_position = 0;
    std::vector<bool> enabled_positons;

    RoundedRect rr_main, rr_main_sh;
    RoundedRect rr_unitstatus, rr_unitstatus_sh;
    RoundedRect rr_uniticon, rr_uniticon_sh;

    PSprite class_icon;

    PText unit_status, class_name;

    PText unit_stat_level_t, unit_stat_level_v;
    PText unit_stat_exp_t, unit_stat_exp_v;
    PText unit_stat_hp_t, unit_stat_hp_v;
    PText unit_stat_dmg_t, unit_stat_dmg_v;
    PText unit_stat_atkspd_t, unit_stat_atkspd_v;

    PText unit_stat_critc_t, unit_stat_critc_v;
    PText unit_stat_kbc_t, unit_stat_kbc_v;
    PText unit_stat_stgc_t, unit_stat_stgc_v;
    PText unit_stat_firec_t, unit_stat_firec_v;
    PText unit_stat_icec_t, unit_stat_icec_v;

    /// Barracks Equipment Text
    PText t_unit_rarepon_name;
    PText t_slot_1_name;
    PText t_slot_2_name;
    PText t_slot_3_name;
    PText t_slot_4_name;
    PText t_slot_5_name;
    std::vector<PText> t_eq_names = {t_slot_1_name, t_slot_2_name, t_slot_3_name, t_slot_4_name, t_slot_5_name};

    PSprite inv_box;
    int grid_sel_x = 0, grid_sel_y = 0, grid_offset_y = 0;

    /// menu background
    PSprite s_menu_bkg;

    /// highlighted unit
    PSprite s_pon_highlight;

    /// class icon
    PSprite s_class_icon;

    /// unit icon
    PSprite s_unit_icon;

    /// weapon icon
    PSprite s_weapon_icon;

    /// weapon2 icon
    PSprite s_weapon2_icon;

    /// armour icon
    PSprite s_armour_icon;

    /// mask icon
    PSprite s_mask_icon;

    int current_selected_pon = 0;
    float item_line_flash = 0;

    bool menu_mode = false;

    struct InvBox {
        ///data
        Item* data;
        int amount = 0;
        int occ_amount = 0;
        bool highlight = false;
        int inv_id;

        ///display
        sf::RectangleShape r_outer;
        sf::RectangleShape r_inner;
        sf::RectangleShape r_highlight;
        PText num, num_shadow;
        PSprite icon;
    };

    std::vector<InvBox> inventory_boxes;
    sf::RectangleShape r_sel;

    RoundedRect rr_itempreview, rr_itempreview_sh;
    PText item_title, item_desc;

    int current_item_id;

    int active_category = 0;
    int active_subcategory = 0;

    int current_menu_position;
    std::vector<float> possible_menu_positions;

    sf::RectangleShape mm_description_background;
    sf::RectangleShape mm_highlighted_tile;

    PSprite highlighted_pon;

    PText t_item_title;
    std::vector<PText> t_item_description;

    std::vector<PataDialogBox> dialog_boxes;

    int goto_id = -1;

public:
    std::string mission_file;
    Camera camera;

    int mission_id = 0;
    float mission_multiplier = 1;

    bool obelisk = false;
    bool mission_started = false;

    bool initialized = false;
    
    void Update();
    void updateInputControls();
    void eventFired(sf::Event event);
    void setTitle(int menu_position);

    void onExit();
    int countOccupied(std::vector<int>);
    void loadInventory();
    void setInventoryPosition();
    void applyEquipment();
    void refreshStats();
    std::string getPreviewText(float stat, float pon_stat, float pon_base_stat, int digits=0);
    std::string getPreviewText(float stat, float pon_stat, float pon_base_stat, float stat2, float pon_stat2, float pon_base_stat2, int digits=0);
    sf::Color getPreviewColorText(float stat, float pon_stat, float pon_base_stat, bool invert_color, int digits=0);
    sf::Color getPreviewColorText(float stat, float pon_stat, float pon_base_stat, float stat2, float pon_stat2, float pon_base_stat2, int digits=0);
    void updatePreviewText();
    void updateButtons();
    Barracks();
    ~Barracks();
};


#endif // BARRACKSMENU_H
