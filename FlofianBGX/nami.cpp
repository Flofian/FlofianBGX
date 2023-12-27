#include "../plugin_sdk/plugin_sdk.hpp"
#include "nami.h"
#include "../spelldb/SpellDB.h"
#include <inttypes.h>
namespace nami {
	std::string VERSION = "1.0.0";
	script_spell* q = nullptr;
	script_spell* w = nullptr;
	script_spell* e = nullptr;
	script_spell* r = nullptr;

	TreeTab* mainMenuTab = nullptr;
	TreeTab* spelldb = nullptr;

	std::map<std::string, TreeTab*> eDB;
	float GLOBALSCALINGFACTOR = 1.0f;
	float MinERemainingTime = 0.1f;
	struct circSpellData {
		float radius = 0.f;
		float delay = -1.f;
		circSpellData(float r = 0.f, float d = -1.f) {
			radius = r * GLOBALSCALINGFACTOR;
			delay = d;
		}
	};
	struct linSpellData {
		float radius;
		bool useBoundingRadius;
		float speed;
		std::vector<collisionable_objects> collision;
		spellslot slot;
		linSpellData(float r, float s) {
			radius = r;
			speed = s;
			collision = {};
		}
		linSpellData(float r, float s, std::vector<collisionable_objects> c, spellslot _slot = spellslot::invalid) {
			radius = r;
			speed = s;
			collision = c;
			useBoundingRadius = true;
			slot = _slot;
		}
		linSpellData() {
			radius = 0;
			speed = 0;
			collision = {};
		}
	};
	std::map<uint32_t, circSpellData> circSpellDB;
	std::map<uint32_t, linSpellData> linSpellDB;
	struct circSpell {
		float castStartTime = 0.f;
		float delay = 0.f;
		vector castPos = vector();
		vector endPos = vector();
		float radius = 0.f;
		uint32_t sender;
		spellslot slot;
		circSpell(float _castStartTime, float _delay, vector _castpos, vector _endpos, float _radius) {
			castStartTime = _castStartTime;
			delay = _delay;
			castPos = _castpos;
			endPos = _endpos;
			radius = _radius;
		}
		circSpell(float _delay, vector _castpos, vector _endpos, float _radius) {
			castStartTime = gametime->get_time();
			delay = _delay;
			castPos = _castpos;
			endPos = _endpos;
			radius = _radius;
		}
		circSpell(vector _castpos, vector _endpos, uint32_t _spellHash, game_object_script send, spellslot _slot) {
			auto s = circSpellDB.find(_spellHash);
			if (s != circSpellDB.end()) {
				delay = s->second.delay;
				radius = s->second.radius;
			}
			castStartTime = gametime->get_time();
			castPos = _castpos;
			endPos = _endpos;
			sender = send->get_network_id();
			slot = _slot;
		}
	};
	struct linSpell {
		linSpellData data;
		vector startpos;
		vector endpos;
		float startTime;
		linSpell(uint32_t hash, vector sp, vector ep) {
			data = linSpellDB[hash];
			startpos = sp;
			endpos = ep;
			startTime = gametime->get_time();
		}
		linSpell() {
			data = linSpellData();
			startpos = vector();
			endpos = vector();
			startTime = gametime->get_time();
		}
	};
	std::vector<circSpell> circularSpells;
	std::vector<linSpell> linearSpells;
	std::vector<game_object_script> missileList;
	

	namespace generalMenu {
		TreeEntry* debug = nullptr;
	}
	namespace qMenu {

	}
	namespace wMenu {

	}
	namespace eMenu {
		TreeEntry* mode = nullptr;
		TreeTab* useOn = nullptr;
		TreeEntry* overwrite = nullptr;
		TreeEntry* remainingTime = nullptr;
	}
	namespace rMenu {

	}namespace drawMenu
	{
		TreeEntry* drawOnlyReady = nullptr;
		TreeEntry* drawRangeQ = nullptr;
		TreeEntry* drawRangeW = nullptr;
		TreeEntry* drawRangeE = nullptr;
		TreeEntry* drawRangeR = nullptr;
		TreeEntry* drawESpells = nullptr;
	}
	namespace colorMenu
	{
		TreeEntry* qColor = nullptr;
		TreeEntry* wColor = nullptr;
		TreeEntry* eColor = nullptr;
		TreeEntry* eSpellColor = nullptr;
		TreeEntry* rColor = nullptr;
	}
	std::set<std::string> supportedSpells = {
		// I want to kill myself
		"BrandW",
		"KarthusQ",
		"CassiopeiaQ",
		"ChogathQ",
		"GragasR",
		"KogmawR",
		"LeonaR",
		"MissFortuneR",
		"NamiQ",
		"SwainW",
		"SyndraQ",
		"VarusE",
		"VeigarW",
		"ViktorR",
		"VolibearE",
		"VolibearR",
		"XerathW",
		"XerathR",

		"BrandQ",
		"BraumQ",
		"CaitlynQ",
		"CaitlynE",
		"CorkiR",
		"DrMundoQ",
		"DravenE",
		"DravenR",
		"JhinR",
		"JinxW",
		"JinxR",
		"KaisaW",
		"KalistaQ",
		"KennenQ",
		"KogMawQ",
		"KogMawE",
		"LeblancE",
		"LeeSinQ",
		"MorganaQ",
		"NamiR",
		"NeekoE",
		"NidaleeQ",
		"NocturneQ",
		"QuinnQ",
		"RenataQ",
		"RengarE",
		"RyzeQ",
		"SamiraQ",
		"SivirQ",
		"SkarnerE",
		"TwistedFateQ",
		"VarusQ",
		"VarusR",
		"VeigarQ",
		"VelkozQ",
		"VelkozW",
		"VexR",
		"XayahE",
		"XinZhaoW",
		"YasuoQ",
		"YoneQ",
		"ZedQ",
		"ZeriQ",
		"ZeriW",
		"ZoeQ",
		"ZoeE",
		"ZyraE"
	};
	std::string spellSlotName(spellslot s) {
		switch (s)
		{
		case spellslot::q:
			return "Q";
			break;
		case spellslot::w:
			return "W";
			break;
		case spellslot::e:
			return "E";
			break;
		case spellslot::r:
			return "R";
			break;
		default:
			return "";
		}
	}
	std::string spellSlotName(spell_instance_script s) {
		if (s->is_auto_attack()) {
			return "AA";
		}
		else {
			return spellSlotName(s->get_spellslot());
		}
	}
	/*void on_draw() {
		auto tab = eDB[myhero->get_name_cstr()];
		if (!tab) {
			console->print("No tab");
			return;
		}
		//draw_manager->add_circle(myhero->get_position(), 50, MAKE_COLOR(255*eDB[myhero->get_model_cstr()]->get_entry("W")->get_bool(), 255, 0, 255));
	}*/
	void initSpellDB() {
		// I feel like i should just get those from the game? but i need to verify anyways

		// Spells with Speed that dont work: CorkiQ, LeblancW, MalphiteR, NeekoQ, SyndraW?
		// Bugged Spells (need to be clamped to max range): KassadinR, LilliaW, ViegoR
		// Vex E size depends on cast range???

		circSpellDB[spell_hash("BrandW")] = circSpellData(260.f, 0.891f);
		// Karthus Q is wrong but i dont know how to see the animation
		circSpellDB[spell_hash("KarthusLayWasteA1")] = circSpellData(160.f, 0.625f + 0.25f);//Karthus Q
		circSpellDB[spell_hash("CassiopeiaQ")] = circSpellData(200.f, 0.693f);
		circSpellDB[spell_hash("Rupture")] = circSpellData(250.f, 1.155f);//Cho Q
		circSpellDB[spell_hash("GragasR")] = circSpellData(400.f, 0.792f);
		circSpellDB[spell_hash("KogMawLivingArtillery")] = circSpellData(240.f, 1.2f);
		circSpellDB[spell_hash("LeonaSolarFlare")] = circSpellData(325.f, 0.891f);
		circSpellDB[spell_hash("MissFortuneScattershot")] = circSpellData(350.f, 0.264f);
		circSpellDB[spell_hash("NamiQ")] = circSpellData(200.f, 0.264f + 0.726f);
		circSpellDB[spell_hash("SwainW")] = circSpellData(325.f, 0.264f + 1.282f);	// uhh delay wrong?
		circSpellDB[spell_hash("SyndraQ")] = circSpellData(210.f, 0.627f);
		circSpellDB[spell_hash("VarusE")] = circSpellData(300.f, 0.792f);
		circSpellDB[spell_hash("VeigarDarkMatter")] = circSpellData(240.f, 1.221f);
		circSpellDB[spell_hash("ViktorChaosStorm")] = circSpellData(350.f, 0.264f);
		circSpellDB[spell_hash("VolibearE")] = circSpellData(325.f, 2.f);
		circSpellDB[spell_hash("VolibearR")] = circSpellData(300.f, 1.f);
		circSpellDB[spell_hash("XerathArcaneBarrage2")] = circSpellData(275.f, 0.792f);
		circSpellDB[spell_hash("XerathLocusPulse")] = circSpellData(200.f, 0.627f);

		linSpellDB[spell_hash("EzrealQ")] = linSpellData(60, 2000, { collisionable_objects::minions, collisionable_objects::yasuo_wall }, spellslot::q);
		linSpellDB[spell_hash("EzrealR")] = linSpellData(160, 2000, { collisionable_objects::yasuo_wall }, spellslot::r);
		linSpellDB[spell_hash("BrandQMissile")] = linSpellData(60, 1600, { collisionable_objects::minions, collisionable_objects::yasuo_wall }, spellslot::q);
		linSpellDB[spell_hash("BraumQMissile")] = linSpellData(60, 1700, { collisionable_objects::minions, collisionable_objects::yasuo_wall }, spellslot::q);
		linSpellDB[spell_hash("CaitlynQ")] = linSpellData(60, 2200, { collisionable_objects::yasuo_wall }, spellslot::q);
		linSpellDB[spell_hash("CaitlynQ2")] = linSpellData(90, 2200, { collisionable_objects::yasuo_wall }, spellslot::q);
		linSpellDB[spell_hash("CaitlynEMissile")] = linSpellData(70, 1600, { collisionable_objects::yasuo_wall, collisionable_objects::minions }, spellslot::e);
		linSpellDB[spell_hash("MissileBarrageMissile")] = linSpellData(40, 2000, { collisionable_objects::yasuo_wall, collisionable_objects::minions }, spellslot::r);
		linSpellDB[spell_hash("MissileBarrageMissile2")] = linSpellData(40, 2000, { collisionable_objects::yasuo_wall, collisionable_objects::minions }, spellslot::r);
		linSpellDB[spell_hash("DrMundoQ")] = linSpellData(60, 2000, { collisionable_objects::yasuo_wall, collisionable_objects::minions }, spellslot::q);
		linSpellDB[spell_hash("DravenR")] = linSpellData(160, 2000, { collisionable_objects::yasuo_wall }, spellslot::r);
		linSpellDB[spell_hash("DravenDoubleShotMissile")] = linSpellData(130, 1400, { collisionable_objects::yasuo_wall }, spellslot::e);
		// Elise E? doesnt deal dmg? Also need to check: EzrealW, Trundle Pillar
		linSpellDB[spell_hash("JhinRShotMis")] = linSpellData(80, 5000, { collisionable_objects::yasuo_wall }, spellslot::r);
		linSpellDB[spell_hash("JhinRShotMis4")] = linSpellData(80, 5000, { collisionable_objects::yasuo_wall }, spellslot::r);
		linSpellDB[spell_hash("JinxWMissile")] = linSpellData(60, 3300, { collisionable_objects::yasuo_wall, collisionable_objects::minions }, spellslot::w);
		linSpellDB[spell_hash("JinxR")] = linSpellData(140, 1700, { collisionable_objects::yasuo_wall }, spellslot::r);
		// Jinx R Speed Changes, but it gets faster, so if my func says it will hit then it definitly hits
		linSpellDB[spell_hash("KaisaW")] = linSpellData(100, 1750, { collisionable_objects::yasuo_wall, collisionable_objects::minions }, spellslot::w);
		linSpellDB[spell_hash("KalistaMysticShotMisTrue")] = linSpellData(40, 2400, { collisionable_objects::yasuo_wall, collisionable_objects::minions }, spellslot::q);
		linSpellDB[spell_hash("KennenShurikenHurlMissile1")] = linSpellData(50, 1700, { collisionable_objects::yasuo_wall, collisionable_objects::minions }, spellslot::q);
		linSpellDB[spell_hash("KogMawQ")] = linSpellData(70, 1650, { collisionable_objects::yasuo_wall, collisionable_objects::minions }, spellslot::q);
		linSpellDB[spell_hash("KogMawVoidOozeMissile")] = linSpellData(120, 1400, { collisionable_objects::yasuo_wall }, spellslot::e);
		linSpellDB[spell_hash("LeblancEMissile")] = linSpellData(55, 1750, { collisionable_objects::yasuo_wall, collisionable_objects::minions }, spellslot::e);
		linSpellDB[spell_hash("LeblancREMissile")] = linSpellData(55, 1750, { collisionable_objects::yasuo_wall, collisionable_objects::minions }, spellslot::e);
		linSpellDB[spell_hash("BlindMonkQOne")] = linSpellData(60, 1800, { collisionable_objects::yasuo_wall, collisionable_objects::minions }, spellslot::q);
		linSpellDB[spell_hash("MorganaQ")] = linSpellData(70, 1200, { collisionable_objects::yasuo_wall, collisionable_objects::minions }, spellslot::q);
		linSpellDB[spell_hash("NamiRMissile")] = linSpellData(250, 850, { collisionable_objects::yasuo_wall }, spellslot::r);
		// Nautilus Q end pos doesnt work
		linSpellDB[spell_hash("NeekoE")] = linSpellData(70, 1300, { collisionable_objects::yasuo_wall }, spellslot::e);
		linSpellDB[spell_hash("JavelinToss")] = linSpellData(40, 1300, { collisionable_objects::yasuo_wall, collisionable_objects::minions }, spellslot::q);
		linSpellDB[spell_hash("NocturneDuskbringer")] = linSpellData(60, 1200, { collisionable_objects::yasuo_wall }, spellslot::q);
		// linSpellDB[spell_hash("PykeQRange")] = linSpellData(40, 1300, { collisionable_objects::yasuo_wall, collisionable_objects::minions }, true);
		// dont know speed
		// qiyana q, idk
		linSpellDB[spell_hash("QuinnQ")] = linSpellData(60, 1550, { collisionable_objects::yasuo_wall, collisionable_objects::minions }, spellslot::q);
		linSpellDB[spell_hash("RenataQ")] = linSpellData(70, 1450, { collisionable_objects::yasuo_wall, collisionable_objects::minions }, spellslot::q);
		linSpellDB[spell_hash("RengarEMis")] = linSpellData(70, 1500, { collisionable_objects::yasuo_wall, collisionable_objects::minions }, spellslot::e);
		linSpellDB[spell_hash("RengarEEmpMis")] = linSpellData(70, 1500, { collisionable_objects::yasuo_wall, collisionable_objects::minions }, spellslot::e);
		// Rumble E
		linSpellDB[spell_hash("RyzeQ")] = linSpellData(55, 1700, { collisionable_objects::yasuo_wall, collisionable_objects::minions }, spellslot::q);
		// ^Only direct hit, not for q over e
		linSpellDB[spell_hash("SamiraQGun")] = linSpellData(60, 2600, { collisionable_objects::yasuo_wall, collisionable_objects::minions }, spellslot::q);
		// Seraphine E
		linSpellDB[spell_hash("SivirQMissile")] = linSpellData(90, 1450, { collisionable_objects::yasuo_wall }, spellslot::q);
		linSpellDB[spell_hash("SivirQMissileReturn")] = linSpellData(100, 1200, { collisionable_objects::yasuo_wall }, spellslot::q);
		linSpellDB[spell_hash("SkarnerFractureMissile")] = linSpellData(70, 1500, { collisionable_objects::yasuo_wall }, spellslot::e);
		// Swain? Q E
		// Slyas E Decelerates
		// Tahm Kench Q Size depends on champ size
		// Taliyah Q Decelerates
		linSpellDB[spell_hash("SealFateMissile")] = linSpellData(40, 1000, { collisionable_objects::yasuo_wall }, spellslot::q);
		linSpellDB[spell_hash("VarusQMissile")] = linSpellData(70, 1900, { collisionable_objects::yasuo_wall }, spellslot::q);
		linSpellDB[spell_hash("VarusRMissile")] = linSpellData(120, 1500, { collisionable_objects::yasuo_wall }, spellslot::r);
		linSpellDB[spell_hash("VeigarBalefulStrikeMis")] = linSpellData(70, 2200, { collisionable_objects::yasuo_wall, collisionable_objects::minions }, spellslot::q);
		// ^ Not sure whats the best way to handle collision
		linSpellDB[spell_hash("VelkozQMissile")] = linSpellData(50, 1300, { collisionable_objects::yasuo_wall, collisionable_objects::minions }, spellslot::q);
		linSpellDB[spell_hash("VelkozQMissileSplit")] = linSpellData(45, 2100, { collisionable_objects::yasuo_wall, collisionable_objects::minions }, spellslot::q);
		linSpellDB[spell_hash("VelkozWMissile")] = linSpellData(87, 1700, { collisionable_objects::yasuo_wall }, spellslot::w);
		// vex q, 2 different speeds
		linSpellDB[spell_hash("VexR")] = linSpellData(130, 1600, { collisionable_objects::yasuo_wall }, spellslot::r);
		// Xayah Q Delay?
		linSpellDB[spell_hash("XayahEMissile")] = linSpellData(80, 4000, { collisionable_objects::yasuo_wall }, spellslot::e);
		linSpellDB[spell_hash("XinZhaoWMissile")] = linSpellData(40, 6250, { collisionable_objects::yasuo_wall }, spellslot::w);
		linSpellDB[spell_hash("YasuoQ3Mis")] = linSpellData(90, 1200, { collisionable_objects::yasuo_wall }, spellslot::q);
		linSpellDB[spell_hash("YoneQ3Missile")] = linSpellData(80, 1500, { collisionable_objects::yasuo_wall }, spellslot::q);
		linSpellDB[spell_hash("ZedQMissile")] = linSpellData(50, 1700, { collisionable_objects::yasuo_wall }, spellslot::q);
		linSpellDB[spell_hash("ZeriQMis")] = linSpellData(40, 2600, { collisionable_objects::yasuo_wall, collisionable_objects::minions }, spellslot::q);
		linSpellDB[spell_hash("ZeriQMisPierce")] = linSpellData(40, 2600, { collisionable_objects::yasuo_wall }, spellslot::q);
		linSpellDB[spell_hash("ZeriQMisEmpowered")] = linSpellData(40, 3400, { collisionable_objects::yasuo_wall, collisionable_objects::minions }, spellslot::q);
		linSpellDB[spell_hash("ZeriQMisEmpoweredPierce")] = linSpellData(40, 3400, { collisionable_objects::yasuo_wall }, spellslot::q);
		linSpellDB[spell_hash("ZeriW")] = linSpellData(40, 2500, { collisionable_objects::yasuo_wall, collisionable_objects::minions }, spellslot::w);
		linSpellDB[spell_hash("ZoeQMissile")] = linSpellData(50, 1200, { collisionable_objects::yasuo_wall, collisionable_objects::minions }, spellslot::q);
		linSpellDB[spell_hash("ZoeQMis2")] = linSpellData(70, 2500, { collisionable_objects::yasuo_wall, collisionable_objects::minions }, spellslot::q);
		linSpellDB[spell_hash("ZoeEMis")] = linSpellData(50, 1850, { collisionable_objects::yasuo_wall, collisionable_objects::minions }, spellslot::e);
		linSpellDB[spell_hash("ZyraE")] = linSpellData(70, 1150, { collisionable_objects::yasuo_wall }, spellslot::e);
	}
	void update_spells() {
		circularSpells.erase(std::remove_if(circularSpells.begin(), circularSpells.end(), [](circSpell x)
			{
				return x.castStartTime + x.delay <= gametime->get_time();
			}), circularSpells.end());
	}
	float timeToHit(circSpell c) {
		return c.castStartTime + c.delay - gametime->get_time();
	}
	float timeToHit(game_object_script missile, game_object_script target) {
		if (!target->is_targetable() || !target->is_visible()) return -1.f;
		auto hash = missile->get_missile_sdata()->get_name_hash();
		auto lit = linSpellDB[hash];
		vector pos = target->get_position().set_z();
		vector currentPos = missile->get_position().set_z();
		vector endpos = missile->missile_get_end_position().set_z();
		float radius = lit.radius + target->get_bounding_radius() * lit.useBoundingRadius;

		auto predinput = prediction_input();
		predinput.collision_objects = lit.collision;
		predinput._from = currentPos;
		predinput.radius = lit.radius;
		predinput.use_bounding_radius = lit.useBoundingRadius;
		auto col = prediction->get_collision({ target->get_position() }, &predinput);
		if (col.size() != 0) {
			auto c = vector();
			for (const auto& x : col) {
				draw_manager->add_circle(x->get_position(), x->get_bounding_radius(), MAKE_COLOR(255, 0, 255, 255));
				if (x->get_distance(currentPos) < c.distance(currentPos)) c = x->get_position();
			}
			if (c != vector()) endpos = c;
		}

		auto rec = geometry::rectangle(currentPos, endpos, radius);
		bool isInside = rec.to_polygon().is_inside(pos);
		if (isInside)
			draw_manager->add_circle(pos, 20, MAKE_COLOR(0, 255, 0, 255));
		if (!isInside) return -1.f;
		auto projinfo = pos.project_on(currentPos, endpos);
		auto distanceLeft = currentPos.distance(projinfo.segment_point);
		return distanceLeft / lit.speed;
	}
	bool semiGuaranteeHitCircular(circSpell c) {
		// this can not check for movementspeed boosts / dashes
		for (const auto& target : entitylist->get_enemy_heroes()) {
			float currentDistance = c.castPos.distance(target);
			float time = timeToHit(c);
			if (time < MinERemainingTime) continue;
			float maxDistance = target->get_move_speed() *time + currentDistance;
			if (maxDistance < c.radius) return true;
		}
		return false;
	}
	bool semiGuaranteeHitLinear(game_object_script missile) {
		auto hash = missile->get_missile_sdata()->get_name_hash();
		auto lit = linSpellDB[hash];
		for (const auto& target : entitylist->get_enemy_heroes()) {
			if (!target->is_targetable() || !target->is_visible()) continue;
			float time = timeToHit(missile, target);
			if (time < MinERemainingTime) continue;
			float currentDistance = target->get_position().project_on(missile->get_position(), missile->missile_get_end_position()).segment_point.distance(target);
			float maxDistance = target->get_move_speed() * time + currentDistance;
			if (maxDistance < lit.radius + lit.useBoundingRadius * target->get_bounding_radius()) return true;
		}
		return false;
	}

	int countWBounces(game_object_script firstTarget) {
		// Nami w targeting info:
		// when jumping from ally to enemy -> always takes nearest enemy
		// when from enemy to ally -> random? atleast no pattern
		// Also first missile has speed of 2500, the bounces only 1500 (according to game data)
		float bounceRange = 800;
		float wBaseHeal = 35 + 20 * w->level() + 0.25 * myhero->get_total_ability_power();
		// I dont care about antiheal, the bounce modifier, heal/shield power, etc
		// Because when doing it my way worst thing that can happen is that i waste some heal? but idc

		if (firstTarget->is_ally()) {
			// Need to split it so i can loop allies->enemies or enemies->allies
			// Okay not sure whats the best way to handle prediction here? If theres any good way?
			// maybe try a linear spell with radius 0 and just look at unit position?
			// Simple way is to estimate traveltime by distance at bounce start, is wrong, but should be close?
			float travelTime = firstTarget->get_distance(myhero) / 2500;
			auto predictedAllyPos = prediction->get_prediction(firstTarget, travelTime).get_unit_position();
			auto enemies = entitylist->get_enemy_heroes();
			int hitCount = (firstTarget->get_max_health() - firstTarget->get_health() > wBaseHeal);
			std::map<uint32_t, vector> firstBouncePredList;
			for (const auto& enemy : enemies) {
				firstBouncePredList[enemy->get_network_id()] = prediction->get_prediction(enemy, travelTime).get_unit_position();
			}
			enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [&](game_object_script x)
				{
					return !x->is_targetable() || !x->is_visible() || firstBouncePredList[x->get_network_id()].distance(firstTarget)> bounceRange;
				}), enemies.end());
			if (enemies.size() == 0) return hitCount;
			auto& nearestEnemy = *std::min_element(enemies.begin(), enemies.end(), [&](game_object_script a, game_object_script b) {
				return predictedAllyPos.distance(firstBouncePredList[a->get_network_id()]) < predictedAllyPos.distance(firstBouncePredList[b->get_network_id()]);
				});
			hitCount += 1;
			// I estimate the time it takes my w to reach the first target, then predict where it and the enemies are
			// then i take the nearest one
			// So now i have the target i w to and the target it will bounce to next
			// next bounce will be random, so i just say what is the max possible
			travelTime += firstBouncePredList[nearestEnemy->get_network_id()].distance(predictedAllyPos) / 1500;
			auto nearestEnemyPos = prediction->get_prediction(nearestEnemy, travelTime).get_unit_position();
			auto allies = entitylist->get_ally_heroes();
			std::map<uint32_t, vector> secondBouncePredList;
			for (const auto& ally : allies) {
				secondBouncePredList[ally->get_network_id()] = prediction->get_prediction(ally, travelTime).get_unit_position();
			}
			allies.erase(std::remove_if(allies.begin(), allies.end(), [&](game_object_script x)
				{
					return !x->is_targetable() || secondBouncePredList[x->get_network_id()].distance(nearestEnemyPos) > bounceRange || 
						x->get_handle() == firstTarget->get_handle() || firstTarget->get_max_health() - firstTarget->get_health() < wBaseHeal;
				}), allies.end());
			return hitCount + (allies.size() > 0);
		}
		if (firstTarget->is_enemy()) {
			float travelTime = firstTarget->get_distance(myhero) / 2500;
			auto firstEnemyPos = prediction->get_prediction(firstTarget, travelTime).get_unit_position();
			std::vector<int> bounceCountList = {};
			for (const auto& ally : entitylist->get_ally_heroes()) {
				auto allyPredPos = prediction->get_prediction(ally, travelTime).get_unit_position();
				if (allyPredPos.distance(firstEnemyPos) > bounceRange) continue;
				auto newTravelTime = travelTime + allyPredPos.distance(firstEnemyPos) / 1500;

				auto enemies = entitylist->get_enemy_heroes();
				std::map<uint32_t, vector> secondBouncePredList;
				for (const auto& enemy : enemies) {
					secondBouncePredList[enemy->get_network_id()] = prediction->get_prediction(enemy, newTravelTime).get_unit_position();
				}
				enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [&](game_object_script x)
					{
						return !x->is_targetable() || !x->is_visible() || secondBouncePredList[x->get_network_id()].distance(allyPredPos) > bounceRange ||x->get_handle() == firstTarget->get_handle();
					}), enemies.end());
				int currentcount = 1 + (ally->get_max_health() - ally->get_health() > wBaseHeal) + (enemies.size() > 0);
				// only count the ally if i actually heal, but allow bouncing even if i dont
				bounceCountList.push_back(currentcount);
			}
			int min = bounceCountList.size() == 0 ? 1 : *std::min_element(bounceCountList.begin(), bounceCountList.end());
			int max = bounceCountList.size() == 0 ? 1 : *std::max_element(bounceCountList.begin(), bounceCountList.end());
			// not sure if this is needed, but crashed before? so i do like this instead of std::minmax_element
			return 10* min + max;

		}
		return 0;
	}

	void on_update() {
		update_spells();
		//console->print("Active Spells: %i", circularSpells.size());

		
		// E Logic
		int eMode = eMenu::mode->get_int();
		if (e->is_ready() && (eMode == 0 || (eMode == 1 && orbwalker->harass()) || (eMode <= 2 && orbwalker->combo_mode()))) {
			int overwrite = eMenu::overwrite->get_int();
			for (const auto& ally : entitylist->get_ally_heroes()) {
				// Do ally on active Spell
				auto tab = eDB[ally->get_model_cstr()];
				if (!tab || !tab->get_entry("enable")->get_bool() || ally->get_distance(myhero)>e->range()) continue;
				auto activeSpell = ally->get_active_spell();
				if (!activeSpell) continue;
				auto target = entitylist->get_object(activeSpell->get_last_target_id());
				bool isTargeted = activeSpell->get_spell_data()->get_targeting_type() == spell_targeting::target;
				bool isTargetingEnemy = target && target->is_valid() && target->is_enemy() && target->is_ai_hero();
				bool isAuto = activeSpell->is_auto_attack() && !ally->is_winding_up();
				std::string spellName = spellSlotName(activeSpell);
				if (spellName == "") continue; // Unsupported Spell, items, idk
				bool isSupported = supportedSpells.find(ally->get_model_cstr() + spellName) != supportedSpells.end();
				bool isEnabled = isSupported ? tab->get_entry(spellName)->get_int() == 2 : tab->get_entry(spellName)->get_bool();
				// this can be compressed, but i keep it like this for clarity
				bool useE = (overwrite == 0 && isEnabled && (!isTargeted || isTargetingEnemy)) ||
							(overwrite == 1 && isTargeted && isTargetingEnemy && isEnabled) ||
							(overwrite == 2 && isAuto && isEnabled) ||
							(overwrite == 3 && isTargeted && isTargetingEnemy) ||
							(overwrite == 4 && isAuto);
				if (useE) e->cast(ally);
				//console->print("%f: %s %s, TargetsEnemy: %i, AA: %i, Enabled: %i, Use E: %i", gametime->get_time(), ally->get_model_cstr(), spellSlotName(activeSpell).c_str(), isTargeted, isAuto, isEnabled, useE);
			}
			if (overwrite != 0) return;
			for (circSpell c : circularSpells) {
				bool guaranteeHit = semiGuaranteeHitCircular(c);
				if (!guaranteeHit) continue;
				const auto& s = entitylist->get_object_by_network_id(c.sender);
				auto tab = eDB[s->get_model_cstr()];
				
				if (!tab || !tab->get_entry("enable")->get_bool() || s->get_distance(myhero) > e->range()) continue;
				std::string spellName = spellSlotName(c.slot);
				if (spellName == "") return;
				bool isEnabled = tab->get_entry(spellName)->get_int() == 1;
				if (isEnabled) e->cast(s);
			}
			for (game_object_script missile : missileList) {
				auto hash = missile->get_missile_sdata()->get_name_hash();
				auto lit = linSpellDB[hash];
				bool guaranteeHit = semiGuaranteeHitLinear(missile);
				if (!guaranteeHit) continue;
				const auto& sender = entitylist->get_object(missile->missile_get_sender_id());
				if (!sender) continue;
				auto tab = eDB[sender->get_model_cstr()];
				if (!tab || !tab->get_entry("enable")->get_bool() || sender->get_distance(myhero) > e->range()) continue;
				std::string spellName = spellSlotName(lit.slot);
				if (spellName == "") return;
				bool isEnabled = tab->get_entry(spellName)->get_int() == 1;
				if (isEnabled) e->cast(sender);
			}
		}
	}	

	void on_draw() {
		update_spells();

		for (const auto& ally : entitylist->get_ally_heroes()) {
			if (ally->get_distance(myhero) > 725) continue;
			int bt = countWBounces(ally);
			draw_manager->add_text(ally->get_position(), MAKE_COLOR(255 * (bt <3), 255 * (bt > 1), 0, 255), 30, "%i", bt);
		}
		for (const auto& enemy : entitylist->get_enemy_heroes()) {
			if (enemy->get_distance(myhero) > 725) continue;
			int b = countWBounces(enemy);
			int minb = int(b / 10);
			int maxb = b % 10;
			draw_manager->add_text(enemy->get_position(), MAKE_COLOR(0,0,255, 255), 30, "MIN: %i MAX: %i", minb, maxb);
		}

		if(drawMenu::drawESpells->get_bool())
		{
			//console->print("%i Circ Spells", circularSpells.size());
			for (circSpell c : circularSpells) {
				auto color = semiGuaranteeHitCircular(c) ? MAKE_COLOR(0, 255, 0, 255) : MAKE_COLOR(255, 255, 0, 255);
				draw_manager->add_circle(c.castPos, c.radius, color);
				draw_manager->add_text(c.castPos, color, 20, "%f", timeToHit(c));
			}
			for (game_object_script missile : missileList) {
				auto hash = missile->get_missile_sdata()->get_name_hash();
				auto lit = linSpellDB[hash];

				//draw_manager->add_circle(missile->missile_get_start_position(), 30, MAKE_COLOR(255, 0, 0, 255));
				//draw_manager->add_circle(missile->missile_get_end_position(), 30, MAKE_COLOR(0, 255, 0, 255));
				//draw_manager->add_circle(missile->get_position(), 30, MAKE_COLOR(0, 0, 255, 255));
				auto points = geometry::rectangle(missile->get_position(), missile->missile_get_end_position(), lit.radius).to_polygon().points;
				auto color = semiGuaranteeHitLinear(missile) ? MAKE_COLOR(0, 255, 0, 255) : MAKE_COLOR(255, 255, 0, 255);
				for (int i = 0; i < points.size(); i++) {
					draw_manager->add_line(points[i].set_z(), points[(i + 1) % points.size()].set_z(), color);
				}
				for (const auto& target : entitylist->get_enemy_heroes()) {
					auto timeLeft = timeToHit(missile, target);

					if (timeLeft > 0)
					{
						draw_manager->add_text(target->get_position() + vector(20, 20), MAKE_COLOR(0, 0, 255, 255), 20, "%f", timeLeft);
						points = geometry::rectangle(missile->get_position(), missile->missile_get_end_position(), lit.radius + target->get_bounding_radius() * lit.useBoundingRadius).to_polygon().points;
						for (int i = 0; i < points.size(); i++) {
							draw_manager->add_line(points[i].set_z(), points[(i + 1) % points.size()].set_z(), MAKE_COLOR(255, 255, 255, 50));
						}
					}
				}
			}
		}

	}

	
	void on_env_draw() {

	}

	void on_process_spell_cast(game_object_script sender, spell_instance_script spell) {
		if (!sender->is_ai_hero() || !sender->is_ally()) return;
		auto name = spell->get_spell_data()->get_name();
		//console->print(name.c_str());
		auto circIterator = circSpellDB.find(spell->get_spell_data()->get_name_hash());
		if (circIterator != circSpellDB.end())
		{
			circularSpells.push_back(circSpell(spell->get_cast_position(), spell->get_end_position(), spell->get_spell_data()->get_name_hash(), sender, spell->get_spellslot()));
		}
		
	}
	void on_create_object(game_object_script obj)
	{
		//if (!obj->get_emitter() || !obj->get_emitter()->is_ai_hero()) return;
		//const auto object_hash = spell_hash_real(obj->get_name_cstr());
		//auto emitter = obj->get_emitter_resources_hash();
		if (obj->is_missile()) {
			auto senderID = obj->missile_get_sender_id();
			if (!senderID) return;
			auto sender = entitylist->get_object(senderID);
			if (!sender || !sender->is_ally()) return;
			auto h = obj->get_missile_sdata()->get_name_hash();
			if (linSpellDB.find(h) != linSpellDB.end()) {
				missileList.push_back(obj);
			}
			//console->print(obj->get_missile_sdata()->get_name_cstr());
			/*if (h == spell_hash("NamiWMissileAlly") || h == spell_hash("NamiWMissileEnemy") || h == spell_hash("NamiWAlly") || h == spell_hash("NamiWEnemy")) {
				console->print("Start Speed: %f", obj->missile_movement_get_current_speed());
				console->print("Start Accel: %f", obj->missile_movement_get_acceleration_magnitude());
				console->print("Target: %s", obj->missile_movement_get_target_unit()->get_model_cstr());
			}*/
		}
	}
	void on_delete_object(game_object_script obj) {

		if (obj->is_missile()) {
			if (entitylist->get_object(obj->missile_get_sender_id())->is_ally())
				missileList.erase(std::remove_if(missileList.begin(), missileList.end(), [obj](game_object_script& missile)
					{
						return missile->get_handle() == obj->get_handle();
					}), missileList.end());
		}
	}

	void load() {
		q = plugin_sdk->register_spell(spellslot::q, 850);
		w = plugin_sdk->register_spell(spellslot::w, 725);
		e = plugin_sdk->register_spell(spellslot::e, 800);
		r = plugin_sdk->register_spell(spellslot::r, 1500);
		// TODO: Check if Q Spelldata is correct
		q->set_skillshot(0.99, 200, FLT_MAX, { collisionable_objects::yasuo_wall }, skillshot_type::skillshot_circle);
		r->set_skillshot(0.5, 250, 850, { collisionable_objects::yasuo_wall }, skillshot_type::skillshot_line);
		initSpellDB();
		// Menu
		mainMenuTab = menu->create_tab("Flofian_Nami", "Flofian Nami");
		mainMenuTab->set_assigned_texture(myhero->get_square_icon_portrait());
		auto generalMenu = mainMenuTab->add_tab("general", "General Settings");
		auto qMenu = mainMenuTab->add_tab("q", "Q Settings");
		auto wMenu = mainMenuTab->add_tab("w", "W Settings");
		auto eMenu = mainMenuTab->add_tab("e", "E Settings");
		{
			eMenu::mode = eMenu->add_combobox("mode", "E Mode", { {"Always", nullptr}, {"Combo + Harass", nullptr},{"Combo", nullptr}, {"Off", nullptr} }, 0);
			eMenu::useOn = eMenu->add_tab("useOn", "Use E on:");
			for (const auto& ally : entitylist->get_ally_heroes()) {
				auto name = ally->get_model_cstr();
				auto tab = eMenu::useOn->add_tab(name, Database::getDisplayName(ally));
				tab->set_texture(ally->get_square_icon_portrait());
				auto enable = tab->add_checkbox("enable", "Enable " + Database::getDisplayName(ally), true);
				tab->add_separator("sep", "Spell Settings");
				enable->set_texture(ally->get_square_icon_portrait());
				tab->add_checkbox("AA", "Auto Attack", true);
				// If i get my sdk access removed for this i deserved it
				auto qDropdown = supportedSpells.find(name + std::string("Q")) != supportedSpells.end();
				auto wDropdown = supportedSpells.find(name + std::string("W")) != supportedSpells.end();
				auto eDropdown = supportedSpells.find(name + std::string("E")) != supportedSpells.end();
				auto rDropdown = supportedSpells.find(name + std::string("R")) != supportedSpells.end();
				TreeEntry* qt;
				TreeEntry* wt;
				TreeEntry* et;
				TreeEntry* rt;
				if (qDropdown)
				{
					qt = tab->add_combobox("Q", "Q Mode", { {"Off", nullptr}, {"When Hitting", nullptr},{"Always", nullptr} }, 1);
				}
				else
				{
					qt = tab->add_checkbox("Q", "Q", ally->get_spell(spellslot::q)->get_spell_data()->get_targeting_type() == spell_targeting::target);
				}
				if (wDropdown)
				{
					wt = tab->add_combobox("W", "W Mode", { {"Off", nullptr}, {"When Hitting", nullptr},{"Always", nullptr} }, 1);
				}
				else
				{
					wt = tab->add_checkbox("W", "W", ally->get_spell(spellslot::w)->get_spell_data()->get_targeting_type() == spell_targeting::target);
				}
				if (eDropdown)
				{
					et = tab->add_combobox("E", "E Mode", { {"Off", nullptr}, {"When Hitting", nullptr},{"Always", nullptr} }, 1);
				}
				else
				{
					et = tab->add_checkbox("E", "E", ally->get_spell(spellslot::e)->get_spell_data()->get_targeting_type() == spell_targeting::target);
				}
				if (rDropdown)
				{
					rt = tab->add_combobox("R", "R Mode", { {"Off", nullptr}, {"When Hitting", nullptr},{"Always", nullptr} }, 1);
				}
				else
				{
					rt = tab->add_checkbox("R", "R", ally->get_spell(spellslot::q)->get_spell_data()->get_targeting_type() == spell_targeting::target);
				}
				qt->set_texture(ally->get_spell(spellslot::q)->get_icon_texture());
				wt->set_texture(ally->get_spell(spellslot::w)->get_icon_texture());
				et->set_texture(ally->get_spell(spellslot::e)->get_icon_texture());
				rt->set_texture(ally->get_spell(spellslot::r)->get_icon_texture());
				eDB[name] = tab;
				// Custom Tooltips:
				switch (ally->get_champion()) {
				case champion_id::Karthus:
					qt->set_tooltip("Delay might be wrong");
					break;
				case champion_id::Yasuo:
				case champion_id::Yone:
					qt->set_tooltip("\"When Hitting\" Mode only checks Q3");
					break;
				case champion_id::Swain:
					wt->set_tooltip("Delay might be wrong");
					break;
				case champion_id::Ryze: 
					qt->set_tooltip("Only works for direct hits");
					break;
				case champion_id::Veigar:
					qt->set_tooltip("Checks only 1 collision");
					break;
				default:
					break;
				}
			}
			eMenu::overwrite = eMenu->add_combobox("overwrite", "Overwrite", { {"None", nullptr}, {"Only Targeted", nullptr},{"Only Auto Attacks", nullptr}, {"All Targeted", nullptr},{"All Auto Attacks", nullptr} }, 0);
			eMenu::overwrite->set_tooltip("Only Auto Attacks / Only Targeted still take the list into account\n"
											"All Auto Attacks / All Targeted ignores the spell settings\n"
											"You still need to enable / disable the champs");
			eMenu::remainingTime = eMenu->add_slider("remainingTime", "Min Remaing Time for Skillshots", 100, 10, 1000);
			eMenu::remainingTime->add_property_change_callback([](TreeEntry* entry) {
				MinERemainingTime = entry->get_int()/1000.f;
				});
		}
		auto rMenu = mainMenuTab->add_tab("r", "R Settings");
		auto drawMenu = mainMenuTab->add_tab("drawings", "Drawings Settings");
		{
			drawMenu::drawOnlyReady = drawMenu->add_checkbox("drawReady", "Draw Only Ready", true);
			drawMenu::drawRangeQ = drawMenu->add_checkbox("drawQ", "Draw Q range", true);
			drawMenu::drawRangeW = drawMenu->add_checkbox("drawW", "Draw W range", true);
			drawMenu::drawRangeE = drawMenu->add_checkbox("drawE", "Draw E range", true);
			drawMenu::drawESpells = drawMenu->add_checkbox("drawESpells", "Draw Spells for E", true);
			drawMenu::drawRangeR = drawMenu->add_checkbox("drawR", "Draw R range", true);

			auto colorMenu = drawMenu->add_tab("color", "Color Settings");

			float qcolor[] = { 0.f, 0.f, 1.f, 1.f };
			colorMenu::qColor = colorMenu->add_colorpick("colorQ", "Q Range Color", qcolor);
			float wcolor[] = { 0.f, 1.f, 0.f, 1.f };
			colorMenu::wColor = colorMenu->add_colorpick("colorW", "W Range Color", wcolor);
			float ecolor[] = { 1.f, 0.f, 1.f, 1.f };
			colorMenu::eColor = colorMenu->add_colorpick("colorE", "E Range Color", ecolor);
			float epcolor[] = { 0.f, 1.f, 0.f, 1.f };
			colorMenu::eSpellColor = colorMenu->add_colorpick("colorESeplls", "E Spell Color", epcolor);
			float rcolor[] = { 1.f, 1.f, 0.f, 1.f };
			colorMenu::rColor = colorMenu->add_colorpick("colorR", "R Range Color", rcolor);
		}
		event_handler<events::on_draw>::add_callback(on_draw);
		event_handler<events::on_env_draw>::add_callback(on_env_draw);
		event_handler<events::on_update>::add_callback(on_update);
		event_handler<events::on_process_spell_cast>::add_callback(on_process_spell_cast);
		event_handler<events::on_create_object>::add_callback(on_create_object);
		event_handler<events::on_delete_object>::add_callback(on_delete_object);
	}
	void unload() {
		plugin_sdk->remove_spell(q);
		plugin_sdk->remove_spell(w);
		plugin_sdk->remove_spell(e);
		plugin_sdk->remove_spell(r);
		menu->delete_tab(mainMenuTab);


		event_handler<events::on_draw>::remove_handler(on_draw);
		event_handler<events::on_env_draw>::add_callback(on_env_draw);
		event_handler<events::on_update>::remove_handler(on_update);
		event_handler<events::on_process_spell_cast>::remove_handler(on_process_spell_cast);
		event_handler<events::on_create_object>::remove_handler(on_create_object);
		event_handler<events::on_delete_object>::remove_handler(on_delete_object);
	}
}