#include "../plugin_sdk/plugin_sdk.hpp"
#include "nautilus.h"
#include "../interruptdb/Interrupt.h"
// TODO: E with prediction, (split into the 3 circles?), multi hit e?

namespace nautilus {
	std::string VERSION = "1.0.0";
	script_spell* q = nullptr;
	script_spell* w = nullptr;
	script_spell* e = nullptr;
	script_spell* r = nullptr;

	TreeTab* mainMenuTab = nullptr;
	TreeTab* spelldb = nullptr;

	std::map<std::uint32_t, TreeEntry*> q_whitelist;
	std::map<std::uint32_t, TreeEntry*> r_whitelist;

	namespace generalMenu {
		TreeEntry* debug = nullptr;
		TreeEntry* waitRunes = nullptr;
		TreeEntry* forceOrb = nullptr;
	}
	namespace qMenu {
		TreeEntry* range = nullptr;
		TreeEntry* hitchance = nullptr;
		TreeTab* whitelist = nullptr;
		TreeEntry* comboQ = nullptr;
		TreeEntry* harassQ = nullptr;
		TreeEntry* interrupt = nullptr;
		TreeEntry* bestWallHK = nullptr;
		TreeEntry* wallMinDistance = nullptr;
	}
	namespace wMenu {
		TreeEntry* autoShield = nullptr;
		TreeEntry* autoShieldValue = nullptr;
		TreeEntry* includeSkillshots = nullptr;
		TreeEntry* shieldTime = nullptr;
		TreeEntry* comboW = nullptr;
	}
	namespace eMenu {
		TreeEntry* range = nullptr;
		TreeEntry* comboE = nullptr;
		TreeEntry* harassE = nullptr;
	}
	namespace rMenu {
		TreeEntry* range = nullptr;
		TreeTab* whitelist = nullptr;
		TreeEntry* comboR = nullptr;
		TreeEntry* minTargets = nullptr;
		TreeEntry* forceSelected = nullptr;
		TreeEntry* interrupt = nullptr;
	}
	namespace drawMenu
	{
		TreeEntry* drawOnlyReady = nullptr;
		TreeEntry* drawRangeQ = nullptr;
		TreeEntry* drawRangeE = nullptr;
		TreeEntry* drawRangeR = nullptr;
		TreeEntry* drawQWall = nullptr;
	}
	namespace colorMenu
	{
		TreeEntry* qColor = nullptr;
		TreeEntry* eColor = nullptr;
		TreeEntry* rColor = nullptr;
	}

	//whitelist helper function
	bool checkWhitelist(game_object_script target, bool is_r=false) {
		auto list = is_r ? r_whitelist : q_whitelist;
		auto entry = list.find(target->get_network_id());
		if (entry == list.end()) return false;	//Dont use on champs that arent in the list (soraka/scuttlecrab in nexus blitz)
		return entry->second->get_bool();
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

	bool getRunesReady() {
		//only true if i dont have any of the buffs
		return !myhero->has_buff({ 
			buff_hash("ASSETS/Perks/Styles/Inspiration/GlacialAugment/GlacialAugmentCooldown.lua"),
			buff_hash("ASSETS/Perks/Styles/Resolve/VeteranAftershock/VeteranAftershockCooldownBuff.lua")});
	}

	vector firstWallHit(vector targetpos, int minHits = 1) {
		vector castVector = (targetpos - myhero->get_position()).normalized() * q->range();
		int wallHits = 0;
		vector first = vector();
		for (int i = 0; i <= 100; i++) {
			vector p = castVector * i / 100.f + myhero->get_position();
			if (p.is_wall()) 
			{
				wallHits++;
				if (first == vector()) first = p;
			}
		}
		return (wallHits > minHits) ? first : vector();
	}
	vector firstWallHitByDirection(vector castVector, int minHits = 1) {
		int wallHits = 0;
		vector first = vector();
		for (int i = 0; i <= 100; i++) {
			vector p = castVector * i / 100.f + myhero->get_position();
			if (p.is_wall()) wallHits++;
			if (p.is_wall())
			{
				wallHits++;
				if (first == vector()) first = p;
			}
		}
		return (wallHits > minHits) ? first : vector();
	}
	vector bestWallQPos() {
		auto mouse = hud->get_hud_input_logic()->get_game_cursor_position();
		auto qVector = (mouse - myhero->get_position()).normalized() * q->range();
		vector furthestPos = myhero->get_position();
		float maxAngleDegrees = 30;
		float stepsPerDegree = 10;
		float steps = stepsPerDegree * maxAngleDegrees;
		float degreeStepSize = M_PI / (180.f * stepsPerDegree);
		for (int i = 0; i < 2 * steps; i++) {
			// i go 1 to the right, then 2 to the left (so 1 left from start), 3 right (2 right from start), 4 left (2 left from start), ...
			qVector = qVector.rotated(degreeStepSize * i * pow(-1, ((i + 1) % 2) + 1));	// looks ugly, but i want to have **2 when 0, ** 1 when 1, ...
			auto wallHit = firstWallHitByDirection(qVector, 8);
			if (wallHit == vector()) continue;
			// the subtraction helps to prefer targets near mouse, even if further away from mouse have a bit more distance
			if (wallHit.distance(myhero) - (i / stepsPerDegree) / 2 > furthestPos.distance(myhero) && wallHit.distance(myhero) > qMenu::wallMinDistance->get_int()) furthestPos = wallHit;
		}

		return furthestPos;
	}

	vector getQCastPos(game_object_script target, bool ignore_spellshield = false) {
		if (target && target->is_valid() && target->get_distance(myhero) < qMenu::range->get_int() && (ignore_spellshield || !target->get_is_cc_immune()) && target->is_visible()) {
			auto pred = q->get_prediction(target);
			if (pred.hitchance < get_hitchance(qMenu::hitchance->get_int())) return vector();
			auto castpos = pred.get_cast_position();
			bool canhit = true;
			for (int i = 0; i <= 1060; i += 5) {
				auto p = ((castpos - myhero->get_position()).normalized() * i) + myhero->get_position();
				if (p.distance(myhero) > pred.get_unit_position().distance(myhero) - target->get_bounding_radius()) break;
				if (p.is_wall()){
					canhit = false;
					break;
				}
			}
			if (canhit) return castpos;

		}
		return vector();
	}
	float calcShieldValue() {
		// Include Runes/Items/Antishield Debuff in the future
		float shield = 40 + 10 * w->level() + (0.07 + 0.01 * w->level())*myhero->get_max_health();

		return shield;
	}
	int countRTargets(game_object_script target) {
		//TODO: can def be better, but at least its something
		int count = 1;
		auto predPos = prediction->get_prediction(target, 0.6f);
		auto hitbox = geometry::rectangle(myhero->get_position(), predPos.get_unit_position(),150).to_polygon();
		for (const game_object_script& enemy : entitylist->get_enemy_heroes()) {
			if (enemy && enemy->is_valid() &&enemy!=target && hitbox.is_inside(prediction->get_prediction(enemy, 0.6f).get_unit_position()) && enemy->is_targetable() && enemy->is_visible() && !enemy->is_dead()) {
				count++;
			}
		}
		return count;
	}

	void automatic() {
		// Auto W Shield
		if (wMenu::autoShield) {
			float t = wMenu::shieldTime->get_int() / 10.0f;
			float incomingdmg = health_prediction->get_incoming_damage(myhero, t, wMenu::includeSkillshots->get_bool());
			if (incomingdmg > calcShieldValue() * wMenu::autoShieldValue->get_int() / 100.f) w->cast();
		}

		// Q interrupt
		if (q->is_ready() && qMenu::interrupt->get_bool()) {
			for (const auto& target : entitylist->get_enemy_heroes()) {
				auto interruptData = InterruptDB::getInterruptable(target);
				if (target && target->is_valid() && target->is_visible() && !target->is_zombie() && target->is_valid_target(q->range()) && interruptData.dangerLevel>=2 && !target->get_is_cc_immune()) {
					auto timeToHit = q->get_prediction(target).get_unit_position().distance(myhero) / q->speed + q->delay;
					auto castpos = getQCastPos(target);
					if (castpos != vector() && interruptData.maxRemainingTime>=timeToHit) {
						q->cast(castpos);
						if (generalMenu::debug->get_bool()) myhero->print_chat(0, "Interrupt Q on %s", InterruptDB::getDisplayName(target).c_str());
					}
				}

			}
		}

		// R interrupt
		if (r->is_ready() && rMenu::interrupt->get_bool()) {
			for (const auto& target : entitylist->get_enemy_heroes()) {
				auto interruptData = InterruptDB::getInterruptable(target);
				if (target && target->is_valid() && target->is_visible() && !target->is_zombie() && target->is_valid_target(r->range()) && interruptData.dangerLevel >= 3 && !target->get_is_cc_immune()) {
					auto timeToHit = 0.5 + myhero->get_distance(target) / 400;	// this is not really accurate but it should work
					if (interruptData.maxRemainingTime >= timeToHit) {
						r->cast(target);
						if (generalMenu::debug->get_bool()) myhero->print_chat(0, "Interrupt R on %s", InterruptDB::getDisplayName(target).c_str());
					}
					
					
				}

			}
		}

		// Force Orb
		if (generalMenu::forceOrb->get_bool()) {
			auto defaulttarget = orbwalker->get_target();
			auto selectedtarget = target_selector->get_selected_target();
			if (defaulttarget && defaulttarget->is_ai_hero() && defaulttarget->has_buff(buff_hash("nautiluspassivecheck")) && defaulttarget!=selectedtarget) {
				for (const auto& target : entitylist->get_enemy_heroes()) {
					if (target && myhero->is_in_auto_attack_range(target) && !target->has_buff(buff_hash("nautiluspassivecheck"))) {
						orbwalker->set_orbwalking_target(target);
					}
				}
			}
		}

	}

	void combo() {
		// Either i dont need to wait for runes or i have them ready
		if (q->is_ready() && qMenu::comboQ->get_bool() && (generalMenu::waitRunes->get_int() != 0 || getRunesReady())) {
			auto target = target_selector->get_target(q, damage_type::magical);
			if (target && target->is_valid() && !target->get_is_cc_immune() && checkWhitelist(target)) {
				auto castpos = getQCastPos(target);
				if (castpos != vector()) 
					q->cast(castpos);
			}
		}
		if (e->is_ready() && eMenu::comboE->get_bool()) {
			for (const game_object_script& enemy : entitylist->get_enemy_heroes()) {
				if (enemy && enemy->is_valid() && enemy->get_distance(myhero) < eMenu::range->get_int() && enemy->is_targetable() && enemy->is_visible() && !enemy->is_dead()) {
					e->cast();
					break;
				}
			}
		}
		if (r->is_ready() && rMenu::comboR->get_bool()) {
			auto selectedTarget = target_selector->get_selected_target();
			if (selectedTarget && selectedTarget->is_valid() && selectedTarget->get_distance(myhero) < r->range() && rMenu::forceSelected->get_bool()) {
				r->cast(selectedTarget);
			}
			else {
				auto target = target_selector->get_target(r, damage_type::magical);
				int rCount = countRTargets(target);
				if (target && target->is_valid() && !target->get_is_cc_immune() && checkWhitelist(target, true) && rCount>=rMenu::minTargets->get_int()) {
					r->cast(target);
				}
			}
		}
	}
	void harass() {
		// Either i dont need to wait for runes or i have them ready
		if (q->is_ready() && qMenu::harassQ->get_bool() && (generalMenu::waitRunes->get_int()==2 || getRunesReady())) {
			auto target = target_selector->get_target(q, damage_type::magical);
			if (target && target->is_valid() && !target->get_is_cc_immune() && checkWhitelist(target)) {
				auto castpos = getQCastPos(target);
				if (castpos != vector())
					q->cast(castpos);
			}
		}
		if (e->is_ready() && eMenu::harassE->get_bool()) {
			for (const game_object_script& enemy : entitylist->get_enemy_heroes()) {
				if (enemy && enemy->is_valid() && enemy->get_distance(myhero) < eMenu::range->get_int() && enemy->is_targetable() && enemy->is_visible()) {
					e->cast();
					break;
				}
			}
		}
	}
	void on_update() {
		if (myhero->is_dead())
			return;
		//auto first for interrupt
		automatic();
		if (qMenu::bestWallHK->get_bool()) {
			auto bestWall = bestWallQPos();
			// i check min distance again? but should also work if i distance to me > 0? but is still distance check so no difference
			if (bestWall.distance(myhero) > qMenu::wallMinDistance->get_int()) q->cast(bestWall);
		}
		if (orbwalker->combo_mode()) combo();
		if (orbwalker->harass()) harass();
	}
	void on_draw()
	{
		if (myhero->is_dead())
		{
			return;
		}
		if ((q->is_ready() || !drawMenu::drawOnlyReady->get_bool()) && drawMenu::drawQWall->get_bool())
		{
			auto bestWall = bestWallQPos();
			if (bestWall.distance(myhero) > qMenu::wallMinDistance->get_int()) draw_manager->add_circle(bestWall, 20, MAKE_COLOR(0,255,255,255));
		}
		if (generalMenu::debug->get_bool()){
			auto mouse = hud->get_hud_input_logic()->get_game_cursor_position();
			auto mouse1 = (mouse - myhero->get_position()).normalized() * 1100 + myhero->get_position();
			draw_manager->add_text(myhero->get_position(), MAKE_COLOR(0, 0, 255, 255), 30, "%i", firstWallHit(mouse1,5) != vector());
			vector castVector = (mouse - myhero->get_position()).normalized() * 1100;
			for (int i = 0; i <= 100; i++) {
				vector p = castVector * i / 100.f + myhero->get_position();
				auto c = p.is_wall() ? MAKE_COLOR(255,0,0,255) : MAKE_COLOR(0,255,0,255);
				draw_manager->add_circle(p, 2.5f, c);
			}
			//draw_manager->add_circle(bestWallQPos(), 20, MAKE_COLOR(0, 255, 255, 255));

			for (const game_object_script& enemy : entitylist->get_enemy_heroes()) {
				auto rhits = countRTargets(enemy);
				if (enemy->get_position().distance(myhero)<rMenu::range->get_int())
					draw_manager->add_text(enemy->get_position() + vector(0, 50), MAKE_COLOR(0, 0, 255, 255), 30, "R: %i", rhits);

				auto pred = q->get_prediction(enemy);
				if (pred.hitchance < get_hitchance(qMenu::hitchance->get_int()) || !enemy->is_visible()) continue;
				auto castpos = pred.get_cast_position();
				bool canhit = true;
				for (int i = 0; i <= 1060; i += 5) {
					auto p = ((castpos - myhero->get_position()).normalized() * i) + myhero->get_position();
					if (p.distance(myhero) > pred.get_unit_position().distance(myhero)-enemy->get_bounding_radius()) break;
					auto c = p.is_wall() ? MAKE_COLOR(255, 0, 0, 255) : MAKE_COLOR(0, 255, 0, 255);
					draw_manager->add_circle(p, 2.5f, c);
					if (p.is_wall()) canhit = false;
				}
				auto c = !canhit ? MAKE_COLOR(255, 0, 0, 255) : MAKE_COLOR(0, 255, 0, 255);
				draw_manager->add_text(enemy->get_position(), c, 30, canhit ? "Can hit" : "Hit Wall");
			}
		}
	}
	void on_env_draw() {
		if (myhero->is_dead())
		{
			return;
		}
		if ((q->is_ready() || !drawMenu::drawOnlyReady->get_bool()) && drawMenu::drawRangeQ->get_bool())
			draw_manager->add_circle(myhero->get_position(), qMenu::range->get_int(), colorMenu::qColor->get_color());
		if ((e->is_ready() || !drawMenu::drawOnlyReady->get_bool()) && drawMenu::drawRangeE->get_bool())
			draw_manager->add_circle(myhero->get_position(), eMenu::range->get_int(), colorMenu::eColor->get_color());
		if ((r->is_ready() || !drawMenu::drawOnlyReady->get_bool()) && drawMenu::drawRangeR->get_bool())
			draw_manager->add_circle(myhero->get_position(), rMenu::range->get_int(), colorMenu::rColor->get_color());
	}

	void on_after_attack_orbwalker(game_object_script target) {
		if (orbwalker->combo_mode() && wMenu::comboW->get_bool() && w->is_ready()) {
			w->cast();
		}
	}

	void load() {
		q = plugin_sdk->register_spell(spellslot::q, 1100);
		w = plugin_sdk->register_spell(spellslot::w, 0);
		e = plugin_sdk->register_spell(spellslot::e, 590);
		r = plugin_sdk->register_spell(spellslot::r, 825);
		q->set_skillshot(0.25, 90, 2000, { collisionable_objects::minions, collisionable_objects::yasuo_wall}, skillshot_type::skillshot_line);

		
		mainMenuTab = menu->create_tab("Flofian_Nautilus", "Flofian Nautilus");
		mainMenuTab->set_assigned_texture(myhero->get_square_icon_portrait());

		//Menu
		{
			auto generalMenu = mainMenuTab->add_tab("General", "General Settings");
			{
				generalMenu::debug = generalMenu->add_checkbox("debug", "Debug", false);
				generalMenu::debug->is_hidden() = true;
				generalMenu::waitRunes = generalMenu->add_combobox("waitRunes", "Only Q with Aftershock/Glacial Augment ready", { {"Always", nullptr}, {"Only in Harass", nullptr}, {"Never", nullptr} }, 1);
				generalMenu::waitRunes->set_tooltip("Gets ignored if you dont have those runes");
				generalMenu::forceOrb = generalMenu->add_checkbox("forceOrb", "Try to force Orbwalker for passive root", true);
				generalMenu::forceOrb->set_tooltip("Gets Ignored if you have a target selected");
			}

			auto qMenu = mainMenuTab->add_tab("Q", "Q Settings");
			{
				qMenu->set_assigned_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
				qMenu::range = qMenu->add_slider("Range", "Q Range", 1000, 900, 1100);
				qMenu::range->add_property_change_callback([](TreeEntry* entry) {
					q->set_range(entry->get_int());
					});
				qMenu::hitchance = qMenu->add_combobox("Hitchance", "Hitchance", { {"Low", nullptr}, {"Medium", nullptr},{"High", nullptr},{"Very High", nullptr} }, 1);
				qMenu::whitelist = qMenu->add_tab("whitelist", "Use Q on");
				qMenu::comboQ = qMenu->add_checkbox("comboQ", "Use Q in Combo", true);
				qMenu::harassQ = qMenu->add_checkbox("harassQ", "Use Q in Harass", true);
				qMenu::interrupt = qMenu->add_checkbox("interrupt", "Use Q to interrupt Spells with Importance >= 2", true);
				qMenu::bestWallHK = qMenu->add_hotkey("bestWall", "Q to hook towards best Wall near Mouse", TreeHotkeyMode::Hold, 5, false);
				qMenu::bestWallHK->set_tooltip("Might have problems with Height Diffrence (River)");
				qMenu::wallMinDistance = qMenu->add_slider("wallMinDistance", "^ Min Distance to Wall", 500, 100, 1000);
			}

			auto wMenu = mainMenuTab->add_tab("W", "W Settings");
			{
				wMenu->set_assigned_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
				wMenu::autoShield = wMenu->add_checkbox("useshield", "Use Shield for Incoming DMG", true);
				wMenu::autoShieldValue = wMenu->add_slider("shieldvalue", "If DMG > Shield * X%", 100, 50, 150);
				wMenu::includeSkillshots = wMenu->add_checkbox("includeSkillshots", "Include Skillshots", true);
				wMenu::shieldTime = wMenu->add_slider("shieldtime", "Incoming DMG in the next X/10 Seconds", 60,20,60);
				wMenu::comboW = wMenu->add_checkbox("comboW", "Use in Combo for AA Reset", true);
			}

			auto eMenu = mainMenuTab->add_tab("E", "E Settings");
			{
				eMenu->set_assigned_texture(myhero->get_spell(spellslot::e)->get_icon_texture());
				eMenu::range = eMenu->add_slider("Range", "E Range", 400, 100, 590);
				eMenu::range->add_property_change_callback([](TreeEntry* entry) {
					e->set_range(entry->get_int());
					});
				eMenu::comboE = eMenu->add_checkbox("comboE", "Use E in Combo", true);
				eMenu::harassE = eMenu->add_checkbox("harassE", "Use E in Harass", true);
			}

			auto rMenu = mainMenuTab->add_tab("R", "R Settings");
			{
				rMenu->set_assigned_texture(myhero->get_spell(spellslot::r)->get_icon_texture());
				rMenu::range = rMenu->add_slider("Range", "R Range", 800, 700, 825);
				rMenu::range->add_property_change_callback([](TreeEntry* entry) {
					r->set_range(entry->get_int());
					});
				rMenu::whitelist = rMenu->add_tab("whitelist", "Use R on");
				rMenu::comboR = rMenu->add_checkbox("comboR", "Use R in Combo", true);
				rMenu::minTargets = rMenu->add_slider("minTargets", "Only if it hits X Targets", 2, 1, 5);
				rMenu::forceSelected = rMenu->add_checkbox("forceSelected", "^ Ignore if Target Selected", true);
				rMenu::forceSelected->set_tooltip("Ignores the Min Targets option if you have forced the target by clicking on them (red circle below them)");
				rMenu::interrupt = rMenu->add_checkbox("interrupt", "Use R to interrupt Spells with Importance >= 3", true);
			}

			auto drawMenu = mainMenuTab->add_tab("drawings", "Drawings Settings");
			{
				drawMenu::drawOnlyReady = drawMenu->add_checkbox("drawReady", "Draw Only Ready", true);
				drawMenu::drawRangeQ = drawMenu->add_checkbox("drawQ", "Draw Q range", true);
				drawMenu::drawRangeE = drawMenu->add_checkbox("drawE", "Draw E range", true);
				drawMenu::drawRangeR = drawMenu->add_checkbox("drawR", "Draw R range", true);
				drawMenu->add_separator("sep1", "");
				drawMenu::drawQWall = drawMenu->add_checkbox("drawQWall", "Draw Q Wall target", true);
				drawMenu->add_separator("sep2", "");

				auto colorMenu = drawMenu->add_tab("color", "Color Settings");

				float qcolor[] = { 0.f, 0.f, 1.f, 1.f };
				colorMenu::qColor = colorMenu->add_colorpick("colorQ", "Q Range Color", qcolor);
				float ecolor[] = { 0.f, 1.f, 0.f, 1.f };
				colorMenu::eColor = colorMenu->add_colorpick("colorE", "E Range Color", ecolor);
				float rcolor[] = { 1.f, 0.f, 0.f, 1.f };
				colorMenu::rColor = colorMenu->add_colorpick("colorR", "R Range Color", rcolor);
			}
			spelldb = mainMenuTab->add_tab("interruptdb", "Interrupt Database");
			InterruptDB::InitializeCancelMenu(spelldb);

			mainMenuTab->add_separator("version", "Version: " + VERSION);
			//init whitelists
			for (auto&& enemy : entitylist->get_enemy_heroes()) {
				auto networkid = enemy->get_network_id();
				q_whitelist[networkid] = qMenu::whitelist->add_checkbox(std::to_string(networkid), enemy->get_model(), true, false);
				q_whitelist[networkid]->set_texture(enemy->get_square_icon_portrait());
				r_whitelist[networkid] = rMenu::whitelist->add_checkbox(std::to_string(networkid), enemy->get_model(), true, false);
				r_whitelist[networkid]->set_texture(enemy->get_square_icon_portrait());
			}
			//init ranges from config file
			q->set_range(qMenu::range->get_int());
			e->set_range(eMenu::range->get_int());
			r->set_range(rMenu::range->get_int());
		}
		event_handler<events::on_draw>::add_callback(on_draw);
		event_handler<events::on_env_draw>::add_callback(on_env_draw);
		event_handler<events::on_update>::add_callback(on_update);
		event_handler<events::on_after_attack_orbwalker>::add_callback(on_after_attack_orbwalker);
	}
	void unload()
	{
		plugin_sdk->remove_spell(q);
		plugin_sdk->remove_spell(w);
		plugin_sdk->remove_spell(e);
		plugin_sdk->remove_spell(r);

		menu->delete_tab(mainMenuTab);

		event_handler<events::on_draw>::remove_handler(on_draw);
		event_handler<events::on_env_draw>::remove_handler(on_env_draw);
		event_handler<events::on_update>::remove_handler(on_update);
		event_handler<events::on_after_attack_orbwalker>::remove_handler(on_after_attack_orbwalker);
	}

}