#include "../plugin_sdk/plugin_sdk.hpp"
#include "nami.h"
#include "../spelldb/SpellDB.h"
#include <inttypes.h>
#include "permashow.hpp"

// Warning: This code is ugly

namespace nami {
	std::string VERSION = "Version: 1.0.0";
	script_spell* q = nullptr;
	script_spell* w = nullptr;
	script_spell* e = nullptr;
	script_spell* r = nullptr;

	TreeTab* mainMenuTab = nullptr;

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
	std::map<uint32_t, TreeEntry*> wLowHPList;
	std::map<uint32_t, TreeEntry*> qDashWhitelist;
	std::unordered_map<uint32_t, prediction_output> qPredictionList;
	std::unordered_map<uint32_t, prediction_output> rPredictionList;

	struct rPos {
		vector direction;
		int enemyHitcount;
		int allyHitcount;	// not sure yet if i do that
	};

	// thank you yorik100, i love you
	struct stasisStruct {
		float stasisTime = 0;
		float stasisStart = 0;
		float stasisEnd = 0;
	};
	struct buffList {
		float godBuff = 0;
		float noKillBuff = 0;
		stasisStruct stasis = {};
	};
	struct particleStruct {
		game_object_script obj = {};
		game_object_script target = {};
		game_object_script owner = {};
		float time = 0;
		float castTime = 0;
		vector castingPos = vector::zero;
		bool isZed = false;
		bool isTeleport = false;
	}; 
	static constexpr uint32_t godBuffList[]
	{
		buff_hash("KayleR"),
		buff_hash("TaricR"),
		buff_hash("SivirE"),
		buff_hash("FioraW"),
		buff_hash("NocturneShroudofDarkness"),
		buff_hash("kindredrnodeathbuff"),
		buff_hash("XinZhaoRRangedImmunity"),
		buff_hash("PantheonE")
	};

	static constexpr uint32_t noKillBuffList[]
	{
		buff_hash("UndyingRage"),
		buff_hash("ChronoShift")
	};

	static constexpr uint32_t stasisBuffList[]
	{
		buff_hash("ChronoRevive"),
		buff_hash("BardRStasis"),
		buff_hash("ZhonyasRingShield"),
		buff_hash("LissandraRSelf")
	}; 
	static constexpr uint32_t immuneSpells[]
	{
		spell_hash("EvelynnR"),
		spell_hash("ZedR"),
		spell_hash("EkkoR"),
		spell_hash("FizzE"),
		spell_hash("FizzETwo"),
		spell_hash("FizzEBuffer"),
		spell_hash("XayahR"),
		spell_hash("VladimirSanguinePool")
	};

	std::vector<particleStruct> particlePredList;
	vector nexusPos;
	vector urfCannon;
	std::unordered_map<uint32_t, stasisStruct> stasisInfo;
	std::unordered_map<uint32_t, float> guardianReviveTime;
	std::unordered_map<uint32_t, float> deathAnimTime;
	std::unordered_map<uint32_t, float> godBuffTime;
	std::unordered_map<uint32_t, float> noKillBuffTime;

	namespace generalMenu {
		TreeEntry* debug = nullptr;
	}
	namespace qMenu {
		TreeEntry* hc;
		TreeEntry* mode;
		TreeEntry* onCC;
		TreeEntry* onParticle;
		TreeEntry* onSpecialSpells;
		TreeEntry* onStasis;
		TreeEntry* onDashes;
		TreeTab* dashWhitelist;
		TreeEntry* range;
		TreeEntry* toggleRadius;
	}
	namespace wMenu {
		TreeEntry* minHealRatio = nullptr;
		TreeEntry* mode = nullptr;
		TreeEntry* minTargets = nullptr;
		TreeEntry* autoWTripleHit = nullptr;
		TreeTab* useOnLowHP = nullptr;
		TreeEntry* wHealMana = nullptr;
	}
	namespace eMenu {
		TreeEntry* mode = nullptr;
		TreeTab* useOn = nullptr;
		TreeEntry* overwrite = nullptr;
		TreeEntry* remainingTime = nullptr;
	}
	namespace rMenu {
		TreeEntry* range = nullptr;
		TreeEntry* hc = nullptr;
		TreeEntry* comboTargets = nullptr;
		TreeEntry* semiKey = nullptr;
		TreeEntry* semiTargets = nullptr;
		TreeEntry* semiSelected = nullptr;
		TreeEntry* flee = nullptr;
	}
	namespace drawMenu {
		TreeEntry* drawOnlyReady = nullptr;
		TreeEntry* drawRangeQ = nullptr;
		TreeEntry* drawRangeW = nullptr;
		TreeEntry* drawRangeE = nullptr;
		TreeEntry* drawRangeR = nullptr;
		TreeEntry* drawESpells = nullptr;
		TreeEntry* drawWTargets = nullptr;
	}
	namespace colorMenu
	{
		TreeEntry* qColor = nullptr;
		TreeEntry* wColor = nullptr;
		TreeEntry* eColor = nullptr;
		TreeEntry* rColor = nullptr;
	}
	namespace interruptMenu {
		TreeEntry* useQ = nullptr;
		TreeEntry* useR = nullptr;
		TreeTab* spelldb = nullptr;
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
				if(drawMenu::drawESpells->get_bool())draw_manager->add_circle(x->get_position(), x->get_bounding_radius(), MAKE_COLOR(255, 0, 255, 255));
				if (x->get_distance(currentPos) < c.distance(currentPos)) c = x->get_position();
			}
			if (c != vector()) endpos = c;
		}

		auto rec = geometry::rectangle(currentPos, endpos, radius);
		bool isInside = rec.to_polygon().is_inside(pos);
		if (isInside&& drawMenu::drawESpells->get_bool())
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

	bool wAllyHeal(game_object_script ally) {
		float wBaseHeal = 35 + 20 * w->level() + 0.25 * myhero->get_total_ability_power();
		return ally->get_max_health() - ally->get_health() >= wBaseHeal * wMenu::minHealRatio->get_int() / 100.f;
	}
	bool wKillEnemy(game_object_script enemy, int hitCount) {
		float wBaseDamage = 20 + 40 * w->level() + 0.55 * myhero->get_total_ability_power();
		float dmgMod = -0.15 + 0.00075 * myhero->get_total_ability_power();
		float wPreDamage = hitCount == 0 ? wBaseDamage : wBaseDamage * powf(dmgMod, hitCount);		// i dont trust powf
		float totalDmg = damagelib->calculate_damage_on_unit(myhero, enemy, damage_type::magical, wPreDamage);
		return totalDmg > enemy->get_real_health();
	}
	int countWBounces(game_object_script firstTarget) {
		// Nami w targeting info:
		// when jumping from ally to enemy -> always takes nearest enemy
		// when from enemy to ally -> random? atleast no pattern
		// Also first missile has speed of 2500, the bounces only 1500 (according to game data)
		float bounceRange = 800;

		if (firstTarget->is_ally()) {
			// Need to split it so i can loop allies->enemies or enemies->allies
			// Okay not sure whats the best way to handle prediction here? If theres any good way?
			// maybe try a linear spell with radius 0 and just look at unit position?
			// Simple way is to estimate traveltime by distance at bounce start, is wrong, but should be close?
			float travelTime = firstTarget->get_distance(myhero) / 2500;
			auto predictedAllyPos = prediction->get_prediction(firstTarget, travelTime).get_unit_position();
			auto enemies = entitylist->get_enemy_heroes();
			int hitCount = wAllyHeal(firstTarget);
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
			hitCount += 1 + wKillEnemy(nearestEnemy, 1);
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
						x->get_handle() == firstTarget->get_handle() || !wAllyHeal(x);
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
				int currentcount = 1 + wKillEnemy(firstTarget, 0) + wAllyHeal(ally) + (enemies.size() > 0);
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
	hit_chance get_hitchance(const int hc)
	{
		switch (hc)
		{
		case 0:
			return hit_chance::medium;
		case 1:
			return hit_chance::high;
		case 2:
			return hit_chance::very_high;
		}
		return hit_chance::medium;
	}
	void particleHandling()
	{
		
		const auto particleQ = qMenu::onParticle->get_bool() && q->is_ready();

		// Checking if particles are valid, if they're not, delete them from the list
		particlePredList.erase(std::remove_if(particlePredList.begin(), particlePredList.end(), [](const particleStruct& x)
			{
				return !x.obj->is_valid() || x.owner->is_dead() || x.time + x.castTime <= gametime->get_time();
			}
		),
			particlePredList.end());

		if (!particleQ ||particlePredList.size()==0) return;

		// Loop through every pred particles
		for (auto& obj : particlePredList)
		{
			// Getting the final cast position
			if (obj.isTeleport)
			{
				if (generalMenu::debug->get_bool()) console->print("Found Teleport");
				obj.target = obj.obj->get_particle_attachment_object();
				if (!obj.target)
					obj.target = obj.obj->get_particle_target_attachment_object();
				if (obj.target && obj.obj->get_position().distance(obj.target->get_position()) <= 0) {
					obj.castingPos = obj.target->get_position().extend(nexusPos, obj.target->is_ai_turret() ? 225 : 100);
				}
				else
				{
					obj.castingPos = obj.obj->get_position().extend(nexusPos, 100);
				}
				if (obj.castingPos.is_wall() || obj.castingPos.is_building())
					obj.castingPos = navmesh->get_nearest_passable_cell_center(obj.castingPos);
			}
			else if (obj.isZed)
			{
				obj.castingPos = obj.target->get_position() + (obj.owner->get_direction() * 75);
				if (obj.castingPos.is_wall() || obj.castingPos.is_building())
					obj.castingPos = navmesh->get_nearest_passable_cell_center(obj.castingPos);
			}

			// Check if cast position isn't too far enough
			if (myhero->get_position().distance(obj.castingPos) > q->range() + q->get_radius()) continue;

			console->print("Particle in range");
			// Gathering enough data to cast on particles
			float pings = ping->get_ping() / 1000.f;
			const auto particleTime = (obj.time + obj.castTime) - gametime->get_time();
			const auto effectiveQRange = q->range() + q->radius;
			const auto effectiveQRadius = myhero->get_position().distance(obj.castingPos) <= q->range() ? q->radius : std::max(1.f, q->radius + q->range() - myhero->get_position().distance(obj.castingPos));
			const auto effectiveQDeviation = q->radius - effectiveQRadius;
			const auto qCanDodge = obj.owner->get_move_speed() * ((q->get_delay() - particleTime) + pings) > effectiveQRadius;
			const auto canQ = particleQ && !qCanDodge && myhero->get_position().distance(obj.castingPos) <= effectiveQRange;
			
		
			// Try to cast Q if possible
			if (canQ && (particleTime - pings + 0.2) <= q->get_delay())
			{
				q->cast(obj.castingPos.extend(myhero->get_position(), effectiveQDeviation));
				console->print("Cast Q on Particle");
				return;
			}
		}
	}
	int isCastMoving(const game_object_script& target)
	{
		if (!target->is_ai_hero())
			return 0;
		if (target->get_spell(spellslot::w)->get_name_hash() == spell_hash("NunuW_Recast") && target->is_playing_animation(buff_hash("Spell2")))
			return 2;
		if (target->get_spell(spellslot::w)->get_name_hash() == spell_hash("AurelionSolWToggle") && target->is_playing_animation(buff_hash("Spell2")))
			return 2;
		if (target->get_spell(spellslot::r)->get_name_hash() == spell_hash("SionR") && target->has_buff(buff_hash("SionR")))
			return 1;
		return 0;
	}
	buffList combinedBuffChecks(const game_object_script& target)
	{
		// This function gets every single buffs that are needed making the 3 functions above completely useless!
		float godBuffTime = 0;
		float noKillBuffTime = 0;
		float stasisTime = 0;
		float stasisStart = 0;
		float stasisEnd = 0;
		for (auto&& buff : target->get_bufflist())
		{
			if (buff == nullptr || !buff->is_valid() || !buff->is_alive()) continue;

			const auto buffHash = buff->get_hash_name();
			if (std::find(std::begin(godBuffList), std::end(godBuffList), buffHash) != std::end(godBuffList))
			{
				const auto isPantheonE = buffHash == buff_hash("PantheonE");
				const auto realRemainingTime = !isPantheonE ? buff->get_remaining_time() : buff->get_remaining_time() + 0.2;
				if (godBuffTime < realRemainingTime && (!isPantheonE || target->is_facing(myhero)) && (buffHash != buff_hash("XinZhaoRRangedImmunity") || myhero->get_position().distance(target->get_position()) > 450))
				{
					godBuffTime = realRemainingTime;
				}
			}
			else if (std::find(std::begin(noKillBuffList), std::end(noKillBuffList), buffHash) != std::end(noKillBuffList))
			{
				if (noKillBuffTime < buff->get_remaining_time())
				{
					noKillBuffTime = buff->get_remaining_time();
				}
			}
			else if (std::find(std::begin(stasisBuffList), std::end(stasisBuffList), buffHash) != std::end(stasisBuffList))
			{
				if (stasisTime < buff->get_remaining_time())
				{
					stasisTime = buff->get_remaining_time();
					stasisStart = buff->get_start();
					stasisEnd = buff->get_end();
				}
			}
		}
		// Get guardian angel revive time if there is one
		if (stasisTime > 0)
		{
			guardianReviveTime[target->get_handle()] = 0.f;
			if(generalMenu::debug->get_bool()) console->print("Removed GAtime because stasis %s", target->get_model_cstr());
		}
		const auto reviveTime = guardianReviveTime[target->get_handle()];
		const float GATime = (stasisTime <= 0 && reviveTime ? reviveTime - gametime->get_time() : 0);
		if (stasisTime < GATime)
		{
			stasisTime = GATime;
			stasisStart = reviveTime - 4;
			stasisEnd = reviveTime;
			if (generalMenu::debug->get_bool()) console->print("Set stasis to revive %s", target->get_model_cstr());
		}
		const stasisStruct& stasisInfo = { .stasisTime = stasisTime, .stasisStart = stasisStart, .stasisEnd = stasisEnd };
		const buffList& buffStruct = { .godBuff = godBuffTime, .noKillBuff = noKillBuffTime, .stasis = stasisInfo };
		return buffStruct;
	}
	bool isYuumiAttached(const game_object_script& target)
	{
		// Check if the target is Yuumi and if it's attached to someone
		return target->is_ai_hero() && target->get_spell(spellslot::w)->get_name_hash() == spell_hash("YuumiWEndWrapper");
	}
	bool customIsValid(const game_object_script& target, float range = FLT_MAX, vector from = vector::zero, bool invul = false)
	{
		// Custom isValid
		if (!target || !target->is_valid())
			return false;

		// If it's Yuumi that is attached then target is not valid
		if (isYuumiAttached(target)) return false;

		if (from == vector::zero)
			from = myhero->get_position();

		//if (ferris_prediction && ferris_prediction->is_hidden() == false && settings::automatic::fowPred->get_bool() && prediction->get_prediction(target, 0.F).hitchance > hit_chance::impossible && from.distance(target) <= range)
		//	return true;

		const auto isCastingImmortalitySpell = (target->get_active_spell() && std::find(std::begin(immuneSpells), std::end(immuneSpells), target->get_active_spell()->get_spell_data()->get_name_hash()) != std::end(immuneSpells)) || target->has_buff(buff_hash("AkshanE2"));
		const auto isValid = !isCastingImmortalitySpell && ((target->is_valid_target(range, from, invul) && target->is_targetable() && target->is_targetable_to_team(myhero->get_team()) && !target->is_invulnerable()));
		return isValid;
	}
	rPos getBestRPos() {
		rPos bestRPos = { .direction = vector(), .enemyHitcount = 0 ,.allyHitcount = 0};
		for (const auto& mainTarget : entitylist->get_enemy_heroes()) {
			rPos currentRPos = { .direction = rPredictionList[mainTarget->get_handle()].get_cast_position(), .enemyHitcount = 0,.allyHitcount = 0};
			for (const game_object_script& enemy : entitylist->get_enemy_heroes()) {
				if (enemy->is_dead() || enemy->is_zombie() || !enemy->is_visible() || enemy->get_is_cc_immune()) continue;
				auto rect = geometry::rectangle(myhero->get_position(), myhero->get_position() + ((currentRPos.direction - myhero->get_position()).normalized() * r->range()), r->radius + enemy->get_bounding_radius()).to_polygon();
				auto pred = rPredictionList[enemy->get_handle()];
				if (rect.is_inside(pred.get_unit_position()) && pred.hitchance >= get_hitchance(rMenu::hc->get_int())) currentRPos.enemyHitcount++;
			}
			if (currentRPos.enemyHitcount > bestRPos.enemyHitcount) bestRPos = currentRPos;
		}
		return bestRPos;
	}
	void on_update() {
		// TODO: either give user control or just use either min or max
		update_spells();
		// tbh i think i just dont do a combo and a harass method and just do all in here
		for (const auto& enemy : entitylist->get_enemy_heroes()) {
			qPredictionList[enemy->get_handle()] = q->get_prediction(enemy);
			rPredictionList[enemy->get_handle()] = r->get_prediction(enemy);
			// man thank you yorik
			const buffList listOfNeededBuffs = combinedBuffChecks(enemy);
			godBuffTime[enemy->get_handle()] = listOfNeededBuffs.godBuff;
			noKillBuffTime[enemy->get_handle()] = listOfNeededBuffs.noKillBuff;
			stasisInfo[enemy->get_handle()] = listOfNeededBuffs.stasis;
			if (!enemy->is_playing_animation(buff_hash("Death")))
			{
				if (generalMenu::debug->get_bool() && guardianReviveTime[enemy->get_handle()]>0) console->print("Removed GA Revive from %s", enemy->get_model_cstr());
				guardianReviveTime[enemy->get_handle()] = -1;
			}
		}

		if (q->is_ready()) {
			// Combo/Harass Cast
			bool modeCast = (qMenu::mode->get_int() == 0 && orbwalker->harass()) || (qMenu::mode->get_int() <= 1 && orbwalker->combo_mode());
			if (modeCast) {
				auto target = target_selector->get_target(q, damage_type::magical);
				if (target) {
					auto pred = qPredictionList[target->get_handle()];
					if (pred.hitchance >= get_hitchance(qMenu::hc->get_int())) {
						q->cast(pred.get_cast_position());
						return;
					}
				}
			}
			// Auto Q
			for (const auto& target : entitylist->get_enemy_heroes()) {
				if (!target || !target->is_valid() || target->get_distance(myhero)>q->range() || target->is_dead()) continue;
				auto pred = qPredictionList[target->get_handle()];
				// Stasis
				if (qMenu::onStasis->get_bool())
				{
					const auto stasisDuration = stasisInfo[target->get_handle()].stasisTime;
					if (((customIsValid(target) || stasisDuration > 0) && !target->is_zombie()))					
					{
						if (stasisDuration > 0 && (stasisDuration + 0.2 - ping->get_ping() / 1000.f) < q->delay) {
							q->cast(pred.get_cast_position());
							if (generalMenu::debug->get_bool()) console->print("Cast Q on Stasis %s", target->get_model_cstr());
						}
					}
				}
				if (!target->is_valid_target(q->range(), myhero->get_position(), true) || target->is_zombie() || !target->is_visible() || target->get_is_cc_immune()) continue;
				// On CC
				if (qMenu::onCC->get_bool() && target->get_immovibility_time() > q->delay && pred.hitchance>=hit_chance::low)
				{
					q->cast(pred.get_cast_position());
					return;
				}
				// Special Spell
				if (qMenu::onSpecialSpells->get_bool()) {
					auto activeSpell = target->get_active_spell();
					auto data = activeSpell->get_spell_data();
					if (activeSpell && !data->is_insta() && !data->mCanMoveWhileChanneling() && !isCastMoving(target) && target->get_champion() != champion_id::Yone)	// hardcoded since fuck this guy
					{
						auto castStartTime = activeSpell->cast_start_time();
						auto castTime = activeSpell->get_spell_data()->mCastTime();
						auto remainingTime = castStartTime - gametime->get_time() + castTime;
						if (remainingTime > 0.8 && pred.hitchance >= hit_chance::low) {	// should be enough, not sure if there are any other than luxR and ezrealR Also maybe change to hitchance immobile
							q->cast(pred.get_cast_position());
							if (generalMenu::debug->get_bool()) console->print("Cast Q on Special %s", target->get_model_cstr());
							return;
						}
					}
				}
				// Dashes
				if (qMenu::onDashes->get_int()!=0 && qDashWhitelist.find(target->get_network_id())!=qDashWhitelist.end() && qDashWhitelist.find(target->get_network_id())->second->get_bool()) {
					//if (generalMenu::debug->get_bool()) console->print("%s is dashing: %i", target->get_model_cstr(), target->is_dashing());
					if ((qMenu::onDashes->get_int() == 2 && target->is_dashing()) || (pred.hitchance == hit_chance::dashing && qMenu::onDashes->get_int() == 1)) {
						q->cast(pred.get_cast_position());
						if (generalMenu::debug->get_bool()) console->print("Cast Q on Dash %s", target->get_model_cstr());
						return;
					}
				}

			}
			// particle Stuff
			if (!particlePredList.empty())
				particleHandling();
		}

		if (w->is_ready())
		{
			// multi bounce logic
			bool modeCast = (wMenu::mode->get_int() == 0 && orbwalker->harass()) || (wMenu::mode->get_int() <= 1 && orbwalker->combo_mode());
			int modeMinTargets = wMenu::minTargets->get_int();
			for (const auto& ally : entitylist->get_ally_heroes()) {
				if (ally->get_distance(myhero) > w->range()) continue;
				int bt = countWBounces(ally);
				if ((bt == 3 && wMenu::autoWTripleHit->get_bool()) || (modeCast && bt >= modeMinTargets)) w->cast(ally);
				
			}
			for (const auto& enemy : entitylist->get_enemy_heroes()) {
				if (enemy->get_distance(myhero) > w->range() || !enemy->is_targetable() || !enemy->is_visible()) continue;
				int b = countWBounces(enemy);
				int minb = int(b / 10);
				int maxb = b % 10;
				if ((maxb == 3 && wMenu::autoWTripleHit->get_bool()) || (modeCast && maxb >= modeMinTargets)) w->cast(enemy);
			}

			// auto low hp
			if(myhero->get_mana_percent()>wMenu::wHealMana->get_int() && !myhero->is_recalling())
			{
				for (const auto& ally : entitylist->get_ally_heroes()) {
					if (ally->get_distance(myhero) > w->range() || ally->is_dead() || !ally->is_targetable()) continue;
					if (ally->get_health_percent() < wLowHPList[ally->get_network_id()]->get_int()) w->cast(ally);
				}
			}
		}
		
		int eMode = eMenu::mode->get_int();
		if (e->is_ready() && !myhero->is_recalling() && (eMode == 0 || (eMode == 1 && orbwalker->harass()) || (eMode <= 2 && orbwalker->combo_mode()))) {
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
				
				if (useE){ 
					e->cast(ally);
					return;
				}

				//console->print("%f: %s %s, TargetsEnemy: %i, AA: %i, Enabled: %i, Use E: %i", gametime->get_time(), ally->get_model_cstr(), spellSlotName(activeSpell).c_str(), isTargeted, isAuto, isEnabled, useE);
			}
			if (overwrite == 0)
			{
				for (circSpell c : circularSpells) {
					bool guaranteeHit = semiGuaranteeHitCircular(c);
					if (!guaranteeHit) continue;
					const auto& s = entitylist->get_object_by_network_id(c.sender);
					auto tab = eDB[s->get_model_cstr()];

					if (!tab || !tab->get_entry("enable")->get_bool() || s->get_distance(myhero) > e->range()) continue;
					std::string spellName = spellSlotName(c.slot);
					if (spellName == "") continue;
					bool isEnabled = tab->get_entry(spellName)->get_int() == 1;
					if (isEnabled) 
					{
						e->cast(s);
						return;
					}
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
					if (spellName == "") continue;
					bool isEnabled = tab->get_entry(spellName)->get_int() == 1;
					if (isEnabled)
					{
						e->cast(sender);
						return;
					}
				}
			}
		}

		if (r->is_ready()) {
			auto selected = target_selector->get_selected_target();
			auto comboTargets = rMenu::comboTargets->get_int();
			auto semiTargets = rMenu::semiTargets->get_int();
			if (selected && rMenu::semiSelected->get_bool() && rMenu::semiKey->get_bool()) {
				r->cast(rPredictionList[selected->get_handle()].get_cast_position());
				if (generalMenu::debug->get_bool()) console->print("Semi R Selected %s", selected->get_model_cstr());
				return;
			}
			rPos bestR = getBestRPos();
			if (bestR.enemyHitcount >= comboTargets && orbwalker->combo_mode()) {
				r->cast(bestR.direction);
				if (generalMenu::debug->get_bool()) console->print("Combo R %i Enemies", bestR.enemyHitcount);
				return;
			}
			if (bestR.enemyHitcount >= semiTargets && rMenu::semiKey->get_bool()) {
				r->cast(bestR.direction);
				if (generalMenu::debug->get_bool()) console->print("Semi R %i Enemies", bestR.enemyHitcount);
				return;
			}
			if (bestR.enemyHitcount > 0 && orbwalker->flee_mode() && myhero->get_health_percent() < rMenu::flee->get_int()) {
				r->cast(bestR.direction);
				if (generalMenu::debug->get_bool()) console->print("Flee R %i Enemies", bestR.enemyHitcount);
				return;
			}
		}

		// Interrupt Logic
		bool qInterrupt = interruptMenu::useQ->get_bool() && q->is_ready();
		bool rInterrupt = interruptMenu::useR->get_bool() && r->is_ready();
		if (qInterrupt || rInterrupt) {
			for (const auto& target : entitylist->get_enemy_heroes()) {
				if (target && target->is_valid() && target->is_visible() && !target->is_zombie() && target->is_valid_target(r->range()) && !target->get_is_cc_immune()) {
					if(Database::getCastingImportance(target) >= 3 && rInterrupt)
					{
						if (target->get_champion() == champion_id::Jhin || target->get_champion() == champion_id::Xerath || target->get_champion() == champion_id::Karthus) r->set_range(2750);
						// ^ I do this because i want to cancel them far away aswell, really hated it when blahajaio tried to cancel katarina r 3 screens away
						auto pred = r->get_prediction(target);
						if(pred.hitchance >= get_hitchance(rMenu::hc->get_int()))
						{
							r->cast(target);
							if (generalMenu::debug->get_bool()) myhero->print_chat(0, "Interrupt R on %s", Database::getDisplayName(target).c_str());
						}
						r->set_range(rMenu::range->get_int());
					}
					if (Database::getCastingImportance(target) >= 1 && qInterrupt)
					{
						auto pred = q->get_prediction(target);
						if (pred.hitchance >= get_hitchance(qMenu::hc->get_int()))
						{
							q->cast(target);
							if (generalMenu::debug->get_bool()) myhero->print_chat(0, "Interrupt Q on %s", Database::getDisplayName(target).c_str());
						}
					}


				}

			}
		}
	}	

	void on_draw() {


		update_spells();
		if(drawMenu::drawWTargets->get_bool()){
			for (const auto& ally : entitylist->get_ally_heroes()) {
				if (ally->get_distance(myhero) > 725) continue;
				int bt = countWBounces(ally);
				draw_manager->add_text(ally->get_position(), MAKE_COLOR(255 * (bt < 3), 255 * (bt > 1), 0, 255), 30, "%i", bt);
			}
			for (const auto& enemy : entitylist->get_enemy_heroes()) {
				if (enemy->get_distance(myhero) > 725 || !enemy->is_targetable() || !enemy->is_visible()) continue;
				int b = countWBounces(enemy);
				int minb = int(b / 10);
				int maxb = b % 10;
				draw_manager->add_text(enemy->get_position(), MAKE_COLOR(0, 0, 255, 255), 30, "MIN: %i MAX: %i", minb, maxb);
			}
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
		if (myhero->is_dead())
		{
			return;
		}

		if ((q->is_ready() || !drawMenu::drawOnlyReady->get_bool()) && drawMenu::drawRangeQ->get_bool())
			draw_manager->add_circle(myhero->get_position(), q->range(), colorMenu::qColor->get_color());
		if ((w->is_ready() || !drawMenu::drawOnlyReady->get_bool()) && drawMenu::drawRangeW->get_bool())
			draw_manager->add_circle(myhero->get_position(), w->range(), colorMenu::wColor->get_color());
		if ((e->is_ready() || !drawMenu::drawOnlyReady->get_bool()) && drawMenu::drawRangeE->get_bool())
			draw_manager->add_circle(myhero->get_position(), e->range(), colorMenu::eColor->get_color());
		if ((r->is_ready() || !drawMenu::drawOnlyReady->get_bool()) && drawMenu::drawRangeR->get_bool())
			draw_manager->add_circle(myhero->get_position(), r->range(), colorMenu::rColor->get_color());
	}

	void on_process_spell_cast(game_object_script sender, spell_instance_script spell) {
		if (sender->is_ai_hero() && sender->is_ally())
		{
			//console->print(spell->get_spell_data()->get_name_cstr());
			auto circIterator = circSpellDB.find(spell->get_spell_data()->get_name_hash());
			if (circIterator != circSpellDB.end())
			{
				circularSpells.push_back(circSpell(spell->get_cast_position(), spell->get_end_position(), spell->get_spell_data()->get_name_hash(), sender, spell->get_spellslot()));
			}
		}
		if (qMenu::onSpecialSpells->get_bool() && sender->is_ai_hero() && sender->is_enemy()) {
			// I do this only in here because fioraW lasts 0.75s and my q takes 0.99s, so if i dont cast immediatly she will likely escape
			if (spell->get_spell_data()->get_name_hash() == spell_hash("FioraW")) {
				auto pred = q->get_prediction(sender);
				if (pred.hitchance >= hit_chance::low) q->cast(pred.get_cast_position());	// Hardcoding since i dont need more? but i need atleast something so its not out of range
			}
		}
		
	}
	void on_create_object(game_object_script obj)
	{
		//if (!obj->get_emitter() || !obj->get_emitter()->is_ai_hero()) return;
		//const auto object_hash = spell_hash_real(obj->get_name_cstr());
		//auto emitter = obj->get_emitter_resources_hash();
		if (obj->is_missile()) {
			auto senderID = obj->missile_get_sender_id();
			if (senderID)
			{
				auto sender = entitylist->get_object(senderID);
				if (sender  && sender->is_ally())
				{
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
		}

		const auto object_hash = spell_hash_real(obj->get_name_cstr());
		const auto emitterHash = obj->get_emitter_resources_hash();

		// I <3 yorik100
		if (!obj->get_emitter() || !obj->get_emitter()->is_enemy() || !obj->get_emitter()->is_ai_hero()) return;

		switch (emitterHash)
		{
		case buff_hash("TwistedFate_R_Gatemarker_Red"):
		{
			const particleStruct& particleData = { .obj = obj, .owner = obj->get_emitter(), .time = gametime->get_time(), .castTime = 1.5, .castingPos = obj->get_position() };
			particlePredList.push_back(particleData);
			return;
		}
		case buff_hash("Ekko_R_ChargeIndicator"):
		{
			const particleStruct& particleData = { .obj = obj, .owner = obj->get_emitter(), .time = gametime->get_time(), .castTime = 0.5, .castingPos = obj->get_position() };
			particlePredList.push_back(particleData);
			return;
		}
		case buff_hash("Pantheon_R_Update_Indicator_Enemy"):
		{
			const auto castPos = obj->get_position() + obj->get_particle_rotation_forward() * 1350;
			const particleStruct& particleData = { .obj = obj, .owner = obj->get_emitter(), .time = gametime->get_time(), .castTime = 2.2, .castingPos = castPos };
			particlePredList.push_back(particleData);
			return;
		}
		case buff_hash("Galio_R_Tar_Ground_Enemy"):
		{
			const particleStruct& particleData = { .obj = obj, .owner = obj->get_emitter(), .time = gametime->get_time(), .castTime = 2.75, .castingPos = obj->get_position() };
			particlePredList.push_back(particleData);
			return;
		}
		case buff_hash("Evelynn_R_Landing"):
		{
			const particleStruct& particleData = { .obj = obj, .owner = obj->get_emitter(), .time = gametime->get_time(), .castTime = 0.85, .castingPos = obj->get_position() };
			particlePredList.push_back(particleData);
			return;
		}
		case buff_hash("TahmKench_W_ImpactWarning_Enemy"):
		{
			const particleStruct& particleData = { .obj = obj, .owner = obj->get_emitter(), .time = gametime->get_time(), .castTime = 0.8, .castingPos = obj->get_position() };
			particlePredList.push_back(particleData);
			return;
		}
		case buff_hash("Zed_R_tar_TargetMarker"):
			if (obj->get_particle_attachment_object() && obj->get_particle_attachment_object()->get_handle() == myhero->get_handle())
			{
				const particleStruct& particleData = { .obj = obj, .target = obj->get_particle_attachment_object(), .owner = obj->get_emitter(), .time = gametime->get_time(), .castTime = 0.95, .castingPos = vector::zero, .isZed = true };
				particlePredList.push_back(particleData);
				return;
			}
		case 1882371666:
		{
			const particleStruct& particleData = { .obj = obj, .target = obj->get_particle_attachment_object(), .owner = obj->get_emitter(), .time = gametime->get_time(), .castTime = obj->get_position().distance(urfCannon) / 2800, .castingPos = obj->get_position() };
			particlePredList.push_back(particleData);
			return;
		}
		}

		if (object_hash == spell_hash("global_ss_teleport_turret_red.troy"))
		{
			if (generalMenu::debug->get_bool()) console->print("Found Teleport ");
			const auto& target = obj->get_particle_attachment_object();
			if (nexusPos != vector::zero)
			{
				const particleStruct& particleData = { .obj = obj, .target = target, .owner = obj->get_emitter(), .time = gametime->get_time(), .castTime = 4.f, .castingPos = vector::zero, .isTeleport = true };
				particlePredList.push_back(particleData);
				return;
			}
		}
		else if (object_hash == spell_hash("global_ss_teleport_target_red.troy"))
		{
			if (generalMenu::debug->get_bool()) console->print("Found Teleport 2");
			const auto& target = obj->get_particle_target_attachment_object();
			if (nexusPos != vector::zero)
			{
				const particleStruct& particleData = { .obj = obj, .target = target, .owner = obj->get_emitter(), .time = gametime->get_time(), .castTime = 4.f, .castingPos = vector::zero, .isTeleport = true };
				particlePredList.push_back(particleData);
				return;
			}
		}
	}
	void on_delete_object(game_object_script obj) {

		if (obj->is_missile()) {
			auto senderID = obj->missile_get_sender_id();
			if (senderID)
			{
				auto sender = entitylist->get_object(senderID);
				if (sender && sender->is_ally())
				{
					missileList.erase(std::remove_if(missileList.begin(), missileList.end(), [obj](game_object_script& missile)
						{
							return missile->get_handle() == obj->get_handle();
						}), missileList.end());
				}
			}
		}
	}
	void on_buff(game_object_script& sender, buff_instance_script& buff, const bool gain)
	{
		if (!buff || !sender) return;

		// Detects if someone is reviving from Guardian Angel
		if (!gain && buff->get_hash_name() == buff_hash("willrevive") && sender->is_playing_animation(buff_hash("Death")) && sender->has_item(ItemId::Guardian_Angel) != spellslot::invalid)
		{
			if (generalMenu::debug->get_bool()) console->print("%s will ga revive", sender->get_model_cstr());
			guardianReviveTime[sender->get_handle()] = deathAnimTime[sender->get_handle()] + 4;
			return;
		}
	}
	void on_buff_gain(game_object_script sender, buff_instance_script buff)
	{
		// Grouping on buff gain && on buff lose together
		on_buff(sender, buff, true);
	}
	void on_buff_lose(game_object_script sender, buff_instance_script buff)
	{
		// Grouping on buff gain && on buff lose together
		on_buff(sender, buff, false);
	}
	void on_network_packet(game_object_script sender, std::uint32_t network_id, pkttype_e type, void* args)
	{
		if (type != pkttype_e::PKT_S2C_PlayAnimation_s || !sender) return;

		const auto& data = (PKT_S2C_PlayAnimationArgs*)args;
		if (!data) return;
		//if (generalMenu::debug->get_bool()) console->print("Found Animation %s on %s", data->animation_name, sender->get_model_cstr());

		if (strcmp(data->animation_name, "Death") == 0)
		{
			deathAnimTime[sender->get_handle()] = gametime->get_time();
			if (generalMenu::debug->get_bool()) console->print("Set DeathAnimTime to %f on %s", gametime->get_time(), sender->get_model_cstr());
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
		{
			generalMenu::debug = generalMenu->add_checkbox("debug", "Debug Prints", false);
		}
		auto qMenu = mainMenuTab->add_tab("q", "Q Settings");
		{
			qMenu->set_assigned_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
			qMenu::hc = qMenu->add_combobox("Hitchance", "Hitchance", { {"Medium", nullptr},{"High", nullptr},{"Very High", nullptr} }, 2);
			qMenu::mode = qMenu->add_combobox("mode", "Q Mode", { {"Combo + Harass", nullptr},{"Combo", nullptr}, {"Off", nullptr} }, 0);
			qMenu->add_separator("sep1", "Auto Q");
			qMenu::onCC = qMenu->add_checkbox("oncc", "On CC", true);
			qMenu::onParticle = qMenu->add_checkbox("onParticle", "On Teleport-like Spells", true);
			qMenu::onParticle->set_tooltip("Teleport, TwistedFate R, TahmKench W, etc");
			qMenu::onSpecialSpells = qMenu->add_checkbox("onSpecialSpells", "On Special Spells", true);
			qMenu::onSpecialSpells->set_tooltip("Fiora W, Long cast times");
			qMenu::onStasis = qMenu->add_checkbox("onStasis", "On Stasis", true);
			qMenu::onDashes = qMenu->add_combobox("onDashes", "On Dashes", { {"Off", nullptr},{"Prediction", nullptr}, {"Always", nullptr} }, 1);
			qMenu::dashWhitelist = qMenu->add_tab("dashWhitelist", "Whitelist for Dashes");
			for (const auto& enemy : entitylist->get_enemy_heroes()) {
				uint32_t id = enemy->get_network_id();
				bool defaultVal = true;
				if (enemy->get_champion() == champion_id::Yasuo || enemy->get_champion() == champion_id::Nilah || enemy->get_champion() == champion_id::Samira || enemy->get_champion() == champion_id::Akali || enemy->get_champion() == champion_id::Kalista) defaultVal = false;
				qDashWhitelist[id] = qMenu::dashWhitelist->add_checkbox(std::to_string(id), Database::getDisplayName(enemy), defaultVal, false);
				qDashWhitelist[id]->set_texture(enemy->get_square_icon_portrait());
			}
			qMenu->add_separator("sep2", "");
			qMenu::range = qMenu->add_slider("range", "Range", 850, 750, 850);
			qMenu::range->add_property_change_callback([](TreeEntry* entry) {
				q->set_range(entry->get_int());
				});
			q->set_range(qMenu::range->get_int());
			qMenu::toggleRadius = qMenu->add_hotkey("toggleCenter", "Try to hit Center", TreeHotkeyMode::Toggle, 0x49, true);
			qMenu::toggleRadius->add_property_change_callback([](TreeEntry* entry) {
				q->set_radius(200 -100 * entry->get_bool());
				});
			q->set_radius(200 - 100 * qMenu::toggleRadius->get_bool());
		}
		auto wMenu = mainMenuTab->add_tab("w", "W Settings");
		{
			wMenu->set_assigned_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
			wMenu::minHealRatio = wMenu->add_slider("minHealRatio", "Min heal Percent for bounces", 75, 0, 150);
			wMenu::minHealRatio->set_tooltip("Only Counts bounces to allies if you heal them\n"
											"If at 100, only count if missingHealth > wHeal\n"
											"If at 50, only count if you waste half your heal at max");
			wMenu::mode = wMenu->add_combobox("mode", "W Mode", { {"Combo + Harass", nullptr},{"Combo", nullptr}, {"Off", nullptr} }, 0);
			wMenu::minTargets = wMenu->add_slider("minTargets", "Min Targets", 2, 1, 3);
			wMenu::autoWTripleHit = wMenu->add_checkbox("autoTripleHit", "Auto W when 3 bounces", true);
			wMenu::useOnLowHP = wMenu->add_tab("useOnLowHP", "Auto W Allies under x% HP");
			wMenu::useOnLowHP->add_separator("sep1", "0 to disable");
			for (const auto& ally : entitylist->get_ally_heroes()) {
				uint32_t id = ally->get_network_id();
				wLowHPList[id] = wMenu::useOnLowHP->add_slider(std::to_string(id), Database::getDisplayName(ally), 50 - 25*(myhero->get_handle() == ally->get_handle()), 0, 100, false);
				wLowHPList[id]->set_texture(ally->get_square_icon_portrait());
			}
			wMenu::wHealMana = wMenu->add_slider("wHealMana", "^Min % Mana to auto Heal", 50, 0, 100);

		}
		auto eMenu = mainMenuTab->add_tab("e", "E Settings");
		{
			eMenu->set_assigned_texture(myhero->get_spell(spellslot::e)->get_icon_texture());
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
		{
			rMenu->set_assigned_texture(myhero->get_spell(spellslot::r)->get_icon_texture()); 
			rMenu::range = rMenu->add_slider("range", "Range", 1500, 1000, 2750);
			rMenu::range->add_property_change_callback([](TreeEntry* entry) {
				r->set_range(entry->get_int());
				});
			r->set_range(rMenu::range->get_int());// when loading
			rMenu::hc = rMenu->add_combobox("Hitchance", "Hitchance", { {"Medium", nullptr},{"High", nullptr},{"Very High", nullptr} }, 2);
			rMenu::comboTargets = rMenu->add_slider("comboTargets", "Min Targets in Combo (0 to disable)", 3, 0, 5);
			rMenu::semiKey = rMenu->add_hotkey("semiKey", "Semi Key", TreeHotkeyMode::Hold, 5, true);
			rMenu::semiTargets = rMenu->add_slider("semiTargets", "Min Targets for Semi R (0 to disable)", 2, 0, 5);
			rMenu::semiSelected = rMenu->add_checkbox("semiSelected", "Force Semi R on selected", true);
			rMenu::semiSelected->set_tooltip("If you click on someone to force that target (red circle under them), ignore how many it can hit");
			rMenu::flee = rMenu->add_slider("flee", "Use R to flee if under x% HP", 30, 0, 100);
		}
		auto drawMenu = mainMenuTab->add_tab("drawings", "Drawings Settings");
		{
			drawMenu::drawOnlyReady = drawMenu->add_checkbox("drawReady", "Draw Only Ready", true);
			drawMenu::drawRangeQ = drawMenu->add_checkbox("drawQ", "Draw Q range", true);
			drawMenu::drawRangeW = drawMenu->add_checkbox("drawW", "Draw W range", true);
			drawMenu::drawRangeE = drawMenu->add_checkbox("drawE", "Draw E range", true);
			drawMenu::drawRangeR = drawMenu->add_checkbox("drawR", "Draw R range", true);


			drawMenu::drawRangeQ->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
			drawMenu::drawRangeW->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
			drawMenu::drawRangeE->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture());
			drawMenu::drawRangeR->set_texture(myhero->get_spell(spellslot::r)->get_icon_texture());

			drawMenu->add_separator("sep1", "Debug Settings");
			drawMenu::drawWTargets = drawMenu->add_checkbox("drawWTargets", "Draw W Hitcount", false);
			drawMenu::drawESpells = drawMenu->add_checkbox("drawESpells", "Draw Spells for E", false);
			drawMenu::drawWTargets->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
			drawMenu::drawESpells->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture());

			drawMenu->add_separator("sep2", "");

			auto colorMenu = drawMenu->add_tab("color", "Color Settings");

			float qcolor[] = { 0.f, 0.f, 1.f, 1.f };
			colorMenu::qColor = colorMenu->add_colorpick("colorQ", "Q Range Color", qcolor);
			float wcolor[] = { 0.f, 1.f, 0.f, 1.f };
			colorMenu::wColor = colorMenu->add_colorpick("colorW", "W Range Color", wcolor);
			float ecolor[] = { 1.f, 0.f, 1.f, 1.f };
			colorMenu::eColor = colorMenu->add_colorpick("colorE", "E Range Color", ecolor);
			float rcolor[] = { 1.f, 1.f, 0.f, 1.f };
			colorMenu::rColor = colorMenu->add_colorpick("colorR", "R Range Color", rcolor);
		}
		auto interruptMenu = mainMenuTab->add_tab("interrupt", "Interrupt Settings");
		{
			interruptMenu::useQ = interruptMenu->add_checkbox("useQ", "Use Q for Importance >= 1", true);
			interruptMenu::useQ->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
			interruptMenu::useR = interruptMenu->add_checkbox("useR", "Use R for Importance >= 3", true);
			interruptMenu::useR->set_texture(myhero->get_spell(spellslot::r)->get_icon_texture());
			interruptMenu::spelldb = interruptMenu->add_tab("interruptdb", "Importance Settings");
			Database::InitializeCancelMenu(interruptMenu::spelldb);
		}

		event_handler<events::on_draw>::add_callback(on_draw);
		event_handler<events::on_env_draw>::add_callback(on_env_draw);
		event_handler<events::on_update>::add_callback(on_update);
		event_handler<events::on_process_spell_cast>::add_callback(on_process_spell_cast);
		event_handler<events::on_create_object>::add_callback(on_create_object);
		event_handler<events::on_delete_object>::add_callback(on_delete_object);
		event_handler<events::on_buff_gain>::add_callback(on_buff_gain);
		event_handler<events::on_buff_lose>::add_callback(on_buff_lose);
		event_handler<events::on_network_packet>::add_callback(on_network_packet);

		const auto nexusPosIt = std::find_if(entitylist->get_all_nexus().begin(), entitylist->get_all_nexus().end(), [](const game_object_script& x) { return x != nullptr && x->is_valid() && x->is_enemy(); });
		if (nexusPosIt != entitylist->get_all_nexus().end())
		{
			const auto& nexusEntity = *nexusPosIt;
			if (nexusEntity->is_valid())
				nexusPos = nexusEntity->get_position();
		}
		urfCannon = myhero->get_team() == game_object_team::order ? vector(13018.f, 14026.f) : vector(1506.f, 676.f);

		permashow::instance.init(mainMenuTab);
		permashow::instance.add_element("Hit Q Center", qMenu::toggleRadius);
		permashow::instance.add_element("Semi R", rMenu::semiKey);

		mainMenuTab->add_separator("version", VERSION);
		mainMenuTab->add_separator("sep1", "Special Thanks to yorik100");
	}
	void unload() {
		plugin_sdk->remove_spell(q);
		plugin_sdk->remove_spell(w);
		plugin_sdk->remove_spell(e);
		plugin_sdk->remove_spell(r);
		menu->delete_tab(mainMenuTab);

		permashow::instance.destroy();

		event_handler<events::on_draw>::remove_handler(on_draw);
		event_handler<events::on_env_draw>::add_callback(on_env_draw);
		event_handler<events::on_update>::remove_handler(on_update);
		event_handler<events::on_process_spell_cast>::remove_handler(on_process_spell_cast);
		event_handler<events::on_create_object>::remove_handler(on_create_object);
		event_handler<events::on_delete_object>::remove_handler(on_delete_object);
		event_handler<events::on_buff_gain>::remove_handler(on_buff_gain);
		event_handler<events::on_buff_lose>::remove_handler(on_buff_lose);
		event_handler<events::on_network_packet>::remove_handler(on_network_packet);
	}
}