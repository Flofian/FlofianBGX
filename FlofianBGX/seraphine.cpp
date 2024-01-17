#include "../plugin_sdk/plugin_sdk.hpp"
#include "seraphine.h"

namespace seraphine {
	std::string VERSION = "1.0.0";
	script_spell* q = nullptr;
	script_spell* qBloom = nullptr;
	//script_spell* q2Bloom = nullptr;
	script_spell* w = nullptr;
	script_spell* e = nullptr;
	script_spell* e2 = nullptr;
	script_spell* r = nullptr;
	TreeTab* mainMenuTab = nullptr;

	bool passiveReady = false;
	std::map<uint32_t, prediction_output> qBloomPredictionList;
	std::map<uint32_t, prediction_output> q2BloomPredictionList;
	std::map<uint32_t, prediction_output> ePredictionList;
	std::map<uint32_t, prediction_output> e2PredictionList;
	std::map<uint32_t, prediction_output> rPredictionList;

	namespace generalMenu {
		TreeEntry* debug = nullptr;
	}
	namespace qMenu {
		TreeEntry* mode = nullptr;
	}
	namespace wMenu {
		TreeEntry* useForShield = nullptr;
		TreeEntry* shieldValue = nullptr;
		TreeEntry* antimelee = nullptr;
	}
	namespace eMenu {
		TreeEntry* mode = nullptr;
	}
	namespace rMenu {
		TreeEntry* comboTargets = nullptr;
		TreeEntry* semiKey = nullptr;
		TreeEntry* semiTargets = nullptr;
		TreeEntry* ignoreSemiHitcount = nullptr;
	}
	namespace predMenu {
		TreeEntry* q = nullptr;
		TreeEntry* q2 = nullptr;
		TreeEntry* e = nullptr;
		TreeEntry* e2 = nullptr;
		TreeEntry* r = nullptr;
		TreeEntry* allowRExtend = nullptr;
	}
	namespace drawMenu {
		TreeEntry* drawOnlyReady = nullptr;
		TreeEntry* drawRangeQ = nullptr;
		TreeEntry* drawRangeW = nullptr;
		TreeEntry* drawRangeE = nullptr;
		TreeEntry* drawRangeR = nullptr;

		TreeEntry* lineThickness = nullptr;
		TreeEntry* glowInsideSize = nullptr;
		TreeEntry* glowInsidePow = nullptr;
		TreeEntry* glowOutsideSize = nullptr;
		TreeEntry* glowOutsidePow = nullptr;
		TreeEntry* useGrad = nullptr;
		TreeEntry* gradColor = nullptr;
	}
	namespace colorMenu
	{
		TreeEntry* qColor = nullptr;
		TreeEntry* wColor = nullptr;
		TreeEntry* eColor = nullptr;
		TreeEntry* rColor = nullptr;
	}
	hit_chance get_hitchance(const int hc)
	{
		return static_cast<hit_chance>(hc + 3);
	}
	const char* hitchance_to_string(hit_chance hc) {
		switch (hc)
		{
		case hit_chance::immobile:
			return "8 - immobile";
		case hit_chance::dashing:
			return "7 - dashing";
		case hit_chance::very_high:
			return "6 - very high";
		case hit_chance::high:
			return "5 - high";
		case hit_chance::medium:
			return "4 - medium";
		case hit_chance::low:
			return "3 - low";
		case hit_chance::impossible:
			return "2 - not possible";
		case hit_chance::out_of_range:
			return "1 - out_of_range";
		case hit_chance::collision:
			return "0 - collision";
		default:
			return "?";
		}
	}

	int countRHits(const vector targetpos) {
		int counter = 0;
		vector endpos = myhero->get_position().extend(targetpos, r->range());
		for (const game_object_script& enemy : entitylist->get_enemy_heroes()) {
			if (enemy->is_dead() || enemy->is_zombie() || !enemy->is_visible() || enemy->get_is_cc_immune() || !enemy->is_targetable()) continue;
			auto rect = geometry::rectangle(myhero->get_position(), endpos, 160.f + enemy->get_bounding_radius()).to_polygon();
			auto pred = rPredictionList[enemy->get_handle()];
			if (rect.is_inside(pred.get_unit_position()) && pred.hitchance >= get_hitchance(predMenu::r->get_int())) {
				counter++;
				if (predMenu::allowRExtend->get_bool()) {
					vector onSegment = pred.get_unit_position().project_on(myhero->get_position(), endpos).segment_point;
					endpos = myhero->get_position().extend(targetpos, r->range() + onSegment.distance(myhero));
				}
			}
		}
		return counter;
	}
	void qLogic() {
		if (!myhero->can_cast() || !q->is_ready()) return;
		bool modeCast = (qMenu::mode->get_int() == 0 && orbwalker->harass()) || (qMenu::mode->get_int() <= 1 && orbwalker->combo_mode());
		if (modeCast) {
			if (passiveReady) {
				auto target = target_selector->get_target(q, damage_type::magical);
				if (target) {
					auto& p = qBloomPredictionList[target->get_handle()];
					auto& p2 = q2BloomPredictionList[target->get_handle()];
					if (p.hitchance >= get_hitchance(predMenu::q->get_int()) && p2.hitchance >= get_hitchance(predMenu::q2->get_int())) {
						q->cast(p.get_cast_position());
						console->print("Casted Double Q on %s with HC1: %i and HC2: %i", target->get_model_cstr(), p.hitchance, p2.hitchance);
					}
				}
			}
			else {
				auto target = target_selector->get_target(q, damage_type::magical);
				if (target) {
					auto& p = qBloomPredictionList[target->get_handle()];
					if (p.hitchance >= get_hitchance(predMenu::q->get_int())) {
						q->cast(p.get_cast_position());
						console->print("Casted Single Q on %s with HC: %i", target->get_model_cstr(), p.hitchance);
					}
				}
			}
		}
	}
	void wLogic() {
		if (!myhero->can_cast() || !w->is_ready()) return;
		if (wMenu::useForShield->get_bool()) {

			float haveShield = myhero->get_all_shield();
			bool isHeal = haveShield > 0 || passiveReady;
			// i hate heal and shield power
			// the heal is based on the number of nearby allies AFTER the 2.5/3 second delay? so i would need to call prediction on them???? maybe even magnet xd?
			// but i think i will just take how many are nearby at first cast
			float missingHealthHeal = (0.025 + 0.005 * w->level()) * myhero->count_allies_in_range(w->range());

			// WIKI: The heal affects clones and counts them for increasing the missing health percentage.
			// WIKI: Surround Sound will affect Untargetable icon untargetable allies.

			float singleShield = 25 + 25 * w->level() + 0.2 * myhero->get_total_ability_power();
			float baseShield = (1 + passiveReady) * singleShield;
			// omg calculating the heal is giga cancer
			// count how much i could shield
			float totalShield = 0.f;
			float totalHeal = 0.f;
			for (const auto& ally : entitylist->get_ally_heroes()) {
				if (ally->get_distance(myhero) < w->range()) {
					// i could reduce how much is coming in during cast time but i doubt thats needed
					float incoming = health_prediction->get_incoming_damage(ally, 2.5f + 0.5f * passiveReady, true);
					totalShield += fmin(incoming, baseShield);	// this means i only count how much i actually shield
					if (isHeal) totalHeal += (ally->get_max_health() - ally->get_health()) * missingHealthHeal;
				}
			}
			// i check if passive ready not isHeal because if i get the free heal thats great, only double cast needs to be worth it
			if (totalShield + totalHeal >= (1 + passiveReady * 0.5) * singleShield * wMenu::shieldValue->get_int() / 10.f) {		// heal value somewhat arbitrary
				w->cast();
				console->print("SingleShield: %f TotalShield: %f TotalHeal: %f", singleShield, totalShield, totalHeal);
			}
		}
		int antimelee = wMenu::antimelee->get_int();
		if (antimelee == 0) {
			if (myhero->count_enemies_in_range(400)) {
				w->cast();
				console->print("Used W Anti Melee Self");
			}
		}
		else if (antimelee == 1) {
			for (const auto& ally : entitylist->get_ally_heroes()) {
				if (myhero->get_distance(ally) < w->range() && ally->count_enemies_in_range(400)) {
					w->cast();
					console->print("Used W Anti Melee Ally");
				}
			}
		}
	}
	void eLogic() {
		if (!myhero->can_cast() || !e->is_ready()) return;
		bool modeCast = (eMenu::mode->get_int() == 0 && orbwalker->harass()) || (eMenu::mode->get_int() <= 1 && orbwalker->combo_mode());
		if (modeCast) {
			if (passiveReady) {
				auto target = target_selector->get_target(e, damage_type::magical);
				if (target) {
					auto& p = ePredictionList[target->get_handle()];
					auto& p2 = e2PredictionList[target->get_handle()];
					if (p.hitchance >= get_hitchance(predMenu::e->get_int()) && p2.hitchance >= get_hitchance(predMenu::e2->get_int())) {
						e->cast(p.get_cast_position());
						console->print("Casted Double E on %s with HC1: %i and HC2: %i", target->get_model_cstr(), p.hitchance, p2.hitchance);
					}
				}
			}
			else {
				auto target = target_selector->get_target(e, damage_type::magical);
				if (target) {
					auto& p = ePredictionList[target->get_handle()];
					if (p.hitchance >= get_hitchance(predMenu::e->get_int())) {
						e->cast(p.get_cast_position());
						console->print("Casted Single E on %s with HC: %i", target->get_model_cstr(), p.hitchance);
					}
				}
			}
		}
	}
	void rLogic() {
		if (!myhero->can_cast() || !r->is_ready()) return;
		// i will still just do the same logic as my sona, maybe in the future find a better way to do that, maybe a mec?
		auto selectedTarget = target_selector->get_selected_target();
		if (selectedTarget && selectedTarget->is_valid() && selectedTarget->get_distance(myhero) < r->range() && rMenu::semiKey->get_bool()) {
			bool ignoreHitcount = rMenu::ignoreSemiHitcount->get_bool();
			int targets = countRHits(selectedTarget->get_position());
			if (selectedTarget->is_visible() && !selectedTarget->is_zombie() && !selectedTarget->is_dead() && selectedTarget->is_targetable() && !selectedTarget->get_is_cc_immune() && (targets >= rMenu::semiTargets->get_int() || (ignoreHitcount && targets >= 1))) {
				r->cast(selectedTarget);
				console->print("Semi R on %i Targets with selected %s", targets, selectedTarget->get_model_cstr());
			}
		}
		if (!selectedTarget) {
			int minTargets = 999;
			if (orbwalker->combo_mode() && rMenu::comboTargets->get_int() > 0) minTargets = rMenu::comboTargets->get_int();
			if (rMenu::semiKey->get_bool() && rMenu::semiTargets->get_int() < minTargets) minTargets = rMenu::semiTargets->get_int();
			// this above just means the mintargets is either semitargets or combotargets, depending on what is active, or if nothing active its 999 which i cant reach
			for (const auto& target : entitylist->get_enemy_heroes()) {
				int hitcount = countRHits(rPredictionList[target->get_handle()].get_cast_position());
				if (hitcount >= minTargets) {
					r->cast(rPredictionList[target->get_handle()].get_cast_position());
					console->print("Used R on %i Targets", hitcount);
				}
			}
		}
	}

	void calcs() {
		passiveReady = myhero->get_hp_bar_stacks() > 2.5;	// should be == 3, but its a float, and i can only have 1,2 or 3
		for (const auto& enemy : entitylist->get_enemy_heroes()) {
			// First i predict where to throw the Q so the bloom hits
			auto qbp = qBloom->get_prediction(enemy);
			// Then i check if i there is collision with windwall, if yes, i set bloom prediction hitchance to collision
			auto collisions = q->get_collision(myhero->get_position(), { qbp.get_cast_position() });
			if (collisions.size() > 0) {
				qbp.hitchance = hit_chance::collision;
			}
			qBloomPredictionList[enemy->get_handle()] = qbp;
			// and since i throw q2 at the same place as q1, i need to pretend like q2 has infinite speed, but a delay of q1CastTime+q2CastTime+qBloomTime+q1Traveltime = 0.25+0.25+0.4+?
			// (after this delay, q2 will be done blooming)
			// this is done the same way as midlanecarry ori, not sure if this is the best way for this
			/* Somehow this doesnt make sense
			auto q2bpi = prediction_input();
			q2bpi.unit = enemy;
			q2bpi.delay = 0.9 +qbp.get_cast_position().distance(myhero) / q->speed;
			q2bpi._from = qbp.get_cast_position();
			q2bpi._range_check_from = qbp.get_cast_position();
			q2bpi.range = q->radius;	// this is wrong, but if i have 500 range and a radius of 200, prediction says out of range for a target 600 away from me
			q2bpi.radius = q->radius;
			q2bpi.type = skillshot_type::skillshot_circle;
			q2bpi.speed = FLT_MAX;
			prediction_input backup = q2bpi;	// man why is this needed?
			auto q2bp = prediction->get_prediction(&q2bpi);
			q2bp.input = backup;
			*/
			// Wow this is shit 
			float delay = 0.9 + qbp.get_cast_position().distance(myhero) / q->speed;
			auto q2bp = prediction->get_prediction(enemy, delay);
			auto expectedPos = q2bp.get_unit_position();
			if (expectedPos.distance(qbp.get_cast_position()) > q->radius) {
				q2bp.hitchance = hit_chance::impossible;
			}
			// TODO: fix whatever this shit is

			// and i dont check collisions here, might add, but would only be needed if there is a windwall spell with 1 second cast time
			// but since i cant have a range check, i check if the first one is impossible, bc then i wouldnt want to cast anyways
			if (qbp.hitchance <= hit_chance::impossible) q2bp.hitchance = qbp.hitchance;
			q2BloomPredictionList[enemy->get_handle()] = q2bp;


			// AND NOW THE SAME SHIT FOR E
			auto epred = e->get_prediction(enemy);
			auto rec = geometry::rectangle(myhero->get_position(), myhero->get_position().extend(epred.get_cast_position(), e->range()), 70 + enemy->get_bounding_radius());
			auto e2pred = e2->get_prediction(enemy);
			// e2pred hitchance doesnt make sense? if it casts somewhere else
			if (rec.to_polygon().is_outside(e2pred.get_unit_position())) e2pred.hitchance = hit_chance::impossible;

			ePredictionList[enemy->get_handle()] = epred;
			e2PredictionList[enemy->get_handle()] = e2pred;

			rPredictionList[enemy->get_handle()] = r->get_prediction(enemy);	// doing this same way as my sona makes no sense since hitchance doesnt matter
		}
	}

	void on_update() {
		calcs();
		qLogic();
		wLogic();
		eLogic();
		rLogic();
	}
	void on_draw() {
		if (!generalMenu::debug->get_bool()) return;
		for (const auto& enemy : entitylist->get_enemy_heroes()) {
			if (!enemy || !enemy->is_hpbar_recently_rendered()) continue;
			vector4 bar, hp;
			enemy->get_health_bar_position(bar, hp);
			if (!vector(bar.x, bar.y).is_on_screen())continue;
			auto& qbp = qBloomPredictionList[enemy->get_handle()];
			auto& q2bp = q2BloomPredictionList[enemy->get_handle()];
			auto& ep = ePredictionList[enemy->get_handle()];
			auto& e2p = e2PredictionList[enemy->get_handle()];
			draw_manager->add_text_on_screen(vector(bar.x, bar.y - 90), MAKE_COLOR(255, 255, 255, 255), 15, "Q Bloom HC:   %s", hitchance_to_string(qbp.hitchance));
			draw_manager->add_text_on_screen(vector(bar.x, bar.y - 70), MAKE_COLOR(255, 255, 255, 255), 15, "Q2 Bloom HC: %s", hitchance_to_string(q2bp.hitchance));
			draw_manager->add_text_on_screen(vector(bar.x, bar.y - 50), MAKE_COLOR(255, 255, 255, 255), 15, "E HC:   %s", hitchance_to_string(ep.hitchance));
			draw_manager->add_text_on_screen(vector(bar.x, bar.y - 30), MAKE_COLOR(255, 255, 255, 255), 15, "E2 HC: %s", hitchance_to_string(e2p.hitchance));
			/*
			draw_manager->add_filled_circle(qbp.get_cast_position(), 15, MAKE_COLOR(0, 255, 0, 255));
			draw_manager->add_filled_circle(q2bp.get_cast_position(), 10, MAKE_COLOR(255, 0, 0, 255));
			*/
		}
	}
	void on_env_draw() {
		if (myhero->is_dead()) return;

		float lt = drawMenu::lineThickness->get_int() / 10.f;
		glow_data glow = glow_data(drawMenu::glowInsideSize->get_int() / 100.f, drawMenu::glowInsidePow->get_int() / 100.f, drawMenu::glowOutsideSize->get_int() / 100.f, drawMenu::glowOutsidePow->get_int() / 100.f);
		if (drawMenu::useGrad->get_bool())
		{
			if ((q->is_ready() || !drawMenu::drawOnlyReady->get_bool()) && drawMenu::drawRangeQ->get_bool())
				draw_manager->add_circle_with_glow_gradient(myhero->get_position(), colorMenu::qColor->get_color(), drawMenu::gradColor->get_color(), q->range(), lt, glow);
			if ((w->is_ready() || !drawMenu::drawOnlyReady->get_bool()) && drawMenu::drawRangeW->get_bool())
				draw_manager->add_circle_with_glow_gradient(myhero->get_position(), colorMenu::wColor->get_color(), drawMenu::gradColor->get_color(), w->range(), lt, glow);
			if ((e->is_ready() || !drawMenu::drawOnlyReady->get_bool()) && drawMenu::drawRangeE->get_bool())
				draw_manager->add_circle_with_glow_gradient(myhero->get_position(), colorMenu::eColor->get_color(), drawMenu::gradColor->get_color(), e->range(), lt, glow);
			if ((r->is_ready() || !drawMenu::drawOnlyReady->get_bool()) && drawMenu::drawRangeR->get_bool())
				draw_manager->add_circle_with_glow_gradient(myhero->get_position(), colorMenu::rColor->get_color(), drawMenu::gradColor->get_color(), r->range(), lt, glow);
		}
		else {
			if ((q->is_ready() || !drawMenu::drawOnlyReady->get_bool()) && drawMenu::drawRangeQ->get_bool())
				draw_manager->add_circle_with_glow(myhero->get_position(), colorMenu::qColor->get_color(), q->range(), lt, glow);
			if ((w->is_ready() || !drawMenu::drawOnlyReady->get_bool()) && drawMenu::drawRangeW->get_bool())
				draw_manager->add_circle_with_glow(myhero->get_position(), colorMenu::wColor->get_color(), w->range(), lt, glow);
			if ((e->is_ready() || !drawMenu::drawOnlyReady->get_bool()) && drawMenu::drawRangeE->get_bool())
				draw_manager->add_circle_with_glow(myhero->get_position(), colorMenu::eColor->get_color(), e->range(), lt, glow);
			if ((r->is_ready() || !drawMenu::drawOnlyReady->get_bool()) && drawMenu::drawRangeR->get_bool())
				draw_manager->add_circle_with_glow(myhero->get_position(), colorMenu::rColor->get_color(), r->range(), lt, glow);
		}
	}

	void load() {
		q = plugin_sdk->register_spell(spellslot::q, 900);
		qBloom = plugin_sdk->register_spell(spellslot::q, 900);
		w = plugin_sdk->register_spell(spellslot::w, 800);
		e = plugin_sdk->register_spell(spellslot::e, 1300);
		e2 = plugin_sdk->register_spell(spellslot::e, 1300);
		r = plugin_sdk->register_spell(spellslot::r, 1200);
		q->set_skillshot(0.25, 350, 1200, { collisionable_objects::yasuo_wall }, skillshot_type::skillshot_circle);
		qBloom->set_skillshot(0.25 + 0.4, 350, 1200, {}, skillshot_type::skillshot_circle);	// time it takes to bloom (i know this makes collision and stuff harder, check calcs function)
		e->set_skillshot(0.25, 70, 1200, { collisionable_objects::yasuo_wall }, skillshot_type::skillshot_line);
		e2->set_skillshot(0.5, 70, 1200, { collisionable_objects::yasuo_wall }, skillshot_type::skillshot_line);
		r->set_skillshot(0.5, 160, 1600, { collisionable_objects::yasuo_wall }, skillshot_type::skillshot_line);

		mainMenuTab = menu->create_tab("FlofianSeraphine", "Flofian Seraphine");
		mainMenuTab->set_assigned_texture(myhero->get_square_icon_portrait());

		{
			auto generalMenu = mainMenuTab->add_tab("general", "General Settings");
			{
				generalMenu::debug = generalMenu->add_checkbox("debug", "Debug", false);
			}
			auto qMenu = mainMenuTab->add_tab("q", "Q Settings");
			{
				qMenu->set_assigned_texture(myhero->get_spell(spellslot::q)->get_icon_texture_by_index(0));
				qMenu::mode = qMenu->add_combobox("mode", "Q Mode", { {"Combo + Harass", nullptr},{"Combo", nullptr}, {"Off", nullptr} }, 0);
			}
			auto wMenu = mainMenuTab->add_tab("w", "W Settings");
			{
				wMenu->set_assigned_texture(myhero->get_spell(spellslot::w)->get_icon_texture_by_index(0));
				wMenu::useForShield = wMenu->add_checkbox("useforshield", "Use for Shield/Heal", true);
				wMenu::shieldValue = wMenu->add_slider("shieldvalue", "  Min Shield Usefulness", 10, 0, 50);
				wMenu::antimelee = wMenu->add_combobox("antigapclose", "Use when enemies too close", { {"Self + Allies", nullptr},{"Self", nullptr}, {"Off", nullptr} }, 0);

			}
			auto eMenu = mainMenuTab->add_tab("e", "E Settings");
			{
				eMenu->set_assigned_texture(myhero->get_spell(spellslot::e)->get_icon_texture_by_index(0));
				eMenu::mode = eMenu->add_combobox("mode", "E Mode", { {"Combo + Harass", nullptr},{"Combo", nullptr}, {"Off", nullptr} }, 0);
			}
			auto rMenu = mainMenuTab->add_tab("r", "R Settings");
			{
				rMenu->set_assigned_texture(myhero->get_spell(spellslot::r)->get_icon_texture_by_index(0));
				rMenu::comboTargets = rMenu->add_slider("ComboTargets", "Min Targets in Combo (0 to disable)", 3, 0, 5);
				rMenu::semiKey = rMenu->add_hotkey("SemiKey", "Semi Key", TreeHotkeyMode::Hold, 0x54, false);
				rMenu::semiTargets = rMenu->add_slider("SemiTargets", "Min Targets for Semi Key", 2, 1, 5);
				rMenu::ignoreSemiHitcount = rMenu->add_checkbox("ignoreSemiHits", "Ignore Hitcount for Semi R if target selected", true);
				rMenu::ignoreSemiHitcount->set_tooltip("If you click on someone to force that target (red circle under them), ignore how many it can hit");
			}
			auto predMenu = mainMenuTab->add_tab("pred", "Prediction Settings");
			{
				predMenu::q = predMenu->add_combobox("q1Hitchance", "Q1 Hitchance", { {"Low", nullptr},{"Medium", nullptr},{"High", nullptr},{"Very High", nullptr} }, 2);
				predMenu::q2 = predMenu->add_combobox("q2Hitchance", "Q2 Hitchance", { {"Low", nullptr}, {"Medium", nullptr},{"High", nullptr},{"Very High", nullptr} }, 1);
				predMenu::e = predMenu->add_combobox("e1Hitchance", "E1 Hitchance", { {"Low", nullptr},{"Medium", nullptr},{"High", nullptr},{"Very High", nullptr} }, 2);
				predMenu::e2 = predMenu->add_combobox("e2Hitchance", "E2 Hitchance", { {"Low", nullptr}, {"Medium", nullptr},{"High", nullptr},{"Very High", nullptr} }, 1);
				predMenu::r = predMenu->add_combobox("rHitchance", "R Hitchance", { {"Low", nullptr},{"Medium", nullptr},{"High", nullptr},{"Very High", nullptr} }, 3);
				predMenu::allowRExtend = predMenu->add_checkbox("allowRExtend", "Include R Extending", true);
			}
			auto drawMenu = mainMenuTab->add_tab("drawings", "Drawings Settings");
			{
				drawMenu::drawOnlyReady = drawMenu->add_checkbox("drawReady", "Draw Only Ready", true);
				drawMenu::drawRangeQ = drawMenu->add_checkbox("drawQ", "Draw Q range", true);
				drawMenu::drawRangeW = drawMenu->add_checkbox("drawW", "Draw W range", true);
				drawMenu::drawRangeE = drawMenu->add_checkbox("drawE", "Draw E range", true);
				drawMenu::drawRangeR = drawMenu->add_checkbox("drawR", "Draw R range", true);


				drawMenu::drawRangeQ->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture_by_index(0));
				drawMenu::drawRangeW->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture_by_index(0));
				drawMenu::drawRangeE->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture_by_index(0));
				drawMenu::drawRangeR->set_texture(myhero->get_spell(spellslot::r)->get_icon_texture());

				drawMenu->add_separator("sep2", "");
				drawMenu::lineThickness = drawMenu->add_slider("lineThickness", "Line Thickness", 10, 10, 50);
				drawMenu::glowInsideSize = drawMenu->add_slider("glowInsideSize", "Glow Inside Size", 5, 0, 100);
				drawMenu::glowInsidePow = drawMenu->add_slider("glowInsidePow", "Glow Inside Power", 90, 0, 100);
				drawMenu::glowOutsideSize = drawMenu->add_slider("glowOutsideSize", "Glow Outside Size", 10, 0, 100);
				drawMenu::glowOutsidePow = drawMenu->add_slider("glowOutsidePow", "Glow Outside Power", 90, 0, 100);
				drawMenu::useGrad = drawMenu->add_checkbox("useGrad", "Use Gradient", false);
				float gradcolor[] = { 0.f, 1.f, 0.f, 1.f };
				drawMenu::gradColor = drawMenu->add_colorpick("gradColor", "Gradient Color", gradcolor);
				drawMenu::useGrad->add_property_change_callback([](TreeEntry* entry) {
					drawMenu::gradColor->is_hidden() = !entry->get_bool();
					});
				drawMenu::gradColor->is_hidden() = !drawMenu::useGrad->get_bool();

				drawMenu->add_separator("sep2", "");
				auto colorMenu = drawMenu->add_tab("color", "Color Settings");

				float qcolor[] = { 1.f, 0.5f, 0.f, 1.f };
				colorMenu::qColor = colorMenu->add_colorpick("colorQ", "Q Range Color", qcolor);
				float wcolor[] = { 0.f, 1.f, 0.f, 1.f };
				colorMenu::wColor = colorMenu->add_colorpick("colorW", "W Range Color", wcolor);
				float ecolor[] =  { 1.f, 1.f, 0.f, 1.f };
				colorMenu::eColor = colorMenu->add_colorpick("colorE", "E Range Color", ecolor);
				float rcolor[] = { 0.f, 0.f, 1.f, 1.f };
				colorMenu::rColor = colorMenu->add_colorpick("colorR", "R Range Color", rcolor);
			}
		}

		event_handler<events::on_update>::add_callback(on_update);
		event_handler<events::on_draw>::add_callback(on_draw);
		event_handler<events::on_env_draw>::add_callback(on_env_draw);
		mainMenuTab->add_separator("version", "Version: " + VERSION);
	}

	void unload() {
		plugin_sdk->remove_spell(q);
		plugin_sdk->remove_spell(qBloom);
		plugin_sdk->remove_spell(w);
		plugin_sdk->remove_spell(e);
		plugin_sdk->remove_spell(e2);
		plugin_sdk->remove_spell(r);
		menu->delete_tab(mainMenuTab);

		event_handler<events::on_update>::remove_handler(on_update);
		event_handler<events::on_draw>::remove_handler(on_draw);
		event_handler<events::on_env_draw>::remove_handler(on_env_draw);
	}
}