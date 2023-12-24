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
	}
	namespace rMenu {

	}

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
	void on_draw() {

	}
	void on_update() {
		// E Logic
		int eMode = eMenu::mode->get_int();
		if (e->is_ready() && orbwalker->can_move() && (eMode == 0 || (eMode == 1 && orbwalker->harass()) || (eMode <= 2 && orbwalker->combo_mode()))) {
			int overwrite = eMenu::overwrite->get_int();
			for (const auto& ally : entitylist->get_ally_heroes()) {
				auto tab = eDB[ally->get_model_cstr()];
				if (!tab || !tab->get_entry("enable")->get_bool() || ally->get_distance(myhero)>e->range()) continue;
				auto activeSpell = ally->get_active_spell();
				if (!activeSpell) return;
				auto target = entitylist->get_object(activeSpell->get_last_target_id());
				bool isTargeted = activeSpell->get_spell_data()->get_targeting_type() == spell_targeting::target && target && target->is_valid() && target->is_enemy();
				bool isAuto = activeSpell->is_auto_attack() && !ally->is_winding_up();
				std::string spellName = spellSlotName(activeSpell);
				if (spellName == "") return; // Unsupported Spell, items, idk
				bool isEnabled = tab->get_entry(spellName)->get_bool();
				// this can be compressed, but i keep it like this for clarity
				bool useE = (overwrite == 0 && isEnabled) ||
							(overwrite == 1 && isTargeted && isEnabled) ||
							(overwrite == 2 && isAuto && isEnabled) ||
							(overwrite == 3 && isTargeted) ||
							(overwrite == 4 && isAuto);
				console->print("%f: %s %s, TargetsEnemy: %i, AA: %i, Enabled: %i, Use E: %i", gametime->get_time(), ally->get_model_cstr(), spellSlotName(activeSpell).c_str(), isTargeted, isAuto, isEnabled, useE);
			}
		}
	}
	
	void on_env_draw() {

	}

	void load() {
		q = plugin_sdk->register_spell(spellslot::q, 850);
		w = plugin_sdk->register_spell(spellslot::w, 725);
		e = plugin_sdk->register_spell(spellslot::e, 800);
		r = plugin_sdk->register_spell(spellslot::r, 1500);
		// TODO: Check if Q Spelldata is correct
		q->set_skillshot(0.99, 200, FLT_MAX, { collisionable_objects::yasuo_wall }, skillshot_type::skillshot_circle);
		r->set_skillshot(0.5, 250, 850, { collisionable_objects::yasuo_wall }, skillshot_type::skillshot_line);
		
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
				// TODO: use e on some skillshot types then calc if they hit
				auto q = tab->add_checkbox("Q", "Q", ally->get_spell(spellslot::q)->get_spell_data()->get_targeting_type() == spell_targeting::target);
				auto w = tab->add_checkbox("W", "W", ally->get_spell(spellslot::w)->get_spell_data()->get_targeting_type() == spell_targeting::target);
				auto e = tab->add_checkbox("E", "E", ally->get_spell(spellslot::e)->get_spell_data()->get_targeting_type() == spell_targeting::target);
				auto r = tab->add_checkbox("R", "R", ally->get_spell(spellslot::r)->get_spell_data()->get_targeting_type() == spell_targeting::target);
				q->set_texture(ally->get_spell(spellslot::q)->get_icon_texture());
				w->set_texture(ally->get_spell(spellslot::w)->get_icon_texture());
				e->set_texture(ally->get_spell(spellslot::e)->get_icon_texture());
				r->set_texture(ally->get_spell(spellslot::r)->get_icon_texture());
				eDB[name] = tab;
			}
			eMenu::overwrite = eMenu->add_combobox("overwrite", "Overwrite", { {"None", nullptr}, {"Only Targeted", nullptr},{"Only Auto Attacks", nullptr}, {"All Targeted", nullptr},{"All Auto Attacks", nullptr} }, 0);
			eMenu::overwrite->set_tooltip("Only Auto Attacks / Only Targeted still take the list into account\n"
											"All Auto Attacks / All Targeted ignores the spell settings\n"
											"You still need to enable / disable the champs");
		}
		auto rMenu = mainMenuTab->add_tab("r", "R Settings");

		event_handler<events::on_draw>::add_callback(on_draw);
		event_handler<events::on_env_draw>::add_callback(on_env_draw);
		event_handler<events::on_update>::add_callback(on_update);
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
	}
}