#include "../plugin_sdk/plugin_sdk.hpp"
#include "nautilus.h"
#include "../spelldb/SpellDB.h"

namespace nautilus {
	std::string VERSION = "1.0.0";
	script_spell* q = nullptr;
	script_spell* w = nullptr;
	script_spell* e = nullptr;
	script_spell* r = nullptr;

	TreeTab* mainMenuTab = nullptr;

	std::map<std::uint32_t, TreeEntry*> q_whitelist;
	std::map<std::uint32_t, TreeEntry*> r_whitelist;

	namespace generalMenu {
		TreeEntry* debug = nullptr;
	}
	namespace qMenu {
		TreeEntry* range = nullptr;
		TreeEntry* hitchance = nullptr;
		TreeTab* whitelist = nullptr;
	}
	namespace wMenu {
		TreeEntry* autoShield = nullptr;
		TreeEntry* autoShieldValue = nullptr;
		TreeEntry* includeSkillshots = nullptr;
		TreeEntry* debugShieldTime = nullptr;
	}
	namespace eMenu {
		TreeEntry* range = nullptr;
	}
	namespace rMenu {
		TreeEntry* range = nullptr;
		TreeTab* whitelist = nullptr;
	}
	namespace drawMenu
	{
		TreeEntry* drawOnlyReady = nullptr;
		TreeEntry* drawRangeQ = nullptr;
		TreeEntry* drawRangeE = nullptr;
		TreeEntry* drawRangeR = nullptr;
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
		int count = 1;
		auto predPos = prediction->get_prediction(target, 0.6f);
		auto hitbox = geometry::rectangle(myhero->get_position(), predPos.get_unit_position(),150).to_polygon();
		for (const game_object_script& enemy : entitylist->get_enemy_heroes()) {
			if (enemy && enemy->is_valid() &&enemy!=target && hitbox.is_inside(prediction->get_prediction(enemy, 0.6f).get_unit_position()) && enemy->is_targetable() && enemy->is_visible()) {
				count++;
			}
		}
		return count;
	}

	void automatic() {
		// Auto W Shield
		if (wMenu::autoShield) {
			float t = wMenu::debugShieldTime->get_int() / 10.0f;
			float incomingdmg = health_prediction->get_incoming_damage(myhero, t, wMenu::includeSkillshots->get_bool());
			if (incomingdmg > calcShieldValue() * wMenu::autoShieldValue->get_int() / 100.f) w->cast();
		}

	}
	void combo() {
		if (q->is_ready()) {
			auto target = target_selector->get_target(q, damage_type::magical);
			if (target && target->is_valid() && !target->get_is_cc_immune() && checkWhitelist(target)) {
				auto castpos = getQCastPos(target);
				if (castpos != vector()) 
					q->cast(castpos);
			}
		}
		if (e->is_ready()) {
			for (const game_object_script& enemy : entitylist->get_enemy_heroes()) {
				if (enemy && enemy->is_valid() && enemy->get_distance(myhero) < eMenu::range->get_int() && enemy->is_targetable() && enemy->is_visible()) {
					e->cast();
					break;
				}
			}
		}
		if (r->is_ready()) {
			auto selectedTarget = target_selector->get_selected_target();
			if (selectedTarget && selectedTarget->is_valid() && selectedTarget->get_distance(myhero) < r->range()) {
				r->cast(selectedTarget);
			}
			else {
				auto target = target_selector->get_target(r, damage_type::magical);
				if (target && target->is_valid() && !target->get_is_cc_immune() && checkWhitelist(target, true)) {
					r->cast(target);
				}
			}
		}
	}
	void harass() {

	}
	void on_update() {
		if (myhero->is_dead())
			return;
		if (orbwalker->combo_mode()) combo();
		if (orbwalker->harass()) harass();
		automatic();
	}
	void on_draw()
	{
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
		if (generalMenu::debug->get_bool()){
			auto mouse = hud->get_hud_input_logic()->get_game_cursor_position();
			for (int i = 0; i <= 1060; i+=5) {
				auto p = ((mouse - myhero->get_position()).normalized() * i)+myhero->get_position();
				auto c = p.is_wall() ? MAKE_COLOR(255, 0, 0, 255) : MAKE_COLOR(0, 255, 0, 255);
				draw_manager->add_circle(p, 2.5f, c);
			}
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
			}

			auto wMenu = mainMenuTab->add_tab("W", "W Settings");
			{
				wMenu->set_assigned_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
				wMenu::autoShield = wMenu->add_checkbox("useshield", "Use Shield for Incoming DMG", true);
				wMenu::autoShieldValue = wMenu->add_slider("shieldvalue", "If DMG > Shield * X%", 100, 50, 150);
				wMenu::includeSkillshots = wMenu->add_checkbox("includeSkillshots", "Include Skillshots", true);
				wMenu::debugShieldTime = wMenu->add_slider("shieldtime", "Incoming DMG in the next X/10 Seconds", 60,20,60);
			}

			auto eMenu = mainMenuTab->add_tab("E", "E Settings");
			{
				eMenu->set_assigned_texture(myhero->get_spell(spellslot::e)->get_icon_texture());
				eMenu::range = eMenu->add_slider("Range", "E Range", 400, 100, 590);
				eMenu::range->add_property_change_callback([](TreeEntry* entry) {
					e->set_range(entry->get_int());
					});
			}

			auto rMenu = mainMenuTab->add_tab("R", "R Settings");
			{
				rMenu->set_assigned_texture(myhero->get_spell(spellslot::r)->get_icon_texture());
				rMenu::range = rMenu->add_slider("Range", "R Range", 800, 700, 825);
				rMenu::range->add_property_change_callback([](TreeEntry* entry) {
					e->set_range(entry->get_int());
					});

				rMenu::whitelist = rMenu->add_tab("whitelist", "Use R on");
			}

			auto drawMenu = mainMenuTab->add_tab("drawings", "Drawings Settings");
			{
				drawMenu::drawOnlyReady = drawMenu->add_checkbox("drawReady", "Draw Only Ready", true);
				drawMenu::drawRangeQ = drawMenu->add_checkbox("drawQ", "Draw Q range", true);
				drawMenu::drawRangeE = drawMenu->add_checkbox("drawE", "Draw E range", true);
				drawMenu::drawRangeR = drawMenu->add_checkbox("drawR", "Draw R range", true);

				auto colorMenu = drawMenu->add_tab("color", "Color Settings");

				float qcolor[] = { 0.f, 0.f, 1.f, 1.f };
				colorMenu::qColor = colorMenu->add_colorpick("colorQ", "Q Range Color", qcolor);
				float ecolor[] = { 0.f, 1.f, 0.f, 1.f };
				colorMenu::eColor = colorMenu->add_colorpick("colorE", "E Range Color", ecolor);
				float rcolor[] = { 1.f, 0.f, 0.f, 1.f };
				colorMenu::rColor = colorMenu->add_colorpick("colorR", "R Range Color", rcolor);
			}
			//init whitelists
			for (auto&& enemy : entitylist->get_enemy_heroes()) {
				auto networkid = enemy->get_network_id();
				q_whitelist[networkid] = qMenu::whitelist->add_checkbox(std::to_string(networkid), enemy->get_model(), true, false);
				r_whitelist[networkid] = rMenu::whitelist->add_checkbox(std::to_string(networkid), enemy->get_model(), true, false);
			}
		}
		event_handler<events::on_draw>::add_callback(on_draw);
		event_handler<events::on_update>::add_callback(on_update);
	}
	void unload()
	{
		plugin_sdk->remove_spell(q);
		plugin_sdk->remove_spell(w);
		plugin_sdk->remove_spell(e);
		plugin_sdk->remove_spell(r);

		event_handler<events::on_draw>::remove_handler(on_draw);
		event_handler<events::on_update>::remove_handler(on_update);
	}

}