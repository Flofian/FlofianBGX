#pragma once
#include "../plugin_sdk/plugin_sdk.hpp"

namespace Database
{
	/*
	I'll try to make this as noob friendly as possible!
	If you're somewhat experienced you can skip this block of text (and probably every comment lol)

	First of all, import this (SpellDB.h) and SpellDB.cpp in your Visual Studio Project, then on your champion (or Utility for that matter) .cpp/.h file, write:
	#include "SpellDB.h"

	Then when you initalize your menu, you'll need to Initialize the Menus you need (e.g. CancelMenu, DashMenu...)
	To do that, you need to declare a TreeTab* like that:

	TreeTab *name = YourMenu->add_tab("spells", "Spells to Cancel:");

	Then call whatever menu you need; e.g.:

	Database::InitializeCancelMenu(spells);
	*/

	// Not to be used
	void InitiateSlot(TreeTab* tab, game_object_script entity, spellslot slot, std::string name, std::string spellName, int defaultValue);

	// Returns a std::string containing the display name of a champion
	// e.g. Kaisa -> Kai'Sa / Monkeyking -> Wukong
	// NOTE: Do not use this in your 'key' for menus! This contains special characters and **WILL** break menus.
	// You can use the string in the display name of the menus without any issue.
	std::string getDisplayName(game_object_script target);

	// Returns Importance via active spell
	// Possible Return Values:
	//	0 = Not Casting / Not Supported
	//	1 = Low Importance
	//	2 = Medium Importance
	//	3 = Critical Importance
	int getCastingImportance(game_object_script target);

	// Initializes Cancel Spell Menu, will only initialize for Champions Supported **and** in-game!
	void InitializeCancelMenu(TreeTab* tab);
}