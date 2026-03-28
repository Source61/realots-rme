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

#include "raw_brush.h"
#include "settings.h"
#include "items.h"
#include "basemap.h"

//=============================================================================
// RAW brush

std::map<uint16_t, RAWBrush*> RAWBrush::disguiseBrushes;

RAWBrush* RAWBrush::getDisguiseBrush(uint16_t secId) {
	auto it = disguiseBrushes.find(secId);
	return (it != disguiseBrushes.end()) ? it->second : nullptr;
}

RAWBrush::RAWBrush(uint16_t itemid) :
	Brush()
{
	itemtype = g_items.getRawItemType(itemid);
}

RAWBrush::~RAWBrush()
{
	////
}

int RAWBrush::getLookID() const
{
	if(itemtype)
		return itemtype->clientID;
	return 0;
}

uint16_t RAWBrush::getItemID() const
{
	return itemtype->id;
}

std::string RAWBrush::getName() const
{
	if(!itemtype)
		return "RAWBrush";

	if(secTypeId > 0)
		return i2s(secTypeId) + " [D>" + i2s(itemtype->clientID) + "] - " + itemtype->name;

	if(itemtype->hookSouth)
		return i2s(itemtype->id) + " - " + itemtype->name + " (Hook South)";
	else if(itemtype->hookEast)
		return i2s(itemtype->id) + " - " + itemtype->name + " (Hook East)";

	return i2s(itemtype->id) + " - " + itemtype->name + itemtype->editorsuffix;
}

void RAWBrush::undraw(BaseMap* map, Tile* tile)
{
	if(tile->ground && tile->ground->getID() == itemtype->id) {
		delete tile->ground;
		tile->ground = nullptr;
	}
	for(ItemVector::iterator iter = tile->items.begin(); iter != tile->items.end();) {
		Item* item = *iter;
		if(item->getID() == itemtype->id) {
			delete item;
			iter = tile->items.erase(iter);
		} else {
			++iter;
		}
	}
}

void RAWBrush::draw(BaseMap* map, Tile* tile, void* parameter)
{
	if(!itemtype) return;

	bool b = parameter? *reinterpret_cast<bool*>(parameter) : false;
	if((g_settings.getInteger(Config::RAW_LIKE_SIMONE) && !b) && itemtype->alwaysOnBottom && itemtype->alwaysOnTopOrder == 2) {
		for(ItemVector::iterator iter = tile->items.begin(); iter != tile->items.end();) {
			Item* item = *iter;
			if(item->getTopOrder() == itemtype->alwaysOnTopOrder) {
				delete item;
				iter = tile->items.erase(iter);
			}
			else
				++iter;
		}
	}
	Item* item = Item::Create(itemtype->id);
	if(item && secTypeId > 0) item->setAttribute("sec_typeid", secTypeId);
	if(item) tile->addItem(item);
}
