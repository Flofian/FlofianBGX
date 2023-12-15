#include "../plugin_sdk/plugin_sdk.hpp"
#include "malphite.h"
#include "permashow.hpp"


namespace malphite {
	std::string VERSION = "b.0.0.0";
	script_spell* q = nullptr;
	script_spell* w = nullptr;
	script_spell* e = nullptr;
	script_spell* r = nullptr;
	script_spell* flash = nullptr;
	TreeTab* mainMenuTab = nullptr;

	bool useRNow = false;

	bool hasComet = false;
	bool hasManaflow = false;

	std::unordered_map<uint32_t, prediction_output> ePredictionList;
	std::unordered_map<uint32_t, prediction_output> rPredictionList;

	struct rPos {
		vector pos = vector();
		int hitcount = 0;
	};
	rPos bestRPos = rPos();
	rPos bestFRPos = rPos();
	rPos flashRTarget = rPos();

	namespace generalMenu {
		TreeEntry* spellfarm = nullptr;
		TreeEntry* spellfarmMana = nullptr;
		TreeEntry* preventCancel = nullptr;
		TreeEntry* turretCheck = nullptr;
	}
	namespace qMenu {
		TreeEntry* autoQHotkey = nullptr;
		TreeEntry* combo = nullptr;
		TreeEntry* harass = nullptr;
		TreeEntry* autoQManaflow = nullptr;
		TreeEntry* autoQComet = nullptr;
		TreeEntry* farmmode = nullptr;
	}
	namespace wMenu {
		TreeEntry* combo = nullptr;
		TreeEntry* harass = nullptr;
		TreeEntry* lasthit = nullptr;
	}
	namespace eMenu {
		TreeEntry* range = nullptr;
		TreeEntry* combo = nullptr;
		TreeEntry* harass = nullptr;
		TreeEntry* autoMinTargets = nullptr;
		TreeEntry* farmmode = nullptr;
		TreeEntry* farmMinTargets = nullptr;
	}
	namespace rMenu {
		TreeEntry* range = nullptr;
		TreeEntry* comboMin = nullptr;
		TreeEntry* flashComboMin = nullptr;
		TreeEntry* autoMin = nullptr;
		TreeEntry* flashAutoMin = nullptr;
		TreeEntry* forceRSelected = nullptr;
	}
	namespace rPredMenu {
		TreeEntry* radius = nullptr;
		TreeEntry* hc = nullptr;
		TreeEntry* predRadius = nullptr;
	}
	namespace drawMenu
	{
		TreeEntry* drawOnlyReady = nullptr;
		TreeEntry* drawRangeQ = nullptr;
		TreeEntry* drawRangeE = nullptr;
		TreeEntry* drawRangeR = nullptr;
		TreeEntry* drawRangeFR = nullptr;
		TreeEntry* drawCircleR = nullptr;
		TreeEntry* drawCircleFR = nullptr;
	}
	namespace colorMenu
	{
		TreeEntry* qColor = nullptr;
		TreeEntry* eColor = nullptr;
		TreeEntry* rColor = nullptr;
		TreeEntry* frColor = nullptr;
		TreeEntry* rCircle = nullptr;
		TreeEntry* frCircle = nullptr;
	}

	hit_chance get_hitchance(const int hc)
	{
		switch (hc)
		{
		case 0:
			return hit_chance::low;
		case 1:
			return hit_chance::medium;
		case 2:
			return hit_chance::high;
		case 3:
			return hit_chance::very_high;
		}
		return hit_chance::medium;
	}

	void updatePredictionList() {
		r->set_speed(1500 + myhero->get_move_speed());
		// I change range here so i dont get pred saying impossible, i want at least flash r range, 
		// and i think if target is 1100 away, pred says impossible, but casting at 1000 still hits
		int oldrange = r->range();
		r->set_range(2000);
		//int oldradius = r->radius;
		//r->set_radius(rPredMenu::predRadius->get_int());
		//maybe that helps? to force pred to make it harder?
		for (const auto& target : entitylist->get_enemy_heroes())
		{
			if (!target->is_valid() || target->is_dead()) continue;
			rPredictionList[target->get_handle()] = r->get_prediction(target);

			// TODO: Check if thats the true time
			ePredictionList[target->get_handle()] = prediction->get_prediction(target, 0.2419f);
		}
		r->set_range(oldrange);
		//r->set_radius(oldradius);
		
	}
	vector canHitAll(std::vector<game_object_script> enemies) {
		std::vector<vector> positions = {};
		for (const auto& target : enemies) {
			if (rPredictionList[target->get_handle()].hitchance < get_hitchance(rPredMenu::hc->get_int())) return vector();
			positions.push_back(rPredictionList[target->get_handle()].get_unit_position());
		}
		mec_circle circle = mec::get_mec(positions);
		if (circle.radius < rPredMenu::radius->get_int()) return circle.center;
		
		return vector();
	}
	void updateBestRPos() {
		//console->print("Updating R");
		std::vector<game_object_script> enemiesInRange = {};
		std::vector<game_object_script> enemiesInFlashRRange = {};
		for (const auto& target : entitylist->get_enemy_heroes()) {
			// TODO: check spellshield? 
			if (target && target->is_valid() && target->get_distance(myhero) < rPredMenu::radius->get_int() + rMenu::range->get_int() + flash->range()
				&& !target->is_dead() && target->is_visible() && target->is_targetable()) {

				enemiesInFlashRRange.push_back(target);
				if (target->get_distance(myhero) < rPredMenu::radius->get_int()+ rMenu::range->get_int()) enemiesInRange.push_back(target);
			}
		}
		std::vector<std::vector<game_object_script>> subsets = {};
		std::vector<game_object_script> current = {};
		//console->print("%i Enemies in R Range", enemiesInRange.size());
		//console->print("%i Enemies in Flash R Range", enemiesInFlashRRange.size());
		// Normal R
		for (int i = 1; i < pow(2,enemiesInRange.size()); i++) {
			current.clear();
			for (int j = 0; j < enemiesInRange.size(); j++) {
				const auto& target = enemiesInRange[j];
				if ((i>>j) & 1) current.push_back(target);
			}
			subsets.push_back(current);
		}
		bestRPos = rPos();
		for (const auto& subset : subsets) {
			vector out = canHitAll(subset);
			if (out != vector() && out.distance(myhero)<r->range() && (subset.size() > bestRPos.hitcount || out.distance(myhero) < bestRPos.pos.distance(myhero))) {
				bestRPos.pos = out;
				bestRPos.hitcount = subset.size();
			}
		}
		// Flash R
		subsets.clear();
		for (int i = 1; i < pow(2, enemiesInFlashRRange.size()); i++) {
			current.clear();
			for (int j = 0; j < enemiesInFlashRRange.size(); j++) {
				const auto& target = enemiesInFlashRRange[j];
				if ((i >> j) & 1) current.push_back(target);
			}
			subsets.push_back(current);
		}
		bestFRPos = rPos();
		for (const auto& subset : subsets) {
			vector out = canHitAll(subset);
			if (out != vector() && out.distance(myhero) < r->range()+400 && (subset.size() > bestFRPos.hitcount || out.distance(myhero) < bestFRPos.pos.distance(myhero))) {
				bestFRPos.pos = out;
				bestFRPos.hitcount = subset.size();
			}
		}
		
	}

	bool canGetLasthit(game_object_script minion, float additionalDmg=0, float additionalDelay=0) {
		float timeToMove = (minion->get_distance(myhero) - myhero->get_attack_range()) / myhero->get_move_speed();
		float healthPred = health_prediction->get_health_prediction(minion, timeToMove + additionalDelay + myhero->get_attack_cast_delay())-additionalDmg;
		return healthPred > 0;
	}

	bool autoQCheck() {
		bool comet = !qMenu::autoQComet->get_bool() || !myhero->has_buff(buff_hash("ASSETS/Perks/Styles/Sorcery/PotentialEnergy/PerkSorceryOutOfCombatCooldownBuff.lua"));
		bool manaflow = !qMenu::autoQManaflow->get_bool() || !myhero->has_buff(buff_hash("ASSETS/Perks/Styles/Sorcery/ArcaneComet/ArcaneCometRechargeSnipe.lua"));
		return comet && manaflow;
	}

	void automatic() {
		if (generalMenu::turretCheck->get_bool() && myhero->is_under_enemy_turret()) return;
		if (r->is_ready()) {
			int mintargets = rMenu::autoMin->get_int();
			int mintargetsflash = rMenu::flashAutoMin->get_int();
			if (useRNow) {
				r->cast(flashRTarget.pos);
				console->print("Used Auto Flash R, hits: %i, min targets: %i", bestFRPos.hitcount, mintargetsflash);
				useRNow = false;
				flashRTarget = rPos();
				return;
			}
			if (mintargetsflash > 0 && bestFRPos.hitcount >= mintargetsflash && flash->is_ready() && bestFRPos.pos != bestRPos.pos) {
				flash->cast(bestFRPos.pos);
				flashRTarget = bestFRPos;
				useRNow = true;
				console->print("Set Auto Flash Target, hits: %i, min targets: %i", bestFRPos.hitcount, mintargetsflash);
				return;
			}
			if (mintargets > 0 && bestRPos.hitcount >= mintargets) {
				r->cast(bestRPos.pos);
				console->print("Used Auto R, hits: %i, min targets: %i", bestRPos.hitcount, mintargets);
				useRNow = false;
				return;
			}
		}
		// Anti Auto Cancel and prevent accidental buffering
		if ((generalMenu::preventCancel->get_bool() && myhero->is_winding_up()) || !myhero->can_cast()) return;
		// Q
		if (q->is_ready() && qMenu::autoQHotkey->get_bool() && autoQCheck()) {
			auto target = target_selector->get_target(q, damage_type::magical);
			if (!target) return;
			q->cast(target);
		}
		// E
		if (e->is_ready()) {
			int minTargets = eMenu::autoMinTargets->get_int();
			if (minTargets == 0) return;
			int hits = 0;
			for (const auto& target : entitylist->get_enemy_heroes()) {
				auto pred = ePredictionList[target->get_handle()];
				if (pred.get_unit_position().distance(myhero) < e->range() && pred.hitchance > hit_chance::impossible && target->is_targetable()) hits++;
			}
			if (hits >= minTargets) e->cast();
		}
	}
	void combo() {
		// R
		if (r->is_ready()){
			auto selTarget = target_selector->get_selected_target();
			if (selTarget && rMenu::forceRSelected->get_bool()) {
				auto predCastPos = rPredictionList[selTarget->get_handle()].get_cast_position();
				if (predCastPos.distance(myhero) < r->range()) r->cast(predCastPos);
			}

			int mintargets = rMenu::comboMin->get_int();
			int mintargetsflash = rMenu::flashComboMin->get_int();
			if (useRNow) {
				r->cast(flashRTarget.pos);
				console->print("Used Combo Flash R, hits: %i, min targets: %i", bestFRPos.hitcount, mintargetsflash);
				useRNow = false;
				flashRTarget = rPos();
				return;
			}
			if (mintargetsflash > 0 && bestFRPos.hitcount >= mintargetsflash && flash->is_ready() && bestFRPos.pos != bestRPos.pos) {
				flash->cast(bestFRPos.pos);
				flashRTarget = bestFRPos;
				useRNow = true;
				console->print("Set Combo Flash Target, hits: %i, min targets: %i", bestFRPos.hitcount, mintargetsflash);
				return;
			}
			if (mintargets > 0 && bestRPos.hitcount >= mintargets) {
				r->cast(bestRPos.pos);
				console->print("Used Combo R, hits: %i, min targets: %i", bestRPos.hitcount, mintargets);
				useRNow = false;
				return;
			}
		}
		// Anti Auto Cancel and prevent accidental buffering
		// TODO: replace with orbwalker->can_move( 0.05f ) ?
		if ((generalMenu::preventCancel->get_bool() && myhero->is_winding_up()) || !myhero->can_cast()) return;
		// Q
		if (q->is_ready() && qMenu::combo->get_bool()) {
			auto target = target_selector->get_target(q, damage_type::magical);
			if (!target) return;
			q->cast(target);
		}
		// E
		if (e->is_ready() && eMenu::combo->get_bool()) {
			for (const auto& target : entitylist->get_enemy_heroes()) {
				auto pred = ePredictionList[target->get_handle()];
				if (pred.get_unit_position().distance(myhero) < e->range() && pred.hitchance>hit_chance::impossible && target->is_targetable()) e->cast();
			}
		}
	}
	void harass() {
		// Q
		if (q->is_ready() && qMenu::harass->get_bool()) {
			auto target = target_selector->get_target(q, damage_type::magical);
			if (!target) return;
			q->cast(target);
		}
		// E
		if (e->is_ready() && eMenu::harass->get_bool()) {
			for (const auto& target : entitylist->get_enemy_heroes()) {
				auto pred = ePredictionList[target->get_handle()];
				if (pred.get_unit_position().distance(myhero) < e->range() && pred.hitchance > hit_chance::impossible) e->cast();
			}
		}
	}

	void farm() {
		if (!generalMenu::spellfarm->get_bool() || myhero->get_mana_percent() < generalMenu::spellfarmMana->get_int()) return;
		if (!orbwalker->lane_clear_mode() && !orbwalker->last_hit_mode()) return;
		// Q Farm
		int qFarmMode = qMenu::farmmode->get_int();
		if (qFarmMode == 0 && orbwalker->last_hit_mode()) qFarmMode = 1;
		if (qFarmMode < 3) {
			//if qfarm is on
			auto minions = entitylist->get_enemy_minions();
			minions.erase(std::remove_if(minions.begin(), minions.end(), [](game_object_script x)
				{
					return !x->is_valid_target(q->range());
				}), minions.end());
			if (qFarmMode > 0) {
				//if only lasthit
				minions.erase(std::remove_if(minions.begin(), minions.end(), [](game_object_script x)
					{
						// 1200 = Malphite q speed, rock spawns 100 units in front of malphite, 0.25 cast time?
						auto timeToHit = (x->get_distance(myhero) - 100) / 1200;
						auto predHealth = health_prediction->get_health_prediction(x, timeToHit +0.25f);
						auto dmg = q->get_damage(x);
						return !(predHealth > 0 && predHealth < dmg);
					}), minions.end());
				if (qFarmMode == 2) {
					minions.erase(std::remove_if(minions.begin(), minions.end(), [](game_object_script x)
						{
							return x->get_minion_type() != 6;
						}), minions.end());
				}
			}
			else {
				minions.erase(std::remove_if(minions.begin(), minions.end(), [](game_object_script x)
					{
						auto timeToHit = (x->get_distance(myhero) - 100) / 1200;
						auto predHealth = health_prediction->get_health_prediction(x, timeToHit + 0.25f);
						auto dmg = q->get_damage(x);
						return !(predHealth > 0 && (canGetLasthit(x, dmg, 0.25f) || predHealth - dmg < 0));
					}), minions.end());
			}

			std::sort(minions.begin(), minions.end(), [](game_object_script a, game_object_script b)
				{
					return a->get_health() < b->get_health();
				});
			if (minions.size() > 0) q->cast(minions.front());
		}

		// E Farm
		int eFarmMode = eMenu::farmmode->get_int();
		if (eFarmMode > 1) return;
		int minTargets = eMenu::farmMinTargets->get_int();
		if (eFarmMode == 1 && !(orbwalker->get_orb_state() & orbwalker_state_flags::fast_lane_clear && orbwalker->lane_clear_mode())) return;
		// TODO: Decide if this uses the actual e range or the one i set in menu
		// TODO: Dont e if theres an enemy i cant lasthit anymore?
		auto eminions = entitylist->get_enemy_minions();
		eminions.erase(std::remove_if(eminions.begin(), eminions.end(), [](game_object_script x)
			{
				return !x->is_valid_target(400);
			}), eminions.end());
		eminions.erase(std::remove_if(eminions.begin(), eminions.end(), [](game_object_script x)
			{
				auto predHealth = health_prediction->get_health_prediction(x, 0.25f);
				auto dmg = e->get_damage(x);
				return !(predHealth > 0 && (canGetLasthit(x, dmg, 0.25f) || predHealth - dmg < 0));
			}), eminions.end());
		if (eminions.size() > minTargets) e->cast();
	}

	void drawFarm() {
		auto minions = entitylist->get_enemy_minions();
		minions.erase(std::remove_if(minions.begin(), minions.end(), [](game_object_script x)
			{
				return !x->is_valid_target(q->range());
			}), minions.end());
		// if we are in always mode, i dont want to q enemies that are then too low for me to lasthit, the 100 is arbitrary, maybe calc how far away it is so i can attack?
		minions.erase(std::remove_if(minions.begin(), minions.end(), [](game_object_script x)
			{
				auto timeToHit = (x->get_distance(myhero) - 100) / 1200;
				auto predHealth = health_prediction->get_health_prediction(x, timeToHit + 0.25f);
				auto dmg = q->get_damage(x);
				return !(predHealth > 0 && (canGetLasthit(x, dmg, 0.25f) || predHealth - dmg < 0));
			}), minions.end());
		for (const auto& minion : minions) {
			draw_manager->add_circle_with_glow(minion->get_position(), MAKE_COLOR(0, 255, 0, 255), minion->get_bounding_radius(), 1, glow_data(0.8, 0.8, 0, 0));
		}
	}

	void on_env_draw() {
		if (myhero->is_dead())
		{
			return;
		}
		if ((q->is_ready() || !drawMenu::drawOnlyReady->get_bool()) && drawMenu::drawRangeQ->get_bool())
			draw_manager->add_circle(myhero->get_position(), q->range(), colorMenu::qColor->get_color());
		if ((w->is_ready() || !drawMenu::drawOnlyReady->get_bool()) && drawMenu::drawRangeE->get_bool())
			draw_manager->add_circle(myhero->get_position(), e->range(), colorMenu::eColor->get_color());
		if ((r->is_ready() || !drawMenu::drawOnlyReady->get_bool())) {
			if (drawMenu::drawRangeR->get_bool()) draw_manager->add_circle(myhero->get_position(), r->range(), colorMenu::rColor->get_color());
			if (drawMenu::drawRangeFR->get_bool()) draw_manager->add_circle(myhero->get_position(), r->range()+flash->range(), colorMenu::frColor->get_color());
		}

	}
	void on_draw() {
		if ((r->is_ready() || !drawMenu::drawOnlyReady->get_bool())) {
			if (drawMenu::drawCircleR->get_bool() && bestRPos.pos != vector() && bestRPos.pos.distance(myhero) < r->range())
			{
				draw_manager->add_circle(bestRPos.pos, rPredMenu::radius->get_int(), colorMenu::rCircle->get_color(), 3);
				draw_manager->add_circle(bestRPos.pos, 5, colorMenu::rCircle->get_color(), 3);
				draw_manager->add_text(bestRPos.pos, colorMenu::rCircle->get_color(), 50, "%i", bestRPos.hitcount);
			}
			if (drawMenu::drawCircleFR->get_bool() && bestFRPos.pos != vector() && bestFRPos.pos.distance(myhero) < r->range() + 400 && bestFRPos.hitcount > bestRPos.hitcount)
			{
				draw_manager->add_circle(bestFRPos.pos, rPredMenu::radius->get_int(), colorMenu::frCircle->get_color(), 3);
				draw_manager->add_circle(bestFRPos.pos, 5, colorMenu::frCircle->get_color(), 3);
				draw_manager->add_text(bestFRPos.pos, colorMenu::frCircle->get_color(), 50, "%i", bestFRPos.hitcount);
			}
		}
		//drawFarm();
	}
	void on_update() {
		permashow::instance.update();
		updatePredictionList();
		updateBestRPos();
		if (orbwalker->combo_mode()) combo();
		automatic();
		// Anti Auto Cancel and prevent accidental buffering, do it after combo/auto to allow r
		if ((generalMenu::preventCancel->get_bool() && myhero->is_winding_up()) || !myhero->can_cast()) return;
		if (orbwalker->harass()) harass();
		farm();
	}
	void on_after_attack_orbwalker(game_object_script target) {
		if (w->is_ready() && ((orbwalker->combo_mode() && wMenu::combo->get_bool()) || (orbwalker->harass() && wMenu::harass->get_bool() && target->is_ai_hero()))) {
			w->cast();
			orbwalker->reset_auto_attack_timer();
			// TODO: Does that make sense? Feels like it from testing, else i just walk away
			// Maybe instead issue order for auto attack?
		}
	}

	void on_unkillable_minion(game_object_script minion) {
		//console->print("found unkillable minion");
		if (!generalMenu::spellfarm->get_bool() || myhero->get_mana_percent() < generalMenu::spellfarmMana->get_int() ||!w->is_ready()) return;
		if (orbwalker->last_hit_mode() || orbwalker->lane_clear_mode()) {
			if (myhero->is_in_auto_attack_range(minion, 50)) {
				auto wautodmg = myhero->get_auto_attack_damage(minion) + 20+10*w->level()+0.2f*myhero->get_total_ability_power() +0.15f*myhero->get_armor();
				auto healthPred = health_prediction->get_health_prediction(minion, myhero->get_attack_cast_delay());
				if(healthPred > 0 && healthPred-wautodmg<0)
				{
					//console->print("used w reset");
					w->cast();
					orbwalker->reset_auto_attack_timer();
				}
			}
		}
	}

	void load() {
		q = plugin_sdk->register_spell(spellslot::q, 625.f);
		w = plugin_sdk->register_spell(spellslot::w, 0.f);
		e = plugin_sdk->register_spell(spellslot::e, 400.f);
		r = plugin_sdk->register_spell(spellslot::r, 1000.f);
		r->set_skillshot(0, 325, 1500, {}, skillshot_type::skillshot_circle);
		
		if (myhero->get_spell(spellslot::summoner1)->get_spell_data()->get_name_hash() == spell_hash("SummonerFlash"))
			flash = plugin_sdk->register_spell(spellslot::summoner1, 400.f);
		else if (myhero->get_spell(spellslot::summoner2)->get_spell_data()->get_name_hash() == spell_hash("SummonerFlash"))
			flash = plugin_sdk->register_spell(spellslot::summoner2, 400.f);

		//Menu init
		{
			auto qTexture = myhero->get_spell(spellslot::q)->get_icon_texture();
			auto wTexture = myhero->get_spell(spellslot::w)->get_icon_texture();
			auto eTexture = myhero->get_spell(spellslot::e)->get_icon_texture();
			auto rTexture = myhero->get_spell(spellslot::r)->get_icon_texture();
			mainMenuTab = menu->create_tab("Flofian_Malphite", "Flofian Malphite");
			mainMenuTab->set_assigned_texture(myhero->get_square_icon_portrait());
			auto generalMenu = mainMenuTab->add_tab("general", "General Settings");
			{
				generalMenu::spellfarm = generalMenu->add_hotkey("spellfarmkey", "Spellfarm", TreeHotkeyMode::Toggle, 0x04, false);
				generalMenu::spellfarmMana = generalMenu->add_slider("spellfarmmana", "Min Mana % for Spellfarm", 30, 0, 100);
				generalMenu::preventCancel = generalMenu->add_checkbox("preventCancel", "Try to Prevent Auto Cancels", true); 
				generalMenu::turretCheck = generalMenu->add_checkbox("TurretCheck", "Dont Auto E/Q under Enemy turret", true);
			}
			auto qMenu = mainMenuTab->add_tab("Q", "Q Settings");
			{
				qMenu->set_assigned_texture(qTexture);
				qMenu::autoQHotkey = qMenu->add_hotkey("autoQHotkey", "Auto Q Toggle", TreeHotkeyMode::Toggle, 0x49, false);
				qMenu::combo = qMenu->add_checkbox("combo", "Use Q in Combo", true);
				qMenu::harass = qMenu->add_checkbox("harass", "Use Q in Harass", true);
				qMenu::autoQComet = qMenu->add_checkbox("autoQComet", "Only Auto Q when Comet Ready", true);
				qMenu::autoQComet->is_hidden() = true;
				qMenu::autoQManaflow = qMenu->add_checkbox("autoQManaflow", "Only Auto Q when Manaflow Ready", true);
				qMenu::autoQManaflow->set_tooltip("Gets Ignored if you have it fully stacked");
				qMenu::autoQManaflow->is_hidden() = true;
				for (const auto& rune : myhero->get_perks()) {
					switch (rune->get_id()) {
					case 8229:
					{
						hasComet = true;
						qMenu::autoQComet->is_hidden() = false;
						qMenu::autoQComet->set_texture(rune->get_texture());
						break;
					}
					
					case 8226:
					{
						hasManaflow = true;
						qMenu::autoQManaflow->is_hidden() = false;
						qMenu::autoQManaflow->set_texture(rune->get_texture());
						break;
					}
					default:
						break;
				}
				}

				qMenu::farmmode = qMenu->add_combobox("farmmode", "Farm Mode", { {"Always", nullptr}, {"Only Lasthit", nullptr},{"Only Lasthit Cannons", nullptr},{"Off", nullptr} }, 2);

			}
			auto wMenu = mainMenuTab->add_tab("W", "W Settings");
			{
				wMenu->set_assigned_texture(wTexture);
				wMenu::combo = wMenu->add_checkbox("combo", "Use W in Combo", true);
				wMenu::harass = wMenu->add_checkbox("harass", "Use W in Harass", true);
				wMenu::lasthit = wMenu->add_checkbox("lasthit", "Use W to lasthit", true);
				wMenu::lasthit->set_tooltip("Only gets used if orb cant get it without w, pls tell me how often its used");
			}
			auto eMenu = mainMenuTab->add_tab("E", "E Settings");
			{
				eMenu->set_assigned_texture(eTexture); 
				eMenu::range = eMenu->add_slider("Range", "Range", 375, 300, 400);
				eMenu::range->add_property_change_callback([](TreeEntry* entry) {
					e->set_range(entry->get_int());
					});
				eMenu::combo = eMenu->add_checkbox("combo", "Use E in Combo", true);
				eMenu::harass = eMenu->add_checkbox("harass", "Use E in Harass", true);
				eMenu::autoMinTargets = eMenu->add_slider("autoMinTargets", "Min Targets to Auto E", 2, 0, 5);
				eMenu::autoMinTargets->set_tooltip("0 to disable");
				eMenu::farmmode = eMenu->add_combobox("farmmode", "Farm Mode", { {"Always", nullptr}, {"Only Fast Laneclear", nullptr},{"Off", nullptr} }, 1);
				eMenu::farmMinTargets = eMenu->add_slider("farmtargets", "[Farm] Min Targets", 3, 1, 7);
			}
			auto rMenu = mainMenuTab->add_tab("R", "R Settings");
			{
				rMenu->set_assigned_texture(rTexture);
				rMenu::range = rMenu->add_slider("Range", "Range", 950, 900, 1000);
				rMenu::range->add_property_change_callback([](TreeEntry* entry) {
					r->set_range(entry->get_int());
					});
				
				rMenu->add_separator("sep1", "");
				rMenu::comboMin = rMenu->add_slider("combomin", "Min Targets in Combo", 2, 0, 5);
				rMenu::flashComboMin = rMenu->add_slider("flashComboMin", "Min  Targets in Combo to Flash R", 3, 0, 5);
				rMenu::autoMin = rMenu->add_slider("autoMin", "Min Targets to Auto R", 3, 0, 5);
				rMenu::flashAutoMin = rMenu->add_slider("flashAutoMin", "Min Targets to Auto Flash R", 4, 0, 5);
				rMenu->add_separator("sep2", "^ 0 to disable ^");
				rMenu::forceRSelected = rMenu->add_checkbox("forceRSelected", "Force R Selected in Combo",true);
			}
			auto rPredMenu = rMenu->add_tab("pred", "Prediction Settings");
			{
				rPredMenu::radius = rPredMenu->add_slider("radius", "Radius", 325, 250, 325);
				
				rPredMenu::hc = rPredMenu->add_combobox("Hitchance", "Hitchance", { {"Low", nullptr}, {"Medium", nullptr},{"High", nullptr},{"Very High", nullptr} }, 2);
				rPredMenu::predRadius = rPredMenu->add_slider("predRadius", "Prediction Radius", 100, 50, 300);
				rPredMenu::predRadius->set_tooltip("Default: 100\nSmaller -> Prediction needs to be more certain");
				rPredMenu::predRadius->add_property_change_callback([](TreeEntry* entry) {
					r->set_radius(entry->get_int());
					});
			}

			auto drawMenu = mainMenuTab->add_tab("drawings", "Draw Settings");
			{
				drawMenu::drawOnlyReady = drawMenu->add_checkbox("drawReady", "Draw Only Ready", true);
				drawMenu::drawRangeQ = drawMenu->add_checkbox("drawQ", "Q Range", true);
				drawMenu::drawRangeE = drawMenu->add_checkbox("drawE", "E Range", true);
				drawMenu::drawRangeR = drawMenu->add_checkbox("drawR", "R Range", true);
				drawMenu::drawRangeFR = drawMenu->add_checkbox("drawFR", "Flash R Range", true);
				drawMenu::drawCircleR = drawMenu->add_checkbox("drawRground", "R Target [Ground]", false);
				drawMenu::drawCircleFR = drawMenu->add_checkbox("drawFRground", "Flash R Target [Ground]", false);

				auto colorMenu = drawMenu->add_tab("color", "Color Settings");

				float qcolor[] = { 0.f, 0.f, 1.f, 1.f };
				colorMenu::qColor = colorMenu->add_colorpick("colorQ", "Q Range", qcolor);
				float ecolor[] = { 1.f, 1.f, 0.f, 1.f };
				colorMenu::eColor = colorMenu->add_colorpick("colorE", "E Range", ecolor);
				float rcolor[] = { 1.f, 0.f, 0.f, 1.f };
				colorMenu::rColor = colorMenu->add_colorpick("colorR", "R Range", rcolor);
				float frcolor[] = { 1.f, 0.f, 0.f, 1.f };
				colorMenu::frColor = colorMenu->add_colorpick("colorFR", "Flash R Range", frcolor);

				float rc[] = { 0.f, 1.f, 0.f, 1.f };
				colorMenu::rCircle = colorMenu->add_colorpick("rCircle", "R Target [Ground]", rc);
				float frc[] = { 0.f, 1.f, 1.f, 1.f };
				colorMenu::frCircle = colorMenu->add_colorpick("frCircle", "Flash R Target [Ground]", frc);

				drawMenu::drawRangeQ->set_texture(qTexture);
				drawMenu::drawRangeE->set_texture(eTexture);
				drawMenu::drawRangeR->set_texture(rTexture);
				drawMenu::drawRangeFR->set_texture(rTexture);
			}
		}
		{
			e->set_range(eMenu::range->get_int());
			r->set_range(rMenu::range->get_int());
			r->set_radius(rPredMenu::predRadius->get_int());
		}

		permashow::instance.init(mainMenuTab);
		permashow::instance.add_element("Spellfarm", generalMenu::spellfarm);
		permashow::instance.add_element("Auto Q", qMenu::autoQHotkey);

		event_handler<events::on_env_draw>::add_callback(on_env_draw);
		event_handler<events::on_draw>::add_callback(on_draw);
		event_handler<events::on_update>::add_callback(on_update);
		event_handler<events::on_after_attack_orbwalker>::add_callback(on_after_attack_orbwalker);
		event_handler<events::on_unkillable_minion>::add_callback(on_unkillable_minion);

	}

	void unload() {
		plugin_sdk->remove_spell(q);
		plugin_sdk->remove_spell(w);
		plugin_sdk->remove_spell(e);
		plugin_sdk->remove_spell(r);

		menu->delete_tab(mainMenuTab);
		permashow::instance.destroy();
		event_handler<events::on_env_draw>::remove_handler(on_env_draw);
		event_handler<events::on_draw>::remove_handler(on_draw);
		event_handler<events::on_update>::remove_handler(on_update);
		event_handler<events::on_after_attack_orbwalker>::remove_handler(on_after_attack_orbwalker);
		event_handler<events::on_unkillable_minion>::remove_handler(on_unkillable_minion);
	}
}