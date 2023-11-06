#include "../plugin_sdk/plugin_sdk.hpp"
#include "sona.h"

namespace sona {
	#define Q_DRAW_COLOR (MAKE_COLOR ( 0, 0, 255, 255 )) 
	#define W_DRAW_COLOR (MAKE_COLOR ( 0, 255, 0, 255 )) 
	#define E_DRAW_COLOR (MAKE_COLOR ( 0, 255, 255, 255 )) 
	#define R_DRAW_COLOR (MAKE_COLOR ( 255, 255, 0, 255 )) 
	script_spell* q = nullptr;
	script_spell* w = nullptr;
	script_spell* e = nullptr;
	script_spell* r = nullptr;

	script_spell* flash = nullptr;

	TreeTab* main_tab = nullptr;

	namespace generalMenu
	{
		TreeEntry* recallCheck = nullptr;
		TreeEntry* turretCheck = nullptr;
		TreeEntry* adaptiveMana = nullptr;
	}

	namespace passiveMenu
	{
		TreeEntry* auraRange = nullptr;
		TreeEntry* useCenterEdge = nullptr;
	}

	namespace qMenu
	{
		TreeEntry* combo = nullptr;
	}

	namespace drawMenu
	{
		TreeEntry* draw_range_p = nullptr;
		TreeEntry* draw_range_q = nullptr;
		TreeEntry* draw_range_w = nullptr;
		TreeEntry* draw_range_r = nullptr;
	}


	bool ally_in_aura_range(const game_object_script& target)
	{
		return target && target->is_valid() && target->is_ally() && !target->is_dead() && target->is_visible() && target->is_targetable()&& target->get_distance(myhero->get_position());
	}

	void on_draw()
	{

		if (myhero->is_dead())
		{
			return;
		}

		if (q->is_ready() && drawMenu::draw_range_q->get_bool())
			draw_manager->add_circle(myhero->get_position(), q->range(), Q_DRAW_COLOR);
		if (w->is_ready() && drawMenu::draw_range_w->get_bool())
			draw_manager->add_circle(myhero->get_position(), w->range(), W_DRAW_COLOR);
		if (r->is_ready() && drawMenu::draw_range_r->get_bool())
			draw_manager->add_circle(myhero->get_position(), r->range(), R_DRAW_COLOR);
	}

	void load() {
		q = plugin_sdk->register_spell(spellslot::q, 825);
		w = plugin_sdk->register_spell(spellslot::w, 1000);
		e = plugin_sdk->register_spell(spellslot::e, 0);
		r = plugin_sdk->register_spell(spellslot::r, 1000);
		r->set_skillshot(0.25f, 140.0f, 2400.0f, { collisionable_objects::yasuo_wall }, skillshot_type::skillshot_line);
		if (myhero->get_spell(spellslot::summoner1)->get_spell_data()->get_name_hash() == spell_hash("SummonerFlash"))
			flash = plugin_sdk->register_spell(spellslot::summoner1, 400.f);
		else if (myhero->get_spell(spellslot::summoner2)->get_spell_data()->get_name_hash() == spell_hash("SummonerFlash"))
			flash = plugin_sdk->register_spell(spellslot::summoner2, 400.f);
		main_tab = menu->create_tab("sona", "Sona");
		main_tab->set_assigned_texture(myhero->get_square_icon_portrait());

		// Menu init
		{
			auto passiveMenu = main_tab->add_tab(myhero->get_model() + ".passive", "Passive Settings");
			{
				passiveMenu::auraRange = passiveMenu->add_slider(myhero->get_model() + ".pAuraRange", "Passive Range", 390, 350, 400);
				passiveMenu::useCenterEdge = passiveMenu->add_checkbox(myhero->get_model() + ".pCenterEdge", "Use Center-Edge Range", true);
			}
			
			auto drawMenu = main_tab->add_tab(myhero->get_model() + ".drawings", "Drawings Settings");
			{
				drawMenu::draw_range_p = drawMenu->add_checkbox(myhero->get_model() + ".drawingP", "Draw Passive range", true);
				drawMenu::draw_range_q = drawMenu->add_checkbox(myhero->get_model() + ".drawingQ", "Draw Q range", true);
				drawMenu::draw_range_w = drawMenu->add_checkbox(myhero->get_model() + ".drawingW", "Draw W range", true);
				drawMenu::draw_range_r = drawMenu->add_checkbox(myhero->get_model() + ".drawingR", "Draw R range", true);
			}
		}

		event_handler<events::on_draw>::add_callback(on_draw);

	}
	void unload()
	{
		plugin_sdk->remove_spell(q);
		plugin_sdk->remove_spell(w);
		plugin_sdk->remove_spell(e);
		plugin_sdk->remove_spell(r);
		if (flash)
			plugin_sdk->remove_spell(flash);

		event_handler<events::on_draw>::remove_handler(on_draw);
	}

}