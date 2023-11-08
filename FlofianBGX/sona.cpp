#include "../plugin_sdk/plugin_sdk.hpp"
#include "sona.h"


namespace sona {
	script_spell* q = nullptr;
	script_spell* w = nullptr;
	script_spell* e = nullptr;
	script_spell* r = nullptr;

	bool adaptiveMana = false;
	float manaPerc = 1;
	bool isAram = false;

	TreeTab* mainMenuTab = nullptr;

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
		TreeEntry* sepAuto = nullptr;
	}
	
	namespace wMenu
	{
		TreeEntry* range = nullptr;
		TreeEntry* autoShield = nullptr;
		TreeEntry* autoShieldHeal = nullptr;
		TreeEntry* autoShieldFactor = nullptr;
		TreeEntry* includeSkillshots = nullptr;
		TreeEntry* comboHealHP = nullptr;
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
		TreeEntry* interrupt = nullptr;
		TreeEntry* hitchance = nullptr;
	}

	namespace drawMenu
	{	
		TreeEntry* drawOnlyReady = nullptr;
		TreeEntry* drawAuraTargets = nullptr;
		TreeEntry* drawRangeQ = nullptr;
		TreeEntry* drawRangeW = nullptr;
		TreeEntry* drawRangeE = nullptr;
		TreeEntry* drawRangeR = nullptr;
	}
	namespace colorMenu
	{
		TreeEntry* pColor = nullptr;
		TreeEntry* qColor = nullptr;
		TreeEntry* wColor = nullptr;
		TreeEntry* eColor = nullptr;
		TreeEntry* rColor = nullptr;
	}

	// Helper functions
	bool isUnderTower(const game_object_script& target)
	{
		for (const auto& turret : entitylist->get_enemy_turrets())
			if (turret && turret->is_valid() && target->get_position().distance(turret->get_position()) <= 775 + target->get_bounding_radius()) //Should be 750, but i want small buffer
				return true;
		return false;
	}
	bool isAllyInAuraRange(const game_object_script& target)
	{
		if (!target || !target->is_valid()) return false;
		bool inRange = target->get_distance(myhero->get_position())<passiveMenu::auraRange->get_int()+passiveMenu::useCenterEdge->get_bool()*target->get_bounding_radius();
		return target && target->is_valid() && target->is_ally() && !target->is_dead() && target->is_visible() && target->is_targetable()&& inRange;
	}
	hit_chance getHitchance(const int hc)
	{
		switch (hc)
		{
		case 0:
			return hit_chance::medium;
		case 1:
			return hit_chance::high;
		case 2:
			return hit_chance::very_high;
		default:
			return hit_chance::high;
		}
	}

	// for Q
	bool canCastQ(bool isAuto) {
		bool autoCheck = !isAuto || (myhero->get_mana_percent() > qMenu::autoMana->get_int() && 
			(!generalMenu::recallCheck->get_bool() || !myhero->is_recalling()) && (!generalMenu::turretCheck->get_bool()||!isUnderTower(myhero)));
		return q->is_ready() && autoCheck;
	}
	int countEnemiesInQRange() {
		int count = 0;
		for (const auto& target : entitylist->get_enemy_heroes()) {
			if (target && target->is_valid() && target->is_valid_target(qMenu::range->get_int(), myhero->get_position())) count++;
		}
		return count;
	}

	// for W
	float wShieldStrength(const game_object_script& target) {
		//TODO Heal and shield power, mode specific buffs/nerf, revitalize, Spirit Visage
		float baseShield = 5 + 20 * w->level() + 0.25f * myhero->get_total_ability_power();
		return baseShield;
	}
	float wHealStrength(const game_object_script& target) {
		//Same as wShieldStrength
		float baseHeal = 15 + 15 * w->level() + 0.15f * myhero->get_total_ability_power();
		if (isAram) {
			return baseHeal * 0.6f;
		}
		return baseHeal;
	}
	int countHealedChamps() {
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
	int countShieldedChamps() {
		bool skillshots = wMenu::includeSkillshots->get_bool();
		int total = 0;
		for (const auto& target : entitylist->get_ally_heroes()) {
			if (target && target->is_valid() && isAllyInAuraRange(target)) {
				float incomingdmg = health_prediction->get_incoming_damage(target, 1.5f, skillshots);
				float minDmgMitigated = 5 + 20 * w->level();		//Not sure about aram, since i shield less but might need to still mitigate same value to get passive stack
				//so maybe: shieldabledmg = fmin(incomingdmg, wShieldStrength(target)) //and tweak wShieldStrength for aram/other modes
				if (incomingdmg > minDmgMitigated) total++;
			}

		}
		return total;
	}

	// for E
	bool isEnemyInERange(const game_object_script& target) {
		int d = eMenu::antiMeleeRange->get_int();
		for (const auto& enemy : entitylist->get_enemy_heroes()) {
			if (enemy && enemy->is_valid()&&!enemy->is_dead()&& enemy->is_visible()){
				if (target->get_distance(enemy) < d) return true;
			}
		}
		return false;
	}


	void automatic() {
		// Auto Q
		if (canCastQ(true)) {
			// Auto Q x Targets
			int autoQTargets = adaptiveMana ? 2*(manaPerc>40) : qMenu::autoTargets->get_int();		// 2 Targets when over 40% Mana, else disabled
			if (autoQTargets > 0 && countEnemiesInQRange() >= autoQTargets) {
				q->cast();
			}
			// Auto Q Amplify
			if (adaptiveMana ? manaPerc>20 : qMenu::amplifyAA->get_bool()) {		// only amplify with more than 20% mana
				int directHitsMin = adaptiveMana ? 1+(manaPerc<40) : qMenu::amplifyDirect->get_int();	// 1 direct hit when over 40 %, else 2
				for (const auto& ally : entitylist->get_ally_heroes()) {
					auto activeSpell = ally->get_active_spell();
					
					if (activeSpell && activeSpell->is_auto_attack() && !ally->is_winding_up() && isAllyInAuraRange(ally)) {
						auto lastTargetId = activeSpell->get_last_target_id();
						auto lastTarget = entitylist->get_object(lastTargetId);
						
						if (lastTarget && lastTarget->is_valid() && lastTarget->is_ai_hero() && countEnemiesInQRange() >= directHitsMin) q->cast();
					}
				}
			}
		}

		// Auto W
		if (w->is_ready() && (!generalMenu::recallCheck->get_bool() || !myhero->is_recalling()) && wMenu::autoShield->get_bool()) {
			int minHealTargets = wMenu::autoShieldHeal->get_int();
			int totalShielded = countShieldedChamps();
			if (totalShielded >= wMenu::autoShieldFactor->get_int() && countHealedChamps() >= minHealTargets) {
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
					if (isAllyInAuraRange(target) && isEnemyInERange(target)) { e->cast(); break; }

				}
				break;

			default: break;
			}
			
		}

		// R interrupt
		if (r->is_ready() && (!generalMenu::recallCheck->get_bool() || !myhero->is_recalling()) && rMenu::interrupt->get_bool()) {
			for (const auto& target : entitylist->get_enemy_heroes()) {
				if (target && target->is_valid() && target->is_visible() && !target->is_zombie() && target->is_valid_target(rMenu::range->get_int()) && target->is_casting_interruptible_spell() >= 2) {
					auto pred = r->get_prediction(target, true);
					if (pred.hitchance >= getHitchance(rMenu::hitchance->get_int())) {
						r->cast(pred.get_cast_position());
						if (generalMenu::debugMode->get_bool()) myhero->print_chat(0, "Interrupt R on %i Targets with hitchance %i", pred.aoe_targets_hit_count(), pred.hitchance);
					}
				}

			}
		}
	}

	void combo() {
		// Q
		if (canCastQ(false)) {
			int minTargets = adaptiveMana ? (2 * (manaPerc > 5) - (manaPerc >= 20)) : qMenu::comboTargets->get_int(); // 0 if under 5%, 2 under 20%, else 1
			if (minTargets > 0 && countEnemiesInQRange() >= minTargets) {
				q->cast();
			}
		}

		// W
		if (w->is_ready()) {
			float healUnderHP =  adaptiveMana ? manaPerc : wMenu::comboHealHP->get_int();
			for (const auto& target : entitylist->get_ally_heroes()) {
				if (target && target->is_valid() && target->get_health_percent() < healUnderHP && target->get_distance(myhero) < wMenu::range->get_int())
					w->cast();
			}
		}

		// E
		if (e->is_ready()) {
			int minTargets =  adaptiveMana ? (4*(manaPerc>10)-(manaPerc>30)) : eMenu::comboTargets->get_int(); // 0 if under 10%, 4 if under 30%, else 3
			int count = 0;
			for (const auto& target : entitylist->get_ally_heroes()) {
				if (target && target->is_valid() && !target->is_dead() && isAllyInAuraRange(target)) count++;
			}
			if (minTargets>0 && count >= minTargets) e->cast();

		}

		// R 
		if (r->is_ready()) {
			auto target = target_selector->get_target(rMenu::range->get_int(), damage_type::magical);
			if (!target) return;
			int minTargets = rMenu::comboTargets->get_int();
			auto pred = r->get_prediction(target, minTargets>1);
			if (pred.hitchance >= getHitchance(rMenu::hitchance->get_int()) && (minTargets == 1 || pred.aoe_targets_hit_count() >= rMenu::comboTargets->get_int())) {
				auto castpos = pred.get_cast_position();
				r->cast(castpos);
				if (generalMenu::debugMode->get_bool()) myhero->print_chat(0, "Combo R on %i Targets with hitchance %i", minTargets > 1 ? pred.aoe_targets_hit_count() : 1, pred.hitchance);
			}
		}
	}

	void harass() {
		// Q
		if (canCastQ(false)) {
			int minTargets = adaptiveMana ? (2 * (manaPerc > 5) - (manaPerc >= 40)) : qMenu::comboTargets->get_int(); // 0 if under 5%, 2 under 40%, else 1
			if (minTargets > 0 && countEnemiesInQRange() >= minTargets) {
				q->cast();
			}
		}
	}

	void semiR() {
		if (myhero->is_dead() || !rMenu::semiKey->get_bool() || !r->is_ready()) return;
		auto target = target_selector->get_target(rMenu::range->get_int(), damage_type::magical);
		if (!target) return;
		int minTargets = rMenu::semiTargets->get_int();
		auto pred = r->get_prediction(target, minTargets>1);
		if (pred.hitchance >= getHitchance(rMenu::hitchance->get_int()) && (minTargets == 1 || pred.aoe_targets_hit_count() >= rMenu::semiTargets->get_int())) {
			auto castpos = pred.get_cast_position();
			r->cast(castpos);
			if (generalMenu::debugMode->get_bool()) myhero->print_chat(0, "Semi R on %i Targets with hitchance %i", minTargets>1 ? pred.aoe_targets_hit_count() : 1, pred.hitchance);
		}
	}
	
	void on_update() {
		if (myhero->is_dead())
			return;
		adaptiveMana = generalMenu::adaptiveMana->get_bool();
		manaPerc = myhero->get_mana_percent();
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

		if ((q->is_ready() || !drawMenu::drawOnlyReady->get_bool()) && drawMenu::drawRangeQ->get_bool())
			draw_manager->add_circle(myhero->get_position(), qMenu::range->get_int(), colorMenu::qColor->get_color());
		if ((w->is_ready() || !drawMenu::drawOnlyReady->get_bool()) && drawMenu::drawRangeW->get_bool())
			draw_manager->add_circle(myhero->get_position(), wMenu::range->get_int(), colorMenu::wColor->get_color());
		if ((e->is_ready() || !drawMenu::drawOnlyReady->get_bool()) && drawMenu::drawRangeE->get_bool())
			draw_manager->add_circle(myhero->get_position(), passiveMenu::auraRange->get_int(), colorMenu::eColor->get_color());
		if ((r->is_ready() || !drawMenu::drawOnlyReady->get_bool()) && drawMenu::drawRangeR->get_bool())
			draw_manager->add_circle(myhero->get_position(), rMenu::range->get_int(), colorMenu::rColor->get_color());


		if (drawMenu::drawAuraTargets->get_bool()) {
			for (const auto& target : entitylist->get_ally_heroes()) {
				if (target->is_me()) continue;
				if (isAllyInAuraRange(target))
					draw_manager->add_circle(target->get_position(), target->get_bounding_radius(), colorMenu::pColor->get_color());
			}
			
		}
	}
	

	void load() {
		q = plugin_sdk->register_spell(spellslot::q, 825);
		w = plugin_sdk->register_spell(spellslot::w, 975);
		e = plugin_sdk->register_spell(spellslot::e, 0);
		r = plugin_sdk->register_spell(spellslot::r, 950);
		r->set_skillshot(0.25f, 140.0f, 2400.0f, { collisionable_objects::yasuo_wall }, skillshot_type::skillshot_line);
		mainMenuTab = menu->create_tab("Flofian_Sona", "Flofian Sona");
		mainMenuTab->set_assigned_texture(myhero->get_square_icon_portrait());

		// Menu init
		{	
			
			auto generalMenu = mainMenuTab->add_tab("General", "General Settings");
			{
				generalMenu::recallCheck = generalMenu->add_checkbox("RecallCheck", "Dont use anything automatically while recalling", true);
				generalMenu::turretCheck = generalMenu->add_checkbox("TurretCheck", "Dont use anything automatically under Enemy turret", true);
				generalMenu::adaptiveMana = generalMenu->add_checkbox("AdaptiveMana", "Adaptive Mana Mode", true);
				generalMenu::adaptiveMana->set_tooltip("This removes most of the sliders and instead uses your mana to calc the number of targets\n"
														"To find the values used by Adaptive Mana Mode, disable it, then hover of the respective Sliders");
				generalMenu::debugMode = generalMenu->add_checkbox("Debug", "Debug Mode", false);
				generalMenu::debugMode->is_hidden() = generalMenu::debugMode->get_bool();		// so only i have it on, dont want to hide it entirely
				generalMenu::adaptiveMana->add_property_change_callback([](TreeEntry* entry) {
					if (entry->get_bool()) {
						qMenu::comboTargets->is_hidden() = true;
						qMenu::harassTargets->is_hidden() = true;
						qMenu::autoTargets->is_hidden() = true;
						qMenu::amplifyAA->is_hidden() = true;
						qMenu::amplifyDirect->is_hidden() = true;
						qMenu::autoMana->is_hidden() = true;
						qMenu::sepAuto->is_hidden() = true;

						wMenu::comboHealHP->is_hidden() = true;

						eMenu::comboTargets->is_hidden() = true;
					}
					else {
						qMenu::comboTargets->is_hidden() = false;
						qMenu::harassTargets->is_hidden() = false;
						qMenu::autoTargets->is_hidden() = false;
						qMenu::amplifyAA->is_hidden() = false;
						qMenu::amplifyDirect->is_hidden() = false;
						qMenu::autoMana->is_hidden() = false;
						qMenu::sepAuto->is_hidden() = false;

						wMenu::comboHealHP->is_hidden() = false;

						eMenu::comboTargets->is_hidden() = false;
					}
				});
			}
			auto passiveMenu = mainMenuTab->add_tab("Passive", "Passive Settings");
			{
				passiveMenu->set_assigned_texture(myhero->get_passive_icon_texture());
				passiveMenu::auraRange = passiveMenu->add_slider("AuraRange", "Aura Range", 390, 350, 400);
				passiveMenu::useCenterEdge = passiveMenu->add_checkbox("CenterEdge", "Use Center-Edge Range", true);
			}
			auto qMenu = mainMenuTab->add_tab("Q", "Q Settings");
			{
				qMenu->set_assigned_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
				qMenu::range = qMenu->add_slider("Range", "Q Range", 800, 750, 825);
				qMenu::comboTargets = qMenu->add_slider("ComboTargets", "Min Targets in Combo (0 to disable)", 1, 0, 2);
				qMenu::comboTargets->set_tooltip("Adaptive Mana:\nunder  5%: Disabled\nunder 20%: 2\nabove 20%: 1");
				qMenu::harassTargets = qMenu->add_slider("HarassTargets", "Min Targets in Harass (0 to disable)", 1, 0, 2);
				qMenu::harassTargets->set_tooltip("Adaptive Mana:\nunder  5%: Disabled\nunder 40%: 2\nabove 40%: 1");
				qMenu::sepAuto = qMenu->add_separator("sep", "Automatic");
				qMenu::autoTargets = qMenu->add_slider("AutoTargets", "Min Targets to Auto-use (0 to disable)", 2, 0, 2);
				qMenu::autoTargets->set_tooltip("Adaptive Mana:\nunder 40%: Disabled\nabove 40%: 2");
				qMenu::amplifyAA = qMenu->add_checkbox("AmplifyAA", "Use to amplify autoattacks", true);
				qMenu::amplifyAA->set_tooltip("Adaptive Mana:\nunder 20%: Disabled");
				qMenu::amplifyDirect = qMenu->add_slider("AmplifyDirect", "^Only when also hitting x direct", 1, 0, 2);
				qMenu::amplifyDirect->set_tooltip("Adaptive Mana:\nunder 20%: Disabled\nunder 40%: 2\nabove 40%: 1");
				qMenu::autoMana = qMenu->add_slider("AutoMana", "Only auto use when above x% mana", 30, 0, 100);
			}
			auto wMenu = mainMenuTab->add_tab("W", "W Settings");
			{
				wMenu->set_assigned_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
				wMenu::range = wMenu->add_slider("Range", "W Range", 975, 950, 1000);
				wMenu::comboHealHP = wMenu->add_slider("ComboHealHP", "Use in combo if ally under x% HP", 60, 0, 100);
				wMenu::comboHealHP->set_tooltip("Adaptive Mana:\nMana% = HP%\nExample: Mana at 50%-> Heal if ally under 50%HP");
				wMenu->add_separator("sep", "Automatic");
				wMenu::autoShield = wMenu->add_checkbox("AutoShield", "Shield Incoming Damage", true);
				wMenu::autoShieldFactor = wMenu->add_slider("AutoShieldFactor", "Only when Shielding x Targets", 1, 1, 5);
				wMenu::autoShieldFactor->set_tooltip("Depending on if you get a passive stack or not");
				wMenu::includeSkillshots = wMenu->add_checkbox("IncludeSkillshots", "Include Skillshots", true);
				wMenu::autoShieldHeal = wMenu->add_slider("AutoShieldHeal", "Only when also healing x Targets", 1, 0, 2);
			}
			auto eMenu = mainMenuTab->add_tab("E", "E Settings");
			{
				eMenu->set_assigned_texture(myhero->get_spell(spellslot::e)->get_icon_texture());
				eMenu::comboTargets = eMenu->add_slider("ComboTargets", "E in combo when x allies in range (0 to disable)", 3, 0, 5);
				eMenu::comboTargets->set_tooltip("Adaptive Mana:\nunder 10%: Disabled\nunder 30%: 4\nabove 30%: 3");
				eMenu::antiMelee = eMenu->add_combobox("AntiMelee", "Anti Melee Mode", { {"Off", nullptr}, {"Self", nullptr}, {"Self + Ally", nullptr} }, 2);
				eMenu::antiMeleeRange = eMenu->add_slider("AntiMeleeRange", "Anti Melee Range", 500, 100, 800);
				eMenu::antiMeleeRange->set_tooltip("Auto E if Enemy in this range");
			}
			auto rMenu = mainMenuTab->add_tab("R", "R Settings");
			{
				
				rMenu->set_assigned_texture(myhero->get_spell(spellslot::r)->get_icon_texture());
				rMenu::range = rMenu->add_slider("Range", "R Range", 950, 900, 1000);
				rMenu::range->add_property_change_callback([](TreeEntry* entry) {
					r->set_range(rMenu::range->get_int());
					});
				rMenu::comboTargets = rMenu->add_slider("ComboTargets", "Min Targets in Combo (0 to disable)", 3, 0, 5);
				rMenu::semiKey = rMenu->add_hotkey("SemiKey", "Semi Key", TreeHotkeyMode::Hold, 0x54, false);
				rMenu::semiTargets = rMenu->add_slider("SemiTargets", "Min Targets for Semi Key", 2, 1, 5);
				rMenu::interrupt = rMenu->add_checkbox("Interrupt", "Use for Interrupt", true);
				rMenu::hitchance = rMenu->add_combobox("Hitchance", "Hitchance", { {"Medium", nullptr},{"High", nullptr},{"Very High", nullptr} }, 1);

			}
			auto drawMenu = mainMenuTab->add_tab("drawings", "Drawings Settings");
			{
				drawMenu::drawOnlyReady = drawMenu->add_checkbox("drawReady", "Draw Only Ready", true);
				drawMenu::drawAuraTargets = drawMenu->add_checkbox("drawP", "Draw Allies in Aura Range", true);
				drawMenu::drawRangeQ = drawMenu->add_checkbox("drawQ", "Draw Q range", true);
				drawMenu::drawRangeW = drawMenu->add_checkbox("drawW", "Draw W range", true);
				drawMenu::drawRangeE = drawMenu->add_checkbox("drawE", "Draw E range", true);
				drawMenu::drawRangeR = drawMenu->add_checkbox("drawR", "Draw R range", true);

				auto colorMenu = drawMenu->add_tab("color", "Color Settings");
				float pcolor[] = { 0.f, 1.f, 0.f, 1.f };
				colorMenu::pColor = colorMenu->add_colorpick("colorP", "Ally in Aura Color", pcolor);

				float qcolor[] = { 0.f, 0.f, 1.f, 1.f };
				colorMenu::qColor = colorMenu->add_colorpick("colorQ", "Q Range Color", qcolor);
				float wcolor[] = { 0.f, 1.f, 0.f, 1.f };
				colorMenu::wColor = colorMenu->add_colorpick("colorW", "W Range Color", wcolor);
				float ecolor[] = { 1.f, 0.f, 1.f, 1.f };
				colorMenu::eColor = colorMenu->add_colorpick("colorE", "E Range Color", ecolor);
				float rcolor[] = { 1.f, 1.f, 0.f, 1.f };
				colorMenu::rColor = colorMenu->add_colorpick("colorR", "R Range Color", rcolor);
			}


			// to set hidden on load when Adaptive Mana is still on
			{
				bool am = generalMenu::adaptiveMana->get_bool();
				qMenu::comboTargets->is_hidden() = am;
				qMenu::harassTargets->is_hidden() = am;
				qMenu::autoTargets->is_hidden() = am;
				qMenu::amplifyAA->is_hidden() = am;
				qMenu::amplifyDirect->is_hidden() = am;
				qMenu::autoMana->is_hidden() = am;
				qMenu::sepAuto->is_hidden() = am;

				wMenu::comboHealHP->is_hidden() = am;

				eMenu::comboTargets->is_hidden() = am;
			}
		}
		
		event_handler<events::on_draw>::add_callback(on_draw);
		event_handler<events::on_update>::add_callback(on_update);
		
		for (const auto& buff : myhero->get_bufflist())
		{
			if (!buff || !buff->is_valid()) continue;
			if (buff->get_hash_name() == buff_hash("HowlingAbyssAura")) isAram = true;
		}

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