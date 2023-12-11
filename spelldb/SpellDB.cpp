// Based on Omori SpellDB (https://github.com/BGXBreaker/BGXAddons)
// Feel free to join Omoris Discord at https://discord.gg/czsJYCYSn8
#include "SpellDB.h"

namespace Database
{
    auto rbuffhashes = { buff_hash("LucianR"), buff_hash("SamiraR")};
    namespace db
    {
        std::map<std::string, TreeEntry*> CanCancel;
        std::map<std::string, TreeEntry*> CanOnAllyBuff;
    }
    std::string getDisplayName(game_object_script target)
    {
        std::map<int, std::string> championNames = {
            {(int)champion_id::AurelionSol, "Aurelion Sol"},
            {(int)champion_id::Chogath, "Cho'Gath"},
            {(int)champion_id::DrMundo, "Dr. Mundo"},
            {(int)champion_id::FiddleSticks, "Fiddlesticks"},
            {(int)champion_id::JarvanIV, "Jarvan IV"},
            {(int)champion_id::Kaisa, "Kai'Sa"},
            {(int)champion_id::Khazix, "Kha'Zix"},
            {(int)champion_id::KogMaw, "Kog'Maw"},
            {(int)champion_id::KSante, "K'Sante"},
            {(int)champion_id::Leblanc, "LeBlanc"},
            {(int)champion_id::LeeSin, "Lee Sin"},
            {(int)champion_id::MasterYi, "Master Yi"},
            {(int)champion_id::MissFortune, "Miss Fortune"},
            {(int)champion_id::MonkeyKing, "Wukong"},
            {(int)champion_id::Nunu, "Nunu and Willump"},
            {(int)champion_id::RekSai, "Rek'Sai"},
            {(int)champion_id::Renata, "Renata Glasc"},
            {(int)champion_id::TahmKench, "Tahm Kench"},
            {(int)champion_id::Velkoz, "Vel'Koz"},
            {5000, "Target Dummy"},
        };


        auto id = target->get_champion();
        if (championNames.count((int)id))
            return championNames[(int)id];
        else
            return target->get_model();
    }

    void InitiateSlot(TreeTab* tab, game_object_script entity, spellslot slot, std::string name, std::string spellName, int defaultValue)
    {
        std::string key;
        void* texture;
        switch (slot)
        {
        case spellslot::q:
            key = "Q";
            texture = entity->get_spell(slot)->get_icon_texture();
            break;
        case spellslot::w:
            key = "W";
            texture = entity->get_spell(slot)->get_icon_texture();
            break;
        case spellslot::e:
            key = "E";
            texture = entity->get_spell(slot)->get_icon_texture();
            break;
        case spellslot::r:
            key = "R";
            texture = entity->get_spell(slot)->get_icon_texture();
            break;
        case spellslot::item_1:
            key = "Q1";
            texture = entity->get_spell(slot)->get_icon_texture_by_index(1);
            break;
        // Ignore because i only do interrupts, not buffs (and to make the errors go away)
        /*case (spellslot)50:
            key = "Rune";
            break;
        case (spellslot)51:
            key = "Item";
            break;*/
        case spellslot::invalid:
            key = "P";
            texture = entity->get_passive_icon_texture();
            break;
        default:
            key = "?";
            texture = entity->get_passive_icon_texture();
            break;
        }
            auto model = entity->get_model();
            auto displayName = getDisplayName(entity);
            auto id = std::to_string((int)entity->get_champion());

            auto t = tab->add_tab(model, "[" + displayName + "]");
            t->set_texture(entity->get_square_icon_portrait());


            db::CanCancel[model + key] = t->add_slider(model + key, "[" + key + "] - " + spellName, defaultValue,0,3,true);
            if (texture != nullptr)
            {
                db::CanCancel[model + key]->set_texture(texture);
            }
    }

    void InitializeCancelMenu(TreeTab* tab)
    {
        std::vector<game_object_script> enemyList = entitylist->get_enemy_heroes();

        for (auto& e : enemyList)
        {
            if (e == nullptr)
                return;

            auto id = e->get_champion();



            switch (id) {
                case champion_id::Akshan:
                    InitiateSlot(tab, e, spellslot::r, "Akshan", "Comeuppance", 2);
                    break;

                case champion_id::Briar:
                    InitiateSlot(tab, e, spellslot::e, "Briar", "Comeuppance", 0);
                    break;

                case champion_id::Caitlyn:
                    InitiateSlot(tab, e, spellslot::r, "Caitlyn", "Ace in the Hole", 3);
                    break;

                case champion_id::FiddleSticks:
                    InitiateSlot(tab, e, spellslot::w, "Fiddlesticks", "Bountiful Harvest", 1);
                    InitiateSlot(tab, e, spellslot::r, "Fiddlesticks", "Crowstorm", 3);
                    break;

                case champion_id::Galio:
                    InitiateSlot(tab, e, spellslot::w, "Galio", "Shield of Durand", 1);
                    InitiateSlot(tab, e, spellslot::r, "Galio", "Hero's Entrance", 3);
                    break;

                case champion_id::Gragas:
                    InitiateSlot(tab, e, spellslot::w, "Gragas", "Drunken Rage", 0);
                    break;

                case champion_id::Irelia:
                    InitiateSlot(tab, e, spellslot::w, "Irelia", "Defiant Dance", 0);
                    break;

                case champion_id::Janna:
                    InitiateSlot(tab, e, spellslot::r, "Janna", "Monsoon", 3);
                    break;

                case champion_id::Jhin:
                    InitiateSlot(tab, e, spellslot::r, "Jhin", "Curtain's Call", 3);
                    break;

                case champion_id::Karthus:
                    InitiateSlot(tab, e, spellslot::r, "Karthus", "Requiem", 3);
                    break;

                case champion_id::Katarina:
                    InitiateSlot(tab, e, spellslot::r, "Katarina", "Death Lotus", 3);
                    break;

                case champion_id::Kayn:
                    InitiateSlot(tab, e, spellslot::r, "Kayn", "Umbral Trespass", 0);
                    break;

                case champion_id::KSante:
                    InitiateSlot(tab, e, spellslot::w, "K'Sante", "Path Maker", 0);

                case champion_id::Lucian:
                    InitiateSlot(tab, e, spellslot::r, "Lucian", "The Culling", 3);
                    break;

                case champion_id::Malzahar:
                    InitiateSlot(tab, e, spellslot::r, "Malzahar", "Death Grasp", 3);
                    break;

                case champion_id::MasterYi:
                    InitiateSlot(tab, e, spellslot::w, "Master Yi", "Meditate", 1);
                    break;

                case champion_id::MissFortune:
                    InitiateSlot(tab, e, spellslot::r, "Miss Fortune", "Bullet Time", 3);
                    break;

                case champion_id::Naafiri:
                    InitiateSlot(tab, e, spellslot::w, "Naafiri", "Comeuppance", 1);
                    break;

                case champion_id::Nunu:
                    InitiateSlot(tab, e, spellslot::w, "Nunu and Willump", "Biggest Snowball Ever!", 2);
                    InitiateSlot(tab, e, spellslot::r, "Nunu and Willump", "Absolute Zero", 3);
                    break;

                case champion_id::Pantheon:
                    InitiateSlot(tab, e, spellslot::q, "Pantheon", "Comet Spear", 1);

                    InitiateSlot(tab, e, spellslot::r, "Pantheon", "Grand Starfall", 3);
                    break;

                case champion_id::Poppy:
                    InitiateSlot(tab, e, spellslot::r, "Poppy", "Keeper's Verdict", 3);
                    break;

                case champion_id::Pyke:
                    InitiateSlot(tab, e, spellslot::q, "Pyke", "Bone Skewer", 1);
                    break;

                case champion_id::Quinn:
                    InitiateSlot(tab, e, spellslot::r, "Quinn", "Behind Enemy Lines", 1);
                    break;

                case champion_id::Rammus:
                    InitiateSlot(tab, e, spellslot::q, "Rammus", "Powerball", 0);
                    break;

                case champion_id::Ryze:
                    InitiateSlot(tab, e, spellslot::r, "Ryze", "Realm Warp", 3);
                    break;

                case champion_id::Samira:
                    InitiateSlot(tab, e, spellslot::r, "Samira", "Inferno Trigger", 3);
                    break;

                case champion_id::Shen:
                    InitiateSlot(tab, e, spellslot::r, "Shen", "Stand United", 3);
                    break;

                case champion_id::Sion:
                    InitiateSlot(tab, e, spellslot::q, "Sion", "Decimating Smash", 2);
                    break;

                case champion_id::TahmKench:
                    InitiateSlot(tab, e, spellslot::w, "Tahm Kench", "Abyssal Dive", 2);
                    break;

                case champion_id::Taliyah:
                    InitiateSlot(tab, e, spellslot::r, "Taliyah", "Weaver's Wall", 1);
                    break;

                case champion_id::TwistedFate:
                    InitiateSlot(tab, e, spellslot::r, "Twisted Fate", "Gate", 3);
                    break;

                case champion_id::Varus:
                    InitiateSlot(tab, e, spellslot::q, "Varus", "Piercing Arrow", 0);
                    break;

                case champion_id::Velkoz:
                    InitiateSlot(tab, e, spellslot::r, "Vel'Koz", "Life Form Disintegration Ray", 4);
                    break;

                case champion_id::Vi:
                    InitiateSlot(tab, e, spellslot::q, "Vi", "Vault Breaker", 1);
                    break;

                case champion_id::Viego:
                    InitiateSlot(tab, e, spellslot::w, "Viego", "Spectral Maw", 1);
                    break;

                case champion_id::Vladimir:
                    InitiateSlot(tab, e, spellslot::e, "Vladimir", "Tides of Blood", 1);
                    break;

                case champion_id::Warwick:
                    InitiateSlot(tab, e, spellslot::q, "Warwick", "Jaws of the Beast", 0);
                    InitiateSlot(tab, e, spellslot::r, "Warwick", "Infinite Duress", 3);
                    break;

                case champion_id::Xerath:
                    InitiateSlot(tab, e, spellslot::q, "Xerath", "Arcanopulse", 1);
                    InitiateSlot(tab, e, spellslot::r, "Xerath", "Rite of the Arcane", 3);
                    break;

                case champion_id::Yuumi:
                    InitiateSlot(tab, e, spellslot::q, "Yuumi", "Prowling Projectile", 0);
                    InitiateSlot(tab, e, spellslot::r, "Yuumi", "Final Chapter", 0);
                    break;

                case champion_id::Zac:
                    InitiateSlot(tab, e, spellslot::e, "Zac", "Elastic Slingshot", 2);
                    break;

                default:
                    break;
            }
        }
        tab->add_separator("sep1", "Change Spell Importance here");
        tab->add_separator("sep2", "Based on Omori SpellDB");
    }

    int getCastingImportance(game_object_script target)
    {
        auto active = target->get_active_spell();
        if (!active)
            return false;
        auto slot = active->get_spellslot();

        std::string key;

        switch (slot)
        {
            case spellslot::q:
                key = "Q";
                break;
            case spellslot::w:
                key = "W";
                break;
            case spellslot::e:
                key = "E";
                break;
            case spellslot::r:
                key = "R";
                break;
            default:
                key = "?";
                break;
        }
        if (target->has_buff(rbuffhashes)) key = "R";

        auto entity = Database::db::CanCancel.find(target->get_model() + key);
        if (entity == Database::db::CanCancel.end())
            return 0;

        return entity->second->get_int();
    }
}