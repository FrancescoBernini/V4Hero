#include "MissionController.h"
#include <iostream>
#include <algorithm>
#include <time.h>
#include <string>
#include <thread>
#include <chrono>
#include <random>
#include "Units/Projectile.h"
#include "../Math/PVector.h"
#include "Units/HitboxFrame.h"
#include "../V4Core.h"

using json = nlohmann::json;

using namespace std;

///move to Func::numDigits later
template <class T>
int numDigits(T number) //stolen from stackoverflow
{
	int digits = 0;
	if (number < 0) digits = 1;
	while (number) {
		number /= 10;
		digits++;
	}
	return digits;
}

MissionController::MissionController()
{
}

float MissionController::Smoothstep(float time) ///use time from 0.00 to 1.00
{
	time = Clamp(time, 0.0, 1.0);
	return time * time * (3 - 2 * time);
}

float MissionController::Clamp(float x, float lowerlimit, float upperlimit)
{
	if (x < lowerlimit)
		x = lowerlimit;
	if (x > upperlimit)
		x = upperlimit;

	return x;
}

int MissionController::layerStr2Enum(string layer)
{
	transform(layer.begin(), layer.end(), layer.begin(), [](unsigned char c){ return std::tolower(c); }); // https://stackoverflow.com/questions/313970
	if(layer == "buildings")
	{
		return BUILDINGS;
	}
	else if(layer == "nature")
	{
		return NATURE;
	}
	else if(layer == "animals")
	{
		return ANIMALS;
	}
	else if(layer == "ui")
	{
		return UI;
	}
}

void MissionController::addDmgCounter(int type, int damage, float baseX, float baseY, int q, int r)
{
	DamageCounter tmp;
	tmp.type = type;
	tmp.damage = damage;

	cout << "MissionController::addDmgCounter(" << type << ", " << damage << ", " << baseX << ", " << baseY << ")" << endl;

	int digits = numDigits(damage);
	string sdigits = to_string(damage);

	cout << "Digits: " << digits << " " << sdigits << endl;

	int separator = 0;
	float init_scale = 1;
	float dg_scale = 4;

	if(damage < 100)
	type = 2;
	else
	type = 3;

	switch(type)
	{
		case 0:
		{
			separator = 12;
			init_scale = 0.8;
			dg_scale = 2.8;
			break;
		}

		case 1:
		{
			separator = 12;
			init_scale = 0.8;
			dg_scale = 2.8;
			break;
		}

		case 2:
		{
			separator = 16;
			init_scale = 0.8;
			dg_scale = 3.5;
			break;
		}

		case 3:
		{
			separator = 18;
			init_scale = 0.8;
			dg_scale = 3.9;
			break;
		}

		case 4:
		{
			separator = 20;
			init_scale = 0.8;
			dg_scale = 4.1;
			break;
		}
	}

	for(int i=0; i<digits; i++)
	{
		string sdigit = string()+sdigits[i];
		int digit = atoi(sdigit.c_str());

		PSprite dg_spr;
		dg_spr.setTexture(dmg_spritesheet.t);
		dg_spr.setTextureRect(dmg_spritesheet.get_bounds((digit*5)+type)); ///rect of the specific damage digit from spritesheet
		dg_spr.setOrigin(dg_spr.getLocalBounds().width/2, dg_spr.getLocalBounds().height);
		dg_spr.qualitySetting = q;
		dg_spr.resSetting = r;

		sf::Vector2f dg_pos(baseX+(i*separator), baseY);

		tmp.spr.push_back(dg_spr);
		tmp.pos.push_back(dg_pos);
		tmp.scale.push_back(dg_scale);
		tmp.scale_goal.push_back(init_scale);
		tmp.mode.push_back(true);
		tmp.alpha.push_back(0);
	}

	dmgCounters.push_back(tmp);
}

void MissionController::addItemsCounter(int id, float baseX, float baseY)
{

}

json parseLootArray(mt19937& gen, uniform_real_distribution<double>& roll, json loot)
{
	if (!loot.is_array())
	{
		return loot;
	}

	int total = 0;
	for (int i = 1; i < loot.size(); i++) // Chances -> Intervals for second loop
	{
		if (loot[i].is_array())
		{
			loot[i][0] += total;
			total = loot[i][0];
		}
		else if (loot[i].is_object() && loot[i].size() == 2)
		{
			int tmp = loot[i]["chance"];
			loot[i].erase("chance");
			total += tmp;
			loot[i]["chance"] = total;
		}
		else
		{
			cout << "[WARNING] Undefined behavior detected while parsing loot: " << loot << " | (Element of array is neither an array nor an object)" << endl;
		}
	}

	if (total < 100)
	{
		cout << "[WARNING] Undefined behavior detected while parsing loot: " << loot << " | (Total chances in array less than 100)" << endl;
	}
	else if (total > 100)
	{
		cout << "[WARNING] Potential data loss detected while parsing loot: " << loot << " | (Total chances in array more than 100)" << endl;
	}

	float n = roll(gen);
	for (int i = 1; i < loot.size(); i++) // Because of the first loop, values are neatly sorted
	{
		if (loot[i].is_array())
		{
			if (n <= float(loot[i][0]) / 100)
			{
				return parseLootArray(gen, roll, loot[i]);
				break;
			}
		}
		else if (loot[i].is_object() && loot[i].size() == 2)
		{
			if (n <= float(loot[i]["chance"]) / 100)
			{
				return loot[i];
				break;
			}
		}
		else
		{
			cout << "[WARNING] Undefined behavior detected while parsing loot: " << loot << " | (Element of array is neither an array nor an object)" << endl;
		}
	}
}


void MissionController::parseEntityLoot(mt19937& gen, uniform_real_distribution<double>& roll, json loot, vector<Entity::Loot>& to_drop)
{
	cout << "parseEntityLoot()" << endl;

	if (loot.is_array())
	{
		if (roll(gen) <= float(loot[0]) / 100)
		{
			Entity::Loot tmp;
			json parsedArray = parseLootArray(gen, roll, loot);
			cout << "parsedArray: " << parsedArray << endl;
			tmp.order_id = v4Core->saveReader.itemReg.getItemByName(parsedArray["item"])->order_id;
			to_drop.push_back(tmp);
		}
	}
	else if (loot.is_object() && loot.size() > 1)
	{
		try
		{
			for (auto it = loot.begin(); it != loot.end(); ++it) // Assume it's an object containing two items to be parsed
			{
				parseEntityLoot(gen, roll, it.value(), to_drop);
			}
		}
		catch (const exception& e)
		{
			if (roll(gen) <= float(loot["chance"]) / 100) // Assume it's the below else if
			{
				Entity::Loot tmp;
				tmp.order_id = v4Core->saveReader.itemReg.getItemByName(loot["item"])->order_id;
				to_drop.push_back(tmp);
			}
		}
	}
	else if (loot.is_object() && loot.size() == 2)
	{
		if (roll(gen) <= float(loot["chance"]) / 100)
		{
			Entity::Loot tmp;
			tmp.order_id = v4Core->saveReader.itemReg.getItemByName(loot["item"])->order_id;
			to_drop.push_back(tmp);
		}
	}
}

void MissionController::cacheEntity(int entityID, shared_ptr<vector<vector<sf::Image>>> swaps, shared_ptr<vector<AnimatedObject::Animation>> spritesheet, shared_ptr<vector<Object>> objects)
{
	cout << "[MissionController] cacheEntity id " << entityID << endl;

	isCached[entityID] = true;

	animation_cache[entityID] = make_shared<AnimationCache>();
	animation_cache[entityID].get()->swaps = swaps;
	animation_cache[entityID].get()->spritesheet = spritesheet;
	animation_cache[entityID].get()->objects = objects;

	cout << "[MissionController] cache created" << endl;
}

void MissionController::spawnEntity(int id, bool collidable, bool attackable, int xpos, int xrange, bool cloneable, float clone_delay, float spawnrate, float stat_mult, int mindmg, int maxdmg, int hp, float ypos, float baseY, sf::Color color, int layer, int parent, nlohmann::json loot, nlohmann::json additional_data)
{
	sf::Clock bm; ///benchmark for spawnEntity function

	uniform_real_distribution<double> roll(0.0, 1.0);

	int mission_level = v4Core->saveReader.mission_levels[curMissionID];
	int mission_diff = 0.85 + v4Core->saveReader.mission_levels[curMissionID] * 0.15;

	///need to somehow optimize this to not copy paste the same code over and over

	cout << "[MissionController] Spawning entity " << " (ID: " << id << ") " << hp << " " << xpos << " " << xrange << " " << ypos << " " << spawnrate << " " << stat_mult << endl;

	bool spawn = false;

	if(roll(v4Core->gen) <= spawnrate / 100)
	{
		spawn = true;
	}

	for(int i = 0; i < additional_data.size(); i++)
	{
		///Force entity to spawn when mission is on a specific level
		if(additional_data.contains("forceSpawnOnLvl"))
		{
			if(mission_level == additional_data["forceSpawnOnLvl"])
			{
				spawn = true;
			}
		}

		///Force entity to spawn when specific item is not obtained
		if(additional_data.contains("forceSpawnIfNotObtained"))
		{
			if(!v4Core->saveReader.invData.checkItemObtainedByName(additional_data["forceSpawnIfNotObtained"]))
			{
				spawn = true;
			}
		}
	}

	cout << "[MissionController::spawnEntity] additional_data: " << additional_data << endl;

	if (spawn)
	{
		switch (id)
		{
			case 0:
			{
				unique_ptr<EndFlag> entity = make_unique<EndFlag>(); 
				entity->setEntityID(id); ///id must be set before LoadConfig so loadAnim can get the right cache ID
				entity.get()->LoadConfig(thisConfig);
				if (additional_data.size() >= 1)
				{
					entity.get()->parseAdditionalData(additional_data);
				}

				if (!isCached[id])
				{
					//cacheEntity(id, entity.get()->all_swaps_img, entity.get()->animation_spritesheet, entity.get()->objects);
				}

				///To be replaced with param file
				entity.get()->entityType = Entity::EntityTypes::PASSIVE;
				entity.get()->entityCategory = Entity::EntityCategories::MISC;

				if (!cloneable)
					tangibleLevelObjects.push_back(std::move(entity));

				break;
			}
			case 1:
			{
				unique_ptr<FeverWorm> entity = make_unique<FeverWorm>();
				entity->setEntityID(id); ///id must be set before LoadConfig so loadAnim can get the right cache ID
				entity.get()->LoadConfig(thisConfig);
				if (additional_data.size() >= 1)
				{
					entity.get()->parseAdditionalData(additional_data);
				}

				if (!isCached[id])
				{
					//cacheEntity(id, entity.get()->all_swaps_img, entity.get()->animation_spritesheet, entity.get()->objects);
				}

				///To be replaced with param file
				entity.get()->entityType = Entity::EntityTypes::PASSIVE;
				entity.get()->entityCategory = Entity::EntityCategories::MISC;

				if (!cloneable)
					tangibleLevelObjects.push_back(std::move(entity));

				break;
			}
			case 2:
			{
				unique_ptr<Kacheek> entity = make_unique<Kacheek>();
				entity->setEntityID(id); ///id must be set before LoadConfig so loadAnim can get the right cache ID
				entity.get()->LoadConfig(thisConfig);
				if (additional_data.size() >= 1)
				{
					entity.get()->parseAdditionalData(additional_data);
				}

				if (!isCached[id])
				{
					//cacheEntity(id, entity.get()->all_swaps_img, entity.get()->animation_spritesheet, entity.get()->objects);
				}

				///To be replaced with param file
				entity.get()->entityType = Entity::EntityTypes::HOSTILE;
				entity.get()->entityCategory = Entity::EntityCategories::ANIMAL;

				if (!cloneable)
					tangibleLevelObjects.push_back(std::move(entity));

				break;
			}
			case 3:
			{
				unique_ptr<Grass1> entity = make_unique<Grass1>();
				entity->setEntityID(id); ///id must be set before LoadConfig so loadAnim can get the right cache ID
				entity.get()->LoadConfig(thisConfig);
				if (additional_data.size() >= 1)
				{
					entity.get()->parseAdditionalData(additional_data);
				}

				if (!isCached[id])
				{
					//cacheEntity(id, entity.get()->all_swaps_img, entity.get()->animation_spritesheet, entity.get()->objects);
				}

				///To be replaced with param file
				entity.get()->entityType = Entity::EntityTypes::NEUTRAL;
				entity.get()->entityCategory = Entity::EntityCategories::NATURE;

				if (!cloneable)
					tangibleLevelObjects.push_back(std::move(entity));

				break;
			}
			case 4:
			{
				unique_ptr<Grass2> entity = make_unique<Grass2>();
				entity->setEntityID(id); ///id must be set before LoadConfig so loadAnim can get the right cache ID
				entity.get()->LoadConfig(thisConfig);
				if (additional_data.size() >= 1)
				{
					entity.get()->parseAdditionalData(additional_data);
				}

				if (!isCached[id])
				{
					//cacheEntity(id, entity.get()->all_swaps_img, entity.get()->animation_spritesheet, entity.get()->objects);
				}

				///To be replaced with param file
				entity.get()->entityType = Entity::EntityTypes::NEUTRAL;
				entity.get()->entityCategory = Entity::EntityCategories::NATURE;

				if (!cloneable)
					tangibleLevelObjects.push_back(std::move(entity));

				break;
			}
			case 5:
			{
				unique_ptr<DroppedItem> entity = make_unique<DroppedItem>();
				entity->setEntityID(id); ///id must be set before LoadConfig so loadAnim can get the right cache ID
				entity.get()->LoadConfig(thisConfig);

				if (!isCached[id])
				{
					//cacheEntity(id, entity.get()->all_swaps_img, entity.get()->animation_spritesheet, entity.get()->objects);
				}

				///To be replaced with param file
				entity.get()->entityType = Entity::EntityTypes::NEUTRAL;
				entity.get()->entityCategory = Entity::EntityCategories::MISC;

				string spritesheet = additional_data["spritesheet"];
				int spritesheet_id = additional_data["spritesheet_id"];
				string picked_item = additional_data["picked_item"];

				entity->spritesheet = spritesheet;
				entity->spritesheet_id = spritesheet_id;
				entity->picked_item = picked_item;

				if (!cloneable)
					tangibleLevelObjects.push_back(std::move(entity));

				break;
			}
			case 6:
			{
				unique_ptr<Kirajin_Yari_1> entity = make_unique<Kirajin_Yari_1>();
				entity->setEntityID(id); ///id must be set before LoadConfig so loadAnim can get the right cache ID
				entity.get()->LoadConfig(thisConfig);
				if (additional_data.size() >= 1)
				{
					entity.get()->parseAdditionalData(additional_data);
				}

				if (!isCached[id])
				{
					//cacheEntity(id, entity.get()->all_swaps_img, entity.get()->animation_spritesheet, entity.get()->objects);
				}

				///To be replaced with param file
				entity.get()->entityType = Entity::EntityTypes::HOSTILE;
				entity.get()->entityCategory = Entity::EntityCategories::ENEMYUNIT;

				if (!cloneable)
					tangibleLevelObjects.push_back(std::move(entity));

				break;
			}
			case 7:
			{
				unique_ptr<TreasureChest> entity = make_unique<TreasureChest>();
				entity->setEntityID(id); ///id must be set before LoadConfig so loadAnim can get the right cache ID
				entity.get()->LoadConfig(thisConfig);
				if (additional_data.size() >= 1)
				{
					entity.get()->parseAdditionalData(additional_data);
				}

				if (!isCached[id])
				{
					//cacheEntity(id, entity.get()->all_swaps_img, entity.get()->animation_spritesheet, entity.get()->objects);
				}

				///To be replaced with param file
				entity.get()->entityType = Entity::EntityTypes::HOSTILE;
				entity.get()->entityCategory = Entity::EntityCategories::OBSTACLE_IRON;

				if (!cloneable)
					tangibleLevelObjects.push_back(std::move(entity));

				break;
			}
			case 8:
			{
				unique_ptr<RockBig> entity = make_unique<RockBig>();
				entity->setEntityID(id); ///id must be set before LoadConfig so loadAnim can get the right cache ID
				entity.get()->LoadConfig(thisConfig);
				if (additional_data.size() >= 1)
				{
					entity.get()->parseAdditionalData(additional_data);
				}

				if (!isCached[id])
				{
					//cacheEntity(id, entity.get()->all_swaps_img, entity.get()->animation_spritesheet, entity.get()->objects);
				}

				///To be replaced with param file
				entity.get()->entityType = Entity::EntityTypes::HOSTILE;
				entity.get()->entityCategory = Entity::EntityCategories::OBSTACLE_ROCK;

				if (!cloneable)
					tangibleLevelObjects.push_back(std::move(entity));

				break;
			}
			case 9:
			{
				unique_ptr<RockSmall> entity = make_unique<RockSmall>();
				entity->setEntityID(id); ///id must be set before LoadConfig so loadAnim can get the right cache ID
				entity.get()->LoadConfig(thisConfig);
				if (additional_data.size() >= 1)
				{
					entity.get()->parseAdditionalData(additional_data);
				}

				if (!isCached[id])
				{
					//cacheEntity(id, entity.get()->all_swaps_img, entity.get()->animation_spritesheet, entity.get()->objects);
				}

				///To be replaced with param file
				entity.get()->entityType = Entity::EntityTypes::HOSTILE;
				entity.get()->entityCategory = Entity::EntityCategories::OBSTACLE_ROCK;

				if (!cloneable)
					tangibleLevelObjects.push_back(std::move(entity));

				break;
			}
			case 10:
			{
				unique_ptr<WoodenSpikes> entity = make_unique<WoodenSpikes>();
				entity->setEntityID(id); ///id must be set before LoadConfig so loadAnim can get the right cache ID
				entity.get()->LoadConfig(thisConfig);
				if (additional_data.size() >= 1)
				{
					entity.get()->parseAdditionalData(additional_data);
				}

				if (!isCached[id])
				{
					//cacheEntity(id, entity.get()->all_swaps_img, entity.get()->animation_spritesheet, entity.get()->objects);
				}

				///To be replaced with param file
				entity.get()->entityType = Entity::EntityTypes::HOSTILE;
				entity.get()->entityCategory = Entity::EntityCategories::OBSTACLE_WOOD;

				if (!cloneable)
					tangibleLevelObjects.push_back(std::move(entity));

				break;
			}
			case 11:
			{
				unique_ptr<RockPile> entity = make_unique<RockPile>();
				entity->setEntityID(id); ///id must be set before LoadConfig so loadAnim can get the right cache ID
				entity.get()->LoadConfig(thisConfig);
				if (additional_data.size() >= 1)
				{
					entity.get()->parseAdditionalData(additional_data);
				}

				if (!isCached[id])
				{
					//cacheEntity(id, entity.get()->all_swaps_img, entity.get()->animation_spritesheet, entity.get()->objects);
				}

				///To be replaced with param file
				entity.get()->entityType = Entity::EntityTypes::HOSTILE;
				entity.get()->entityCategory = Entity::EntityCategories::OBSTACLE_ROCK;

				if (!cloneable)
					tangibleLevelObjects.push_back(std::move(entity));

				break;
			}
			case 12:
			{
				unique_ptr<KirajinHut> entity = make_unique<KirajinHut>();
				entity->setEntityID(id); ///id must be set before LoadConfig so loadAnim can get the right cache ID
				entity.get()->LoadConfig(thisConfig);
				if (additional_data.size() >= 1)
				{
					entity.get()->parseAdditionalData(additional_data);
				}

				if (!isCached[id])
				{
					//cacheEntity(id, entity.get()->all_swaps_img, entity.get()->animation_spritesheet, entity.get()->objects);
				}

				///To be replaced with param file
				entity.get()->entityType = Entity::EntityTypes::HOSTILE;
				entity.get()->entityCategory = Entity::EntityCategories::BUILDING_REGULAR;

				if (!cloneable)
					tangibleLevelObjects.push_back(std::move(entity));

				break;
			}
			case 13:
			{
				unique_ptr<KirajinGuardTower> entity = make_unique<KirajinGuardTower>();
				entity->setEntityID(id); ///id must be set before LoadConfig so loadAnim can get the right cache ID
				entity.get()->LoadConfig(thisConfig);
				if (additional_data.size() >= 1)
				{
					entity.get()->parseAdditionalData(additional_data);
				}

				if (!isCached[id])
				{
					//cacheEntity(id, entity.get()->all_swaps_img, entity.get()->animation_spritesheet, entity.get()->objects);
				}

				///To be replaced with param file
				entity.get()->entityType = Entity::EntityTypes::HOSTILE;
				entity.get()->entityCategory = Entity::EntityCategories::BUILDING_REGULAR;

				if (!cloneable)
					tangibleLevelObjects.push_back(std::move(entity));

				break;
			}
			case 14:
			{
				unique_ptr<KirajinPoweredTowerSmall> entity = make_unique<KirajinPoweredTowerSmall>();
				entity->setEntityID(id); ///id must be set before LoadConfig so loadAnim can get the right cache ID
				entity.get()->LoadConfig(thisConfig);
				if (additional_data.size() >= 1)
				{
					entity.get()->parseAdditionalData(additional_data);
				}

				if (!isCached[id])
				{
					//cacheEntity(id, entity.get()->all_swaps_img, entity.get()->animation_spritesheet, entity.get()->objects);
				}

				///To be replaced with param file
				entity.get()->entityType = Entity::EntityTypes::HOSTILE;
				entity.get()->entityCategory = Entity::EntityCategories::BUILDING_IRON;

				if (!cloneable)
					tangibleLevelObjects.push_back(std::move(entity));

				break;
			}
			case 15:
			{
				unique_ptr<KirajinPoweredTowerBig> entity = make_unique<KirajinPoweredTowerBig>();
				entity->setEntityID(id); ///id must be set before LoadConfig so loadAnim can get the right cache ID
				entity.get()->LoadConfig(thisConfig);
				if (additional_data.size() >= 1)
				{
					entity.get()->parseAdditionalData(additional_data);
				}

				if (!isCached[id])
				{
					//cacheEntity(id, entity.get()->all_swaps_img, entity.get()->animation_spritesheet, entity.get()->objects);
				}

				///To be replaced with param file
				entity.get()->entityType = Entity::EntityTypes::HOSTILE;
				entity.get()->entityCategory = Entity::EntityCategories::BUILDING_IRON;

				if (!cloneable)
					tangibleLevelObjects.push_back(std::move(entity));

				break;
			}
			case 16:
			{
				unique_ptr<Kirajin_Yari_2> entity = make_unique<Kirajin_Yari_2>();
				entity->setEntityID(id); ///id must be set before LoadConfig so loadAnim can get the right cache ID
				entity.get()->LoadConfig(thisConfig);
				if (additional_data.size() >= 1)
				{
					entity.get()->parseAdditionalData(additional_data);
				}

				if (!isCached[id])
				{
					//cacheEntity(id, entity.get()->all_swaps_img, entity.get()->animation_spritesheet, entity.get()->objects);
				}

				///To be replaced with param file
				entity.get()->entityType = Entity::EntityTypes::HOSTILE;
				entity.get()->entityCategory = Entity::EntityCategories::ENEMYUNIT;

				if (!cloneable)
					tangibleLevelObjects.push_back(std::move(entity));
				else
					kirajins.push_back(std::move(entity));

				break;
			}
		}

		Entity* entity = new Entity;

		if(!cloneable)
		{
			entity = tangibleLevelObjects[tangibleLevelObjects.size()-1].get();
		}
		else
		{
			if(id == 16)
			entity = kirajins[kirajins.size()-1].get();
		}

		entity->spawnOrderID = spawnOrder;
		spawnOrder++;

		if(id != 5) ///ID 5 = dropped item, it has an exclusive loading type
		{
			entity->setEntityID(id);

			entity->floorY = ypos; ///in case Y gets overriden, save the position where the floor is

			if(xrange != 0)
			entity->setGlobalPosition(sf::Vector2f(xpos + (rand() % xrange), ypos));
			else
			entity->setGlobalPosition(sf::Vector2f(xpos, ypos));

			entity->setColor(color);

			entity->cloneable = cloneable;
			entity->respawnTime = clone_delay;

			entity->spawn_x = entity->getGlobalPosition().x;

			entity->isCollidable = collidable;
			entity->isAttackable = attackable;

			vector<Entity::Loot> new_loot;
			parseEntityLoot(v4Core->gen, roll, loot, new_loot);

			entity->loot_table = new_loot;
			entity->curHP = hp * mission_diff;
			entity->maxHP = hp * mission_diff;

			if(!entity->custom_dmg)
			{
				entity->mindmg = mindmg;
				entity->maxdmg = maxdmg;
			}

			entity->stat_multiplier = 1 + ((mission_diff - 1) * 0.333);

			if(additional_data.contains("hidden"))
			{
				entity->layer = -layer;
			}
			else
			{
				entity->layer = layer;
			}

			entity->parent = parent;
			entity->additional_data = additional_data;
		}
		else
		{
			entity->setEntityID(id);
			entity->manual_mode = true;

			///This unique entity needs to be loaded differently, read additional data for spritesheet info to be passed from the item registry.
			string spritesheet = additional_data["spritesheet"];
			int spritesheet_id = additional_data["spritesheet_id"];
			string picked_item = additional_data["picked_item"];

			cout << "[DroppedItem] Selecting spritesheet " << spritesheet << " with id " << spritesheet_id << endl;

			vector<Object>* obj;
			obj = entity->objects.get();

			cout << "[DroppedItem] Loading from memory" << endl;
			vector<char> di_data = droppeditem_spritesheet[spritesheet].retrieve_char();
			cout << "[DroppedItem] Vector loaded. Size: " << di_data.size() << endl;
			(*obj)[0].tex_obj.loadFromMemory(&di_data[0], di_data.size());
			cout << "[DroppedItem] Setting smooth" << endl;
			(*obj)[0].tex_obj.setSmooth(true);
			cout << "[DroppedItem] Setting texture" << endl;
			(*obj)[0].s_obj.setTexture((*obj)[0].tex_obj);
			cout << "[DroppedItem] Marking as unexported" << endl;
			(*obj)[0].exported = false;
			cout << "[DroppedItem] Loading done." << endl;

			(*obj)[0].s_obj.qualitySetting = qualitySetting;
			(*obj)[0].s_obj.resSetting = resSetting;

			//entity->(*objects)[0].s_obj.setOrigin(entity->(*objects)[0].s_obj.getLocalBounds().width/2, entity->(*objects)[0].s_obj.getLocalBounds().height/2);

			entity->cur_pos = float(spritesheet_id-1) / 60.0;

			entity->animation_bounds[0] = droppeditem_spritesheet[spritesheet].retrieve_rect_as_map();

			entity->setGlobalPosition(sf::Vector2f(xpos, ypos));

			entity->setColor(color);

			entity->isCollidable = collidable;
			entity->isAttackable = attackable;

			entity->layer = 9999;
			entity->parent = -1;
		}
	}

	cout << "[MissionController] Loading finished. Loading took " << bm.getElapsedTime().asSeconds() << " seconds" << endl;
}

void MissionController::spawnProjectile(PSprite& sprite, float xPos, float yPos, float speed, float hspeed, float vspeed, float angle, float max_dmg, float min_dmg, float crit, bool enemy)
{
	unique_ptr<Spear> p = make_unique<Spear>(sprite);

	p.get()->xPos = xPos;
	p.get()->yPos = yPos;
	p.get()->speed = speed;
	p.get()->hspeed = hspeed;
	p.get()->vspeed = vspeed;
	p.get()->angle = angle;
	p.get()->max_dmg = max_dmg;
	p.get()->min_dmg = min_dmg;
	p.get()->crit = crit;
	p.get()->enemy = enemy;

	levelProjectiles.push_back(std::move(p));
}

void MissionController::addPickedItem(std::string spritesheet, int spritesheet_id, std::string picked_item)
{
	cout << "MissionController::addPickedItem(" << spritesheet << ", " << spritesheet_id << ", " << picked_item << ")" << endl;

	if(picked_item != "potion_1" && picked_item != "potion_2") ///Check for potions
	{
		PickedItem tmp;
		tmp.circle.setFillColor(sf::Color(255,255,255,192));
		//tmp.circle.setRadius(50 * resRatioX);
		///set radius in draw loop to get appropriate resratiox size
		tmp.item_name = picked_item;

		///This unique entity needs to be loaded differently, read additional data for spritesheet info to be passed from the item registry.
		vector<char> di_data = droppeditem_spritesheet[spritesheet].retrieve_char();

		sf::Texture tex_obj;
		tex_obj.loadFromMemory(&di_data[0], di_data.size());
		tex_obj.setSmooth(true);

		tmp.item.setTexture(tex_obj);
		tmp.item.setTextureRect(droppeditem_spritesheet[spritesheet].retrieve_rect_as_map()[spritesheet_id-1]);
		tmp.bounds = sf::Vector2f(droppeditem_spritesheet[spritesheet].retrieve_rect_as_map()[spritesheet_id-1].width, droppeditem_spritesheet[spritesheet].retrieve_rect_as_map()[spritesheet_id-1].height);

		tmp.item.qualitySetting = qualitySetting;
		tmp.item.resSetting = resSetting;

		pickedItems.push_back(tmp);
	}
	else
	{
		float heal_factor = 0;

		if(picked_item == "potion_1")
		heal_factor = 0.2;
		if(picked_item == "potion_2")
		heal_factor = 0.5;

		for(int i=0; i<units.size(); i++)
		{
			PlayableUnit* unit = units[i].get();

			unit->current_hp += unit->max_hp*heal_factor;

			if(unit->current_hp >= unit->max_hp)
			unit->current_hp = unit->max_hp;

			cout << "Incrementing unit " << i << " hp by " << unit->max_hp*heal_factor << endl;
		}
	}
}

void MissionController::submitPickedItems()
{
	for(int i=0; i<pickedItems.size(); i++)
	{
		InventoryData::InventoryItem invItem;
		v4Core->saveReader.invData.addItem(v4Core->saveReader.itemReg.getItemByName(pickedItems[i].item_name)->order_id);
		if(pickedItems[i].item_name == "item_grubby_map") ///Grubby map
		{
			///Check if Patapine Grove missions doesnt exist, and if Patapine Grove is not unlocked already
			if((!v4Core->saveReader.isMissionUnlocked(3)) && (!v4Core->saveReader.isMissionUnlocked(2)) && (v4Core->saveReader.locations_unlocked == 1))
			{
				///Add first patapine mission and unlock second location
				v4Core->saveReader.missions_unlocked.push_back(2);
				v4Core->saveReader.locations_unlocked = 2;
			}
		}

		if(pickedItems[i].item_name == "item_digital_blueprint")
		{
			///Check if Ejiji Cliffs missions doesnt exist, and if Ejiji Cliffs is not unlocked already
			if((!v4Core->saveReader.isMissionUnlocked(5)) && (!v4Core->saveReader.isMissionUnlocked(4)) && (v4Core->saveReader.locations_unlocked == 2))
			{
				///Add first patapine mission and unlock second location
				v4Core->saveReader.missions_unlocked.push_back(4);
				v4Core->saveReader.locations_unlocked = 3;
			}
		}
	}

}

void MissionController::updateMissions()
{
	///When this function is called, the mission has been completed successfully
	cout << "MissionController::updateMissions(): " << curMissionID << endl;

	switch(curMissionID)
	{
		case 2:
		{
			if(!v4Core->saveReader.isMissionUnlocked(3))
			{
				v4Core->saveReader.missions_unlocked.push_back(3);
				v4Core->saveReader.mission_levels[3] = 2;

				auto it = std::find(v4Core->saveReader.missions_unlocked.begin(), v4Core->saveReader.missions_unlocked.end(), 2);
				v4Core->saveReader.missions_unlocked.erase(it);
			}

			break;
		}

		case 4:
		{
			if(!v4Core->saveReader.isMissionUnlocked(5))
			{
				v4Core->saveReader.missions_unlocked.push_back(5);
				v4Core->saveReader.mission_levels[5] = 2;

				auto it = std::find(v4Core->saveReader.missions_unlocked.begin(), v4Core->saveReader.missions_unlocked.end(), 4);
				v4Core->saveReader.missions_unlocked.erase(it);
			}

			break;
		}
	}

	if(v4Core->saveReader.mission_levels[curMissionID] != 0)
	v4Core->saveReader.mission_levels[curMissionID] += 1;
}

void MissionController::addUnitThumb(int unit_class)
{
	UnitThumb tmp;
	tmp.unit_class = unit_class;
	tmp.hpbar_back.loadFromFile("resources/graphics/mission/hpbar_back.png", qualitySetting, 1);
	tmp.hpbar_ins.loadFromFile("resources/graphics/mission/hpbar_ins.png", qualitySetting, 1);
	tmp.unit_count.createText(f_font, 26, sf::Color::White, "", qualitySetting, 1);
	tmp.unit_count_shadow.createText(f_font, 26, sf::Color::Black, "", qualitySetting, 1);
	tmp.width = tmp.hpbar_ins.getLocalBounds().width;
	unitThumbs.push_back(tmp);
}

void MissionController::Initialise(Config &config,std::string backgroundString,V4Core &_v4Core)
{
	v4Core = &_v4Core;
	//sf::Context context;

	PSprite ps_temp;
	ps_temp.loadFromFile("resources/graphics/item/icon/spear.png",1);
	ps_temp.setRepeated(false);
	ps_temp.setTextureRect(sf::IntRect(0,0,ps_temp.t.getSize().x,ps_temp.t.getSize().y)); ///affect later with ratio
	ps_temp.setOrigin(ps_temp.t.getSize().x,0);
	ps_temp.setColor(sf::Color(255,255,255,255));
	ps_temp.setPosition(0,0);

	s_proj = ps_temp;
	s_proj.scaleX=0.15f;
	s_proj.scaleY=0.15f;

	int q = config.GetInt("textureQuality");
	qualitySetting = q;
	resSetting = 1;

	dmg_spritesheet.load("resources/graphics/mission/damagesheet.png", q, 1);

	missionEnd = false;
	failure = false;

	//ctor
	f_font.loadFromFile(config.fontPath);

	if(config.fontPath == "resources/fonts/p4kakupop-pro.ttf")
	f_moji.loadFromFile("resources/fonts/mojipon.otf");
	else
	f_moji.loadFromFile(config.fontPath);

	f_unicode.loadFromFile("resources/fonts/p4kakupop-pro.ttf");

	//f_font.loadFromFile("resources/fonts/arial.ttf");
	t_timerMenu.setFont(f_font);
	t_timerMenu.setCharacterSize(38);
	t_timerMenu.setFillColor(sf::Color::White);
	//f_font.loadFromFile("resources/fonts/arial.ttf");
	//t_cutscene_text.setFont(f_font);

	//t_cutscene_text.setCharacterSize(35);
	//t_cutscene_text.setFillColor(sf::Color::White);
	//t_cutscene_text.setString(Func::ConvertToUtf8String(config.strRepo.GetUnicodeString(L"intro_cutscene_1")));
	//t_cutscene_text.setOrigin(t_cutscene_text.getGlobalBounds().width/2,t_cutscene_text.getGlobalBounds().height/2);
	thisConfig = &config;

	std::vector<std::string> commands;
	commands.push_back("patapata");
	commands.push_back("ponpon");
	commands.push_back("chakachaka");
	commands.push_back("ponchaka");
	commands.push_back("ponpata");
	commands.push_back("donchaka");
	commands.push_back("party");
	commands.push_back("dondon");
	commands.push_back("chakapata");
	command_padding = 720 * 0.2; ///Like 2% screen width or smth idk

	command_descs.clear();
	command_inputs.clear();

	std::vector<std::string> command_lang_keys = {"nav_onward", "nav_attack", "nav_defend", "nav_charge", "nav_retreat", "nav_jump", "nav_party", "nav_summon"};
	std::vector<sf::String> command_lang_buttons = {L"□-□-□-〇",L"〇-〇-□-〇",L"△-△-□-〇",L"〇-〇-△-△",L"〇-□-〇-□",L"×-×-△-△",L"□-〇-×-△",L"×-××-××"};

	///first four
	for(int i=0; i<4; i++)
	{

		PText t_command_desc;
		t_command_desc.createText(f_font, 28, sf::Color(128,128,128,255), Func::ConvertToUtf8String(thisConfig->strRepo.GetUnicodeString(command_lang_keys[i]))+":", qualitySetting, 1);
		command_descs.push_back(t_command_desc);

		PText t_command;
		t_command.createText(f_unicode, 28, sf::Color(128,128,128,255), command_lang_buttons[i], qualitySetting, 1);
		command_inputs.push_back(t_command);
	}

	///second four
	for(int i=4; i<8; i++)
	{

		PText t_command_desc;
		t_command_desc.createText(f_font, 28, sf::Color(128,128,128,255), Func::ConvertToUtf8String(thisConfig->strRepo.GetUnicodeString(command_lang_keys[i]))+":", qualitySetting, 1);
		command_descs.push_back(t_command_desc);

		PText t_command;
		t_command.createText(f_unicode, 28, sf::Color(128,128,128,255), command_lang_buttons[i], qualitySetting, 1);
		command_inputs.push_back(t_command);
	}


	isInitialized = true;
	// this is outside the loop
	startAlpha = 255;
	endAlpha = 0;
	targetTime = sf::seconds(10);


	fade.setPosition(sf::Vector2f(0,0));
	fade.setFillColor(sf::Color(0,0,0,0));
	fade.setSize(sf::Vector2f(800,600));
	currentCutsceneId=0;

	sb_win_jingle.loadFromFile("resources/sfx/level/victory.ogg");
	sb_lose_jingle.loadFromFile("resources/sfx/level/failure.ogg");

	sb_cheer1.loadFromFile("resources/sfx/level/cheer1.ogg");
	sb_cheer2.loadFromFile("resources/sfx/level/cheer2.ogg");
	sb_cheer3.loadFromFile("resources/sfx/level/cheer1.ogg");

	t_win.createText(f_moji, 56, sf::Color(222, 83, 0, 255), Func::ConvertToUtf8String(config.strRepo.GetUnicodeString(L"mission_complete")), q, 1);
	t_win_outline.createText(f_moji, 56, sf::Color(255, 171, 0, 255), Func::ConvertToUtf8String(config.strRepo.GetUnicodeString(L"mission_complete")), q, 1);
	t_win_outline.setOutlineColor(sf::Color(255, 171, 0, 255));
	t_win_outline.setOutlineThickness(10);
	t_lose.createText(f_moji, 56, sf::Color(138, 15, 26, 255), Func::ConvertToUtf8String(config.strRepo.GetUnicodeString(L"mission_failed")), q, 1);
	t_lose_outline.createText(f_moji, 56, sf::Color(254, 48, 55, 255),Func::ConvertToUtf8String(config.strRepo.GetUnicodeString(L"mission_failed")), q, 1);
	t_lose_outline.setOutlineColor(sf::Color(254, 48, 55, 255));
	t_lose_outline.setOutlineThickness(10);

	t_win.setOrigin(t_win.getLocalBounds().width/2, t_win.getLocalBounds().height/2);
	t_win_outline.setOrigin(t_win_outline.getLocalBounds().width/2, t_win_outline.getLocalBounds().height/2);

	t_lose.setOrigin(t_lose.getLocalBounds().width/2, t_lose.getLocalBounds().height/2);
	t_lose_outline.setOrigin(t_lose_outline.getLocalBounds().width/2, t_lose_outline.getLocalBounds().height/2);

	bar_win.loadFromFile("resources/graphics/mission/bar_win.png", q, 1);
	bar_lose.loadFromFile("resources/graphics/mission/bar_lose.png", q, 1);

	bar_win.setOrigin(bar_win.getLocalBounds().width/2, bar_win.getLocalBounds().height/2);
	bar_lose.setOrigin(bar_lose.getLocalBounds().width/2, bar_lose.getLocalBounds().height/2);

	ifstream sprdata("resources/graphics/item/itemdata/item_spritesheets.dat");
	string buff="";

	while(getline(sprdata, buff))
	{
		if(buff.find("#") == std::string::npos)
		{
			///Valid spritesheet. Load it
			cout << "[Item spritesheets] Loading spritesheet " << "resources/graphics/item/itemdata/"+buff+".png" << endl;
			droppeditem_spritesheet[buff].load("resources/graphics/item/itemdata/"+buff+".png", qualitySetting, resSetting);
		}
	}
	ctrlTips.create(110, f_font, 28, sf::String(L"Onward: □-□-□-O	   Attack: O-O-□-O		Defend: △-△-□-O			  Charge: O-O-△-△\nRetreat: O-□-O-□		  Jump: X-X-△-△		  Party: □-O-X-△		  Summon: X-XX-XX") ,q, sf::Color(128,128,128,255));

	spear_hit_enemy.loadFromFile("resources/sfx/level/spear_hit_enemy.ogg");
	spear_hit_iron.loadFromFile("resources/sfx/level/spear_hit_iron.ogg");
	spear_hit_rock.loadFromFile("resources/sfx/level/spear_hit_rock.ogg");
	spear_hit_solid.loadFromFile("resources/sfx/level/spear_hit_solid.ogg");
	s_heal.loadFromFile("resources/sfx/level/picked_heal.ogg");

	cout << "initialization finished" << endl;
}
void MissionController::StartMission(std::string missionFile, bool showCutscene, int missionID, float mission_multiplier)
{
	thisConfig->thisCore->saveToDebugLog("Starting mission");

	curMissionID = missionID;

	fade_alpha = 255;
	missionEnd = false;
	playJingle = false;
	textBounce = false;
	textCurX = -1280;
	barCurX = 1920;
	textDestX = 640;
	barDestX = 640;
	textCurScale = 1;
	textDestScale = 1;
	fade_alpha = 255;
	fadeout_alpha = 0;
	playCheer[0] = false;
	playCheer[1] = false;
	playCheer[2] = false;
	spawnOrder = 0;


	//sf::Context context;
	int quality = thisConfig->GetInt("textureQuality");
	float ratioX, ratioY;

	army_x = 0;
	camera.camera_x=480;
	showTimer = false;

	switch(quality)
	{
		case 0: ///low
		{
			ratioX = thisConfig->GetInt("resX") / float(640);
			ratioY = thisConfig->GetInt("resY") / float(360);
			break;
		}

		case 1: ///med
		{
			ratioX = thisConfig->GetInt("resX") / float(1280);
			ratioY = thisConfig->GetInt("resY") / float(720);
			break;
		}

		case 2: ///high
		{
			ratioX = thisConfig->GetInt("resX") / float(1920);
			ratioY = thisConfig->GetInt("resY") / float(1080);
			break;
		}

		case 3: ///ultra
		{
			ratioX = thisConfig->GetInt("resX") / float(3840);
			ratioY = thisConfig->GetInt("resY") / float(2160);
			break;
		}
	}

	patapon_y = 720 - 141;
	floor_y = 720 - 100;

	/**
	if(showCutscene)
	{
		cutscene_text_identifiers.push_back(L"intro_cutscene_1");
		cutscene_text_identifiers.push_back(L"intro_cutscene_2");
		cutscene_text_identifiers.push_back(L"intro_cutscene_3");
		cutscene_text_identifiers.push_back(L"intro_cutscene_4");
		cutscene_text_identifiers.push_back(L"intro_cutscene_5");
		cutscene_lengths.push_back(4);
		cutscene_lengths.push_back(4);
		cutscene_lengths.push_back(3);
		cutscene_lengths.push_back(3);
		cutscene_lengths.push_back(2);
		cutscene_blackscreens.push_back(true);
		cutscene_blackscreens.push_back(true);
		cutscene_blackscreens.push_back(true);
		cutscene_blackscreens.push_back(true);
		cutscene_blackscreens.push_back(true);
		currentCutsceneId=0;
		cutscenesLeft=true;
		isFinishedLoading=true;

	}
	else
	{
		inCutscene = false;
		cutscene_blackscreens.clear();
		cutscene_lengths.clear();
		cutscene_text_identifiers.clear();
		cutscenesLeft=false;
	}*/

	tangibleLevelObjects.clear();
	levelProjectiles.clear();

	string bgName; ///background
	string songName; ///bgm
	string missionName; ///rpc_name
	string missionImg; ///rpc_img
	string forceWeather; // Force Weather

	string buff;

	ifstream elist("resources/units/entitylist.dat");

	vector<string> entity_list;

	while(getline(elist, buff))
	{
		if(buff[0] != '#')
		{
			if(buff.size() > 0)
			{
				entity_list.push_back(buff.substr(buff.find_last_of(',')+1));
			}
		}
	}

	elist.close();

	ifstream mf("resources/missions/"+missionFile);
	cout << "Attempting to read a mission " << "resources/missions/" << missionFile << endl;

	bool accepted = false;
	float ver = 0.0;

	json mjson;
	mf >> mjson;

	mf.close();

	try
	{
		json mission_data = mjson["params"];

		missionName = mission_data["name"];
		bgName = mission_data["background"];
		songName = mission_data["bgm"];
		missionImg = mission_data["rich_presence"];
		if(mission_data.contains("force_weather"))
		{
			forceWeather = mission_data["force_weather"];
		}
		else
		{
			forceWeather = "";
		}
	}
	catch(const exception& e)
	{
	 	cerr << "[ERROR] An error occured while loading mission: resources/missions/" << missionFile << ". Error: " << e.what() << endl;
	 	// Somehow return to obelisk
	 	return;
	}
	
	try
	{
		json mission_cutscenes = mjson["cutscenes"];

		//for(int i = 0; i < mission_cutscenes.size(); i++)
		//{
		//	Cutscene temp;
		//	temp.load(mission_cutscenes);
		//	cutscenes.push_back(temp);
		//}
	}
	catch(const exception& e)
	{
		// Continue regardless of error,  cutscenes might not occur
	}

	try
	{
		json mission_entities = mjson["entities"];

		int layer_idx_buildings = 0;
		int layer_idx_nature = 0;
		int layer_idx_animals = 0;

		for(int i = 0; i < mission_entities.size(); i++)
		{
			json cur_entity = mission_entities[i];

			int id;
			bool collidable;
			bool attackable;

			if(cur_entity.contains("id"))
			{
				id = cur_entity["id"];
			}
			else
			{
				id = 2; // Kacheek by default
			}

			try
			{
				ifstream ent_default_params("resources/units/entity/" + entity_list[id] + ".p4p");
				json ent_defaults;
				ent_default_params >> ent_defaults;

				try
				{
					json ent_params = cur_entity["params"];

					float xpos;
					float xrange;
					bool cloneable;
					float clone_delay;
					float spawnrate;
					float stat_mult;
					int mindmg;
					int maxdmg;
					int hp;
					float ypos;
					float baseY;
					int alpha;
					json loot;
					int layer;
					int parent;
					sf::Color* color;
					json ent_custom_params; // update spawnEntity to handle json for this instead

					if(ent_params.contains("xpos")) { xpos = ent_params["xpos"]; } else { xpos = 1000; }
					if(ent_params.contains("xrange")) { xrange = ent_params["xrange"]; } else { xrange = 0; }
					if(ent_params.contains("cloneable")) { cloneable = ent_params["cloneable"]; } else { cloneable = false; }
					if(ent_params.contains("clone_delay")) { clone_delay = ent_params["clone_delay"]; } else { clone_delay = 10; }
					if(ent_params.contains("spawnrate")) { spawnrate = ent_params["spawnrate"]; } else { spawnrate = 100; }
					if(ent_params.contains("stat_mult")) { stat_mult = ent_params["stat_mult"]; } else { stat_mult = 1; }

					if(ent_params.contains("mindmg"))
					{
						mindmg = ent_params["mindmg"];
					}
					else
					{
						if(ent_defaults.contains("mindmg"))
						{
							mindmg = ent_defaults["mindmg"];
						}
						else
						{
							mindmg = 1;
						}
					}
					if(ent_params.contains("maxdmg"))
					{
						maxdmg = ent_params["maxdmg"];
					}
					else
					{
						if(ent_defaults.contains("maxdmg"))
						{
							maxdmg = ent_defaults["maxdmg"];
						}
						else
						{
							maxdmg = 1;
						}
					}

					if(ent_params.contains("hp")) { hp = ent_params["hp"]; } else { hp = ent_defaults["baseHP"]; }
					if(ent_params.contains("ypos")) { ypos = ent_params["ypos"]; } else { ypos = ent_defaults["baseY"]; }
					if(ent_defaults.contains("baseY")) { baseY = ent_defaults["baseY"]; } else { baseY = patapon_y; };
					if(ent_params.contains("alpha")) { alpha = ent_params["alpha"]; } else { alpha = 255; }
					if(ent_params.contains("color"))
					{
						string clr = ent_params["color"];
						int r = stoi(clr.substr(0, 2), 0, 16);
						int g = stoi(clr.substr(2, 2), 0, 16);
						int b = stoi(clr.substr(4, 2), 0, 16);
						color = new sf::Color(r, g, b, alpha);
					}
					else
					{ 
						color = new sf::Color(255, 255, 255, 255);
					}
					if(ent_params.contains("layer"))
					{
						switch(layerStr2Enum(ent_params["layer"]))
						{
							case BUILDINGS:
							{
								if(layer_idx_buildings >= 50) // 0-49
								{
									layer_idx_buildings = 0;
								}
								layer = BUILDINGS + layer_idx_buildings;
								layer_idx_buildings++;
								break;
							}
							case NATURE:
							{
								if(layer_idx_nature >= 100) // 50-149
								{
									layer_idx_nature = 0;
								}
								layer = NATURE + layer_idx_nature;
								layer_idx_nature++;
								break;
							}
							default: case ANIMALS:
							{
								if(layer_idx_animals >= 500) // 150-649
								{
									layer_idx_animals = 0;
								}
								layer = ANIMALS + layer_idx_animals;
								layer_idx_animals++;
								break;
							}
							case UI:
							{
								layer = 9999;
								break;
							}
						}
					}
					else
					{
						if(layer_idx_animals >= 500) // 150-649
						{
							layer_idx_animals = 0;
						}
						layer = ANIMALS + layer_idx_animals;
						layer_idx_animals++;
					}

					if(ent_params.contains("parent")) { parent = ent_params["parent"]; } else { parent = -1; }
					
					if(ent_params.contains("loot"))
					{
						loot = ent_params["loot"];
					}
					else
					{
						loot = ent_defaults["loot"];
					}
					
					collidable = ent_defaults["collidable"];
					attackable = ent_defaults["attackable"];

					if(ent_params.contains("custom_params"))
					{
						ent_custom_params = ent_params["custom_params"];
					}
					else
					{
						// Do nothing
					}

					cout << "[MissionController] Spawning an entity: " << id << endl;
					spawnEntity(id, collidable, attackable, xpos, xrange, cloneable, clone_delay, spawnrate, stat_mult, mindmg, maxdmg, hp, ypos, baseY, *color, layer, parent, loot, ent_custom_params);
				}
				catch(const exception& e)
				{
					cerr << "[ERROR] An error occured while loading mission entity params from: resources/missions/" << missionFile << ". Error: " << e.what() << endl;
				}

				ent_default_params.close();
			}
			catch(const exception& e)
			{
				cerr << "[ERROR] An error occured while loading default entity params from: resources/units/entity/" << entity_list[id] << ". Error: " << e.what() << endl;
			}
		}
	}
	catch(const exception& e)
	{
		cerr << "[ERROR] An error occured while loading mission entities from: resources/missions/" << missionFile << ". Error: " << e.what() << endl;
	}

	///make this unit load based on how the army is built later
	int army_size = v4Core->saveReader.ponReg.pons.size();

	unique_ptr<Hatapon> wip_hatapon = make_unique<Hatapon>(); ///Hatapon's a special snowflake and isn't in the saveReader.ponreg.pons vector lol
	wip_hatapon.get()->LoadConfig(thisConfig);
	wip_hatapon.get()->setUnitID(0);
	units.push_back(std::move(wip_hatapon));
	addUnitThumb(0);

	for(int i = 0; i < army_size; i++)
	{
		cout << "[DEBUG] Trying to find pon: " << i << endl;
		Pon* current_pon = v4Core->saveReader.ponReg.GetPonByID(i);
		cout << "[DEBUG] Making pon with class: " << current_pon->pon_class << endl;
		switch(current_pon->pon_class)
		{
			case -1: ///this was earlier 0 which i dont understand because pon class 0 = yaripon lol
			{
				cout << "[DEBUG] What? Hatapon detected in saveReader.ponreg.pons" << endl;
				break;
			}
			case 0:
			{
				unique_ptr<Yaripon> wip_pon = make_unique<Yaripon>();
				wip_pon.get()->LoadConfig(thisConfig);
				wip_pon.get()->setUnitID(current_pon->pon_class+1); ///have to set unit ID from 0 to 1 because 0 is already occupied by Hatapon
				wip_pon.get()->mindmg = current_pon->pon_min_dmg;
				wip_pon.get()->maxdmg = current_pon->pon_max_dmg;
				wip_pon.get()->current_hp = current_pon->pon_hp;
				wip_pon.get()->max_hp = current_pon->pon_hp;

				cout << "Checking equipment slots: ";
				for(int s=0; s<current_pon->slots.size(); s++)
				cout << current_pon->slots[s] << " ";

				cout << endl;

				if(current_pon->slots[0] != -1)
				wip_pon.get()->applyEquipment(v4Core->saveReader.invData.items[current_pon->slots[0]].item->order_id, 0);
				else
				cout << "[ERROR] Yaripon has an empty equipment slot 1" << endl;

				if(current_pon->slots[1] != -1)
				wip_pon.get()->applyEquipment(v4Core->saveReader.invData.items[current_pon->slots[1]].item->order_id, 1);
				else
				cout << "[ERROR] Yaripon has an empty equipment slot 2" << endl;

				units.push_back(std::move(wip_pon));
				break;
			}
			case 2:
			{
				/*
				unique_ptr<Tatepon> wip_pon = make_unique<Tatepon>();
				wip_pon.get()->LoadConfig(thisConfig);
				wip_pon.get()->setUnitID(current_pon->pon_class);
				wip_pon.get()->mindmg = current_pon->pon_min_dmg;
				wip_pon.get()->maxdmg = current_pon->pon_max_dmg;
				wip_pon.get()->current_hp = current_pon->pon_hp;
				wip_pon.get()->max_hp = current_pon->pon_hp;
				wip_pon.get()->applyEquipment(v4Core->saveReader.invData.items[current_pon->slots[0]].item->order_id, 0);
				wip_pon.get()->applyEquipment(v4Core->saveReader.invData.items[current_pon->slots[1]].item->order_id, 1);
				units.push_back(std::move(wip_pon));
				break;*/
				break;
			}
			/*case 3:
			{
				unique_ptr<Yumipon> wip_pon = make_unique<Yumipon>();
				break;
			}
			case 4:
			{
				unique_ptr<Hero> wip_pon = make_unique<Hero>();
				break;
			}*/ ///For later lol
		}
		/*if(current_pon->pon_class != 0)
		{
			wip_pon.get()->LoadConfig(thisConfig);
			wip_pon.get()->setUnitID(current_pon->pon_class);
			wip_pon.get()->mindmg = current_pon->pon_min_dmg;
			wip_pon.get()->maxdmg = current_pon->pon_max_dmg;
			wip_pon.get()->current_hp = current_pon->pon_hp;
			wip_pon.get()->max_hp = current_pon->pon_hp;
			wip_pon.get()->applyWeapon(v4Core->saveReader.invData.GetItemByInvID(current_pon->weapon_invItem_id).item->item_type, v4Core->saveReader.invData.GetItemByInvID(current_pon->weapon_invItem_id).item->item_id, 1);
			wip_pon.get()->applyHelm(v4Core->saveReader.invData.GetItemByInvID(current_pon->armour_invItem_id).item->item_id);
			units.push_back(std::move(wip_pon));
		}*/ ///-_-

		bool has_thumb = false;
		for(int i = 0; i < unitThumbs.size(); i++)
		{
			if(unitThumbs[i].unit_class == current_pon->pon_class) has_thumb = true;
		}
		if(!has_thumb)
		{
			addUnitThumb(current_pon->pon_class);
		}
	}

	cout << "Loading background " << bgName << endl;
	Background bg_new;
	test_bg = bg_new;

	test_bg.Load(bgName, *thisConfig);//config.GetString("debugBackground"));

	cout << "Set rich presence to " << missionImg << endl;

	string fm = "Playing mission: "+missionName;
	v4Core->changeRichPresence(fm.c_str(), missionImg.c_str(), "logo");

	rhythm.config = thisConfig;
	rhythm.LoadTheme(songName); // thisConfig->GetString("debugTheme")
	missionTimer.restart();

	cout << "MissionController::StartMission(): finished" << endl;
	thisConfig->thisCore->saveToDebugLog("Mission loading finished.");

	isFinishedLoading=true;
	v4Core->loadingWaitForKeyPress();

	rhythm.Start();
}
void MissionController::StopMission()
{
	rhythm.Stop();
	isInitialized = false;
}
void MissionController::DoKeyboardEvents(sf::RenderWindow &window, float fps, InputController& inputCtrl)
{
	/**
	if(sf::Keyboard::isKeyPressed(sf::Keyboard::Num5))
	{
		missionEnd = true;
	}

	if(sf::Keyboard::isKeyPressed(sf::Keyboard::M))
	{
		if(!debug_map_drop)
		{
			auto item = v4core->saveReader.itemreg.GetItemByID(23);
			vector<string> data = {item->spritesheet, to_string(item->spritesheet_id), to_string(23)};

			spawnEntity("droppeditem",5,0,500,0,600,0,0,1,sf::Color::White,0,0,vector<Entity::Loot>(), data);

			debug_map_drop = true;
		}
	}

	if(sf::Keyboard::isKeyPressed(sf::Keyboard::L))
	{
		for(int i=0; i<units.size(); i++)
		{

				units[i].get()->setUnitHP(units[i].get()->getUnitHP() - (25.0/fps));

				if(units[i].get()->getUnitHP() <= 0)
				{
					units[i].get()->setUnitHP(0);
				}
		}
	}**/

	if(!missionEnd)
	{
		if((inputCtrl.isKeyHeld(InputController::Keys::LTRIGGER)) && (inputCtrl.isKeyHeld(InputController::Keys::RTRIGGER)) && (inputCtrl.isKeyHeld(InputController::Keys::SQUARE)))
		{
			if(inputCtrl.isKeyPressed(InputController::Keys::SELECT))
			{
				std::vector<sf::String> a = {"Show hitboxes","Hide hitboxes","Heal units"};

				PataDialogBox db;
				db.Create(f_font, "Debug menu", a, thisConfig->GetInt("textureQuality"));
				db.id = 999;
				dialog_boxes.push_back(db);
			}
		}
		else if(inputCtrl.isKeyPressed(InputController::Keys::START))
		{
			std::vector<sf::String> a = {Func::ConvertToUtf8String(thisConfig->strRepo.GetUnicodeString(L"nav_yes")),Func::ConvertToUtf8String(thisConfig->strRepo.GetUnicodeString(L"nav_no"))};

			PataDialogBox db;
			db.Create(f_font, Func::ConvertToUtf8String(thisConfig->strRepo.GetUnicodeString(L"mission_backtopatapolis")), a, thisConfig->GetInt("textureQuality"));
			dialog_boxes.push_back(db);
		}
	}
}

vector<MissionController::CollisionEvent> MissionController::DoCollisionForObject(HitboxFrame* currentObjectHitBoxFrame,float currentObjectX,float currentObjectY,int collisionObjectID,vector<string> collisionData)
{
	///Added vector because there can be more than just one collision events! Like colliding with grass AND with opponent
	vector<MissionController::CollisionEvent> collisionEvents;

	for(int i=0; i<tangibleLevelObjects.size(); i++)
	{
		for(int h=0; h<tangibleLevelObjects[i]->hitboxes.size(); h++)
		{
			//cout << "tangibleLevelObjects[" << i << "][" << h << "]" << endl;

			/// NEW COLLISION SYSTEM:
			/// Separating axis theorem
			/// we check an axis at a time
			/// 8 axes in total, aligned with the normal of each face of each shape
			/// thankfully because we are only using rectangles, there are two pairs of parallel sides
			/// so we only need to check 4 axes, as the other 4 are all parallel.
			///
			/// in each axis we calculate the vector projection onto the axis between the origin and each corner of each box
			/// and find the maximum projection and minimum projection for each shape
			/// then we check if min2>max1 or min1>max2 there has been a collision in this axis
			/// there has to be a collision in ALL axes for actual collision to be confirmed,
			/// so we can stop checking if we find a single non-collision.




			/// axis 1: obj1 "sideways" We start with sideways because it is less likely to contain a collision
			//cout<<"Collision step for projectile at X: "<<currentObjectX<<" against object at x: "<<tangibleLevelObjects[i]->global_x<<endl;

			float currentAxisAngle = currentObjectHitBoxFrame->rotation;
			vector<sf::Vector2f> currentVertices = currentObjectHitBoxFrame->getCurrentVertices();
			PVector pv1 = PVector::getVectorCartesian(0,0,currentVertices[0].x+currentObjectX,currentVertices[0].y+currentObjectY);
			PVector pv2 = PVector::getVectorCartesian(0,0,currentVertices[1].x+currentObjectX,currentVertices[1].y+currentObjectY);
			PVector pv3 = PVector::getVectorCartesian(0,0,currentVertices[2].x+currentObjectX,currentVertices[2].y+currentObjectY);
			PVector pv4 = PVector::getVectorCartesian(0,0,currentVertices[3].x+currentObjectX,currentVertices[3].y+currentObjectY);

			pv1.angle =-atan2(currentVertices[0].y+currentObjectY, currentVertices[0].x+currentObjectX);
			pv2.angle =-atan2(currentVertices[1].y+currentObjectY, currentVertices[1].x+currentObjectX);
			pv3.angle =-atan2(currentVertices[2].y+currentObjectY, currentVertices[2].x+currentObjectX);
			pv4.angle =-atan2(currentVertices[3].y+currentObjectY, currentVertices[3].x+currentObjectX);

			float proj1 = pv1.GetScalarProjectionOntoAxis(currentAxisAngle);
			float proj2 = pv2.GetScalarProjectionOntoAxis(currentAxisAngle);
			float proj3 = pv3.GetScalarProjectionOntoAxis(currentAxisAngle);
			float proj4 = pv4.GetScalarProjectionOntoAxis(currentAxisAngle);

			CollidableObject* target = tangibleLevelObjects[i].get();

			bool isCollision = DoCollisionStepInAxis(currentAxisAngle,&(target->hitboxes[h].hitboxObject),target,currentObjectHitBoxFrame,currentObjectX,currentObjectY);
			if (!isCollision)
			{
				continue;
			}
			//cout<<"COLLISION FOUND IN axis 1"<<endl;

			/// axis 2: obj1 "up"
			currentAxisAngle = 3.14159265358/2+currentObjectHitBoxFrame->rotation;
			bool isCollision2 = DoCollisionStepInAxis(currentAxisAngle,&(target->hitboxes[h].hitboxObject),target,currentObjectHitBoxFrame,currentObjectX,currentObjectY);
			if (!isCollision2)
			{
				continue;
			}
			//cout<<"COLLISION FOUND IN axis 2 (up)"<<endl;

			/// axis 3: obj2 "up" (we add the 90 degrees from before to its current rotation)
			currentAxisAngle = target->hitboxes[h].hitboxObject.rotation + 3.14159265358/2;

			bool isCollision3 = DoCollisionStepInAxis(currentAxisAngle,&(target->hitboxes[h].hitboxObject),target,currentObjectHitBoxFrame,currentObjectX,currentObjectY);
			if (!isCollision3)
			{
				continue;
			}
			//cout<<"COLLISION FOUND IN axis 3 (up2)"<<endl;

			/// axis 4: obj2 "sideways"
			currentAxisAngle = target->hitboxes[h].hitboxObject.rotation;

			bool isCollision4 = DoCollisionStepInAxis(currentAxisAngle,&(target->hitboxes[h].hitboxObject),target,currentObjectHitBoxFrame,currentObjectX,currentObjectY);
			if (!isCollision4)
			{
				continue;
			}

			/// we have a collision
			if (isCollision&&isCollision2&&isCollision3&&isCollision4)
			{
				Entity* entity = tangibleLevelObjects[i].get();

				//std::cout << "[COLLISION_SYSTEM]: Found a collision"<<endl;
				CollisionEvent cevent;
				cevent.collided = true;
				//cevent.collidedEntityID = -1;
				cevent.isAttackable = target->isAttackable;
				cevent.isCollidable = target->isCollidable;
				cevent.collidedEntityCategory = entity->entityCategory;

				target->OnCollide(target, collisionObjectID, collisionData);

				collisionEvents.push_back(cevent);
			}
			else
			{
				cout<<"Something is very wrong"<<endl;
			}
		}
	}

	return collisionEvents;
}

vector<MissionController::CollisionEvent> MissionController::DoCollisionForUnit(HitboxFrame* currentObjectHitBoxFrame,float currentObjectX,float currentObjectY,int collisionObjectID,vector<string> collisionData)
{
	///Added vector because there can be more than just one collision events! Like colliding with grass AND with opponent
	vector<MissionController::CollisionEvent> collisionEvents;

	for(int i=0; i<units.size(); i++)
	{
		for(int h=0; h<units[i]->hitboxes.size(); h++)
		{
			//cout << "tangibleLevelObjects[" << i << "][" << h << "]" << endl;

			/// NEW COLLISION SYSTEM:
			/// Separating axis theorem
			/// we check an axis at a time
			/// 8 axes in total, aligned with the normal of each face of each shape
			/// thankfully because we are only using rectangles, there are two pairs of parallel sides
			/// so we only need to check 4 axes, as the other 4 are all parallel.
			///
			/// in each axis we calculate the vector projection onto the axis between the origin and each corner of each box
			/// and find the maximum projection and minimum projection for each shape
			/// then we check if min2>max1 or min1>max2 there has been a collision in this axis
			/// there has to be a collision in ALL axes for actual collision to be confirmed,
			/// so we can stop checking if we find a single non-collision.




			/// axis 1: obj1 "sideways" We start with sideways because it is less likely to contain a collision
			//cout<<"Collision step for projectile at X: "<<currentObjectX<<" against object at x: "<<tangibleLevelObjects[i]->global_x<<endl;

			float currentAxisAngle = currentObjectHitBoxFrame->rotation;
			vector<sf::Vector2f> currentVertices = currentObjectHitBoxFrame->getCurrentVertices();
			PVector pv1 = PVector::getVectorCartesian(0,0,currentVertices[0].x+currentObjectX,currentVertices[0].y+currentObjectY);
			PVector pv2 = PVector::getVectorCartesian(0,0,currentVertices[1].x+currentObjectX,currentVertices[1].y+currentObjectY);
			PVector pv3 = PVector::getVectorCartesian(0,0,currentVertices[2].x+currentObjectX,currentVertices[2].y+currentObjectY);
			PVector pv4 = PVector::getVectorCartesian(0,0,currentVertices[3].x+currentObjectX,currentVertices[3].y+currentObjectY);

			pv1.angle =-atan2(currentVertices[0].y+currentObjectY, currentVertices[0].x+currentObjectX);
			pv2.angle =-atan2(currentVertices[1].y+currentObjectY, currentVertices[1].x+currentObjectX);
			pv3.angle =-atan2(currentVertices[2].y+currentObjectY, currentVertices[2].x+currentObjectX);
			pv4.angle =-atan2(currentVertices[3].y+currentObjectY, currentVertices[3].x+currentObjectX);

			float proj1 = pv1.GetScalarProjectionOntoAxis(currentAxisAngle);
			float proj2 = pv2.GetScalarProjectionOntoAxis(currentAxisAngle);
			float proj3 = pv3.GetScalarProjectionOntoAxis(currentAxisAngle);
			float proj4 = pv4.GetScalarProjectionOntoAxis(currentAxisAngle);

			CollidableObject* target = units[i].get();

			bool isCollision = DoCollisionStepInAxis(currentAxisAngle,&(target->hitboxes[h].hitboxObject),target,currentObjectHitBoxFrame,currentObjectX,currentObjectY);
			if (!isCollision)
			{
				continue;
			}
			//cout<<"COLLISION FOUND IN axis 1"<<endl;

			/// axis 2: obj1 "up"
			currentAxisAngle = 3.14159265358/2+currentObjectHitBoxFrame->rotation;
			bool isCollision2 = DoCollisionStepInAxis(currentAxisAngle,&(target->hitboxes[h].hitboxObject),target,currentObjectHitBoxFrame,currentObjectX,currentObjectY);
			if (!isCollision2)
			{
				continue;
			}
			//cout<<"COLLISION FOUND IN axis 2 (up)"<<endl;

			/// axis 3: obj2 "up" (we add the 90 degrees from before to its current rotation)
			currentAxisAngle = target->hitboxes[h].hitboxObject.rotation + 3.14159265358/2;

			bool isCollision3 = DoCollisionStepInAxis(currentAxisAngle,&(target->hitboxes[h].hitboxObject),target,currentObjectHitBoxFrame,currentObjectX,currentObjectY);
			if (!isCollision3)
			{
				continue;
			}
			//cout<<"COLLISION FOUND IN axis 3 (up2)"<<endl;

			/// axis 4: obj2 "sideways"
			currentAxisAngle = target->hitboxes[h].hitboxObject.rotation;

			bool isCollision4 = DoCollisionStepInAxis(currentAxisAngle,&(target->hitboxes[h].hitboxObject),target,currentObjectHitBoxFrame,currentObjectX,currentObjectY);
			if (!isCollision4)
			{
				continue;
			}

			/// we have a collision
			if (isCollision&&isCollision2&&isCollision3&&isCollision4)
			{
				//std::cout << "[COLLISION_SYSTEM]: Found a collision"<<endl;
				PlayableUnit* unit = units[i].get();

				CollisionEvent cevent;
				cevent.collided = true;
				//cevent.collidedEntityID = -1;
				cevent.isAttackable = target->isAttackable;
				cevent.isCollidable = target->isCollidable;
				cevent.collidedEntityCategory = 2;

				if(unit->defend)
				{
					if(unit->charged)
					cevent.defend_factor = 0.25;
					else
					cevent.defend_factor = 0.5;
				}

				target->OnCollide(target, collisionObjectID, collisionData);

				collisionEvents.push_back(cevent);
			}
			else
			{
				cout<<"Something is very wrong"<<endl;
			}
		}
	}

	return collisionEvents;
}

float MissionController::pataponMaxProjection(float axisAngle, int id)
{
	PlayableUnit* target = units[id].get();

	float currentAxisAngle = 0;
	HitboxFrame tmp = target->hitboxes[0].getRect();

	std::vector<sf::Vector2f> currentVertices = tmp.getCurrentVertices();

	PVector pv1 = PVector::getVectorCartesian(0,0,currentVertices[0].x+target->getGlobalPosition().x,currentVertices[0].y+target->getGlobalPosition().y);
	PVector pv2 = PVector::getVectorCartesian(0,0,currentVertices[1].x+target->getGlobalPosition().x,currentVertices[1].y+target->getGlobalPosition().y);
	PVector pv3 = PVector::getVectorCartesian(0,0,currentVertices[2].x+target->getGlobalPosition().x,currentVertices[2].y+target->getGlobalPosition().y);
	PVector pv4 = PVector::getVectorCartesian(0,0,currentVertices[3].x+target->getGlobalPosition().x,currentVertices[3].y+target->getGlobalPosition().y);

	pv1.angle =-atan2(currentVertices[0].y+target->getGlobalPosition().y, currentVertices[0].x+target->getGlobalPosition().x);
	pv2.angle =-atan2(currentVertices[1].y+target->getGlobalPosition().y, currentVertices[1].x+target->getGlobalPosition().x);
	pv3.angle =-atan2(currentVertices[2].y+target->getGlobalPosition().y, currentVertices[2].x+target->getGlobalPosition().x);
	pv4.angle =-atan2(currentVertices[3].y+target->getGlobalPosition().y, currentVertices[3].x+target->getGlobalPosition().x);

	float proj1 = pv1.GetScalarProjectionOntoAxis(axisAngle);
	float proj2 = pv2.GetScalarProjectionOntoAxis(axisAngle);
	float proj3 = pv3.GetScalarProjectionOntoAxis(axisAngle);
	float proj4 = pv4.GetScalarProjectionOntoAxis(axisAngle);

	/*if(axisAngle!=0){
		cout<<"NEW MAX TEST"<<endl;
		cout<<"Angle: "<<pv1.angle<<" distance: "<<pv1.distance<<" current X: "<<currentVertices[0].x+target->x<<" current Y: "<<currentVertices[0].y+target->y<<" proj: "<<proj1<<endl;
		cout<<"Angle: "<<pv2.angle<<" distance: "<<pv2.distance<<" current X: "<<currentVertices[1].x+target->x<<" current Y: "<<currentVertices[1].y+target->y<<" proj: "<<proj2<<endl;
		cout<<"Angle: "<<pv3.angle<<" distance: "<<pv3.distance<<" current X: "<<currentVertices[2].x+target->x<<" current Y: "<<currentVertices[2].y+target->y<<" proj: "<<proj3<<endl;
		cout<<"Angle: "<<pv4.angle<<" distance: "<<pv4.distance<<" current X: "<<currentVertices[3].x+target->x<<" current Y: "<<currentVertices[3].y+target->y<<" proj: "<<proj4<<endl;
	}*/
	float maxProjectionObj1 = max(max(max(proj1,proj2),proj3),proj4);
	float minProjectionObj1 = min(min(min(proj1,proj2),proj3),proj4);
	return maxProjectionObj1;
}

float MissionController::pataponMinProjection(float axisAngle, int id)
{
	PlayableUnit* target = units[id].get();

	float currentAxisAngle = 0;
	HitboxFrame tmp = target->hitboxes[0].getRect();

	std::vector<sf::Vector2f> currentVertices = tmp.getCurrentVertices();

	PVector pv1 = PVector::getVectorCartesian(0,0,currentVertices[0].x+target->getGlobalPosition().x,currentVertices[0].y+target->getGlobalPosition().y);
	PVector pv2 = PVector::getVectorCartesian(0,0,currentVertices[1].x+target->getGlobalPosition().x,currentVertices[1].y+target->getGlobalPosition().y);
	PVector pv3 = PVector::getVectorCartesian(0,0,currentVertices[2].x+target->getGlobalPosition().x,currentVertices[2].y+target->getGlobalPosition().y);
	PVector pv4 = PVector::getVectorCartesian(0,0,currentVertices[3].x+target->getGlobalPosition().x,currentVertices[3].y+target->getGlobalPosition().y);
	pv1.angle =-atan2(currentVertices[0].y+target->getGlobalPosition().y, currentVertices[0].x+target->getGlobalPosition().x);
	pv2.angle =-atan2(currentVertices[1].y+target->getGlobalPosition().y, currentVertices[1].x+target->getGlobalPosition().x);
	pv3.angle =-atan2(currentVertices[2].y+target->getGlobalPosition().y, currentVertices[2].x+target->getGlobalPosition().x);
	pv4.angle =-atan2(currentVertices[3].y+target->getGlobalPosition().y, currentVertices[3].x+target->getGlobalPosition().x);

	float proj1 = pv1.GetScalarProjectionOntoAxis(axisAngle);
	float proj2 = pv2.GetScalarProjectionOntoAxis(axisAngle);
	float proj3 = pv3.GetScalarProjectionOntoAxis(axisAngle);
	float proj4 = pv4.GetScalarProjectionOntoAxis(axisAngle);


	float maxProjectionObj1 = max(max(max(proj1,proj2),proj3),proj4);
	float minProjectionObj1 = min(min(min(proj1,proj2),proj3),proj4);
	return minProjectionObj1;
}
bool MissionController::DoCollisionStepInAxis(float currentAxisAngle,HitboxFrame* currentHitboxFrame,AnimatedObject* targetObject, HitboxFrame* currentObjectHitBoxFrame,float currentObjectX,float currentObjectY)
{
	std::vector<sf::Vector2f> currentVertices = currentObjectHitBoxFrame->getCurrentVertices();

	if(currentVertices.size() < 4)
	cout << "Vertices alert!!! " << currentVertices.size() << endl;

	if(currentVertices.size() >= 4)
	{
		PVector pv1 = PVector::getVectorCartesian(0,0,currentVertices[0].x+currentObjectX,currentVertices[0].y+currentObjectY);
		PVector pv2 = PVector::getVectorCartesian(0,0,currentVertices[1].x+currentObjectX,currentVertices[1].y+currentObjectY);
		PVector pv3 = PVector::getVectorCartesian(0,0,currentVertices[2].x+currentObjectX,currentVertices[2].y+currentObjectY);
		PVector pv4 = PVector::getVectorCartesian(0,0,currentVertices[3].x+currentObjectX,currentVertices[3].y+currentObjectY);
		pv1.angle =-atan2(currentVertices[0].y+currentObjectY, currentVertices[0].x+currentObjectX);
		pv2.angle =-atan2(currentVertices[1].y+currentObjectY, currentVertices[1].x+currentObjectX);
		pv3.angle =-atan2(currentVertices[2].y+currentObjectY, currentVertices[2].x+currentObjectX);
		pv4.angle =-atan2(currentVertices[3].y+currentObjectY, currentVertices[3].x+currentObjectX);

		float proj1 = pv1.GetScalarProjectionOntoAxis(currentAxisAngle);
		float proj2 = pv2.GetScalarProjectionOntoAxis(currentAxisAngle);
		float proj3 = pv3.GetScalarProjectionOntoAxis(currentAxisAngle);
		float proj4 = pv4.GetScalarProjectionOntoAxis(currentAxisAngle);


		float maxProjectionObj1 = max(max(max(proj1,proj2),proj3),proj4);
		float minProjectionObj1 = min(min(min(proj1,proj2),proj3),proj4);

		float maxProjectionObj2 = currentHitboxFrame->maxProjection(currentAxisAngle, targetObject->getGlobalPosition().x,targetObject->getGlobalPosition().y);
		float minProjectionObj2 = currentHitboxFrame->minProjection(currentAxisAngle, targetObject->getGlobalPosition().x,targetObject->getGlobalPosition().y);
		if(maxProjectionObj1>minProjectionObj2 && minProjectionObj1<maxProjectionObj2)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}
void MissionController::DoMovement(sf::RenderWindow &window, float fps, InputController& inputCtrl)
{
	/** Make Patapon walk (temporary) **/
	float booster=1.0;
	if (rhythm.current_perfect == 4)
	{
		booster=1.2;
	}
	if(missionEnd)
	{
		booster = 1.0;
	}

	/** Find the farthest unit in your army (for calculations) **/
	int farthest_id = -1;
	float temp_pos = -9999;

	for(int i=0; i<units.size(); i++)
	{
		PlayableUnit* unit = units[i].get();

		if(temp_pos <= unit->getGlobalPosition().x+unit->local_x)
		{
			temp_pos = unit->getGlobalPosition().x+unit->local_x;
			farthest_id = i;
		}
	}

	/** Patapon movement **/

	if(farthest_id != -1)
	{
		PlayableUnit* farthest_unit = units[farthest_id].get();

		bool foundCollision = false;

		for(int i=0; i<tangibleLevelObjects.size(); i++)
		{
			for(int h=0; h<tangibleLevelObjects[i]->hitboxes.size(); h++)
			{
				//cout << "tangibleLevelObjects[" << i << "][" << h << "]" << endl;

				float proposedXPos = farthest_unit->getGlobalPosition().x;

				/// NEW COLLISION SYSTEM:
				/// Separating axis theorem
				/// we check an axis at a time
				/// 8 axes in total, aligned with the normal of each face of each shape
				/// thankfully because we are only using rectangles, there are two pairs of parallel sides
				/// so we only need to check 4 axes, as the other 4 are all parallel.
				///
				/// in each axis we calculate the vector projection onto the axis between the origin and each corner of each box
				/// and find the maximum projection and minimum projection for each shape
				/// then we check if min2>max1 or min1>max2 there has been a collision in this axis
				/// there has to be a collision in ALL axes for actual collision to be confirmed,
				/// so we can stop checking if we find a single non-collision.

				/// axis 1: obj1 "sideways" We start with sideways because it is less likely to contain a collision

				float currentAxisAngle = 0;
				HitboxFrame tmp = farthest_unit->hitboxes[0].getRect();

				CollidableObject* target = tangibleLevelObjects[i].get();
				Entity* entity = tangibleLevelObjects[i].get();

				if(entity->entityID != 5)
				{
					if(entity->isCollidable)
					{
						if(farthest_unit->getGlobalPosition().x >= entity->getGlobalPosition().x+entity->hitboxes[0].o_x)
						foundCollision = true;
					}
				}

				bool isCollision = DoCollisionStepInAxis(currentAxisAngle,&(target->hitboxes[h].hitboxObject),target,&tmp,proposedXPos,farthest_unit->getGlobalPosition().y);
				if (!isCollision)
				{
					continue;
				}
				//cout<<"COLLISION FOUND IN axis 1"<<endl;

				/// axis 2: obj1 "up"
				currentAxisAngle = 3.14159265358/2;
				bool isCollision2 = DoCollisionStepInAxis(currentAxisAngle,&(target->hitboxes[h].hitboxObject),target,&tmp,proposedXPos,farthest_unit->getGlobalPosition().y);
				if (!isCollision2)
				{
					continue;
				}
				//cout<<"COLLISION FOUND IN axis 2 (up)"<<endl;

				/// axis 3: obj2 "up" (we add the 90 degrees from before to its current rotation)
				currentAxisAngle = target->hitboxes[h].hitboxObject.rotation + currentAxisAngle;

				bool isCollision3 = DoCollisionStepInAxis(currentAxisAngle,&(target->hitboxes[h].hitboxObject),target,&tmp,proposedXPos,farthest_unit->getGlobalPosition().y);
				if (!isCollision3)
				{
					continue;
				}
				//cout<<"COLLISION FOUND IN axis 3 (up2)"<<endl;

				/// axis 4: obj2 "sideways"
				currentAxisAngle = target->hitboxes[h].hitboxObject.rotation;

				bool isCollision4 = DoCollisionStepInAxis(currentAxisAngle,&(target->hitboxes[h].hitboxObject),target,&tmp,proposedXPos,farthest_unit->getGlobalPosition().y);
				if (!isCollision4)
				{
					continue;
				}

				/// we have a collision
				if (isCollision&&isCollision2&&isCollision3&&isCollision4)
				{
					///check if unit should be prevented from passing through
					if(target->isCollidable)
					foundCollision = true;

					///the entity can still react
					target->OnCollide(target);

					//std::cout << "[COLLISION_SYSTEM]: Found a collision"<<endl;
				}
				else
				{
					cout<<"Something is very wrong"<<endl;
				}
			}
		}

		if((camera.walk) || ((missionEnd) && (!failure)))
		{
			float pataDistance = 240 * booster;

			if(walkBackwards)
			pataDistance = -pataDistance;

			float diff = (Smoothstep(walkClock.getElapsedTime().asSeconds()/2)*pataDistance)-(Smoothstep(prevTime/2)*pataDistance);
			prevTime = walkClock.getElapsedTime().asSeconds();

			float proposedXPos = farthest_unit->getGlobalPosition().x + diff;

			camera.pataSpeed = (2 * 60 * booster);

			if(walkBackwards)
			camera.pataSpeed = -camera.pataSpeed;

			//cout << "global_x: " << farthest_unit->global_x << endl;
			//cout << "proposedXPos = " << proposedXPos << endl;
			/// use the right hand side of the patapon sprite to check for collisions. This should be changed if the patapon walks to the left
			//float proposedXPosRight = proposedXPos + farthest_unit->hitBox.left + farthest_unit->hitBox.width;
			/// need to have it check for collision and stop if blocked by kacheek here.

			/// right now it is very basic checking only in X axis. Jumping over a
			/// kacheek will not be possible.

			/// if the new position is inside a kacheek, don't move. If we found anything,
			if (!foundCollision)
			{
				if(!missionEnd)
				army_x += diff;
				else
				army_x += 120.0 / fps;
			}
		}
	}

	/** Set global positions for the units **/

	for(int i=0; i<units.size(); i++)
	{
		PlayableUnit* unit = units[i].get();

		bool foundCollision = false;

		for(int i=0; i<tangibleLevelObjects.size(); i++)
		{
			for(int h=0; h<tangibleLevelObjects[i]->hitboxes.size(); h++)
			{
				//cout << "tangibleLevelObjects[" << i << "][" << h << "]" << endl;

				float proposedXPos = unit->getGlobalPosition().x;

				/// NEW COLLISION SYSTEM:
				/// Separating axis theorem
				/// we check an axis at a time
				/// 8 axes in total, aligned with the normal of each face of each shape
				/// thankfully because we are only using rectangles, there are two pairs of parallel sides
				/// so we only need to check 4 axes, as the other 4 are all parallel.
				///
				/// in each axis we calculate the vector projection onto the axis between the origin and each corner of each box
				/// and find the maximum projection and minimum projection for each shape
				/// then we check if min2>max1 or min1>max2 there has been a collision in this axis
				/// there has to be a collision in ALL axes for actual collision to be confirmed,
				/// so we can stop checking if we find a single non-collision.

				/// axis 1: obj1 "sideways" We start with sideways because it is less likely to contain a collision

				float currentAxisAngle = 0;
				HitboxFrame tmp = unit->hitboxes[0].getRect();

				CollidableObject* target = tangibleLevelObjects[i].get();
				Entity* entity = tangibleLevelObjects[i].get();

				if(entity->entityID != 5)
				{
					if(entity->isCollidable)
					{
						if(unit->getGlobalPosition().x >= entity->getGlobalPosition().x+entity->hitboxes[0].o_x)
						foundCollision = true;
					}
				}

				bool isCollision = DoCollisionStepInAxis(currentAxisAngle,&(target->hitboxes[h].hitboxObject),target,&tmp,proposedXPos,unit->getGlobalPosition().y);
				if (!isCollision)
				{
					continue;
				}
				//cout<<"COLLISION FOUND IN axis 1"<<endl;

				/// axis 2: obj1 "up"
				currentAxisAngle = 3.14159265358/2;
				bool isCollision2 = DoCollisionStepInAxis(currentAxisAngle,&(target->hitboxes[h].hitboxObject),target,&tmp,proposedXPos,unit->getGlobalPosition().y);
				if (!isCollision2)
				{
					continue;
				}
				//cout<<"COLLISION FOUND IN axis 2 (up)"<<endl;

				/// axis 3: obj2 "up" (we add the 90 degrees from before to its current rotation)
				currentAxisAngle = target->hitboxes[h].hitboxObject.rotation + currentAxisAngle;

				bool isCollision3 = DoCollisionStepInAxis(currentAxisAngle,&(target->hitboxes[h].hitboxObject),target,&tmp,proposedXPos,unit->getGlobalPosition().y);
				if (!isCollision3)
				{
					continue;
				}
				//cout<<"COLLISION FOUND IN axis 3 (up2)"<<endl;

				/// axis 4: obj2 "sideways"
				currentAxisAngle = target->hitboxes[h].hitboxObject.rotation;

				bool isCollision4 = DoCollisionStepInAxis(currentAxisAngle,&(target->hitboxes[h].hitboxObject),target,&tmp,proposedXPos,unit->getGlobalPosition().y);
				if (!isCollision4)
				{
					continue;
				}

				/// we have a collision
				if (isCollision&&isCollision2&&isCollision3&&isCollision4)
				{
					///check if unit should be prevented from passing through
					if(target->isCollidable)
					foundCollision = true;

					///the entity can still react
					target->OnCollide(target);

					//std::cout << "[COLLISION_SYSTEM]: Found a collision"<<endl;
				}
				else
				{
					cout<<"Something is very wrong"<<endl;
				}
			}
		}

		if(!unit->dead)
		{
			if(unit->local_x < unit->dest_local_x)
			{
				if(!foundCollision)
				unit->local_x += 200.0 / fps;

				/*if(unit->enemy_in_range)
				{
					if(unit->getAnimationSegment() != "walk_focused")
					unit->setAnimationSegment("walk_focused", true);
				}
				else
				{
					if(unit->getAnimationSegment() != "walk")
					unit->setAnimationSegment("walk", true);
				}*/
			}
			if(unit->local_x > unit->dest_local_x)
			{
				unit->local_x -= 200.0 / fps;

				/*if(unit->enemy_in_range)
				{
					if(unit->getAnimationSegment() != "walk_focused")
					unit->setAnimationSegment("walk_focused", true);
				}
				else
				{
					if(unit->getAnimationSegment() != "walk")
					unit->setAnimationSegment("walk", true);
				}*/
			}
		}

		switch(unit->getUnitID())
		{
			case 0: ///Hatapon
			{
				unit->setGlobalPosition(sf::Vector2f(army_x, 500));
				break;
			}

			case 1: ///Yaripon
			{
				unit->setGlobalPosition(sf::Vector2f(army_x + 100 + (50 * i), patapon_y));
				break;
			}

			case 2: ///Tatepon
			{
				unit->setGlobalPosition(sf::Vector2f(army_x + 200 + (50 * i), patapon_y));
				break;
			}
		}
	}
}

void MissionController::DoRhythm(InputController& inputCtrl)
{
	/** Call Rhythm functions **/

		if((rhythm.current_song == "patapata") || (rhythm.current_song == "chakapata"))
		{
			//cout << "set walk true" << endl;
			camera.walk = true;

			if(rhythm.current_song == "chakapata")
			walkBackwards = true;
			else
			walkBackwards = false;

			if(!startWalking)
			{
				walkClock.restart();
				prevTime = 0;

				startWalking = true;
			}
		}
		else
		{
			//cout << "set walk false" << endl;
			camera.walk = false;

			startWalking = false;
		}

		if((rhythm.rhythmController.current_drum == "pata") or (rhythm.rhythmController.current_drum == "pon") or (rhythm.rhythmController.current_drum == "chaka") or (rhythm.rhythmController.current_drum == "don"))
		{
			rhythm.rhythmController.current_drum = "";
			rhythm.current_song = "";
		}

		rhythm.rhythmController.config = thisConfig;
		rhythm.config = thisConfig;

		rhythm.doRhythm(inputCtrl);
}

void MissionController::ClearMissionMemory()
{
	vector<std::unique_ptr<Entity>>().swap(tangibleLevelObjects);
	vector<std::unique_ptr<PlayableUnit>>().swap(units);
	vector<std::unique_ptr<Kirajin_Yari_2>>().swap(kirajins);
	rhythm.Clear();

	levelProjectiles.clear();

	droppeditem_spritesheet.clear();
	dmgCounters.clear();
	droppedItems.clear();
	pickedItems.clear();
	unitThumbs.clear();
}

void MissionController::DoMissionEnd(sf::RenderWindow& window, float fps)
{
	/** Make the missionEndTimer unusable until the mission is not finished **/
	if(!missionEnd)
	missionEndTimer.restart();

	/** Mission end cheering **/

	if(missionEnd)
	{
		if(!failure)
		{
			if(missionEndTimer.getElapsedTime().asMilliseconds() >= 2500)
			{
				if(!playCheer[0])
				{
					s_cheer.stop();
					s_cheer.setBuffer(sb_cheer1);
					s_cheer.setVolume(float(thisConfig->GetInt("masterVolume"))*(float(thisConfig->GetInt("sfxVolume"))/100.f));
					s_cheer.play();
					playCheer[0] = true;
				}
			}

			if(missionEndTimer.getElapsedTime().asMilliseconds() >= 4500)
			{
				if(!playCheer[1])
				{
					s_cheer.stop();
					s_cheer.setBuffer(sb_cheer2);
					s_cheer.setVolume(float(thisConfig->GetInt("masterVolume"))*(float(thisConfig->GetInt("sfxVolume"))/100.f));
					s_cheer.play();
					playCheer[1] = true;
				}
			}

			if(missionEndTimer.getElapsedTime().asMilliseconds() >= 6500)
			{
				if(!playCheer[2])
				{
					s_cheer.stop();
					s_cheer.setBuffer(sb_cheer3);
					s_cheer.setVolume(float(thisConfig->GetInt("masterVolume"))*(float(thisConfig->GetInt("sfxVolume"))/100.f));
					s_cheer.play();
					playCheer[2] = true;
				}
			}

			if(missionEndTimer.getElapsedTime().asMilliseconds() >= 8000)
			{
				if(!playJingle)
				{
					s_jingle.setBuffer(sb_win_jingle);
					s_jingle.setVolume(float(thisConfig->GetInt("masterVolume"))*(float(thisConfig->GetInt("sfxVolume"))/100.f));
					s_jingle.play();
					playJingle = true;
				}
			}
		}
		else
		{
			if(!playJingle)
			{
				s_jingle.setBuffer(sb_lose_jingle);
				s_jingle.setVolume(float(thisConfig->GetInt("masterVolume"))*(float(thisConfig->GetInt("sfxVolume"))/100.f));
				s_jingle.play();
				playJingle = true;
			}
		}
	}

	/** Make the camera follow Patapons until the jingle is played **/

	if(missionEndTimer.getElapsedTime().asMilliseconds() < 7700)
	{
		camera.followobject_x = army_x * (window.getSize().x / float(1280));
	}

	/** Mission fade in and fade out **/

	if(!missionEnd)
	{
		if(fade_alpha > 0)
		{
			fade_alpha -= float(500) / fps;
		}

		if(fade_alpha <= 0)
		{
			fade_alpha = 0;
		}
	}
	else
	{
		if(!failure)
		{
			if(missionEndTimer.getElapsedTime().asMilliseconds() >= 11000)
			{
				if(fade_alpha < 255)
				{
					fade_alpha += float(250) / fps;
				}

				if(fade_alpha >= 255)
				{
					fade_alpha = 255;
				}
			}
		}
		else
		{
			if(missionEndTimer.getElapsedTime().asMilliseconds() >= 1000)
			{
				if(fade_alpha < 255)
				{
					fade_alpha += float(250) / fps;
				}

				if(fade_alpha >= 255)
				{
					fade_alpha = 255;
				}
			}
		}
	}

	fade_box.setSize(sf::Vector2f(window.getSize().x, window.getSize().y));
	fade_box.setFillColor(sf::Color(0,0,0,fade_alpha));
	window.draw(fade_box);

	/** Mission end event (Mission complete/Mission failed screen + transition to Patapolis **/

	if(!failure) ///Victory
	{
		if(missionEndTimer.getElapsedTime().asMilliseconds() >= 11500)
		{
			if(!textBounce)
			{
				if(missionEndTimer.getElapsedTime().asMilliseconds() >= 13050)
				{
					textCurScale = 1.4;
					textBounce = true;
				}
			}

			t_win.setOrigin(t_win.getLocalBounds().width/2, t_win.getLocalBounds().height/2);
			t_win_outline.setOrigin(t_win_outline.getLocalBounds().width/2, t_win_outline.getLocalBounds().height/2);

			if(barCurX > barDestX)
			{
				barCurX -= (abs(barCurX - barDestX) * 5) / fps;
			}
			else
			{
				barCurX = barDestX;
			}

			if(textCurX < textDestX)
			{
				textCurX += (abs(textCurX - textDestX) * 5) / fps;
			}
			else
			{
				textCurX = textDestX;
			}
			if(textCurScale > textDestScale)
			{
				textCurScale -= (abs(textCurScale - textDestScale) * 5) / fps;
			}
			else
			{
				textCurScale = textDestScale;
			}

			t_win.setScale(textCurScale);
			t_win_outline.setScale(textCurScale);

			bar_win.setPosition(barCurX, 360);
			t_win.setPosition(textCurX-7, 360-14);
			t_win_outline.setPosition(textCurX+2, 360-4);

			bar_win.draw(window);
			t_win_outline.draw(window);
			t_win.draw(window);

			if(missionEndTimer.getElapsedTime().asMilliseconds() >= 17000)
			{
				if(fadeout_alpha < 255)
				{
					fadeout_alpha += float(250) / fps;
				}

				if(fadeout_alpha >= 255)
				{
					fadeout_alpha = 255;
				}

				fadeout_box.setSize(sf::Vector2f(window.getSize().x, window.getSize().y));
				fadeout_box.setFillColor(sf::Color(0,0,0,fadeout_alpha));
				window.draw(fadeout_box);
			}

			if(missionEndTimer.getElapsedTime().asMilliseconds() > 19000)
			{
				cout << "End mission" << endl;
				/// A wall is unyielding, so it does nothing when collided with.

				/// note we don't call the parent function. It does nothing, it just serves
				/// as an incomplete function to be overridden by child classes.
				/// end the mission

				StopMission();

				cout << "Add the picked up items to item repository" << endl;
				submitPickedItems();
				updateMissions();
				ClearMissionMemory();

				cout << "Go to Patapolis" << endl;

				sf::Thread loadingThreadInstance(&V4Core::loadingThread, v4Core);
				v4Core->continue_loading = true;
				v4Core->window.setActive(false);
				loadingThreadInstance.launch();

				v4Core->mainMenu.patapolisMenu.doWaitKeyPress = false;
				v4Core->mainMenu.patapolisMenu.Show();
				v4Core->mainMenu.patapolisMenu.is_active = true;
				v4Core->mainMenu.patapolisMenu.screenFade.Create(thisConfig, 0, 512);

				if (!v4Core->mainMenu.patapolisMenu.initialised)
				{
					/// patapolis might not be initialised because we could be running the pre-patapolis scripted first mission.
					cout << "[ENDFLAG] Initialize Patapolis for the first time" << endl;
					v4Core->mainMenu.patapolisMenu.Initialise(thisConfig, v4Core, &v4Core->mainMenu);
				}
				else
				{
					cout << "Don't initialize Patapolis, just show it again" << endl;
				}

				v4Core->mainMenu.patapolisMenu.location = 3;
				v4Core->mainMenu.patapolisMenu.SetTitle(3);
				v4Core->mainMenu.patapolisMenu.camPos = v4Core->mainMenu.patapolisMenu.locations[3];
				v4Core->mainMenu.patapolisMenu.fade_alpha = 255;

				while(missionEndTimer.getElapsedTime().asMilliseconds() < 21000)
				{
					///halt loading for a second
				}

				v4Core->loadingWaitForKeyPress();

				v4Core->continue_loading=false;

				v4Core->changeRichPresence("In Patapolis", "logo", "");
			}
		}
	}
	else ///Failure
	{
		if(missionEndTimer.getElapsedTime().asMilliseconds() >= 2500)
		{
			t_lose.setOrigin(t_lose.getLocalBounds().width/2, t_lose.getLocalBounds().height/2);
			t_lose_outline.setOrigin(t_lose_outline.getLocalBounds().width/2, t_lose_outline.getLocalBounds().height/2);

			if(barCurX > barDestX)
			{
				barCurX -= (abs(barCurX - barDestX) * 5) / fps;
			}
			else
			{
				barCurX = barDestX;
			}

			if(textCurX < textDestX)
			{
				textCurX += (abs(textCurX - textDestX) * 5) / fps;
			}
			else
			{
				textCurX = textDestX;
			}

			t_lose.setScale(textCurScale);
			t_lose_outline.setScale(textCurScale);

			bar_lose.setPosition(barCurX, 360);
			t_lose.setPosition(textCurX-7, 360-14);
			t_lose_outline.setPosition(textCurX+2, 360-4);

			bar_lose.draw(window);
			t_lose_outline.draw(window);
			t_lose.draw(window);

			if(missionEndTimer.getElapsedTime().asMilliseconds() >= 6000)
			{
				if(fadeout_alpha < 255)
				{
					fadeout_alpha += float(250) / fps;
				}

				if(fadeout_alpha >= 255)
				{
					fadeout_alpha = 255;
				}

				fadeout_box.setSize(sf::Vector2f(window.getSize().x, window.getSize().y));
				fadeout_box.setFillColor(sf::Color(0,0,0,fadeout_alpha));
				window.draw(fadeout_box);
			}

			if(missionEndTimer.getElapsedTime().asMilliseconds() >= 8000)
			{
				/** End flag executes the mission victory code, so mission failed code needs to be executed separately here. **/

				cout << "End mission" << endl;

				StopMission();
				ClearMissionMemory();

				cout << "Go to Patapolis" << endl;

				sf::Thread loadingThreadInstance(&V4Core::loadingThread, v4Core);
				v4Core->continue_loading=true;
				v4Core->window.setActive(false);
				loadingThreadInstance.launch();

				v4Core->mainMenu.patapolisMenu.doWaitKeyPress = false;
				v4Core->mainMenu.patapolisMenu.Show();
				v4Core->mainMenu.patapolisMenu.is_active = true;
				v4Core->mainMenu.patapolisMenu.screenFade.Create(thisConfig, 0, 512);

				if (!v4Core->mainMenu.patapolisMenu.initialised)
				{
					/// patapolis might not be initialised because we could be running the pre-patapolis scripted first mission.
					cout << "[ENDFLAG] Initialize Patapolis for the first time" << endl;
					v4Core->mainMenu.patapolisMenu.Initialise(thisConfig, v4Core, &v4Core->mainMenu);
				}
				else
				{
					cout << "Don't initialize Patapolis, just show it again" << endl;
				}

				v4Core->mainMenu.patapolisMenu.location = 3;
				v4Core->mainMenu.patapolisMenu.SetTitle(3);
				v4Core->mainMenu.patapolisMenu.camPos = v4Core->mainMenu.patapolisMenu.locations[3];
				v4Core->mainMenu.patapolisMenu.fade_alpha = 255;

				while(missionEndTimer.getElapsedTime().asMilliseconds() < 10000)
				{
					///halt loading for a second
				}

				v4Core->loadingWaitForKeyPress();

				v4Core->continue_loading=false;

				v4Core->changeRichPresence("In Patapolis", "logo", "");
			}
		}
	}
}

void MissionController::DoVectorCleanup(vector<int> units_rm, vector<int> dmg_rm, vector<int> tlo_rm, vector<int> pr_rm)
{
	//cout << "MissionController::DoVectorCleanup" << endl;
	//cout << units_rm.size() << " " << dmg_rm.size() << " " << tlo_rm.size() << " " << pr_rm.size() << endl;

	for(int i=0; i<units_rm.size(); i++)
	{
		units.erase(units.begin()+(units_rm[i]-i));
		cout << "Erased unit " << units_rm[i] << endl;
	}

	for(int i=0; i<dmg_rm.size(); i++)
	{
		dmgCounters.erase(dmgCounters.begin()+(dmg_rm[i] - i));
		cout << "Erased dmgCounter " << dmg_rm[i] << endl;
	}

	for(int i=0; i<tlo_rm.size(); i++)
	{
		tangibleLevelObjects.erase(tangibleLevelObjects.begin()+(tlo_rm[i]-i));
		cout << "Erased tangibleLevelObject " << tlo_rm[i] << endl;
	}

	for(int i=0; i<pr_rm.size(); i++)
	{
		levelProjectiles.erase(levelProjectiles.begin()+(pr_rm[i]-i));
		cout << "Erased levelProjectile " << pr_rm[i] << endl;
	}
}

std::vector<int> MissionController::DrawProjectiles(sf::RenderWindow& window)
{
	/** Projectile management **/

	vector<int> pr_e; ///projectiles to erase

	/// step 1: all projectiles have gravity applied to them
	for(int i=0; i<levelProjectiles.size(); i++)
	{
		Projectile* p = levelProjectiles[i].get();
		float xspeed = p->GetXSpeed();
		float yspeed = p->GetYSpeed();
		yspeed += (gravity/fps);
		p->SetNewSpeedVector(xspeed,yspeed);
		p->Update(window,fps);
	}

	/// step 3: any projectiles that hit any collidableobject are informed
	for(int i=0; i<levelProjectiles.size(); i++)
	{
		bool removeProjectile = false;

		Projectile* p = levelProjectiles[i].get();
		float ypos = p->yPos;
		float xpos = p->xPos;
		HitboxFrame tmp;
		tmp.time = 0;
		tmp.g_x = 0;
		tmp.g_y = 0;
		tmp.clearVertices();
		tmp.addVertex(-3,-1); /// "top left"
		tmp.addVertex(3,-1); /// "top right"
		tmp.addVertex(-3,1); /// "bottom left"
		tmp.addVertex(3,1); /// "bottom right"
		tmp.rotation = -p->angle;

		if(ypos > floor_y)
		{
			///create hit sound
			projectile_sounds.emplace_back();

			projectile_sounds[projectile_sounds.size()-1].setBuffer(spear_hit_solid);

			projectile_sounds[projectile_sounds.size()-1].setVolume(float(thisConfig->GetInt("masterVolume"))*(float(thisConfig->GetInt("sfxVolume"))/100.f));
			projectile_sounds[projectile_sounds.size()-1].play();

			removeProjectile = true;
		}

		///calculate projectile damage
		///and pass it to a special vector called collisionData
		///which passes whatever you'd like to the collided animation object
		///so you can put anything and react with it in the individual entity classes
		///in projectiles' case, im transferring the damage dealt

		if(!removeProjectile)
		{
			int min_dmg = p->min_dmg;
			int max_dmg = p->max_dmg;
			int bound = max_dmg - min_dmg + 1;
			int ranDmg = rand() % bound;
			int total = min_dmg + ranDmg;

			///sending damage dealt
			vector<string> collisionData = {to_string(total)};

			///retrieve collision event
			vector<CollisionEvent> cevent;

			///check whether the projectile was on enemy side or ally side
			if(!p->enemy)
			{
				///Do collisions for all entities (ally projectiles)
				cevent = DoCollisionForObject(&tmp, xpos, ypos, p->projectile_id, collisionData);
			}
			else
			{
				///Do collisions for all units (enemy projectiles)
				cevent = DoCollisionForUnit(&tmp, xpos, ypos, p->projectile_id, collisionData);
			}

			for(int e=0; e<cevent.size(); e++)
			{
				if(cevent[e].collided)
				{
					///add damage counter
					if((cevent[e].isAttackable) && (cevent[e].isCollidable))
					{
						///create hit sound
						projectile_sounds.emplace_back();

						///locate the right buffer
						switch(cevent[e].collidedEntityCategory)
						{
							case -1:
							{
								break;
							}

							case Entity::EntityCategories::ANIMAL:
							{
								projectile_sounds[projectile_sounds.size()-1].setBuffer(spear_hit_enemy);
								break;
							}

							case Entity::EntityCategories::ENEMYUNIT:
							{
								projectile_sounds[projectile_sounds.size()-1].setBuffer(spear_hit_enemy);
								break;
							}

							case Entity::EntityCategories::BUILDING_REGULAR:
							{
								projectile_sounds[projectile_sounds.size()-1].setBuffer(spear_hit_solid);
								break;
							}

							case Entity::EntityCategories::OBSTACLE_IRON:
							{
								projectile_sounds[projectile_sounds.size()-1].setBuffer(spear_hit_iron);
								break;
							}

							case Entity::EntityCategories::BUILDING_IRON:
							{
								projectile_sounds[projectile_sounds.size()-1].setBuffer(spear_hit_iron);
								break;
							}

							case Entity::EntityCategories::OBSTACLE_ROCK:
							{
								projectile_sounds[projectile_sounds.size()-1].setBuffer(spear_hit_rock);
								break;
							}

							case Entity::EntityCategories::OBSTACLE_WOOD:
							{
								projectile_sounds[projectile_sounds.size()-1].setBuffer(spear_hit_solid);
								break;
							}
						}

						projectile_sounds[projectile_sounds.size()-1].setVolume(float(thisConfig->GetInt("masterVolume"))*(float(thisConfig->GetInt("sfxVolume"))/100.f));
						projectile_sounds[projectile_sounds.size()-1].play();

						addDmgCounter(0, total*cevent[e].defend_factor, xpos, ypos, qualitySetting, resSetting);

						removeProjectile = true;
						break; ///break the for loop here to prevent double hits
					}
				}
			}
		}

		p->Draw(window,fps);

		if(removeProjectile)
		pr_e.push_back(i);
	}

	return pr_e;
}

void MissionController::drawCommandList(sf::RenderWindow& window)
{
	///first four
	for(int i=0; i<4; i++)
	{
		command_inputs[i].setOrigin(command_inputs[i].getLocalBounds().width, command_inputs[i].getLocalBounds().height / 2);
		command_inputs[i].setPosition(-21 + (i+1)*((1280-42) / 4.f), 644);

		command_descs[i].setOrigin(command_descs[i].getLocalBounds().width, command_descs[i].getLocalBounds().height / 2);
		command_descs[i].setPosition(-21 + (i+1)*((1280-42) / 4.f) - command_inputs[i].getLocalBounds().width - 8, 644);

		command_descs[i].draw(window);
		command_inputs[i].draw(window);
	}

	///second four
	for(int i=4; i<8; i++)
	{
		command_inputs[i].setOrigin(command_inputs[i].getLocalBounds().width, command_inputs[i].getLocalBounds().height / 2);
		command_inputs[i].setPosition(-21 + (i-4+1)*((1280-42) / 4.f), 673);

		command_descs[i].setOrigin(command_descs[i].getLocalBounds().width, command_descs[i].getLocalBounds().height / 2);
		command_descs[i].setPosition(-21 + (i-4+1)*((1280-42) / 4.f) - command_inputs[i].getLocalBounds().width - 8, 673);

		command_descs[i].draw(window);
		command_inputs[i].draw(window);
	}
}

void MissionController::DrawUnitThumbs(sf::RenderWindow& window)
{
	for(int i=0; i<unitThumbs.size(); i++)
	{
		float resRatioX = window.getSize().x / float(1280);
		float resRatioY = window.getSize().y / float(720);

		int farthest_id = -1;
		float temp_pos = -9999;

		int curunits = 0;
		float maxunithp = 0;
		float curunithp = 0;

		for(int u=0; u<units.size(); u++)
		{
			if(units[u].get()->getUnitID() == unitThumbs[i].unit_class)
			{
				curunits++;

				PlayableUnit* unit = units[u].get();

				if(temp_pos <= unit->getGlobalPosition().x)
				{
					temp_pos = unit->getGlobalPosition().x;
					farthest_id = u;
				}

				maxunithp += unit->getUnitMaxHP();
				curunithp += unit->getUnitHP();
			}
		}

		if(farthest_id != -1)
		{
			float hp_percentage = curunithp / maxunithp;

			unitThumbs[i].circle.setRadius(28*resRatioX);
			unitThumbs[i].circle.setOrigin(unitThumbs[i].circle.getLocalBounds().width/2,unitThumbs[i].circle.getLocalBounds().height/2);
			unitThumbs[i].circle.setPosition((52+(64*i))*resRatioX, (72*resRatioY));
			window.draw(unitThumbs[i].circle);

			// The code from earlier copied sprites from farthest_units per frame
			// The new code will instead just draw the same sprite but with modified parameters
			// So we don't waste time on copying three(!) sprites per frame per unit. Very wasteful

			//unitThumbs[i].thumb = (*obj)[0].s_obj;
			//unitThumbs[i].thumb.setScale(0.7,0.7);

			//Get Farthest unit and save old stats
			PlayableUnit* farthest_unit = units[farthest_id].get();
			//float old_scale = (*obj)[0].s_obj.scaleX;
			//float old_x = (*obj)[0].s_obj.getPosition().x;
			//float old_y = (*obj)[0].s_obj.getPosition().y;
			vector<Object>* obj;
			obj = farthest_unit->objects.get();

			(*obj)[0].s_obj.setScale(0.7, 0.7);

			int manual_x,manual_y;
			switch(unitThumbs[i].unit_class)
			{
				case 0:
				{
					manual_x = 6;
					manual_y = -40;
					break;
				}
				case 1:
				{
					manual_x = 0;
					manual_y = 14;
					break;
				}
				case 2:
				{
					manual_x = 0;
					manual_y = 14;
					break;
				}
			}

			//unitThumbs[i].thumb.setPosition(52+(64*i)+manual_x, 60+manual_y);
			//unitThumbs[i].thumb.draw(window);

			//Draw the same object without copying it
			(*obj)[0].s_obj.setPosition(52 + (64 * i) + manual_x, 60 + manual_y);
			(*obj)[0].s_obj.draw(window);

			//Bring the old stats back
			//(*obj)[0].s_obj.setPosition(old_x, old_y);
			//(*obj)[0].s_obj.setScale(old_scale, old_scale);

			if(unitThumbs[i].unit_class != 0)
			{

				//unitThumbs[i].equip_1 = (*obj)[1].s_obj;
				//unitThumbs[i].equip_2 = (*obj)[2].s_obj;

				(*obj)[1].s_obj.setPosition(52+(64*i)+manual_x+(((*obj)[1].s_obj.getPosition().x-farthest_unit->getGlobalPosition().x)*0.7), 60+manual_y+(((*obj)[1].s_obj.getPosition().y-farthest_unit->getGlobalPosition().y)*0.7));
				(*obj)[2].s_obj.setPosition(52+(64*i)+manual_x+(((*obj)[2].s_obj.getPosition().x-farthest_unit->getGlobalPosition().x)*0.7), 60+manual_y+(((*obj)[2].s_obj.getPosition().y-farthest_unit->getGlobalPosition().y)*0.7));

				(*obj)[1].s_obj.setScale(0.7, 0.7);
				(*obj)[2].s_obj.setScale(0.7, 0.7);

				(*obj)[1].s_obj.draw(window);
				(*obj)[2].s_obj.draw(window);
			}

			unitThumbs[i].hpbar_back.setOrigin(unitThumbs[i].hpbar_back.getLocalBounds().width/2, unitThumbs[i].hpbar_back.getLocalBounds().height/2);
			unitThumbs[i].hpbar_back.setPosition(52+(64*i), 32);
			unitThumbs[i].hpbar_back.draw(window);

			unitThumbs[i].hpbar_ins.setOrigin(0, unitThumbs[i].hpbar_ins.getLocalBounds().height/2);
			unitThumbs[i].hpbar_ins.setTextureRect(sf::IntRect(0,0,unitThumbs[i].width*hp_percentage,unitThumbs[i].hpbar_ins.getLocalBounds().height));
			unitThumbs[i].hpbar_ins.setPosition(52+(64*i)-27, 32);

			if(hp_percentage > 0.70)
			unitThumbs[i].hpbar_ins.setColor(sf::Color(0,255,0,255));
			else if(hp_percentage > 0.35)
			unitThumbs[i].hpbar_ins.setColor(sf::Color(245,230,66,255));
			else
			unitThumbs[i].hpbar_ins.setColor(sf::Color(212,0,0,255));

			unitThumbs[i].hpbar_ins.draw(window);

			if(unitThumbs[i].unit_class != 0)
			{
				unitThumbs[i].unit_count_shadow.setString(to_string(curunits));
				unitThumbs[i].unit_count_shadow.setOrigin(unitThumbs[i].unit_count_shadow.getLocalBounds().width/2, unitThumbs[i].unit_count_shadow.getLocalBounds().height/2);
				unitThumbs[i].unit_count_shadow.setPosition(52+(64*i)+28, 98);
				unitThumbs[i].unit_count_shadow.draw(window);

				unitThumbs[i].unit_count.setString(to_string(curunits));
				unitThumbs[i].unit_count.setOrigin(unitThumbs[i].unit_count.getLocalBounds().width/2, unitThumbs[i].unit_count.getLocalBounds().height/2);
				unitThumbs[i].unit_count.setPosition(52+(64*i)+26, 96);
				unitThumbs[i].unit_count.draw(window);
			}
		}
	}
}

void MissionController::DrawPickedItems(sf::RenderWindow& window)
{
	for(int i=0; i<pickedItems.size(); i++)
	{
		float resRatioX = window.getSize().x / float(1280);
		float resRatioY = window.getSize().y / float(720);

		pickedItems[i].circle.setRadius(25*resRatioX);
		pickedItems[i].circle.setOrigin(pickedItems[i].circle.getLocalBounds().width/2,pickedItems[i].circle.getLocalBounds().height/2);
		pickedItems[i].circle.setPosition((1230 - 54*i)*resRatioX, (50*resRatioY));
		window.draw(pickedItems[i].circle);

		pickedItems[i].item.setOrigin(pickedItems[i].bounds.x/2, pickedItems[i].bounds.y/2);
		pickedItems[i].item.setPosition(1230 - 54*i, 50);
		pickedItems[i].item.setScale(0.8,0.8);
		pickedItems[i].item.draw(window);
	}
}

void MissionController::DrawHitboxes(sf::RenderWindow& window)
{
	for(int i=0; i<units.size(); i++)
		{
			PlayableUnit* unit = units[i].get();

			for(int h=0; h<unit->hitboxes.size(); h++)
			{
				HitboxFrame* currentHitbox = &(unit->hitboxes[h].hitboxObject);

				sf::ConvexShape convex;
				convex.setFillColor(sf::Color(150, 50, 250));
				// resize it to 5 points
				std::vector<sf::Vector2f> currentVertices = currentHitbox->getCurrentVertices();
				convex.setPointCount(currentVertices.size());

				for (int j=0; j<currentVertices.size(); j++)
				{
					sf::Vector2f currentPoint = currentVertices[j];
					currentPoint.x = currentPoint.x + currentHitbox->g_x + unit->global_x + unit->local_x;
					currentPoint.y = currentPoint.y + currentHitbox->g_y + unit->global_y + unit->local_y;
					//cout<<"DRAWING POINT: "<<currentVertices.size()<<" x: "<<currentPoint.x<<" y: "<<currentPoint.y<<endl;
					sf::CircleShape shape(5);
					shape.setFillColor(sf::Color(100, 250, 50));
					shape.setPosition(currentPoint.x-2.5,currentPoint.y-2.5);
					window.draw(shape);
					convex.setPoint(j, currentPoint);
					//cout << "convex.setPoint(" << j << ", " << currentPoint.x << " " << currentPoint.y << ");" << endl;
				}

				window.draw(convex);
			}
		}

		for(int i=0; i<tangibleLevelObjects.size(); i++)
		{
			Entity* entity = tangibleLevelObjects[i].get();

			for(int h=0; h<entity->hitboxes.size(); h++)
			{
				HitboxFrame* currentHitbox = &(entity->hitboxes[h].hitboxObject);

				sf::ConvexShape convex;
				convex.setFillColor(sf::Color(150, 50, 250));
				// resize it to 5 points
				std::vector<sf::Vector2f> currentVertices = currentHitbox->getCurrentVertices();
				convex.setPointCount(currentVertices.size());

				for (int j=0; j<currentVertices.size(); j++)
				{

					sf::Vector2f currentPoint = currentVertices[j];
					currentPoint.x = currentPoint.x + currentHitbox->g_x + entity->global_x + entity->local_x;
					currentPoint.y = currentPoint.y + currentHitbox->g_y + entity->global_y + entity->local_y;
					//cout<<"DRAWING POINT: "<<currentVertices.size()<<" x: "<<currentPoint.x<<" y: "<<currentPoint.y<<endl;
					sf::CircleShape shape(5);
					shape.setFillColor(sf::Color(100, 250, 50));
					shape.setPosition(currentPoint.x-2.5,currentPoint.y-2.5);
					window.draw(shape);
					convex.setPoint(j, currentPoint);
					//cout << "convex.setPoint(" << j << ", " << currentPoint.x << " " << currentPoint.y << ");" << endl;

				}

				window.draw(convex);
			}
		}
}

std::vector<int> MissionController::DrawDamageCounters(sf::RenderWindow& window)
{
	vector<int> dmg_rm;

	for(int i=0; i<dmgCounters.size(); i++)
	{
		float a=0;

		for(int d=0; d<dmgCounters[i].spr.size(); d++)
		{
			if(dmgCounters[i].display_timer.getElapsedTime().asMilliseconds() > 70*d)
			{
				float curScale = dmgCounters[i].scale[d];
				float destScale = dmgCounters[i].scale_goal[d];

				if(dmgCounters[i].mode[d])
				{
					curScale -= float(14) / fps;
					dmgCounters[i].alpha[d] += float(1800) / fps;

					if(curScale <= destScale)
					{
						dmgCounters[i].mode[d] = false;
						destScale = 1;
					}

					if(dmgCounters[i].alpha[d] >= 255)
					dmgCounters[i].alpha[d] = 255;
				}

				if(!dmgCounters[i].mode[d])
				{
					if(!dmgCounters[i].mode[d])
					{
						curScale += float(8) / fps;

						if(curScale >= destScale)
						{
							curScale = destScale;
						}
					}
				}

				if(dmgCounters[i].display_timer.getElapsedTime().asMilliseconds() > 70*d + 1000)
				{
					if(!dmgCounters[i].mode[d])
					{
						dmgCounters[i].pos[d].y += float(60) / fps;
						dmgCounters[i].alpha[d] -= float(500) / fps;

						if(dmgCounters[i].alpha[d] <= 0)
						dmgCounters[i].alpha[d] = 0;
					}
				}

				dmgCounters[i].scale[d] = curScale;
				dmgCounters[i].scale_goal[d] = destScale;

				dmgCounters[i].spr[d].setPosition(dmgCounters[i].pos[d].x, dmgCounters[i].pos[d].y-((curScale-1)*10));
				dmgCounters[i].spr[d].setScale(curScale, curScale);
				dmgCounters[i].spr[d].setColor(sf::Color(255,255,255,dmgCounters[i].alpha[d]));

				dmgCounters[i].spr[d].draw(window);

				a += dmgCounters[i].alpha[d];
			}
		}

		if(a <= 1)
		dmg_rm.push_back(i);
	}

	return dmg_rm;
}

std::vector<int> MissionController::DrawEntities(sf::RenderWindow& window)
{
	//cout << "[MissionController::DrawEntities] Start" << endl;
	vector<int> tlo_rm;

	/** Find the farthest unit in your army (for calculations) **/
	int farthest_id = -1;
	float temp_pos = -9999;

	//cout << "[MissionController::DrawEntities] Find farthest" << endl;
	for(int i=0; i<units.size(); i++)
	{
		PlayableUnit* unit = units[i].get();

		if(temp_pos <= unit->getGlobalPosition().x)
		{
			temp_pos = unit->getGlobalPosition().x;
			farthest_id = i;
		}
	}

	for (int i=0; i<tangibleLevelObjects.size(); i++)
	{
		//cout << "[MissionController::DrawEntities] Process entity " << i << endl;
		Entity* entity = tangibleLevelObjects[i].get();

		entity->fps = fps;

		if(entity->getEntityID() == 1)
		{
			entity->doRhythm(rhythm.current_song, rhythm.rhythmController.current_drum, rhythm.GetCombo(), rhythm.GetRealCombo(), rhythm.advanced_prefever, rhythm.r_gui.beatBounce, rhythm.GetSatisfaction());

			if(!missionEnd)
			entity->Draw(window);
		}
		else
		{
			///Check if entity is off bounds, if yes, don't render it.
			entity->offbounds = false;

			if(entity->getGlobalPosition().x > (camera.followobject_x)/(window.getSize().x / float(1280))+2400)
			entity->offbounds = true;

			if(entity->getGlobalPosition().x < (camera.followobject_x)/(window.getSize().x / float(1280))-1000)
			entity->offbounds = true;

			entity->distance_to_unit = abs(temp_pos - entity->getGlobalPosition().x);

			///Check for entity attack measures
			if(entity->doAttack())
			{
				///ID 6,16 = Kirajin Yari
				if((entity->getEntityID() == 6) || (entity->getEntityID() == 16))
				{
					cout << "Entity " << i << " threw a spear!" << endl;

					float rand_hs = (rand() % 1000) / float(10);
					float rand_vs = (rand() % 1000) / float(10);

					float rand_rad = (rand() % 200000000) / float(1000000000);

					float mindmg = entity->mindmg*entity->stat_multiplier;
					float maxdmg = entity->maxdmg*entity->stat_multiplier;

					float xpos = entity->getGlobalPosition().x+entity->hitBox.left+entity->hitBox.width/2;
					float ypos = entity->getGlobalPosition().y+entity->hitBox.top+entity->hitBox.height/2;

					vector<Object>* obj;
					obj = entity->objects.get();

					spawnProjectile((*obj)[1].s_obj, xpos, ypos, 800, -450-rand_hs, -450+rand_vs, -2.58 + rand_rad, maxdmg, mindmg, 0, true);
				}
			}

			///Check if entity's parent is still alive, if not, kill the entity
			int parent = entity->parent;

			///Check if parent is defined
			if(parent != -1)
			{
				///Look for parent
				auto it = find_if(tangibleLevelObjects.begin(), tangibleLevelObjects.end(), [&parent](const std::unique_ptr<Entity>& obj) {return obj.get()->spawnOrderID == parent;});

				if(it != tangibleLevelObjects.end())
				{
					///Parent has been found!

					auto index = std::distance(tangibleLevelObjects.begin(), it);
					//cout << "My parent is currently residing at " << index << endl;

					///Check if it's dead
					if(tangibleLevelObjects[index].get()->dead)
					{
						///Kill the entity
						entity->die();
					}
				}
				else ///Parent can't be found
				{
					///Kill the entity
					entity->die();
				}
			}

			//cout << "[MissionController::DrawEntities] Draw entity" << endl;
			entity->Draw(window);
		}

		//cout << "[MissionController::DrawEntities] Check if finished" << endl;
		if(entity->ready_to_erase)
		tlo_rm.push_back(i);
	}

	//cout << "[MissionController::DrawEntities] End" << endl;
	return tlo_rm;
}

std::vector<int> MissionController::DrawUnits(sf::RenderWindow& window)
{
	vector<int> units_rm;

	int farthest_id = -1;
	int closest_entity_id = -1;
	int closest_entity_pos = 9999;

	bool hatapon = false;

	auto max_distance = std::max_element( units.begin(), units.end(),
							 []( unique_ptr<PlayableUnit> &a, unique_ptr<PlayableUnit> &b )
							 {
								 return a->global_x < b->global_x;
							 } );

	farthest_id = distance(units.begin(), max_distance);

	/** Units draw loop and entity range detection **/

	for(int i=0; i<tangibleLevelObjects.size(); i++)
	{
		Entity* entity = tangibleLevelObjects[i].get();

		if((entity->entityType == Entity::EntityTypes::HOSTILE) && (!entity->dead))
		{
			if(entity->getGlobalPosition().x + entity->hitboxes[0].o_x < closest_entity_pos)
			{
				closest_entity_pos = entity->getGlobalPosition().x + entity->hitboxes[0].o_x;
				closest_entity_id = i;
			}
		}
	}

	if(farthest_id != -1)
	{
		bool inRange = true;

		if(closest_entity_id == -1)
		{
			inRange = false;
		}
		else
		{
			Entity* closest_entity = tangibleLevelObjects[closest_entity_id].get();

			//cout << "Check if entity is in range" << endl;

			for(int i=0; i<units.size(); i++)
			{
				if(units[i].get()->getUnitID() != 0)
				{
					if((closest_entity->entityType == Entity::EntityTypes::HOSTILE) && (!closest_entity->dead))
					{
						//cout << "Range of unit " << i << ": " << abs((units[i].get()->getGlobalPosition().x) - closest_entity->getGlobalPosition().x) - 110 << endl;
						//cout << "Dest local x: " << units[i].get()->dest_local_x << endl;

						if(abs((units[i].get()->global_x) - (closest_entity->getGlobalPosition().x + closest_entity->hitboxes[0].o_x)) > 900)
						inRange = false;
					}
				}
			}

			//cout << "In range: " << inRange << endl;
		}

		/** Draw the units **/
		for(int i=0; i<units.size(); i++)
		{
			PlayableUnit* unit = units[i].get();

			if(closest_entity_id != -1)
			{
				Entity* closest_entity = tangibleLevelObjects[closest_entity_id].get();

				unit->entity_distance = abs((unit->global_x) - (closest_entity->getGlobalPosition().x + (closest_entity->hitboxes[0].o_x - closest_entity->hitboxes[0].o_width / 2)));
				//cout << "Distance to nearest entity for unit " << i << ": " << unit->entity_distance << " (" << unit->global_x << " " << closest_entity->getGlobalPosition().x << " " << closest_entity->hitboxes[0].o_x << ")" << endl;

				if((closest_entity->entityType == Entity::EntityTypes::HOSTILE) && (!closest_entity->dead) && (inRange))
				{
					unit->enemy_in_range = true;
					//cout << "Unit " << i << " distance to closest enemy is " << unit->entity_distance << " pixels" << endl;
				}
				else
				{
					unit->enemy_in_range = false;
					unit->dest_local_x = 0;
				}
			}
			else
			{
				unit->enemy_in_range = false;
			}

			if(unit->getUnitID() != 0)
			{
				float unit_distance = 9999;

				for(int a=0; a<units.size(); a++)
				{
					if(a != i)
					{
						if(units[a].get()->getUnitID() == 1)
						{
							float gx = units[a].get()->getGlobalPosition().x;
							float my_gx = unit->getGlobalPosition().x;

							float dis = abs(gx-my_gx);

							if(dis < unit_distance)
							unit_distance = dis;
						}
					}
				}

				unit->unit_distance = unit_distance;

				//cout << "Unit " << i << " distance to another unit is " << unit_distance << " pixels" << endl;
			}

			/** Verify if Hatapon exists **/
			if(unit->getUnitID() == 0)
			{
				if(unit->getUnitHP() > 0)
				hatapon = true;
			}

			/** Execute unit features when mission is not over **/
			if(!missionEnd)
			{
				unit->doRhythm(rhythm.current_song, rhythm.rhythmController.current_drum, rhythm.GetCombo());

				if(unit->getUnitID() != 0) /// Yaripon
				{
					if(unit->doAttack())
					{
						cout << "Unit " << i << " threw a spear!" << endl;

						float rand_hs = (rand() % 1000) / float(10);
						float rand_vs = (rand() % 1000) / float(10);

						float rand_rad = (rand() % 200000000) / float(1000000000);

						int rhythm_acc = rhythm.current_perfect; ///Check how many perfect measures has been done to improve the spears throwing
						float fever_boost = 0.8;

						if(rhythm.GetCombo() >= 11) ///Check for fever to boost the spears damage
						fever_boost = 1.0;

						float mindmg = unit->mindmg * (0.8 + (rhythm_acc*0.05)) * fever_boost;
						float maxdmg = unit->maxdmg * (0.8 + (rhythm_acc*0.05)) * fever_boost;

						if(unit->defend)
						{
							mindmg = mindmg * 0.75;
							maxdmg = maxdmg * 0.75;

							if(unit->charged)
							{
								mindmg = mindmg * 1.666;
								maxdmg = maxdmg * 1.666;
							}
						}
						else
						{
							if(unit->charged)
							{
								mindmg = mindmg*2.2;
								maxdmg = maxdmg*2.2;
							}
						}

						///Make the spears be thrown with worse velocity when player is drumming bad (10% punishment)
						rand_hs *= 0.9 + (rhythm_acc*0.025);
						rand_vs *= 0.9 + (rhythm_acc*0.025);

						///This way, the lowest damage is dmg * 0.64 (36% punishment) and highest damage is 100% of the values

						float xpos = unit->getGlobalPosition().x+unit->hitBox.left+unit->hitBox.width/2;
						float ypos = unit->getGlobalPosition().y+unit->hitBox.top+unit->hitBox.height/2;

						float vspeed = -450+rand_vs;

						if(unit->defend)
						{
							if(unit->isFever)
							vspeed = fabs(vspeed)-250;
						}

						vector<Object>* obj;
						obj = unit->objects.get();

						spawnProjectile((*obj)[1].s_obj, xpos, ypos, 800, 450+rand_hs, vspeed, -0.58 - rand_rad, maxdmg, mindmg, 0);
					}
				}
			}

			if(missionEnd)
			{
				if(!failure)
				unit->doMissionEnd();
			}

			unit->fps = fps;
			unit->Draw(window);

			if(unit->ready_to_erase)
			units_rm.push_back(i);
		}
	}

	/** Fail the mission if Hatapon is dead or when Hatapon is the only unit remaining **/

	if((!hatapon) || ((hatapon) && (units.size() <= 1)))
	{
		missionEnd = true;
		failure = true;
		rhythm.Stop();
	}

	return units_rm;
}

void MissionController::Update(sf::RenderWindow &window, float cfps, InputController& inputCtrl)
{
	//cout << "[MissionController] Update START FRAME" << endl;

	///remove stopped sounds
	for(int i=projectile_sounds.size()-1; i>0; i--)
	{
		if(projectile_sounds[i].getStatus() == sf::Sound::Status::Stopped)
		{
			projectile_sounds.erase(projectile_sounds.begin()+i);
		}
		else
		{
			break;
		}
	}

	//cout << "[MissionController] Sort" << endl;

	///Sort tangibleLevelObjects to prioritize rendering layers
	std::sort(tangibleLevelObjects.begin(), tangibleLevelObjects.end(),
			  [](const std::unique_ptr<Entity>& a, const std::unique_ptr<Entity>& b)
				{
					return a.get()->layer < b.get()->layer;
				});

	///Globally disable the controls when Dialogbox is opened, but preserve original controller for controlling the DialogBoxes later
	InputController o_inputCtrl;
	InputController cur_inputCtrl;

	cur_inputCtrl = inputCtrl;

	if(dialog_boxes.size() > 0)
	{
		o_inputCtrl = inputCtrl;

		InputController a;
		cur_inputCtrl = a;
	}

	/** Update loop, everything here happens per each frame of the game **/
	fps = cfps;

	/** Apply the keyMap from parent class **/

	/** Execute camera and background **/

	//cout << "[MissionController] Camera & BG" << endl;
	camera.Work(window,fps,cur_inputCtrl);
	test_bg.setCamera(camera);
	test_bg.Draw(window);

	/** Execute Keyboard events and Movement **/

	//cout << "[MissionController] Input & Movement" << endl;
	DoKeyboardEvents(window,fps,cur_inputCtrl);
	DoMovement(window,fps,cur_inputCtrl);

	vector<int> k_e;

	//cout << "[MissionController] Clones" << endl;
	/** Spawn cloneable entities **/
	for(int i=0; i<kirajins.size(); i++)
	{
		Kirajin_Yari_2* entity = kirajins[i].get();

		///Check if the kirajin should respawn or not
		bool respawn = true;

		for(int t=0; t<tangibleLevelObjects.size(); t++)
		{
			if(tangibleLevelObjects[t].get()->entityID == entity->entityID)
			{
				if(tangibleLevelObjects[t].get()->cloneable)
				{
					///check if unit is already spawned
					if(tangibleLevelObjects[t].get()->spawnOrderID == entity->spawnOrderID)
					respawn = false;
				}
			}
		}

		///Check if entity's parent is still alive, if not, kill the entity
		int parent = entity->parent;
		bool dead_parent = false;

		///Check if parent is defined
		if(parent != -1)
		{
			///Look for parent
			auto it = find_if(tangibleLevelObjects.begin(), tangibleLevelObjects.end(), [&parent](const std::unique_ptr<Entity>& obj) {return obj.get()->spawnOrderID == parent;});

			if(it != tangibleLevelObjects.end())
			{
				///Parent has been found!

				auto index = std::distance(tangibleLevelObjects.begin(), it);
				//cout << "My parent is currently residing at " << index << endl;

				///Check if it's dead
				if(tangibleLevelObjects[index].get()->dead)
				{
					///Kill the entity
					dead_parent = true;
				}
			}
			else ///Parent can't be found
			{
				///Kill the entity
				dead_parent = true;
			}
		}

		/** Find the farthest unit in your army (for calculations) **/
		int farthest_id = -1;
		float temp_pos = -9999;

		for(int i=0; i<units.size(); i++)
		{
			PlayableUnit* unit = units[i].get();

			if(temp_pos <= unit->getGlobalPosition().x)
			{
				temp_pos = unit->getGlobalPosition().x;
				farthest_id = i;
			}
		}

		entity->distance_to_unit = abs(temp_pos - entity->getGlobalPosition().x);

		if(entity->distance_to_unit <= 1000)
		{
			if(!dead_parent)
			{
				if(respawn)
				{
					if(entity->respawn_clock.getElapsedTime().asSeconds() > entity->respawnTime)
					{
						entity->respawn_clock.restart();

						tangibleLevelObjects.push_back(make_unique<Kirajin_Yari_2>(*entity));
					}
				}
			}
			else
			{
				///remove this specific kirajin
				k_e.push_back(i);
			}
		}
	}

	for(int i=0; i<k_e.size(); i++)
	{
		kirajins.erase(kirajins.begin()+k_e[i]-i);
	}

	/** Draw all Entities **/
	//cout << "[MissionController] Entities" << endl;

	vector<int> tlo_rm = DrawEntities(window);

	/** Draw all Units **/

	//cout << "[MissionController] Units" << endl;
	vector<int> units_rm = DrawUnits(window);

	/** Draw projectiles **/

	//cout << "[MissionController] Projectiles" << endl;
	vector<int> pr_rm = DrawProjectiles(window);

	//cout << "[MissionController] Clouds" << endl;
	/** Draw message clouds **/
	for(int e=0; e<tangibleLevelObjects.size(); e++)
	{
		Entity* entity = tangibleLevelObjects[e].get();
		entity->doMessages(window, fps, inputCtrl);
	}

	/** Draw hitboxes **/

	if(showHitboxes)
	{
		//cout << "[MissionController] Hitboxes" << endl;
		DrawHitboxes(window);
	}

	/** Draw damage counters **/
	//cout << "[MissionController] Damage" << endl;
	vector<int> dmg_rm = DrawDamageCounters(window);

	/**  Draw static UI elements **/

	auto lastView = window.getView();
	window.setView(window.getDefaultView());

	/**

	if(cutscenesLeft && !inCutscene && isMoreCutscenes())
	{
		StartCutscene(cutscene_text_identifiers[currentCutsceneId],cutscene_blackscreens[currentCutsceneId],cutscene_lengths[currentCutsceneId]);
	}

	sf::Time currentTime = timer.getElapsedTime();
	int currentAlpha = startAlpha;
	if (currentTime >= targetTime && inCutscene)
	{
		// oops: currentAlpha = endAlpha; // make certain that the alpha is at its final destination
		//you are done
		if(!isMoreCutscenes())
		{
			currentAlpha = startAlpha;
			inCutscene = false;
			if(isBlackScreenCutscene)
			{
				inFadeTransition = true;
				timer.restart();
				targetTime = sf::seconds(2);
			}
			else
			{
				FinishLastCutscene();
			}
			cutscenesLeft=false;
		}
		else
		{
			/// next cutscene
			currentCutsceneId++;
			StartCutscene(cutscene_text_identifiers[currentCutsceneId],cutscene_blackscreens[currentCutsceneId],cutscene_lengths[currentCutsceneId]);
		}
	}
	else if (currentTime >= targetTime && !inCutscene && inFadeTransition)
	{
		currentAlpha = endAlpha;
		inFadeTransition = false;
		FinishLastCutscene();
	}
	else if (!inCutscene && inFadeTransition)
	{
		currentAlpha = startAlpha + (endAlpha - startAlpha) * (currentTime.asMilliseconds() / (targetTime.asMilliseconds()+0.0));
	}
	else if (inCutscene && isBlackScreenCutscene)
	{
		currentAlpha = startAlpha;
	}
	else if (inCutscene)
	{
		currentAlpha = startAlpha;
	}
	// apply alpha to whatever colour is previously set
	if((inFadeTransition || inCutscene) && isBlackScreenCutscene)
	{
		sf::Color fadeColor = fade.getFillColor();
		fadeColor.a = currentAlpha;
		fade.setFillColor(fadeColor);
		fade.setSize(sf::Vector2f(window.getSize().x,window.getSize().y));

		fade.setPosition(0,0);
		window.draw(fade);
	}
	if (inCutscene)
	{
		for (int i=0; i<t_cutscene_text.size(); i++)
		{
			sf::Text currentLine = t_cutscene_text[i];

			currentLine.setPosition(window.getSize().x/2,300 + 39*i);
			sf::Time currentTime = timer.getElapsedTime();

			window.draw(currentLine);
		}
	}*/

	/** Draw the timer (Static UI element) **/

	if(showTimer)
	{
		t_timerMenu.setString(Func::ConvertToUtf8String(std::to_string(missionTimer.getElapsedTime().asSeconds())+" Seconds"));
		t_timerMenu.setOrigin(t_timerMenu.getGlobalBounds().width/2,t_timerMenu.getGlobalBounds().height/2);
		t_timerMenu.setPosition(window.getSize().x/2,100);
		window.draw(t_timerMenu);
	}

	/** Draw floor **/

	float resRatioX = window.getSize().x / float(1280);
	float resRatioY = window.getSize().y / float(720);
	r_floor.setSize(sf::Vector2f(1280*resRatioX, 110*resRatioY));
	r_floor.setFillColor(sf::Color::Black);
	r_floor.setPosition(0,610*resRatioY);
	window.draw(r_floor);

	//cout << "[MissionController] UI" << endl;
	drawCommandList(window);
	DrawUnitThumbs(window);
	DrawPickedItems(window);

	/** If mission isn't finished, execute and draw Rhythm **/

	if(!missionEnd)
	{
		//ctrlTips.x = 0;
		//ctrlTips.y = (720-ctrlTips.ySize);
		//ctrlTips.draw(window);

		//cout << "[MissionController] Rhythm" << endl;
		rhythm.fps = fps;
		DoRhythm(cur_inputCtrl);
		rhythm.Draw(window);
	}

	/** Execute all mission end related things **/
	//cout << "[MissionController] Mission End" << endl;
	DoMissionEnd(window, fps);

	//cout << "[MissionController] Dialogboxes" << endl;
	if(dialog_boxes.size() > 0)
	{
		if(o_inputCtrl.isKeyPressed(InputController::Keys::CROSS))
		{
			switch(dialog_boxes[dialog_boxes.size()-1].CheckSelectedOption())
			{
				case 0:
				{
					if(dialog_boxes[dialog_boxes.size()-1].id == 0)
					{
						cout << "Return to Patapolis" << endl;
						dialog_boxes[dialog_boxes.size()-1].Close();

						missionEnd = true;
						failure = true;
						rhythm.Stop();
					}
					else if(dialog_boxes[dialog_boxes.size()-1].id == 999)
					{
						cout << "Enable hitboxes" << endl;
						showHitboxes = true;
						dialog_boxes[dialog_boxes.size()-1].Close();
					}

					break;
				}

				case 1:
				{
					if(dialog_boxes[dialog_boxes.size()-1].id == 0)
					{
						cout << "Back to Mission" << endl;
						dialog_boxes[dialog_boxes.size()-1].Close();
					}
					else if(dialog_boxes[dialog_boxes.size()-1].id == 999)
					{
						cout << "Disable hitboxes" << endl;
						showHitboxes = false;
						dialog_boxes[dialog_boxes.size()-1].Close();
					}

					break;
				}

				case 2:
				{
					if(dialog_boxes[dialog_boxes.size()-1].id == 999)
					{
						cout << "Heal all units" << endl;
						for(int u = 0; u < units.size(); u++)
						{
							units[u].get()->current_hp = units[u].get()->max_hp;
						}

						dialog_boxes[dialog_boxes.size()-1].Close();
					}

					break;
				}
			}
		}
	}

	vector<int> db_e; ///dialog box erase

	for(int i = 0; i < dialog_boxes.size(); i++)
	{
		dialog_boxes[i].x = 640;
		dialog_boxes[i].y = 360;
		dialog_boxes[i].Draw(window, fps, o_inputCtrl);

		if(dialog_boxes[i].closed)
		db_e.push_back(i);
	}

	for(int i = 0; i < db_e.size(); i++)
	{
		dialog_boxes.erase(dialog_boxes.begin()+db_e[i]-i);
	}

	window.setView(lastView);

	/** Remove vector objects that are no longer in use **/

	//cout << "[MissionController] Cleanup" << endl;
	DoVectorCleanup(units_rm, dmg_rm, tlo_rm, pr_rm);

	//cout << "[MissionController] FRAME END" << endl;
}
void MissionController::FinishLastCutscene()
{
	/// runs when the last cutscene of a sequence is done
}
bool MissionController::isMoreCutscenes()
{
	/// returns true if there are more cutscenes
	return currentCutsceneId<cutscene_text_identifiers.size()-1;
}
void MissionController::StartCutscene(const std::wstring& text,bool isBlackScreen, int TimeToShow)
{
	/// because the description needs to be able to go over multiple lines, we have to split it into a series of lines
	t_cutscene_text.clear();
	std::vector<std::wstring> wordsinDesc = Func::Split(thisConfig->strRepo.GetUnicodeString(text),' ');
	sf::String oldTotalString;
	sf::String currentTotalString;
	int maxWidth = thisConfig->GetInt("resX") * 0.4;
	timer.restart();
	inCutscene = true;
	isBlackScreenCutscene = isBlackScreen;
	targetTime = sf::seconds(TimeToShow);
	/// we split it into words, then go word by word testing the width of the string
	for (int i=0; i<wordsinDesc.size(); i++)
	{
		std::wstring currentWord = wordsinDesc[i];
		currentTotalString = currentTotalString + Func::ConvertToUtf8String(currentWord) + L" ";
		sf::Text t_newLine;
		t_newLine.setFont(f_font);
		t_newLine.setCharacterSize(35);
		t_newLine.setFillColor(sf::Color::White);
		t_newLine.setString(currentTotalString);
		if (t_newLine.getGlobalBounds().width>maxWidth)
		{
			/// when the string is too long, we go back to the last string and lock it in, then start a new line
			currentTotalString = oldTotalString;
			t_newLine.setString(currentTotalString);
			t_newLine.setOrigin(t_newLine.getGlobalBounds().width/2,t_newLine.getGlobalBounds().height/2);
			t_cutscene_text.push_back(t_newLine);
			oldTotalString = currentWord+L" ";
			currentTotalString = currentWord+L" ";
		}
		oldTotalString = currentTotalString;
		/// if there are no more words, finish up the current line
		if (i+1==wordsinDesc.size())
		{
			currentTotalString = oldTotalString;
			t_newLine.setString(currentTotalString);
			t_newLine.setOrigin(t_newLine.getGlobalBounds().width/2,t_newLine.getGlobalBounds().height/2);
			t_cutscene_text.push_back(t_newLine);
			oldTotalString = "";
			currentTotalString = "";
		}
	}
}
MissionController::~MissionController()
{
	//dtor

}
