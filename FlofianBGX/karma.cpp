#include "../plugin_sdk/plugin_sdk.hpp"
#include "karma.h"
namespace karma {
	script_spell* q = nullptr;
	script_spell* w = nullptr;
	script_spell* e = nullptr;
	script_spell* r = nullptr;
	script_spell* rq = nullptr;
	script_spell* qColDummy = nullptr;
	script_spell* rqColDummy = nullptr;
	TreeTab* mainMenuTab = nullptr;

	bool allowR;

	std::map<uint32_t, prediction_output> qPredList;
	std::map<uint32_t, prediction_output> rqMissPredList;
	namespace generalMenu {
		TreeEntry* debug = nullptr;
	}
	namespace qMenu {
		TreeEntry* explosionSize = nullptr;
		TreeEntry* hc = nullptr;
		TreeEntry* range = nullptr;
		TreeEntry* mode = nullptr;
		TreeEntry* allowMinion = nullptr;
	}
	namespace rqMenu {
		TreeEntry* onMultiQ = nullptr;
		TreeEntry* useIfMalignance = nullptr;
		TreeEntry* alwaysManual = nullptr;
	}
	namespace wMenu {
		TreeEntry* mode = nullptr;
		TreeEntry* rangespeedchecks = nullptr;
	}
	namespace rwMenu {
		TreeEntry* useBelow = nullptr;
		TreeEntry* useAlwaysBelow = nullptr;
	}
	namespace eMenu {
		TreeEntry* useForShield = nullptr;
		TreeEntry* shieldValue = nullptr;
		TreeEntry* includeSkillshots = nullptr;
		TreeEntry* useMSforW = nullptr;
	}
	namespace reMenu {
		TreeEntry* useForAoe = nullptr;
		TreeEntry* useForSingleBigDmg = nullptr;
	}
	namespace drawMenu {
		TreeEntry* drawOnlyReady = nullptr;
		TreeEntry* drawRangeQ = nullptr;
		TreeEntry* drawRangeW = nullptr;
		TreeEntry* drawRangeE = nullptr;

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
	}
	hit_chance get_hitchance(const int hc)
	{
		return static_cast<hit_chance>(hc + 4);
	}
	


	std::vector<game_object_script> getQExplosionTargets(vector pos) {
		std::vector<game_object_script> out;
		auto mypos = myhero->get_position();
		auto col1 = qColDummy->get_collision(myhero->get_position(), { mypos.extend(pos,q->range()) });
		std::sort(col1.begin(), col1.end(), [](game_object_script a, game_object_script b) {
			return myhero->get_distance(a) < myhero->get_distance(b);
			});
		auto point = vector();
		if (col1.size() != 0) {
			auto firstCol = col1.at(0);
			if (firstCol->get_bounding_radius() == 0) return out;	// i cba
			auto firstPos = qPredList[firstCol->get_handle()].get_unit_position();	// check pred pos
			auto projInfo = firstPos.project_on(mypos, mypos.extend(pos, q->range()));
			if (geometry::rectangle(mypos, mypos.extend(pos, q->range()), q->radius).to_polygon().is_inside(firstPos)) {
				// if center is inside, then the collision point is the distance to him minus his bounding radius ? minus spell radius?
				auto d = fmax(projInfo.line_point.distance(myhero) - firstCol->get_bounding_radius(), 0);
				point = mypos.extend(pos, d);
			}
			// somehow the collision detection is over sensitive, so it thinks it collides when in reality i just miss
			else if (geometry::rectangle(mypos, mypos.extend(pos, q->range()), q->radius + firstCol->get_bounding_radius()).to_polygon().is_inside(firstPos)) {
				//auto dist_to_center = projInfo.line_point.distance(firstPos);	// segment point, check is on segment
				auto dist_to_outside = projInfo.line_point.distance(firstPos) - q->radius;
				auto angle = std::acosf(dist_to_outside / firstCol->get_bounding_radius());
				auto offset = sin(angle) * firstCol->get_bounding_radius();
				auto d = fmax(projInfo.line_point.distance(myhero) - offset, 0);
				point = mypos.extend(pos, d);
			}
		}
		if (point != vector()) {
			auto poly = geometry::rectangle(mypos, mypos.extend(pos, q->range()), q->radius).to_polygon();
			for (int i = 0; i < poly.points.size(); i++) {
			}
			for (const auto& enemy : entitylist->get_enemy_heroes()) {
				if (!enemy || !enemy->is_valid() || !enemy->is_targetable() || enemy->is_dead() || enemy->get_distance(myhero) > 1500) continue;
				auto predpos = qPredList[enemy->get_handle()].get_unit_position();	// wrong in theory, but should be close enough i think
				if (predpos.distance(point) < qMenu::explosionSize->get_int() && enemy->is_visible()) {
					out.push_back(enemy);
				}
			}
		}
		return out;
	}
	vector canHitAll(std::vector<game_object_script> enemies) {
		std::vector<vector> positions = {};
		for (const auto& target : enemies) {
			if (rqMissPredList[target->get_handle()].hitchance < get_hitchance(qMenu::hc->get_int())) return vector();
			positions.push_back(rqMissPredList[target->get_handle()].get_unit_position());
		}
		mec_circle circle = mec::get_mec(positions);
		if (circle.radius < qMenu::explosionSize->get_int()) {
			auto endPos = myhero->get_position().extend(circle.center, 950);	// q range, but i dont want to change with menu?
			for (vector pos : positions) {
				if (endPos.distance(pos) > qMenu::explosionSize->get_int()) return vector();	// filter out if i cant hit all?
			}
			return circle.center;
		}
		return vector();
	}

	bool wChecks(game_object_script target) {
		bool faster = myhero->get_move_speed() > target->get_move_speed();
		bool close = myhero->get_distance(target) < 400;
		bool eReady = e->is_ready(0.25);

		return faster || close || eReady;
	}

	float eShieldValue(bool isR = false) {
		if (isR) return 35 + 45 * e->level() + 1.05 * myhero->get_total_ability_power() - 25 + 50 * r->level();
		return 35 + 45 * e->level() + 0.6 * myhero->get_total_ability_power();
	}
	float shieldableDmg(game_object_script ally, bool isR = false) {
		// i still dont have access to heal and shield power, and i dont think spirit visage really matters
		float incoming = health_prediction->get_incoming_damage(ally, 2.5, false);
		if (eMenu::includeSkillshots->get_bool())
			incoming += health_prediction->get_incoming_damage(ally, 0.25, true) - health_prediction->get_incoming_damage(ally, 0.25, false);
		if (!isR) return fmin(incoming, eShieldValue());
		// if r shield, main is bigger + secondary
		float shieldable = fmin(incoming, eShieldValue(true));
		for (const auto& secondary : entitylist->get_ally_heroes()) {
			if (!secondary || secondary->get_handle() == ally->get_handle() || secondary->get_distance(ally) > 675) continue;
			float inc = health_prediction->get_incoming_damage(ally, 2.5, false) +
				eMenu::includeSkillshots->get_bool() *
				(health_prediction->get_incoming_damage(ally, 0.25, true) - health_prediction->get_incoming_damage(ally, 0.25, false));
			shieldable += fmin(inc, eShieldValue(true) * 0.3);
		}
		return shieldable;

	}

	void qLogic() {
		if (!myhero->can_cast() || !q->is_ready()) return;
		bool modeCast = (qMenu::mode->get_int() == 0 && orbwalker->harass()) || (qMenu::mode->get_int() <= 1 && orbwalker->combo_mode());

		if (modeCast) {
			// rq miss + rq aoe?
			if (allowR && (r->is_ready() || myhero->has_buff(buff_hash("KarmaMantra")))) {
				std::vector<game_object_script> enemiesInRange = {};
				for (const auto& target : entitylist->get_enemy_heroes()) {
					if (target && target->is_valid() && target->get_distance(myhero) < q->range() + qMenu::explosionSize->get_int()
						&& target->get_distance(myhero) > q->range() - qMenu::explosionSize->get_int()
						&& !target->is_dead() && target->is_visible() && target->is_targetable()) {
						enemiesInRange.push_back(target);
					}
				}
				std::vector<std::vector<game_object_script>> subsets = {};
				std::vector<game_object_script> current = {};
				for (int i = 1; i < pow(2, enemiesInRange.size()); i++) {
					current.clear();
					for (int j = 0; j < enemiesInRange.size(); j++) {
						const auto& target = enemiesInRange[j];
						if ((i >> j) & 1) current.push_back(target);
					}
					subsets.push_back(current);
				}
				vector bestQRPos = vector();
				int bestQRHitcount = 0;
				for (const auto& subset : subsets) {
					vector out = canHitAll(subset);
					if (out != vector() && subset.size() > bestQRHitcount) {
						bestQRPos = out;
						bestQRHitcount = subset.size();
					}
				}
				auto b = myhero->get_buff(buff_hash("KarmaMantra"));
				if (bestQRHitcount >= rqMenu::onMultiQ->get_int() || (b && b->get_remaining_time() < 1 && bestQRHitcount > 0) || (b && rqMenu::alwaysManual->get_bool() && bestQRHitcount > 0)) {	//use if r runs out
					r->cast();
					q->cast(bestQRPos);
					console->print("Used RQMultiMiss on %i", bestQRHitcount);
				}
			}

			auto target = target_selector->get_target(q, damage_type::magical);
			if (target) {
				auto pred = qPredList[target->get_handle()];
				// q on minion / multihit
				if (qMenu::allowMinion->get_bool()) {
					auto targets = entitylist->get_enemy_heroes();
					auto t1 = entitylist->get_enemy_minions();
					auto t2 = entitylist->get_jugnle_mobs_minions();
					targets.insert(targets.end(), t1.begin(), t1.end());
					targets.insert(targets.end(), t2.begin(), t2.end());
					int bestHitCount = 0;
					vector bestHitPos = vector();
					for (const auto& enemy : targets) {
						if (!enemy || !enemy->is_valid() || !enemy->is_targetable() || enemy->is_dead() || enemy->get_distance(myhero) > 1500) continue;
						auto pred2 = qPredList[enemy->get_handle()];
						if (pred2.hitchance < get_hitchance(qMenu::hc->get_int())) continue;
						if (health_prediction->get_health_prediction(enemy, pred2.get_unit_position().distance(myhero) / q->speed + 0.25) < 0) continue;
						auto hits = getQExplosionTargets(pred2.get_cast_position());
						if (std::find_if(hits.begin(), hits.end(), [hits, target](game_object_script x) {
							return x->get_handle() == target->get_handle();
							}) != hits.end() && hits.size() > bestHitCount) {
							bestHitCount = hits.size();
							bestHitPos = pred2.get_cast_position();

						}

					}
					if (bestHitCount > 1 && rqMenu::onMultiQ->get_bool() && allowR) {
						r->cast();
						q->cast(bestHitPos);
						return;
					}
					else if (bestHitCount > 0) {
						q->cast(bestHitPos);
						return;
					}
				}
				// normal q + malignance
				if (pred.hitchance >= get_hitchance(qMenu::hc->get_int())) {
					if (myhero->has_item(ItemId::Malignance) != spellslot::invalid && r->is_ready() && allowR) r->cast();
					q->cast(pred.get_cast_position());
					return;
				}

			}
		}
	}
	void wLogic() {
		if (!myhero->can_cast() || !w->is_ready()) return;
		bool modeCast = (wMenu::mode->get_int() == 0 && orbwalker->harass()) || (wMenu::mode->get_int() <= 1 && orbwalker->combo_mode());
		auto target = target_selector->get_target(w, damage_type::magical);
		if (!target)  return;
		bool checks = wChecks(target);
		bool useR = myhero->get_health_percent() <= rwMenu::useBelow->get_int() ;
		if (modeCast && (checks || !wMenu::rangespeedchecks->get_bool() || useR)) {
			if (useR) r->cast();
			w->cast(target);
		}
	}
	void eLogic() {
		if (!myhero->can_cast() || !e->is_ready()) return;
		float minShieldValue = eShieldValue() * eMenu::shieldValue->get_int() / 100;
		// re aoe
		if (r->is_ready() && allowR && reMenu::useForAoe->get_bool()) {
			float bestShieldVal = 0;
			game_object_script bestTarget;
			for (const auto& mainTarget : entitylist->get_ally_heroes()) {
				if (!mainTarget || !mainTarget->is_targetable_to_team(myhero->get_team()) || mainTarget->get_distance(myhero) > e->range()) continue;
				float currentShield = shieldableDmg(mainTarget, true);
				if (currentShield > bestShieldVal) {
					bestShieldVal = currentShield;
					bestTarget = mainTarget;
				}
			}
			if (bestShieldVal > minShieldValue * 2) {		// the 2 is arbitrary, i just want that the shield is useful enough
				r->cast();
				e->cast(bestTarget);
				console->print("Used RE AOE for %f dmg", bestShieldVal);
				return;
			}
		}


		float maxDmg = 0;
		game_object_script target;
		// normal e
		for (const auto& ally : entitylist->get_ally_heroes()) {
			if (!ally || !ally->is_targetable_to_team(myhero->get_team()) || ally->get_distance(myhero) > e->range()) continue;
			float incomingDmg = health_prediction->get_incoming_damage(ally, 2.5, false);
			// to only get the skillshots, i only want them in a short time, to not shield stuff that wont hit
			if (eMenu::includeSkillshots->get_bool())
				incomingDmg += health_prediction->get_incoming_damage(ally, 0.25, true) - health_prediction->get_incoming_damage(ally, 0.25, false);
			if (incomingDmg > maxDmg) {
				maxDmg = incomingDmg;
				target = ally;
			}
		}
		if (maxDmg > minShieldValue && target) {
			// r bigSingle
			if (r->is_ready() && allowR && reMenu::useForSingleBigDmg->get_bool() &&
				maxDmg > minShieldValue - 25 + 50 * r->level() + 0.45 * myhero->get_total_ability_power()) {
				r->cast();
				console->print("Used RE Single for %f dmg", maxDmg);
			}
			e->cast(target);
			return;
		}

		// e for w movespeed
		for (const auto& enemy : entitylist->get_enemy_heroes()) {
			auto b = enemy->get_buff(buff_hash("KarmaSpiritBind"));
			if (b && b->get_caster() && b->get_caster()->get_handle() == myhero->get_handle()) {
				if (enemy->get_distance(myhero) > 700 && eMenu::useMSforW->get_bool()) e->cast(myhero);
				console->print("Used E For W MS");
			}
		}

	}

	void calcs() {
		allowR = myhero->get_health_percent() >= rwMenu::useAlwaysBelow->get_bool();
		// i am not sure if this is the right way to do this? for the minions
		auto targets = entitylist->get_enemy_heroes();
		auto t1 = entitylist->get_enemy_minions();
		auto t2 = entitylist->get_jugnle_mobs_minions();
		targets.insert(targets.end(), t1.begin(), t1.end());
		targets.insert(targets.end(), t2.begin(), t2.end());
		for (const auto& enemy : targets) {
			if (!enemy || !enemy->is_valid() || !enemy->is_targetable() || enemy->is_dead() || enemy->get_distance(myhero) > 1500) continue;
			qPredList[enemy->get_handle()] = q->get_prediction(enemy, false, -1, { collisionable_objects::minions, collisionable_objects::yasuo_wall, collisionable_objects::heroes });
			if (enemy->is_ai_hero()) rqMissPredList[enemy->get_handle()] = prediction->get_prediction(enemy, 0.8); // 0.25 delay + 0.55 travel time
		}

	}

	void on_update() {
		calcs();
		qLogic();
		wLogic();
		eLogic();
	}
	void on_draw() {
		if (!generalMenu::debug->get_bool()) return;
		auto mouse = hud->get_hud_input_logic()->get_game_cursor_position();
		auto mypos = myhero->get_position();

		/*prediction_input pin1 = prediction_input();
		pin1.collision_objects = { collisionable_objects::heroes, collisionable_objects::minions };
		pin1.delay = 0.25;
		pin1.range = q->range();
		pin1.radius = q->radius;
		pin1.speed = q->speed;
		prediction_input pin2 = pin1;
		pin2.radius = 160;*/
		auto col1 = qColDummy->get_collision(myhero->get_position(), { mypos.extend(mouse,q->range()) });
		auto col2 = rqColDummy->get_collision(myhero->get_position(), { mypos.extend(mouse,q->range()) });
		//auto col1 = prediction->get_collision({ mypos.extend(mouse,q->range())}, &pin1);
		//auto col2 = prediction->get_collision({ mypos.extend(mouse,q->range()) }, &pin2);
		// not sure if sorting is needed or if thats true by default
		std::sort(col1.begin(), col1.end(), [](game_object_script a, game_object_script b) {
			return myhero->get_distance(a) < myhero->get_distance(b);
			});
		std::sort(col2.begin(), col2.end(), [](game_object_script a, game_object_script b) {
			return myhero->get_distance(a) < myhero->get_distance(b);
			});
		auto point = vector();
		if (col1.size() != 0) {
			auto firstCol = col1.at(0);
			if (firstCol->get_bounding_radius() == 0) return;	// i cba
			auto firstPos = firstCol->get_position();	// check pred pos
			auto projInfo = firstPos.project_on(mypos, mypos.extend(mouse, q->range()));
			if (geometry::rectangle(mypos, mypos.extend(mouse, q->range()), q->radius).to_polygon().is_inside(firstPos)) {
				// if center is inside, then the collision point is the distance to him minus his bounding radius ? minus spell radius?
				auto d = fmax(projInfo.line_point.distance(myhero) - firstCol->get_bounding_radius(), 0);
				point = mypos.extend(mouse, d);
				draw_manager->add_circle(point, 20, MAKE_COLOR(255, 255, 0, 255), 2);
				draw_manager->add_line(mypos, mypos.extend(mouse, q->range()), MAKE_COLOR(255, 255, 255, 255));
			}
			// somehow the collision detection is over sensitive, so it thinks it collides when in reality i just miss
			else if (geometry::rectangle(mypos, mypos.extend(mouse, q->range()), q->radius + firstCol->get_bounding_radius()).to_polygon().is_inside(firstPos)) {
				//auto dist_to_center = projInfo.line_point.distance(firstPos);	// segment point, check is on segment
				draw_manager->add_circle(projInfo.line_point, 20, MAKE_COLOR(255, 0, 255, 255), 2);
				auto dist_to_outside = projInfo.line_point.distance(firstPos) - q->radius;
				draw_manager->add_line(firstPos, firstPos.extend(projInfo.line_point, dist_to_outside), MAKE_COLOR(255, 255, 255, 255));
				auto angle = std::acosf(dist_to_outside / firstCol->get_bounding_radius());
				// i hate my life
				//auto a1 = (mypos - firstPos).polar();
				//auto a2 = (mypos - projInfo.line_point).polar();
				//if (a1 - a2 > 0) angle = -angle;	// not needed with cause i just check distance 
				//auto dir = (projInfo.line_point - firstPos).normalized();
				auto offset = sin(angle) * firstCol->get_bounding_radius();
				auto d = fmax(projInfo.line_point.distance(myhero) - offset, 0);
				point = mypos.extend(mouse, d);

				//point = firstPos + (dir.rotated(angle) * firstCol->get_bounding_radius());
				draw_manager->add_circle(point, 20, MAKE_COLOR(255, 0, 0, 255), 2);
			}
		}
		if (point != vector()) {
			auto poly = geometry::rectangle(mypos, mypos.extend(mouse, q->range()), q->radius).to_polygon();
			for (int i = 0; i < poly.points.size(); i++) {
				draw_manager->add_line(poly.points[i], poly.points[(i + 1) % poly.points.size()], MAKE_COLOR(255, 255, 255, 255));
			}
			draw_manager->add_circle(point, qMenu::explosionSize->get_int(), MAKE_COLOR(0, 0, 255, 255), 2);
			for (const auto& enemy : entitylist->get_enemy_heroes()) {
				if (enemy->get_distance(point) < qMenu::explosionSize->get_int() && enemy->is_visible()) {
					draw_manager->add_circle(enemy->get_position(), enemy->get_bounding_radius(), MAKE_COLOR(0, 255, 0, 255), 2);
				}
			}
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
		}
		else {
			if ((q->is_ready() || !drawMenu::drawOnlyReady->get_bool()) && drawMenu::drawRangeQ->get_bool())
				draw_manager->add_circle_with_glow(myhero->get_position(), colorMenu::qColor->get_color(), q->range(), lt, glow);
			if ((w->is_ready() || !drawMenu::drawOnlyReady->get_bool()) && drawMenu::drawRangeW->get_bool())
				draw_manager->add_circle_with_glow(myhero->get_position(), colorMenu::wColor->get_color(), w->range(), lt, glow);
			if ((e->is_ready() || !drawMenu::drawOnlyReady->get_bool()) && drawMenu::drawRangeE->get_bool())
				draw_manager->add_circle_with_glow(myhero->get_position(), colorMenu::eColor->get_color(), e->range(), lt, glow);
		}
	}


	void load() {
		q = plugin_sdk->register_spell(spellslot::q, 950);
		rq = plugin_sdk->register_spell(spellslot::q, 950);
		qColDummy = plugin_sdk->register_spell(spellslot::q, 950);
		rqColDummy = plugin_sdk->register_spell(spellslot::q, 950);
		w = plugin_sdk->register_spell(spellslot::w, 675);
		e = plugin_sdk->register_spell(spellslot::e, 800);
		r = plugin_sdk->register_spell(spellslot::r, 0);
		q->set_skillshot(0.25, 60, 1700, { collisionable_objects::yasuo_wall }, skillshot_type::skillshot_line);
		rq->set_skillshot(0.25, 80, 1700, { collisionable_objects::yasuo_wall }, skillshot_type::skillshot_line);
		qColDummy->set_skillshot(0.25, 60, 1700, { collisionable_objects::minions, collisionable_objects::heroes }, skillshot_type::skillshot_line);
		rqColDummy->set_skillshot(0.25, 80, 1700, { collisionable_objects::minions, collisionable_objects::heroes }, skillshot_type::skillshot_line);

		mainMenuTab = menu->create_tab("FlofianKarma", "Flofian Karma");
		mainMenuTab->set_assigned_texture(myhero->get_square_icon_portrait());

		{
			auto generalMenu = mainMenuTab->add_tab("general", "General Settings");
			{
				generalMenu::debug = generalMenu->add_checkbox("debug", "Debug", false);
			}
			generalMenu->is_hidden() = true;
			auto qMenu = mainMenuTab->add_tab("q", "Q Settings");
			{
				qMenu->set_assigned_texture(myhero->get_spell(spellslot::q)->get_icon_texture_by_index(0));
				qMenu::explosionSize = qMenu->add_slider("explosionSize", "[Pred] Explosion Size", 250, 200, 280);
				qMenu::hc = qMenu->add_combobox("Hitchance", "[Pred] Hitchance", { {"Medium", nullptr},{"High", nullptr},{"Very High", nullptr} }, 2);
				qMenu::range = qMenu->add_slider("range", "[Pred] Range", 900, 750, 950);
				qMenu::range->add_property_change_callback([](TreeEntry* entry) {
					q->set_range(entry->get_int());
					});
				q->set_range(qMenu::range->get_int());
				qMenu::mode = qMenu->add_combobox("mode", "Q Mode", { {"Combo + Harass", nullptr},{"Combo", nullptr}, {"Off", nullptr} }, 0);
				qMenu::allowMinion = qMenu->add_checkbox("allowMinion", "Allow Q on Minion", true);
			}
			auto rqMenu = mainMenuTab->add_tab("rq", "RQ Settings");
			{
				rqMenu->set_assigned_texture(myhero->get_spell(spellslot::q)->get_icon_texture_by_index(1));
				rqMenu::onMultiQ = rqMenu->add_slider("onMultiQ", "Use if Q can hit x enemies", 2, 1, 5);
				rqMenu::alwaysManual = rqMenu->add_checkbox("alwaysManual", " ^ Ignore if manual R", false);
				rqMenu::useIfMalignance = rqMenu->add_checkbox("useIfMalignance", "Use more often if you have malignance", true);
			}
			auto wMenu = mainMenuTab->add_tab("w", "W Settings");
			{
				wMenu->set_assigned_texture(myhero->get_spell(spellslot::w)->get_icon_texture_by_index(0));
				wMenu::mode = wMenu->add_combobox("mode", "W Mode", { {"Combo + Harass", nullptr},{"Combo", nullptr}, {"Off", nullptr} }, 0);
				wMenu::rangespeedchecks = wMenu->add_checkbox("rangespeedchecks", "^Additional Checks", true);
				wMenu::rangespeedchecks->set_tooltip("Only used when you are faster than the enemy, when the enemy is close, or when you have E ready");
			}
			auto rwMenu = mainMenuTab->add_tab("rw", "RW Settings");
			{
				rwMenu->set_assigned_texture(myhero->get_spell(spellslot::w)->get_icon_texture_by_index(1));
				rwMenu::useBelow = rwMenu->add_slider("useBelow", "Use if below x% HP", 50, 0, 100);
				rwMenu::useAlwaysBelow = rwMenu->add_slider("useAlwaysBelow", "Dont use R for anything else if below x% HP", 20, 0, 100);
			}
			auto eMenu = mainMenuTab->add_tab("e", "E Settings");
			{
				eMenu->set_assigned_texture(myhero->get_spell(spellslot::e)->get_icon_texture_by_index(0));
				eMenu::useForShield = eMenu->add_checkbox("useShield", "Use to Shield", true);
				eMenu::shieldValue = eMenu->add_slider("shieldvalue", "If DMG > Shield * X%", 100, 50, 150);
				eMenu::includeSkillshots = eMenu->add_checkbox("includeSkillshots", "Include Skillshots", true);
				eMenu::useMSforW = eMenu->add_checkbox("useMSforW", "Use for movespeed for W", true);
			}
			auto reMenu = mainMenuTab->add_tab("re", "RE Settings");
			{
				reMenu->set_assigned_texture(myhero->get_spell(spellslot::e)->get_icon_texture_by_index(1));
				reMenu::useForAoe = reMenu->add_checkbox("useAoe", "Use for AOE Shield", true);
				reMenu::useForSingleBigDmg = reMenu->add_checkbox("useSingleBigDmg", "Use for Big Shield", true);
			}
			auto drawMenu = mainMenuTab->add_tab("drawings", "Drawings Settings");
			{
				drawMenu::drawOnlyReady = drawMenu->add_checkbox("drawReady", "Draw Only Ready", true);
				drawMenu::drawRangeQ = drawMenu->add_checkbox("drawQ", "Draw Q range", true);
				drawMenu::drawRangeW = drawMenu->add_checkbox("drawW", "Draw W range", true);
				drawMenu::drawRangeE = drawMenu->add_checkbox("drawE", "Draw E range", true);


				drawMenu::drawRangeQ->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture_by_index(0));
				drawMenu::drawRangeW->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture_by_index(0));
				drawMenu::drawRangeE->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture_by_index(0));

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

				float qcolor[] = { 0.f, 1.f, 0.f, 1.f };
				colorMenu::qColor = colorMenu->add_colorpick("colorQ", "Q Range Color", qcolor);
				float wcolor[] = { 1.f, 0.5f, 0.f, 1.f };
				colorMenu::wColor = colorMenu->add_colorpick("colorW", "W Range Color", wcolor);
				float ecolor[] = { 1.f, 1.f, 0.f, 1.f };
				colorMenu::eColor = colorMenu->add_colorpick("colorE", "E Range Color", ecolor);
			}
		}

		event_handler<events::on_update>::add_callback(on_update);
		event_handler<events::on_draw>::add_callback(on_draw);
		event_handler<events::on_env_draw>::add_callback(on_env_draw);

	}

	void unload() {
		plugin_sdk->remove_spell(q);
		plugin_sdk->remove_spell(w);
		plugin_sdk->remove_spell(e);
		plugin_sdk->remove_spell(r);
		menu->delete_tab(mainMenuTab);

		event_handler<events::on_update>::remove_handler(on_update);
		event_handler<events::on_draw>::remove_handler(on_draw);
		event_handler<events::on_env_draw>::remove_handler(on_env_draw);
	}
}