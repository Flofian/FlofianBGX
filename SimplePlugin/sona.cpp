#include "../plugin_sdk/plugin_sdk.hpp"
#include "sona.h"

#define BASEKEY "Flofian."+myhero->get_model()

namespace sona {
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
		TreeEntry* debugMode = nullptr;
	}

	namespace passiveMenu
	{
		TreeEntry* auraRange = nullptr;
		TreeEntry* useCenterEdge = nullptr;
	}

	namespace qMenu
	{
		TreeEntry* range = nullptr;
		TreeEntry* comboTargets = nullptr;
		TreeEntry* harassTargets = nullptr;
		TreeEntry* autoTargets = nullptr;
		TreeEntry* amplifyAA = nullptr;
		TreeEntry* amplifyDirect = nullptr;
		TreeEntry* autoMana = nullptr;
	}
	
	namespace wMenu
	{
		TreeEntry* range = nullptr;
		TreeEntry* autoShield = nullptr;
		TreeEntry* autoShieldHeal = nullptr;
		TreeEntry* autoShieldFactor = nullptr;
		TreeEntry* includeSkillshots = nullptr;
	}

	namespace eMenu {
		TreeEntry* antiMelee = nullptr;
		TreeEntry* antiMeleeRange = nullptr;
		TreeEntry* comboTargets = nullptr;
	}

	namespace rMenu {
		TreeEntry* range = nullptr;
		TreeEntry* comboTargets = nullptr;
		TreeEntry* semiKey = nullptr;
		TreeEntry* semiTargets = nullptr;
	}

	namespace drawMenu
	{
		TreeEntry* draw_range_p = nullptr;
		TreeEntry* draw_range_q = nullptr;
		TreeEntry* draw_range_w = nullptr;
		TreeEntry* draw_range_e = nullptr;
		TreeEntry* draw_range_r = nullptr;
		
	}
	namespace colorMenu
	{
		TreeEntry* pColor = nullptr;
		TreeEntry* qColor = nullptr;
		TreeEntry* wColor = nullptr;
		TreeEntry* eColor = nullptr;
		TreeEntry* rColor = nullptr;
	}

	float wShieldStrength(const game_object_script& target) {
		//TODO Heal and shield power, mode specific buffs/nerf, revitalize, Spirit Visage
		float baseShield = 5 + 20 * w->level() + 0.25f * myhero->get_total_ability_power();
		return baseShield;
	}
	float wHealStrength(const game_object_script& target) {
		//Same as wShieldStrength
		float baseHeal = 15 + 15 * w->level() + 0.15f * myhero->get_total_ability_power();
		return baseHeal;
	}

	bool isUnderTower(const game_object_script& target)
	{
		for (const auto& turret : entitylist->get_enemy_turrets())
			if (turret && turret->is_valid() && target->get_position().distance(turret->get_position()) <= 750 + target->get_bounding_radius())
				return true;
		return false;
	}

	bool allyInAuraRange(const game_object_script& target)
	{
		if (!target || !target->is_valid()) return false;
		bool inRange = target->get_distance(myhero->get_position())<passiveMenu::auraRange->get_int()+passiveMenu::useCenterEdge->get_bool()*target->get_bounding_radius();
		return target && target->is_valid() && target->is_ally() && !target->is_dead() && target->is_visible() && target->is_targetable()&& inRange;
	}
	bool canCastQ(bool isAuto) {
		bool autoCheck = !isAuto || (myhero->get_mana_percent() > qMenu::autoMana->get_int() && 
			(!generalMenu::recallCheck->get_bool() || !myhero->is_recalling()) && (!generalMenu::turretCheck->get_bool()||!isUnderTower(myhero)));
		return q->is_ready() && autoCheck;
	}

	bool isEnemyInERange(const game_object_script& target) {
		int d = eMenu::antiMeleeRange->get_int();
		for (const auto& enemy : entitylist->get_enemy_heroes()) {
			if (target->get_distance(enemy) < d) return true;
		}
		return false;
	}

	bool canCastW() {
		return w->is_ready() && (!generalMenu::recallCheck->get_bool() || !myhero->is_recalling());
	}
	int enemiesInQRange() {
		int count = 0;
		for (const auto& target : entitylist->get_enemy_heroes()) {
			if (target && target->is_valid() && target->is_valid_target(qMenu::range->get_int(), myhero->get_position())) count++;
		}
		return count;
	}
	int countAlliesHealed() {
		bool selfHeal = false;
		bool healAlly = false;
		for (const auto& target : entitylist->get_ally_heroes()) {
			if (target && target->is_valid() && target->get_distance(myhero) < wMenu::range->get_int() && target->get_max_health() - target->get_health() >= wHealStrength(target)) {
				if (target->is_me()) {
					selfHeal = true;
				}
				else {
					healAlly = true;
				}
			}
		}
		return selfHeal + healAlly;
	}

	int calcShieldedChamps() {
		bool skillshots = wMenu::includeSkillshots->get_bool();
		int total = 0;
		for (const auto& target : entitylist->get_ally_heroes()) {
			if (target && target->is_valid() && allyInAuraRange(target)) {
				float incomingdmg = health_prediction->get_incoming_damage(target, 1.5f, skillshots);
				float minDmgMitigated = 5 + 20 * w->level();		//Not sure about aram, since i shield less but might need to still mitigate same value to get passive stack
				//so maybe: shieldabledmg = fmin(incomingdmg, wShieldStrength(target)) //and tweak wShieldStrength for aram/other modes
				if (incomingdmg > minDmgMitigated) total++;
			}

		}
		return total;
	}

	void automatic() {
		// Auto Q
		if (canCastQ(true)) {
			// Auto Q x Targets
			int autoQTargets = qMenu::autoTargets->get_int();
			if (autoQTargets > 0 && enemiesInQRange() >= autoQTargets) {
				q->cast();
			}
			// Auto Q Amplify
			if (qMenu::amplifyAA->get_bool()) {
				int directHitsMin = qMenu::amplifyDirect->get_int();
				for (const auto& ally : entitylist->get_ally_heroes()) {
					auto activeSpell = ally->get_active_spell();
					
					if (activeSpell && activeSpell->is_auto_attack() && !ally->is_winding_up() && allyInAuraRange(ally)) {
						if (enemiesInQRange() >= directHitsMin) q->cast();
					}
				}
			}
		}

		// Auto W
		if (canCastW() && wMenu::autoShield->get_bool()) {
			int minHealTargets = wMenu::autoShieldHeal->get_int();
			int totalShielded = calcShieldedChamps();
			//myhero->print_chat(0, "Heal: %i Shield: %i", countAlliesHealed(), totalShielded);
			if (totalShielded >= wMenu::autoShieldFactor->get_int() && countAlliesHealed() >= minHealTargets) {
				w->cast();
			}
		}

		// Auto E
		if (e->is_ready() && (!generalMenu::recallCheck->get_bool() || !myhero->is_recalling())) {
			switch (eMenu::antiMelee->get_int()) {
			case 1:
				if (isEnemyInERange(myhero)) e->cast();
				break;
			case 2:
				for (const auto& target : entitylist->get_ally_heroes()) {
					if (allyInAuraRange(target) && isEnemyInERange(target)) { e->cast(); break; }

				}
				break;

			default: break;
			}
			
		}

		
	}

	void combo() {
		// Q
		if (canCastQ(false)) {
			int minTargets = qMenu::comboTargets->get_int();
			if (minTargets > 0 && enemiesInQRange() >= minTargets) {
				q->cast();
			}
		}

		// R 
		if (r->is_ready()) {
			auto target = target_selector->get_target(rMenu::range->get_int(), damage_type::magical);
			if (!target) return;
			auto pred = r->get_prediction(target, true);
			if (pred.hitchance >= hit_chance::high && pred.aoe_targets_hit_count() >= rMenu::comboTargets->get_int()) {
				auto castpos = pred.get_cast_position();
				r->cast(castpos);
				if (generalMenu::debugMode->get_bool()) myhero->print_chat(0, "Combo R on %i Targets with hitchance %i", pred.aoe_targets_hit_count(), pred.hitchance);
			}
		}
	}

	void harass() {
		// Q
		if (canCastQ(false)) {
			int minTargets = qMenu::harassTargets->get_int();
			if (minTargets > 0 && enemiesInQRange() >= minTargets) {
				q->cast();
			}
		}
	}

	void semiR() {
		if (myhero->is_dead() || !rMenu::semiKey->get_bool() || !r->is_ready()) return;
		auto target = target_selector->get_target(rMenu::range->get_int(), damage_type::magical);
		if (!target) return;
		auto pred = r->get_prediction(target, true);
		if (pred.hitchance >= hit_chance::high && pred.aoe_targets_hit_count() >= rMenu::semiTargets->get_int()) {
			auto castpos = pred.get_cast_position();
			r->cast(castpos);
			if (generalMenu::debugMode->get_bool()) myhero->print_chat(0, "Semi R on %i Targets with hitchance %i", pred.aoe_targets_hit_count(), pred.hitchance);
		}
	}
	
	void on_update() {
		if (myhero->is_dead())
			return;
		automatic();
		if (orbwalker->combo_mode()) combo();
		if (orbwalker->harass()) harass();
		semiR();
	}

	void on_draw()
	{
		if (myhero->is_dead())
		{
			return;
		}

		if (q->is_ready() && drawMenu::draw_range_q->get_bool())
			draw_manager->add_circle(myhero->get_position(), qMenu::range->get_int(), colorMenu::qColor->get_color());
		if (w->is_ready() && drawMenu::draw_range_w->get_bool())
			draw_manager->add_circle(myhero->get_position(), wMenu::range->get_int(), colorMenu::wColor->get_color());
		if (e->is_ready() && drawMenu::draw_range_e->get_bool())
			draw_manager->add_circle(myhero->get_position(), passiveMenu::auraRange->get_int(), colorMenu::eColor->get_color());
		if (r->is_ready() && drawMenu::draw_range_r->get_bool())
			draw_manager->add_circle(myhero->get_position(), rMenu::range->get_int(), colorMenu::rColor->get_color());


		if (drawMenu::draw_range_p->get_bool()) {
			for (const auto& target : entitylist->get_ally_heroes()) {
				if (target->is_me()) continue;
				if (allyInAuraRange(target))
					draw_manager->add_circle(target->get_position(), target->get_bounding_radius(), colorMenu::pColor->get_color());
			}
		}
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
		main_tab = menu->create_tab(BASEKEY, "Flofian Sona");
		main_tab->set_assigned_texture(myhero->get_square_icon_portrait());

		// Menu init
		{
			auto generalMenu = main_tab->add_tab(BASEKEY + ".general", "General Settings");
			{
				generalMenu::recallCheck = generalMenu->add_checkbox(BASEKEY + ".gRecall", "Dont use anything automatically while recalling", true);
				generalMenu::turretCheck = generalMenu->add_checkbox(BASEKEY + ".gTurret", "Dont use anything automatically under Enemy turret", true);
				generalMenu::adaptiveMana = generalMenu->add_checkbox(BASEKEY + ".gAdaptiveMana", "Does nothing yet", true);
				generalMenu::debugMode = generalMenu->add_checkbox(BASEKEY + ".debug", "Debug Mode", false);
			}
			auto passiveMenu = main_tab->add_tab(BASEKEY + ".passive", "Passive Settings");
			{
				passiveMenu->set_assigned_texture(myhero->get_passive_icon_texture());
				passiveMenu::auraRange = passiveMenu->add_slider(BASEKEY + ".pAuraRange", "Aura Range", 390, 350, 400);
				passiveMenu::useCenterEdge = passiveMenu->add_checkbox(BASEKEY + ".pCenterEdge", "Use Center-Edge Range", true);
			}
			auto qMenu = main_tab->add_tab(BASEKEY + ".q", "Q Settings");
			{
				qMenu->set_assigned_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
				qMenu::range = qMenu->add_slider(BASEKEY + ".qRange", "Q Range", 800, 750, 825);
				qMenu::comboTargets = qMenu->add_slider(BASEKEY + ".qComboTargets", "Min Targets in Combo (0 to disable)", 1, 0, 2);
				qMenu::harassTargets = qMenu->add_slider(BASEKEY + ".qHarassTargets", "Min Targets in Harass (0 to disable)", 1, 0, 2);
				qMenu->add_separator(BASEKEY + ".qsep", "Automatic");
				qMenu::autoTargets = qMenu->add_slider(BASEKEY + ".qAutoTargets", "Min Targets to Auto-use (0 to disable)", 2, 0, 2);
				qMenu::amplifyAA = qMenu->add_checkbox(BASEKEY + "qAmplifyAA", "Use to amplify autoattacks", true);
				qMenu::amplifyDirect = qMenu->add_slider(BASEKEY + ".qAmplifyDirect", "^Only when also hitting x direct", 1, 0, 2);
				qMenu::autoMana = qMenu->add_slider(BASEKEY + "qAutoMana", "Only auto use when above x% mana", 30, 0, 100);
			}
			auto wMenu = main_tab->add_tab(BASEKEY + ".w", "W Settings");
			{
				wMenu->set_assigned_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
				wMenu::range = wMenu->add_slider(BASEKEY + ".wRange", "W Range", 975, 950, 1000);
				wMenu::autoShield = wMenu->add_checkbox(BASEKEY + ".wAutoShield", "Shield Incoming Damage", true);
				wMenu::autoShieldFactor = wMenu->add_slider(BASEKEY + ".wAutoShieldFactor", "Only when Shielding x Targets", 1, 1, 5);
				wMenu::autoShieldFactor->set_tooltip("Depending on if you get a passive stack or not");
				wMenu::includeSkillshots = wMenu->add_checkbox(BASEKEY + ".wIncludeSkillshots", "Include Skillshots", true);
				wMenu::autoShieldHeal = wMenu->add_slider(BASEKEY + ".wAutoShieldHeal", "Only when also healing x Targets", 1, 0, 2);
			}
			
			auto eMenu = main_tab->add_tab(BASEKEY + ".e", "E Settings");
			{
				eMenu->set_assigned_texture(myhero->get_spell(spellslot::e)->get_icon_texture());
				eMenu::antiMelee = eMenu->add_combobox(BASEKEY + ".eAntiMelee", "Anti Melee Mode", { {"Off", nullptr}, {"Self", nullptr}, {"Self + Ally", nullptr} }, 2);
				eMenu::antiMeleeRange = eMenu->add_slider(BASEKEY + ".eAntiMeleeRange", "Anti Melee Range", 500, 100, 800);
				eMenu::antiMeleeRange->set_tooltip("Auto E if Enemy in this range");
			}

			auto rMenu = main_tab->add_tab(BASEKEY + ".r", "R Settings");
			{
				rMenu->set_assigned_texture(myhero->get_spell(spellslot::r)->get_icon_texture());
				rMenu->set_tooltip("Currently does not use Bounding Radius");
				rMenu::range = rMenu->add_slider(BASEKEY + ".rRange", "R Range", 950, 900, 1000);
				rMenu::comboTargets = rMenu->add_slider(BASEKEY + ".rComboTargets", "Min Targets in Combo (0 to disable)", 3, 0, 5);
				rMenu::semiKey = rMenu->add_hotkey(BASEKEY + ".rSemiKey", "Semi Key", TreeHotkeyMode::Hold, 0x54, false);
				rMenu::semiTargets = rMenu->add_slider(BASEKEY + ".rSemiTargets", "Min Targets for Semi Key", 2, 1, 5);
			}
			
			auto drawMenu = main_tab->add_tab(BASEKEY + ".drawings", "Drawings Settings");
			{
				drawMenu::draw_range_p = drawMenu->add_checkbox(BASEKEY + ".drawingP", "Draw Allies in Aura Range", true);
				drawMenu::draw_range_q = drawMenu->add_checkbox(BASEKEY + ".drawingQ", "Draw Q range", true);
				drawMenu::draw_range_w = drawMenu->add_checkbox(BASEKEY + ".drawingW", "Draw W range", true);
				drawMenu::draw_range_e = drawMenu->add_checkbox(BASEKEY + ".drawingE", "Draw E range", true);
				drawMenu::draw_range_r = drawMenu->add_checkbox(BASEKEY + ".drawingR", "Draw R range", true);
				auto colorMenu = drawMenu->add_tab(BASEKEY + ".color", "Color Settings");
				float pcolor[] = { 0.f, 1.f, 0.f, 1.f };
				colorMenu::pColor = colorMenu->add_colorpick(BASEKEY + ".colorP", "Ally in Aura Color", pcolor);

				float qcolor[] = { 0.f, 0.f, 1.f, 1.f };
				colorMenu::qColor = colorMenu->add_colorpick(BASEKEY + ".colorQ", "Q Range Color", qcolor);
				float wcolor[] = { 0.f, 1.f, 0.f, 1.f };
				colorMenu::wColor = colorMenu->add_colorpick(BASEKEY + ".colorW", "W Range Color", wcolor);
				float ecolor[] = { 1.f, 0.f, 1.f, 1.f };
				colorMenu::eColor = colorMenu->add_colorpick(BASEKEY + ".colorE", "E Range Color", ecolor);
				float rcolor[] = { 1.f, 1.f, 0.f, 1.f };
				colorMenu::rColor = colorMenu->add_colorpick(BASEKEY + ".colorR", "R Range Color", rcolor);
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
		if (flash)
			plugin_sdk->remove_spell(flash);

		event_handler<events::on_draw>::remove_handler(on_draw);
		event_handler<events::on_update>::remove_handler(on_update);
	}
	
}