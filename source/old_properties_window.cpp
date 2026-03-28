//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#include "main.h"

#include <wx/grid.h>

#include "tile.h"
#include "item.h"
#include "complexitem.h"
#include "town.h"
#include "house.h"
#include "map.h"
#include "editor.h"
#include "creature.h"

#include "gui.h"
#include "application.h"
#include "old_properties_window.h"
#include "container_properties_window.h"
#include "iomap_sec.h"
#include <cmath>

// ============================================================================
// Old Properties Window

enum { RACE_ID_SPIN = wxID_HIGHEST + 100, SEC_RADIUS_SPIN = wxID_HIGHEST + 101 };

BEGIN_EVENT_TABLE(OldPropertiesWindow, wxDialog)
	EVT_SET_FOCUS(OldPropertiesWindow::OnFocusChange)
	EVT_BUTTON(wxID_OK, OldPropertiesWindow::OnClickOK)
	EVT_BUTTON(wxID_CANCEL, OldPropertiesWindow::OnClickCancel)
	EVT_SPINCTRL(RACE_ID_SPIN, OldPropertiesWindow::OnRaceIdChanged)
	EVT_SPINCTRL(SEC_RADIUS_SPIN, OldPropertiesWindow::OnSecRadiusChanged)
END_EVENT_TABLE()

OldPropertiesWindow::OldPropertiesWindow(wxWindow* win_parent, const Map* map, const Tile* tile_parent, Item* item, wxPoint pos) :
	ObjectPropertiesWindowBase(win_parent, "Item Properties", map, tile_parent, item, pos),
	count_field(nullptr),
	direction_field(nullptr),
	action_id_field(nullptr),
	unique_id_field(nullptr),
	door_id_field(nullptr),
	depot_id_field(nullptr),
	splash_type_field(nullptr),
	text_field(nullptr),
	description_field(nullptr),
	destination_field(nullptr)
{
	ASSERT(edit_item);

	wxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);
	if(Container* container = dynamic_cast<Container*>(edit_item)) {
		// Container
		wxSizer* boxsizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Container Properties");

		wxFlexGridSizer* subsizer = newd wxFlexGridSizer(2, 10, 10);
		subsizer->AddGrowableCol(1);

		addItemIdRows(subsizer, item);

		subsizer->Add(newd wxStaticText(this, wxID_ANY, "Action ID"));
		action_id_field = newd wxSpinCtrl(this, wxID_ANY, i2ws(edit_item->getActionID()), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 0xFFFF, edit_item->getActionID());
		subsizer->Add(action_id_field, wxSizerFlags(1).Expand());

		subsizer->Add(newd wxStaticText(this, wxID_ANY, "Unique ID"));
		unique_id_field = newd wxSpinCtrl(this, wxID_ANY, i2ws(edit_item->getUniqueID()), wxDefaultPosition, wxSize(-1, 20), wxSP_ARROW_KEYS, 0, 0xFFFF, edit_item->getUniqueID());
		subsizer->Add(unique_id_field, wxSizerFlags(1).Expand());

		boxsizer->Add(subsizer, wxSizerFlags(0).Expand());

		// Now we add the subitems!
		wxSizer* contents_sizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Contents");

		bool use_large_sprites = g_settings.getBoolean(Config::USE_LARGE_CONTAINER_ICONS);
		wxSizer* horizontal_sizer = nullptr;
		const int additional_height_increment = (use_large_sprites? 40 : 24);
		int additional_height = 0;

		int32_t maxColumns;
		if(use_large_sprites) {
			maxColumns = 6;
		} else {
			maxColumns = 12;
		}

		for(uint32_t index = 0; index < container->getVolume(); ++index) {
			if(!horizontal_sizer) {
				horizontal_sizer = newd wxBoxSizer(wxHORIZONTAL);
			}

			Item* item = container->getItem(index);
			ContainerItemButton* containerItemButton = newd ContainerItemButton(this, use_large_sprites, index, map, item);

			container_items.push_back(containerItemButton);
			horizontal_sizer->Add(containerItemButton);

			if(((index + 1) % maxColumns) == 0) {
				contents_sizer->Add(horizontal_sizer);
				horizontal_sizer = nullptr;
				additional_height += additional_height_increment;
			}
		}

		if(horizontal_sizer != nullptr) {
			contents_sizer->Add(horizontal_sizer);
			additional_height += additional_height_increment;
		}

		boxsizer->Add(contents_sizer, wxSizerFlags(2).Expand());

		topsizer->Add(boxsizer, wxSizerFlags(0).Expand().Border(wxALL, 20));

		//SetSize(260, 150 + additional_height);
	} else if(edit_item->canHoldText() || edit_item->canHoldDescription()) {
		// Book
		wxSizer* boxsizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Writeable Properties");

		wxFlexGridSizer* subsizer = newd wxFlexGridSizer(2, 10, 10);
		subsizer->AddGrowableCol(1);

		addItemIdRows(subsizer, item);

		subsizer->Add(newd wxStaticText(this, wxID_ANY, "Action ID"));
		action_id_field = newd wxSpinCtrl(this, wxID_ANY, i2ws(edit_item->getActionID()), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 0xFFFF, edit_item->getActionID());
		action_id_field->SetSelection(-1, -1);
		subsizer->Add(action_id_field, wxSizerFlags(1).Expand());

		subsizer->Add(newd wxStaticText(this, wxID_ANY, "Unique ID"));
		unique_id_field = newd wxSpinCtrl(this, wxID_ANY, i2ws(edit_item->getUniqueID()), wxDefaultPosition, wxSize(-1, 20), wxSP_ARROW_KEYS, 0, 0xFFFF, edit_item->getUniqueID());
		subsizer->Add(unique_id_field, wxSizerFlags(1).Expand());

		boxsizer->Add(subsizer, wxSizerFlags(1).Expand());

		wxSizer* textsizer = newd wxBoxSizer(wxVERTICAL);
		textsizer->Add(newd wxStaticText(this, wxID_ANY, "Text"), wxSizerFlags(1).Center());
		text_field = newd wxTextCtrl(this, wxID_ANY, wxstr(item->getText()), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
		textsizer->Add(text_field, wxSizerFlags(7).Expand());

		boxsizer->Add(textsizer, wxSizerFlags(2).Expand());

		topsizer->Add(boxsizer, wxSizerFlags(0).Expand().Border(wxALL, 20));

		//SetSize(220, 310);
	} else if(edit_item->isSplash() || edit_item->isFluidContainer()) {
		// Splash
		wxSizer* boxsizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Splash Properties");

		wxFlexGridSizer* subsizer = newd wxFlexGridSizer(2, 10, 10);
		subsizer->AddGrowableCol(1);

		addItemIdRows(subsizer, item);

		subsizer->Add(newd wxStaticText(this, wxID_ANY, "Type"));

		// Splash types
		splash_type_field = newd wxChoice(this, wxID_ANY);
		if(edit_item->isFluidContainer()) {
			splash_type_field->Append(wxstr(Item::LiquidID2Name(LIQUID_NONE)), newd int32_t(LIQUID_NONE));
		}

		for(SplashType splashType = LIQUID_FIRST; splashType != LIQUID_LAST; ++splashType) {
			splash_type_field->Append(wxstr(Item::LiquidID2Name(splashType)), newd int32_t(splashType));
		}

		if(item->getSubtype()) {
			const std::string& what = Item::LiquidID2Name(item->getSubtype());
			if(what == "Unknown") {
				splash_type_field->Append(wxstr(Item::LiquidID2Name(LIQUID_NONE)), newd int32_t(LIQUID_NONE));
			}
			splash_type_field->SetStringSelection(wxstr(what));
		} else {
			splash_type_field->SetSelection(0);
		}

		subsizer->Add(splash_type_field, wxSizerFlags(1).Expand());

		subsizer->Add(newd wxStaticText(this, wxID_ANY, "Action ID"));
		action_id_field = newd wxSpinCtrl(this, wxID_ANY, i2ws(edit_item->getActionID()), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 0xFFFF, edit_item->getActionID());
		subsizer->Add(action_id_field, wxSizerFlags(1).Expand());

		subsizer->Add(newd wxStaticText(this, wxID_ANY, "Unique ID"));
		unique_id_field = newd wxSpinCtrl(this, wxID_ANY, i2ws(edit_item->getUniqueID()), wxDefaultPosition, wxSize(-1, 20), wxSP_ARROW_KEYS, 0, 0xFFFF, edit_item->getUniqueID());
		subsizer->Add(unique_id_field, wxSizerFlags(1).Expand());

		boxsizer->Add(subsizer, wxSizerFlags(1).Expand());

		topsizer->Add(boxsizer, wxSizerFlags(0).Expand().Border(wxALL, 20));

		//SetSize(220, 190);
	} else if(Depot* depot = dynamic_cast<Depot*>(edit_item)) {
		// Depot
		wxSizer* boxsizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Depot Properties");
		wxFlexGridSizer* subsizer = newd wxFlexGridSizer(2, 10, 10);

		subsizer->AddGrowableCol(1);
		addItemIdRows(subsizer, item);

		const Towns& towns = map->towns;
		subsizer->Add(newd wxStaticText(this, wxID_ANY, "Depot ID"));
		depot_id_field = newd wxChoice(this, wxID_ANY);
		int to_select_index = 0;
		if(towns.count() > 0) {
			bool found = false;
			for(TownMap::const_iterator town_iter = towns.begin();
					town_iter != towns.end();
					++town_iter)
			{
				if(town_iter->second->getID() == depot->getDepotID()) {
					found = true;
				}
				depot_id_field->Append(wxstr(town_iter->second->getName()), newd int(town_iter->second->getID()));
				if(!found) ++to_select_index;
			}
			if(!found) {
				if(depot->getDepotID() != 0) {
					depot_id_field->Append("Undefined Town (id:" + i2ws(depot->getDepotID()) + ")", newd int(depot->getDepotID()));
				}
			}
		}
		depot_id_field->Append("No Town", newd int(0));
		if(depot->getDepotID() == 0) {
			to_select_index = depot_id_field->GetCount() - 1;
		}
		depot_id_field->SetSelection(to_select_index);

		subsizer->Add(depot_id_field, wxSizerFlags(1).Expand());

		boxsizer->Add(subsizer, wxSizerFlags(5).Expand());
		topsizer->Add(boxsizer, wxSizerFlags(0).Expand().Border(wxALL, 20));
		//SetSize(220, 140);
	} else {
		// Normal item
		Door* door = dynamic_cast<Door*>(edit_item);
		Teleport* teleport = dynamic_cast<Teleport*>(edit_item);

		wxString description;
		if(door) {
			ASSERT(tile_parent);
			description = "Door Properties";
		} else if(teleport) {
			description = "Teleport Properties";
		} else {
			description = "Item Properties";
		}

		wxSizer* boxsizer = newd wxStaticBoxSizer(wxVERTICAL, this, description);

		int num_items = 4;
		//if(item->canHoldDescription()) num_items += 1;
		if(door) num_items += 1;
		if(teleport) num_items += 1;

		wxFlexGridSizer* subsizer = newd wxFlexGridSizer(2, 10, 10);
		subsizer->AddGrowableCol(1);

		addItemIdRows(subsizer, item);

		subsizer->Add(newd wxStaticText(this, wxID_ANY, (item->isCharged()? "Charges" : "Count")));
		int max_count = 100;
		if(item->isClientCharged()) max_count = 250;
		if(item->isExtraCharged()) max_count = 65500;
		count_field = newd wxSpinCtrl(this, wxID_ANY, i2ws(edit_item->getCount()), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, max_count, edit_item->getCount());
		if(!item->isStackable() && !item->isCharged()) {
			count_field->Enable(false);
		}
		subsizer->Add(count_field, wxSizerFlags(1).Expand());

		subsizer->Add(newd wxStaticText(this, wxID_ANY, "Action ID"));
		action_id_field = newd wxSpinCtrl(this, wxID_ANY, i2ws(edit_item->getActionID()), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 0xFFFF, edit_item->getActionID());
		subsizer->Add(action_id_field, wxSizerFlags(1).Expand());

		subsizer->Add(newd wxStaticText(this, wxID_ANY, "Unique ID"));
		unique_id_field = newd wxSpinCtrl(this, wxID_ANY, i2ws(edit_item->getUniqueID()), wxDefaultPosition, wxSize(-1, 20), wxSP_ARROW_KEYS, 0, 0xFFFF, edit_item->getUniqueID());
		subsizer->Add(unique_id_field, wxSizerFlags(1).Expand());

		/*
		if(item->canHoldDescription()) {
			subsizer->Add(newd wxStaticText(this, wxID_ANY, "Description"));
			description_field = newd wxTextCtrl(this, wxID_ANY, edit_item->getText(), wxDefaultPosition, wxSize(-1, 20));
			subsizer->Add(description_field, wxSizerFlags(1).Expand());
		}
		*/

		if(door) {
			subsizer->Add(newd wxStaticText(this, wxID_ANY, "Door ID"));
			door_id_field = newd wxSpinCtrl(this, wxID_ANY, i2ws(door->getDoorID()), wxDefaultPosition, wxSize(-1, 20), wxSP_ARROW_KEYS, 0, 0xFF, door->getDoorID());
			if(!edit_tile || !edit_tile->isHouseTile()) {
				door_id_field->Disable();
			}
			subsizer->Add(door_id_field, wxSizerFlags(1).Expand());
		}

		boxsizer->Add(subsizer, wxSizerFlags(1).Expand());
		topsizer->Add(boxsizer, wxSizerFlags(0).Expand().Border(wxLEFT | wxRIGHT, 20));

		if(teleport) {
			destination_field = new PositionCtrl(this, "Destination", teleport->getX(), teleport->getY(), teleport->getZ(), map->getWidth(), map->getHeight());
			topsizer->Add(destination_field, wxSizerFlags(0).Expand().Border(wxLEFT | wxRIGHT, 20));
		}
	}

	// Others attributes
	const ItemType& type = g_items.getItemType(edit_item->getID());
	wxStaticBoxSizer* others_sizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Others");
	wxFlexGridSizer* others_subsizer = newd wxFlexGridSizer(2, 5, 10);
	others_subsizer->AddGrowableCol(1);
	others_subsizer->Add(newd wxStaticText(this, wxID_ANY, "Stackable"));
	others_subsizer->Add(newd wxStaticText(this, wxID_ANY, b2yn(type.stackable)));
	others_subsizer->Add(newd wxStaticText(this, wxID_ANY, "Movable"));
	others_subsizer->Add(newd wxStaticText(this, wxID_ANY, b2yn(type.moveable)));
	others_subsizer->Add(newd wxStaticText(this, wxID_ANY, "Pickupable"));
	others_subsizer->Add(newd wxStaticText(this, wxID_ANY, b2yn(type.pickupable)));
	others_subsizer->Add(newd wxStaticText(this, wxID_ANY, "Hangable"));
	others_subsizer->Add(newd wxStaticText(this, wxID_ANY, b2yn(type.isHangable)));
	others_subsizer->Add(newd wxStaticText(this, wxID_ANY, "Block Missiles"));
	others_subsizer->Add(newd wxStaticText(this, wxID_ANY, b2yn(type.blockMissiles)));
	others_subsizer->Add(newd wxStaticText(this, wxID_ANY, "Block Pathfinder"));
	others_subsizer->Add(newd wxStaticText(this, wxID_ANY, b2yn(type.blockPathfinder)));
	others_subsizer->Add(newd wxStaticText(this, wxID_ANY, "Has Elevation"));
	others_subsizer->Add(newd wxStaticText(this, wxID_ANY, b2yn(type.hasElevation)));
	others_sizer->Add(others_subsizer, wxSizerFlags(1).Expand());
	topsizer->Add(others_sizer, wxSizerFlags(0).Expand().Border(wxLEFT | wxRIGHT | wxBOTTOM, 20));

	wxSizer* subsizer = newd wxBoxSizer(wxHORIZONTAL);
	subsizer->Add(newd wxButton(this, wxID_OK, "OK"), wxSizerFlags(1).Center());
	subsizer->Add(newd wxButton(this, wxID_CANCEL, "Cancel"), wxSizerFlags(1).Center());
	topsizer->Add(subsizer, wxSizerFlags(0).Center().Border(wxLEFT | wxRIGHT | wxBOTTOM, 20));

	SetSizerAndFit(topsizer);
	Centre(wxBOTH);
}

OldPropertiesWindow::OldPropertiesWindow(wxWindow* win_parent, const Map* map, const Tile* tile_parent, Creature* creature, wxPoint pos) :
	ObjectPropertiesWindowBase(win_parent, "Creature Properties", map, tile_parent, creature, pos),
	count_field(nullptr),
	direction_field(nullptr),
	action_id_field(nullptr),
	unique_id_field(nullptr),
	door_id_field(nullptr),
	depot_id_field(nullptr),
	splash_type_field(nullptr),
	text_field(nullptr),
	description_field(nullptr),
	race_id_field(nullptr),
	creature_name_label(nullptr),
	amount_field(nullptr),
	sec_radius_field(nullptr),
	visual_radius_label(nullptr),
	destination_field(nullptr)
{
	ASSERT(edit_creature);

	int currentRaceId = edit_creature->getRaceID();

	wxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);
	wxSizer* boxsizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Creature Properties");

	wxFlexGridSizer* subsizer = newd wxFlexGridSizer(2, 10, 10);
	subsizer->AddGrowableCol(1);

	// Race ID (editable)
	subsizer->Add(newd wxStaticText(this, wxID_ANY, "Race ID"));
	race_id_field = newd wxSpinCtrl(this, RACE_ID_SPIN, i2ws(currentRaceId), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 9999, currentRaceId);
	subsizer->Add(race_id_field, wxSizerFlags(1).Expand());

	// Creature name (read-only, updates with race ID)
	subsizer->Add(newd wxStaticText(this, wxID_ANY, "Name"));
	creature_name_label = newd wxStaticText(this, wxID_ANY, "\"" + wxstr(edit_creature->getName()) + "\"");
	subsizer->Add(creature_name_label, wxSizerFlags(1).Expand());

	subsizer->Add(newd wxStaticText(this, wxID_ANY, "Spawn time"));
	count_field = newd wxSpinCtrl(this, wxID_ANY, i2ws(edit_creature->getSpawnTime()), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 99999, edit_creature->getSpawnTime());
	subsizer->Add(count_field, wxSizerFlags(1).Expand());

	subsizer->Add(newd wxStaticText(this, wxID_ANY, "Direction"));
	direction_field = newd wxChoice(this, wxID_ANY);

	for(Direction dir = DIRECTION_FIRST; dir <= DIRECTION_LAST; ++dir) {
		direction_field->Append(wxstr(Creature::DirID2Name(dir)), newd int32_t(dir));
	}
	direction_field->SetSelection(edit_creature->getDirection());
	subsizer->Add(direction_field, wxSizerFlags(1).Expand());

	boxsizer->Add(subsizer, wxSizerFlags(1).Expand());

	topsizer->Add(boxsizer, wxSizerFlags(3).Expand().Border(wxALL, 20));
	//SetSize(220, 0);

	wxSizer* std_sizer = newd wxBoxSizer(wxHORIZONTAL);
	std_sizer->Add(newd wxButton(this, wxID_OK, "OK"), wxSizerFlags(1).Center());
	std_sizer->Add(newd wxButton(this, wxID_CANCEL, "Cancel"), wxSizerFlags(1).Center());
	topsizer->Add(std_sizer, wxSizerFlags(0).Center().Border(wxLEFT | wxRIGHT | wxBOTTOM, 20));

	SetSizerAndFit(topsizer);
	Centre(wxBOTH);
}

OldPropertiesWindow::OldPropertiesWindow(wxWindow* win_parent, const Map* map, const Tile* tile_parent, Spawn* spawn, wxPoint pos) :
	ObjectPropertiesWindowBase(win_parent, "Spawn Properties", map, tile_parent, spawn, pos),
	count_field(nullptr),
	direction_field(nullptr),
	action_id_field(nullptr),
	unique_id_field(nullptr),
	door_id_field(nullptr),
	depot_id_field(nullptr),
	splash_type_field(nullptr),
	text_field(nullptr),
	description_field(nullptr),
	race_id_field(nullptr),
	creature_name_label(nullptr),
	amount_field(nullptr),
	sec_radius_field(nullptr),
	visual_radius_label(nullptr),
	destination_field(nullptr)
{
	ASSERT(edit_spawn);

	// Get creature on this tile for race ID display
	Creature* tileCreature = edit_tile ? edit_tile->creature : nullptr;
	int currentRaceId = tileCreature ? tileCreature->getRaceID() : 0;

	wxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);
	wxSizer* boxsizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Spawn Properties");

	wxFlexGridSizer* subsizer = newd wxFlexGridSizer(2, 10, 10);
	subsizer->AddGrowableCol(1);

	bool isSec = edit_spawn->getSecRadius() > 0;

	if(isSec) {
		// Creature info on the spawn tile
		if(tileCreature) {
			subsizer->Add(newd wxStaticText(this, wxID_ANY, "Race ID"));
			race_id_field = newd wxSpinCtrl(this, RACE_ID_SPIN, i2ws(currentRaceId), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 9999, currentRaceId);
			subsizer->Add(race_id_field, wxSizerFlags(1).Expand());

			subsizer->Add(newd wxStaticText(this, wxID_ANY, "Name"));
			creature_name_label = newd wxStaticText(this, wxID_ANY, "\"" + wxstr(tileCreature->getName()) + "\"");
			subsizer->Add(creature_name_label, wxSizerFlags(1).Expand());

			subsizer->Add(newd wxStaticText(this, wxID_ANY, "Spawn time"));
			count_field = newd wxSpinCtrl(this, wxID_ANY, i2ws(tileCreature->getSpawnTime()), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 99999, tileCreature->getSpawnTime());
			subsizer->Add(count_field, wxSizerFlags(1).Expand());
		}

		subsizer->Add(newd wxStaticText(this, wxID_ANY, "Radius"));
		sec_radius_field = newd wxSpinCtrl(this, SEC_RADIUS_SPIN, i2ws(edit_spawn->getSecRadius()), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 9999, edit_spawn->getSecRadius());
		subsizer->Add(sec_radius_field, wxSizerFlags(1).Expand());

		subsizer->Add(newd wxStaticText(this, wxID_ANY, "Visual radius"));
		visual_radius_label = newd wxStaticText(this, wxID_ANY, i2ws(edit_spawn->getSize()));
		subsizer->Add(visual_radius_label);

		subsizer->Add(newd wxStaticText(this, wxID_ANY, "Amount"));
		amount_field = newd wxSpinCtrl(this, wxID_ANY, i2ws(edit_spawn->getAmount()), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 999, edit_spawn->getAmount());
		subsizer->Add(amount_field, wxSizerFlags(1).Expand());
	} else {
		// Non-SEC spawn: just the radius
		subsizer->Add(newd wxStaticText(this, wxID_ANY, "Spawn size"));
		count_field = newd wxSpinCtrl(this, wxID_ANY, i2ws(edit_spawn->getSize()), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, g_settings.getInteger(Config::MAX_SPAWN_RADIUS), edit_spawn->getSize());
		subsizer->Add(count_field, wxSizerFlags(1).Expand());
	}

	boxsizer->Add(subsizer, wxSizerFlags(1).Expand());

	topsizer->Add(boxsizer, wxSizerFlags(3).Expand().Border(wxALL, 20));

	wxSizer* std_sizer = newd wxBoxSizer(wxHORIZONTAL);
	std_sizer->Add(newd wxButton(this, wxID_OK, "OK"), wxSizerFlags(1).Center());
	std_sizer->Add(newd wxButton(this, wxID_CANCEL, "Cancel"), wxSizerFlags(1).Center());
	topsizer->Add(std_sizer, wxSizerFlags(0).Center().Border(wxLEFT | wxRIGHT | wxBOTTOM, 20));

	SetSizerAndFit(topsizer);
	Centre(wxBOTH);
}

OldPropertiesWindow::~OldPropertiesWindow()
{
	// Warning: edit_item may no longer be valid, DONT USE IT!
	if(splash_type_field) {
		for(uint32_t i = 0; i < splash_type_field->GetCount(); ++i) {
			delete reinterpret_cast<int*>(splash_type_field->GetClientData(i));
		}
	}
	if(direction_field) {
		for(uint32_t i = 0; i < direction_field->GetCount(); ++i) {
			delete reinterpret_cast<int*>(direction_field->GetClientData(i));
		}
	}
	if(depot_id_field) {
		for(uint32_t i = 0; i < depot_id_field->GetCount(); ++i) {
			delete reinterpret_cast<int*>(depot_id_field->GetClientData(i));
		}
	}
}

void OldPropertiesWindow::OnFocusChange(wxFocusEvent& event)
{
	wxWindow* win = event.GetWindow();
	if(wxSpinCtrl* spin = dynamic_cast<wxSpinCtrl*>(win))
		spin->SetSelection(-1, -1);
	else if(wxTextCtrl* text = dynamic_cast<wxTextCtrl*>(win))
		text->SetSelection(-1, -1);
}

void OldPropertiesWindow::OnRaceIdChanged(wxSpinEvent& event) {
	if(!creature_name_label) return;
	int raceId = event.GetPosition();
	auto monIt = IOMapSec::monsterTypes.find(raceId);
	if(monIt != IOMapSec::monsterTypes.end()) {
		creature_name_label->SetLabel("\"" + wxstr(monIt->second.name) + "\"");
	} else {
		creature_name_label->SetLabel("(unknown)");
	}
}

void OldPropertiesWindow::OnSecRadiusChanged(wxSpinEvent& event) {
	if(!visual_radius_label) return;
	int secRadius = event.GetPosition();
	int visual = std::max(1, (int)std::sqrt((double)secRadius));
	visual_radius_label->SetLabel(i2ws(visual));
}

void OldPropertiesWindow::addItemIdRows(wxSizer* sizer, Item* item) {
	const int32_t* secTypeId = item->getIntegerAttribute("sec_typeid");
	if(secTypeId) {
		// Disguised item: show the real TypeID as ID, and the disguise target
		sizer->Add(newd wxStaticText(this, wxID_ANY, "ID " + i2ws(*secTypeId)));
		sizer->Add(newd wxStaticText(this, wxID_ANY, "\"" + wxstr(item->getName()) + "\""));
		sizer->Add(newd wxStaticText(this, wxID_ANY, "Disguise ID"));
		sizer->Add(newd wxStaticText(this, wxID_ANY, i2ws(item->getClientID())));
	} else {
		sizer->Add(newd wxStaticText(this, wxID_ANY, "ID " + i2ws(item->getClientID())));
		sizer->Add(newd wxStaticText(this, wxID_ANY, "\"" + wxstr(item->getName()) + "\""));
		// Show Disguise ID only if objects.srv marks this item as a disguise type
		auto infoIt = IOMapSec::objectInfo.find(item->getClientID());
		if(infoIt != IOMapSec::objectInfo.end() && infoIt->second.isDisguise) {
			sizer->Add(newd wxStaticText(this, wxID_ANY, "Disguise ID"));
			sizer->Add(newd wxStaticText(this, wxID_ANY, i2ws(infoIt->second.disguiseTarget)));
		}
	}
}

void OldPropertiesWindow::OnClickOK(wxCommandEvent& WXUNUSED(event))
{
	if(edit_item) {
		int new_uid = (unique_id_field ? unique_id_field->GetValue() : 0);
		int new_aid = (action_id_field ? action_id_field->GetValue() : 0);
		bool uid_changed = false;
		bool aid_changed = false;

		if(!edit_item->getDepot()) {
			uid_changed = new_uid != edit_item->getUniqueID();
			aid_changed = new_aid != edit_item->getActionID();

			if(uid_changed) {
				if(new_uid != 0 && (new_uid < rme::MinUniqueId || new_uid > rme::MaxUniqueId)) {
					wxString message = "Unique ID must be between %d and %d.";
					g_gui.PopupDialog(this, "Error", wxString::Format(message, rme::MinUniqueId, rme::MaxUniqueId), wxOK);
					return;
				}
				if(g_gui.GetCurrentMap().hasUniqueId(new_uid)) {
					g_gui.PopupDialog(this, "Error", "Unique ID must be unique, this UID is already taken.", wxOK);
					return;
				}
			}

			if(aid_changed) {
				if(new_aid != 0 && (new_aid < rme::MinActionId || new_aid > rme::MaxActionId)) {
					wxString message = "Action ID must be between %d and %d.";
					g_gui.PopupDialog(this, "Error", wxString::Format(message, rme::MinActionId, rme::MaxActionId), wxOK);
					return;
				}
			}
		}

		if(edit_item->canHoldText() || edit_item->canHoldDescription()) {
			// Book
			std::string text = nstr(text_field->GetValue());
			if(text.length() >= 0xFFFF) {
				g_gui.PopupDialog(this, "Error", "Text is longer than 65535 characters, this is not supported by OpenTibia. Reduce the length of the text.", wxOK);
				return;
			}
			if(edit_item->canHoldText() && text.length() > edit_item->getMaxWriteLength()) {
				int ret = g_gui.PopupDialog(this, "Error", "Text is longer than the maximum supported length of this book type, do you still want to change it?", wxYES | wxNO);
				if(ret != wxID_YES) {
					return;
				}
			}
			edit_item->setText(text);
		} else if(edit_item->isSplash() || edit_item->isFluidContainer()) {
			// Splash
			int* new_type = reinterpret_cast<int*>(splash_type_field->GetClientData(splash_type_field->GetSelection()));
			if(new_type) {
				edit_item->setSubtype(*new_type);
			}
			// Clean up client data
		} else if(Depot* depot = edit_item->getDepot()) {
			// Depot
			int* new_depotid = reinterpret_cast<int*>(depot_id_field->GetClientData(depot_id_field->GetSelection()));
			depot->setDepotID(*new_depotid);
		} else {
			// Normal item
			Door* door = edit_item->getDoor();
			Teleport* teleport = edit_item->getTeleport();

			int new_count = count_field? count_field->GetValue() : 1;
			std::string new_desc;
			if(edit_item->canHoldDescription() && description_field) {
				description_field->GetValue();
			}
			uint8_t new_door_id = 0;
			if(door) {
				new_door_id = door_id_field->GetValue();
			}

			/*
			if(edit_item->canHoldDescription()) {
				if(new_desc.length() > 127) {
					g_gui.PopupDialog("Error", "Description must be shorter than 127 characters.", wxOK);
					return;
				}
			}
			*/

			if(door && g_settings.getInteger(Config::WARN_FOR_DUPLICATE_ID)) {
				if(edit_tile && edit_tile->isHouseTile()) {
					const House* house = edit_map->houses.getHouse(edit_tile->getHouseID());
					if(house) {
						Position pos = house->getDoorPositionByID(new_door_id);
						if(pos.isValid() && pos != edit_tile->getPosition()) {
							int ret = g_gui.PopupDialog(this, "Warning", "This doorid conflicts with another one in this house, are you sure you want to continue?", wxYES | wxNO);
							if(ret == wxID_NO) {
								return;
							}
						}
					}
				}
			}

			if(teleport) {
				Position destination = destination_field->GetPosition();
				if(!edit_map->getTile(destination) || edit_map->getTile(destination)->isBlocking()) {
					int ret = g_gui.PopupDialog(this, "Warning", "This teleport leads nowhere, or to an invalid location. Do you want to change the destination?", wxYES | wxNO);
					if(ret == wxID_YES) {
						return;
					}
				}
				teleport->setDestination(destination);
			}

			// Done validating, set the values.
			if(edit_item->canHoldDescription()) {
				edit_item->setText(new_desc);
			}
			if(edit_item->isStackable() || edit_item->isCharged()) {
				edit_item->setSubtype(new_count);
			}
			if(door) {
				door->setDoorID(new_door_id);
			}
		}

		if(uid_changed) {
			edit_item->setUniqueID(new_uid);
		}

		if(aid_changed) {
			edit_item->setActionID(new_aid);
		}

	} else if(edit_creature) {
		int new_spawntime = count_field->GetValue();
		edit_creature->setSpawnTime(new_spawntime);

		// Apply race ID change
		if(race_id_field) {
			int newRaceId = race_id_field->GetValue();
			auto monIt = IOMapSec::monsterTypes.find(newRaceId);
			if(monIt != IOMapSec::monsterTypes.end()) {
				const std::string& newName = monIt->second.name;
				CreatureType* type = g_creatures[newName];
				if(!type) type = g_creatures.addMissingCreatureType(newName, false);
				edit_creature->setName(newName);
				edit_creature->setRaceID(newRaceId);
			}
		}

		int* new_dir = reinterpret_cast<int*>(direction_field->GetClientData(
			direction_field->GetSelection()));

		if(new_dir) {
			edit_creature->setDirection((Direction)*new_dir);
		}
	} else if(edit_spawn) {
		if(sec_radius_field) {
			int newSecRadius = sec_radius_field->GetValue();
			edit_spawn->setSecRadius(newSecRadius);
			edit_spawn->setSize(std::max(1, (int)std::sqrt((double)newSecRadius)));
		} else if(count_field) {
			edit_spawn->setSize(count_field->GetValue());
		}
		if(amount_field) edit_spawn->setAmount(amount_field->GetValue());
		// Save creature changes from the spawn dialog (SEC mode)
		Creature* tileCreature = edit_tile ? const_cast<Tile*>(edit_tile)->creature : nullptr;
		if(tileCreature) {
			if(count_field) tileCreature->setSpawnTime(count_field->GetValue());
			if(race_id_field) {
				int newRaceId = race_id_field->GetValue();
				auto monIt = IOMapSec::monsterTypes.find(newRaceId);
				if(monIt != IOMapSec::monsterTypes.end()) {
					const std::string& newName = monIt->second.name;
					CreatureType* type = g_creatures[newName];
					if(!type) type = g_creatures.addMissingCreatureType(newName, false);
					tileCreature->setName(newName);
					tileCreature->setRaceID(newRaceId);
				}
			}
		}
	}
	EndModal(1);
}

void OldPropertiesWindow::OnClickCancel(wxCommandEvent& WXUNUSED(event))
{
	// Just close this window
	EndModal(0);
}

void OldPropertiesWindow::Update()
{
	Container* container = dynamic_cast<Container*>(edit_item);
	if(container) {
		for(uint32_t i = 0; i < container->getVolume(); ++i) {
			container_items[i]->setItem(container->getItem(i));
		}
	}
	wxDialog::Update();
}
