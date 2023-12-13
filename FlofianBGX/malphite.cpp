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
	TreeEntry* debugkey = nullptr;

	std::unordered_map<uint32_t, prediction_output> rPredictionList;

	struct rPos {
		vector pos = vector();
		int hitcount = 0;
	};
	rPos bestRPos = rPos();
	rPos bestFRPos = rPos();

	namespace generalMenu {

	}
	namespace qMenu {
		TreeEntry* autoQHotkey = nullptr;
	}
	namespace wMenu {

	}
	namespace eMenu {

	}
	namespace rMenu {
		TreeEntry* range = nullptr;
		TreeEntry* radius = nullptr;
		TreeEntry* hc = nullptr;
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
		// I change range here so i dont get pred saying impossible, i want at least flash r range, 
		// and i think if target is 1100 away, pred says impossible, but casting at 1000 still hits
		int oldrange = r->range();
		r->set_range(2000);
		//prediction_input p = prediction_input{
		for (const auto& target : entitylist->get_enemy_heroes())
		{
			if (!target->is_valid() || target->is_dead()) continue;
			prediction_output p = r->get_prediction(target);
			rPredictionList[target->get_handle()] = p;
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
		r->set_speed(1500 + myhero->get_move_speed());
		updatePredictionList();
		updateBestRPos();
		if ((debugkey->get_bool() || useRNow) && r->is_ready()) {
			if (bestFRPos.pos != bestRPos.pos) {
				flash->cast(bestFRPos.pos);
				useRNow = true;
				//r->cast(bestFRPos.pos);
			}
			else {
				r->cast(bestFRPos.pos);
				useRNow = false;
			}
		}
		//console->print("Center: %f %f %f Hitcount: %i", bestRPos.pos.x, bestRPos.pos.y, bestRPos.pos.z, bestRPos.hitcount);
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
			debugkey = mainMenuTab->add_hotkey("debug", "debug", TreeHotkeyMode::Hold, 0x53, false);
			auto qMenu = mainMenuTab->add_tab("Q", "Q Settings");
			{
				qMenu->set_assigned_texture(qTexture);
				qMenu::autoQHotkey = qMenu->add_hotkey("autoQHotkey", "Auto Q Toggle", TreeHotkeyMode::Hold, 0x05, false);

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
			}

			auto drawMenu = mainMenuTab->add_tab("drawings", "Draw Settings");
			{
				drawMenu::drawOnlyReady = drawMenu->add_checkbox("drawReady", "Draw Only Ready", true);
				drawMenu::drawRangeQ = drawMenu->add_checkbox("drawQ", "Q Range", true);
				drawMenu::drawRangeE = drawMenu->add_checkbox("drawE", "E Range", true);
				drawMenu::drawRangeR = drawMenu->add_checkbox("drawR", "R Range", true);
				drawMenu::drawRangeFR = drawMenu->add_checkbox("drawFR", "Flash R Range", true);
				drawMenu::drawCircleR = drawMenu->add_checkbox("drawRground", "R Target [Ground]", true);
				drawMenu::drawCircleFR = drawMenu->add_checkbox("drawFRground", "Flash R Target [Ground]", true);

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
		permashow::instance.init(mainMenuTab);
		permashow::instance.add_element("Auto Q", qMenu::autoQHotkey);

		event_handler<events::on_env_draw>::add_callback(on_env_draw);
		event_handler<events::on_draw>::add_callback(on_draw);

		event_handler<events::on_update>::add_callback(on_update);
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
	}
}