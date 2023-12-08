#include "../plugin_sdk/plugin_sdk.hpp"
#include "hwei.h"
#include "../spelldb/SpellDB.h"


namespace hwei {
	std::string VERSION = "b0.0.1";
	script_spell* r = nullptr;
	script_spell* qq = nullptr;

	TreeEntry* DEBUGsemiKey = nullptr;

	TreeTab* mainMenuTab = nullptr;
	namespace drawMenu
	{
		TreeEntry* drawOnlyReady = nullptr;
		TreeEntry* drawRangeQ = nullptr;
		TreeEntry* drawRangeW = nullptr;
		TreeEntry* drawRangeE = nullptr;
		TreeEntry* drawRangeR = nullptr;
	}
	namespace colorMenu
	{
		TreeEntry* qColor = nullptr;
		TreeEntry* wColor = nullptr;
		TreeEntry* eColor = nullptr;
		TreeEntry* rColor = nullptr;
	}

	void on_draw() {

		if (myhero->is_dead())
		{
			return;
		}
		// TODO: wtf do i even draw
		//if ((q->is_ready() || !drawMenu::drawOnlyReady->get_bool()) && drawMenu::drawRangeQ->get_bool())
		//	draw_manager->add_circle(myhero->get_position(), 1000, colorMenu::qColor->get_color());
		draw_manager->add_circle(myhero->get_position(), 1000, MAKE_COLOR(255, 0, 0, 255));
	}
	void on_update() {
		if (DEBUGsemiKey->get_bool()) {
			//q->cast(hud->get_hud_input_logic()->get_game_cursor_position());
			qq->cast();
		}

	}

	void load() {
		r = plugin_sdk->register_spell(spellslot::r, 1300);
		qq = plugin_sdk->register_spell(spellslot::q, 100);
		qq->set_spell_slot_category(spellslot::q);
		mainMenuTab = menu->create_tab("Flofian_Hwei", "Flofian Hwei");
		mainMenuTab->set_assigned_texture(myhero->get_square_icon_portrait());

		DEBUGsemiKey = mainMenuTab->add_hotkey("DEBUGsemi", "DEBUGSEMI", TreeHotkeyMode::Hold, 0x4B,false);

		event_handler<events::on_draw>::add_callback(on_draw);
		event_handler<events::on_update>::add_callback(on_update);

	}
	void unload() {
		plugin_sdk->remove_spell(r);
		plugin_sdk->remove_spell(qq);
		event_handler<events::on_draw>::remove_handler(on_draw);
		event_handler<events::on_update>::remove_handler(on_update);

	}
}