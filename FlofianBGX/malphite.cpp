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
		TreeEntry* preventCancel = nullptr;
	}
	namespace qMenu {
		TreeEntry* autoQHotkey = nullptr;
		TreeEntry* combo = nullptr;
		TreeEntry* harass = nullptr;
	}
	namespace wMenu {
		TreeEntry* combo = nullptr;
		TreeEntry* harass = nullptr;
	}
	namespace eMenu {
		TreeEntry* range = nullptr;
		TreeEntry* combo = nullptr;
		TreeEntry* harass = nullptr;
		TreeEntry* autoMinTargets = nullptr;
	}
	namespace rMenu {
		TreeEntry* range = nullptr;
		TreeEntry* radius = nullptr;
		TreeEntry* hc = nullptr;
		TreeEntry* comboMin = nullptr;
		TreeEntry* flashComboMin = nullptr;
		TreeEntry* autoMin = nullptr;
		TreeEntry* flashAutoMin = nullptr;

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
		//prediction_input p = prediction_input{
		for (const auto& target : entitylist->get_enemy_heroes())
		{
			if (!target->is_valid() || target->is_dead()) continue;
			rPredictionList[target->get_handle()] = r->get_prediction(target);
			// TODO: Check if thats the true time
			ePredictionList[target->get_handle()] = prediction->get_prediction(target, 0.2419f);
		}
		r->set_range(oldrange);

		
	}

	vector canHitAll(std::vector<game_object_script> enemies) {
		std::vector<vector> positions = {};
		for (const auto& target : enemies) {
			if (rPredictionList[target->get_handle()].hitchance < get_hitchance(rMenu::hc->get_int())) return vector();
			positions.push_back(rPredictionList[target->get_handle()].get_unit_position());
		}
		mec_circle circle = mec::get_mec(positions);
		if (circle.radius < rMenu::radius->get_int()) return circle.center;
		
		return vector();
	}

	void updateBestRPos() {
		//console->print("Updating R");
		std::vector<game_object_script> enemiesInRange = {};
		std::vector<game_object_script> enemiesInFlashRRange = {};
		for (const auto& target : entitylist->get_enemy_heroes()) {
			// TODO: check spellshield? 
			if (target && target->is_valid() && target->get_distance(myhero) < rMenu::radius->get_int() + rMenu::range->get_int() + flash->range()
				&& !target->is_dead() && target->is_visible()) {

				enemiesInFlashRRange.push_back(target);
				if (target->get_distance(myhero) < rMenu::radius->get_int()+ rMenu::range->get_int()) enemiesInRange.push_back(target);
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

	void automatic() {
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
		// E
		if (e->is_ready()) {
			int minTargets = eMenu::autoMinTargets->get_int();
			if (minTargets == 0) return;
			int hits = 0;
			for (const auto& target : entitylist->get_enemy_heroes()) {
				auto pred = ePredictionList[target->get_handle()];
				if (pred.get_unit_position().distance(myhero) < e->range() && pred.hitchance > hit_chance::impossible) hits++;
			}
			if (hits >= minTargets) e->cast();
		}
	}
	void combo() {
		// R
		if (r->is_ready()){
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
				if (pred.get_unit_position().distance(myhero) < e->range() && pred.hitchance>hit_chance::impossible) e->cast();
			}
		}
	}
	void harass() {
		// Anti Auto Cancel and prevent accidental buffering
		if ((generalMenu::preventCancel->get_bool() && myhero->is_winding_up()) || !myhero->can_cast()) return;
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
		if (drawMenu::drawCircleR->get_bool() && bestRPos.pos != vector() && bestRPos.pos.distance(myhero)<r->range()) 
		{
			draw_manager->add_circle(bestRPos.pos, rMenu::radius->get_int(), colorMenu::rCircle->get_color(), 3);
			draw_manager->add_circle(bestRPos.pos, 5, colorMenu::rCircle->get_color(), 3);
			draw_manager->add_text(bestRPos.pos, colorMenu::rCircle->get_color(), 50, "%i", bestRPos.hitcount);
		}
		if (drawMenu::drawCircleFR->get_bool() && bestFRPos.pos != vector() && bestFRPos.pos.distance(myhero) < r->range()+400 && bestFRPos.hitcount>bestRPos.hitcount)
		{
			draw_manager->add_circle(bestFRPos.pos, rMenu::radius->get_int(), colorMenu::frCircle->get_color(), 3);
			draw_manager->add_circle(bestFRPos.pos, 5, colorMenu::frCircle->get_color(), 3);
			draw_manager->add_text(bestFRPos.pos, colorMenu::frCircle->get_color(), 50, "%i", bestFRPos.hitcount);
		}
	}
	void on_update() {
		permashow::instance.update();
		updatePredictionList();
		updateBestRPos();
		/*if ((debugkey->get_bool() || useRNow) && r->is_ready()) {
			if (bestFRPos.pos != bestRPos.pos) {
				flash->cast(bestFRPos.pos);
				useRNow = true;
				//r->cast(bestFRPos.pos);
			}
			else {
				r->cast(bestFRPos.pos);
				useRNow = false;
			}
		}*/
		//console->print("Center: %f %f %f Hitcount: %i", bestRPos.pos.x, bestRPos.pos.y, bestRPos.pos.z, bestRPos.hitcount);

		if (orbwalker->combo_mode()) combo();
		if (orbwalker->harass()) harass();
		automatic();
	}
	void on_after_attack_orbwalker(game_object_script target) {
		if (w->is_ready() && ((orbwalker->combo_mode() && wMenu::combo->get_bool()) || (orbwalker->harass() && wMenu::harass->get_bool() && target->is_ai_hero()))) {
			w->cast();
			orbwalker->reset_auto_attack_timer();
			// TODO: Does that make sense? Feels like it from testing, else i just walk away
			// Maybe instead issue order for auto attack?
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
			mainMenuTab = menu->create_tab("FlofianMalphite", "Flofian Malphite");
			mainMenuTab->set_assigned_texture(myhero->get_square_icon_portrait());
			auto generalMenu = mainMenuTab->add_tab("general", "General Settings");
			{
				generalMenu::preventCancel = generalMenu->add_checkbox("preventCancel", "Try to Prevent Auto Cancels", true);
			}
			auto qMenu = mainMenuTab->add_tab("Q", "Q Settings");
			{
				qMenu->set_assigned_texture(qTexture);
				qMenu::autoQHotkey = qMenu->add_hotkey("autoQHotkey", "Auto Q Toggle", TreeHotkeyMode::Hold, 0x05, false);
				qMenu::combo = qMenu->add_checkbox("combo", "Use Q in Combo", true);
				qMenu::harass = qMenu->add_checkbox("harass", "Use Q in Harass", true);

			}
			auto wMenu = mainMenuTab->add_tab("W", "W Settings");
			{
				wMenu->set_assigned_texture(wTexture);
				wMenu::combo = wMenu->add_checkbox("combo", "Use W in Combo", true);
				wMenu::harass = wMenu->add_checkbox("harass", "Use W in Harass", true);
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
			}
			auto rMenu = mainMenuTab->add_tab("R", "R Settings");
			{
				rMenu->set_assigned_texture(rTexture);
				rMenu::range = rMenu->add_slider("Range", "Range", 950, 900, 1000);
				rMenu::range->add_property_change_callback([](TreeEntry* entry) {
					r->set_range(entry->get_int());
					});
				rMenu::radius = rMenu->add_slider("radius", "Radius", 325, 250, 325);
				rMenu::radius->add_property_change_callback([](TreeEntry* entry) {
					r->set_radius(entry->get_int());
					});
				rMenu::hc = rMenu->add_combobox("Hitchance", "Hitchance", { {"Low", nullptr}, {"Medium", nullptr},{"High", nullptr},{"Very High", nullptr} }, 2);
				rMenu->add_separator("sep1", "");
				rMenu::comboMin = rMenu->add_slider("combomin", "Min Targets in Combo", 2, 0, 5);
				rMenu::flashComboMin = rMenu->add_slider("flashComboMin", "Min  Targets in Combo to Flash R", 3, 0, 5);
				rMenu::autoMin = rMenu->add_slider("autoMin", "Min Targets to Auto R", 3, 0, 5);
				rMenu::flashAutoMin = rMenu->add_slider("flashAutoMin", "Min Targets to Auto Flash R", 4, 0, 5);
				rMenu->add_separator("sep2", "^ 0 to disable ^");

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
			r->set_radius(rMenu::radius->get_int());
		}

		permashow::instance.init(mainMenuTab);
		permashow::instance.add_element("Auto Q", qMenu::autoQHotkey);

		event_handler<events::on_env_draw>::add_callback(on_env_draw);
		event_handler<events::on_draw>::add_callback(on_draw);
		event_handler<events::on_update>::add_callback(on_update);
		event_handler<events::on_after_attack_orbwalker>::add_callback(on_after_attack_orbwalker);
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
	}
}