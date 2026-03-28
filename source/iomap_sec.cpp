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

#include "iomap_sec.h"
#include "complexitem.h"
#include "map.h"
#include "tile.h"
#include "item.h"
#include "gui.h"

#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

bool IOMapSec::disguisesLoaded = false;
std::map<uint16_t, uint16_t> IOMapSec::disguiseMap;

// Case-insensitive string comparison
static bool iequals(const std::string& a, const std::string& b) {
  if(a.size() != b.size()) return false;
  for(size_t i = 0; i < a.size(); ++i) {
    if(std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i])) return false;
  }
  return true;
}

// =============================================================================
// Parser helpers
// =============================================================================

void IOMapSec::skipWhitespace(const std::string& line, size_t& pos) {
  while(pos < line.size() && (line[pos] == ' ' || line[pos] == '\t')) ++pos;
}

int32_t IOMapSec::readNumber(const std::string& line, size_t& pos) {
  skipWhitespace(line, pos);
  bool negative = false;
  if(pos < line.size() && line[pos] == '-') {
    negative = true;
    ++pos;
  }
  int64_t val = 0;
  while(pos < line.size() && std::isdigit((unsigned char)line[pos])) {
    val = val * 10 + (line[pos] - '0');
    ++pos;
  }
  return (int32_t)(negative ? -val : val);
}

std::string IOMapSec::readIdentifier(const std::string& line, size_t& pos) {
  skipWhitespace(line, pos);
  std::string result;
  while(pos < line.size() && (std::isalnum((unsigned char)line[pos]) || line[pos] == '_')) {
    result += line[pos];
    ++pos;
  }
  return result;
}

std::string IOMapSec::readQuotedString(const std::string& line, size_t& pos) {
  // Expect opening quote
  if(pos >= line.size() || line[pos] != '"') return "";
  ++pos;
  std::string result;
  while(pos < line.size() && line[pos] != '"') {
    if(line[pos] == '\\' && pos + 1 < line.size()) {
      ++pos;
      if(line[pos] == 'n') result += '\n';
      else if(line[pos] == '"') result += '"';
      else if(line[pos] == '\\') result += '\\';
      else { result += '\\'; result += line[pos]; }
    } else {
      result += line[pos];
    }
    ++pos;
  }
  if(pos < line.size()) ++pos; // skip closing quote
  return result;
}

bool IOMapSec::parseAttributes(const std::string& line, size_t& pos, ParsedItem& item) {
  while(pos < line.size()) {
    skipWhitespace(line, pos);
    if(pos >= line.size()) break;
    char c = line[pos];
    // If we hit comma or closing brace, we're done with this item's attributes
    if(c == ',' || c == '}') break;

    std::string attrName = readIdentifier(line, pos);
    if(attrName.empty()) break;

    skipWhitespace(line, pos);
    if(pos >= line.size() || line[pos] != '=') {
      warning("Expected '=' after attribute '%s'", attrName.c_str());
      return false;
    }
    ++pos; // skip '='

    if(iequals(attrName, "Content")) {
      skipWhitespace(line, pos);
      if(pos >= line.size() || line[pos] != '{') {
        warning("Expected '{' after Content=");
        return false;
      }
      ++pos; // skip '{'
      if(!parseItemList(line, pos, item.children)) return false;
      // parseItemList consumes the closing '}'
    } else if(iequals(attrName, "String")) {
      skipWhitespace(line, pos);
      item.text = readQuotedString(line, pos);
      item.hasText = true;
    } else if(iequals(attrName, "Editor")) {
      skipWhitespace(line, pos);
      item.editor = readQuotedString(line, pos);
      item.hasEditor = true;
    } else if(iequals(attrName, "Amount")) {
      item.amount = readNumber(line, pos);
    } else if(iequals(attrName, "Charges")) {
      item.charges = readNumber(line, pos);
    } else if(iequals(attrName, "KeyNumber")) {
      item.keyNumber = readNumber(line, pos);
    } else if(iequals(attrName, "KeyholeNumber")) {
      item.keyholeNumber = readNumber(line, pos);
    } else if(iequals(attrName, "Level")) {
      item.doorLevel = readNumber(line, pos);
    } else if(iequals(attrName, "DoorQuestNumber")) {
      item.doorQuestNumber = readNumber(line, pos);
    } else if(iequals(attrName, "DoorQuestValue")) {
      item.doorQuestValue = readNumber(line, pos);
    } else if(iequals(attrName, "ChestQuestNumber")) {
      item.chestQuestNumber = readNumber(line, pos);
    } else if(iequals(attrName, "ContainerLiquidType")) {
      item.containerLiquidType = readNumber(line, pos);
    } else if(iequals(attrName, "PoolLiquidType")) {
      item.poolLiquidType = readNumber(line, pos);
    } else if(iequals(attrName, "AbsTeleportDestination")) {
      item.absTeleportDest = readNumber(line, pos);
    } else if(iequals(attrName, "Responsible")) {
      item.responsible = readNumber(line, pos);
    } else if(iequals(attrName, "RemainingExpireTime")) {
      item.remainingExpireTime = readNumber(line, pos);
    } else if(iequals(attrName, "SavedExpireTime")) {
      item.savedExpireTime = readNumber(line, pos);
    } else if(iequals(attrName, "RemainingUses")) {
      item.remainingUses = readNumber(line, pos);
    } else {
      warning("Unknown attribute '%s'", attrName.c_str());
      // Try to skip the value
      skipWhitespace(line, pos);
      if(pos < line.size() && line[pos] == '"') {
        readQuotedString(line, pos);
      } else {
        readNumber(line, pos);
      }
    }
  }
  return true;
}

bool IOMapSec::parseItem(const std::string& line, size_t& pos, ParsedItem& item) {
  skipWhitespace(line, pos);
  if(pos >= line.size() || !std::isdigit((unsigned char)line[pos])) return false;
  item.id = (uint16_t)readNumber(line, pos);
  return parseAttributes(line, pos, item);
}

bool IOMapSec::parseItemList(const std::string& line, size_t& pos, std::vector<ParsedItem>& items) {
  while(pos < line.size()) {
    skipWhitespace(line, pos);
    if(pos >= line.size()) break;
    if(line[pos] == '}') {
      ++pos; // consume closing brace
      return true;
    }
    if(line[pos] == ',') {
      ++pos;
      skipWhitespace(line, pos);
      continue;
    }
    ParsedItem item;
    if(!parseItem(line, pos, item)) break;
    items.push_back(std::move(item));
  }
  return true;
}

Item* IOMapSec::createItemFromParsed(const ParsedItem& parsed) {
  // .sec TypeIDs are raw client/sprite IDs — translate to OTB server ID
  uint16_t clientId = parsed.id;
  uint16_t lookupId = clientId;

  // If this is a disguise item, use the disguise target's clientID for OTB lookup
  auto disguiseIt = disguiseMap.find(clientId);
  if(disguiseIt != disguiseMap.end()) {
    lookupId = disguiseIt->second;
  }

  // Find OTB server ID from client ID
  uint16_t serverId = lookupId;
  auto it = g_items.clientToServer.find(lookupId);
  if(it != g_items.clientToServer.end()) {
    serverId = it->second;
  }

  // If SEC data has Content children, ensure we create a Container even if
  // OTB doesn't mark this item as one (e.g. quest chests, item shelves)
  Item* item;
  if(!parsed.children.empty() && !g_items.getItemType(serverId).isContainer()) {
    item = new Container(serverId);
  } else {
    item = Item::Create(serverId);
  }
  if(!item) return nullptr;

  // Store original .sec TypeID (clientID) for round-trip saving
  if(lookupId != clientId) {
    item->setAttribute("sec_typeid", (int32_t)clientId);
  }

  // Amount (stackable count, fluid type)
  if(parsed.amount >= 0) item->setSubtype(parsed.amount);
  // Charges
  if(parsed.charges >= 0) item->setSubtype(parsed.charges);
  // Container liquid type (fluid containers)
  if(parsed.containerLiquidType >= 0) item->setSubtype(parsed.containerLiquidType);
  // Pool liquid type (splashes)
  if(parsed.poolLiquidType >= 0) item->setSubtype(parsed.poolLiquidType);

  // Text
  if(parsed.hasText) item->setText(parsed.text);
  // Editor → description
  if(parsed.hasEditor) item->setDescription(parsed.editor);

  // KeyNumber — store as action ID for OTBM compatibility
  if(parsed.keyNumber >= 0) item->setActionID(parsed.keyNumber);
  // KeyholeNumber — store as custom attribute
  if(parsed.keyholeNumber >= 0) item->setAttribute("keyholeid", parsed.keyholeNumber);

  // ChestQuestNumber — store as custom attribute
  if(parsed.chestQuestNumber >= 0) item->setAttribute("chestquestnumber", parsed.chestQuestNumber);

  // Door attributes
  Door* door = item->getDoor();
  if(door) {
    if(parsed.doorLevel >= 0) door->setDoorID(parsed.doorLevel);
  }
  if(parsed.doorQuestNumber >= 0) item->setAttribute("doorquestnumber", parsed.doorQuestNumber);
  if(parsed.doorQuestValue >= 0) item->setAttribute("doorquestvalue", parsed.doorQuestValue);

  // Teleport destination
  Teleport* teleport = item->getTeleport();
  if(teleport && parsed.absTeleportDest != INT32_MIN) {
    int x, y, z;
    UnpackAbsoluteCoordinate(parsed.absTeleportDest, x, y, z);
    teleport->setDestination(Position(x, y, z));
  }

  // Responsible
  if(parsed.responsible >= 0) item->setAttribute("responsible", parsed.responsible);
  // Expire/uses times
  if(parsed.remainingExpireTime >= 0) item->setAttribute("remainingexpiretime", parsed.remainingExpireTime);
  if(parsed.savedExpireTime >= 0) item->setAttribute("savedexpiretime", parsed.savedExpireTime);
  if(parsed.remainingUses >= 0) item->setAttribute("remaininguses", parsed.remainingUses);

  // Container contents (recursive)
  Container* container = item->getContainer();
  if(container && !parsed.children.empty()) {
    for(const auto& child : parsed.children) {
      Item* childItem = createItemFromParsed(child);
      if(childItem) container->getVector().push_back(childItem);
    }
  }

  return item;
}

// =============================================================================
// Load
// =============================================================================

bool IOMapSec::loadSectorFile(Map& map, const std::string& filepath, int sector_x, int sector_y, int floor_z) {
  std::ifstream file(filepath);
  if(!file.is_open()) {
    warning("Could not open sector file: %s", filepath.c_str());
    return false;
  }

  std::string line;
  while(std::getline(file, line)) {
    // Skip empty lines and comments
    if(line.empty() || line[0] == '#') continue;

    // Parse X-Y: prefix
    size_t pos = 0;
    // Read offset X
    if(pos >= line.size() || !std::isdigit((unsigned char)line[pos])) continue;
    int offsetX = 0;
    while(pos < line.size() && std::isdigit((unsigned char)line[pos])) {
      offsetX = offsetX * 10 + (line[pos] - '0');
      ++pos;
    }
    if(pos >= line.size() || line[pos] != '-') continue;
    ++pos;
    // Read offset Y
    int offsetY = 0;
    while(pos < line.size() && std::isdigit((unsigned char)line[pos])) {
      offsetY = offsetY * 10 + (line[pos] - '0');
      ++pos;
    }
    if(pos >= line.size() || line[pos] != ':') continue;
    ++pos;
    skipWhitespace(line, pos);

    if(offsetX < 0 || offsetX > 31 || offsetY < 0 || offsetY > 31) {
      warning("Invalid sector offset %d-%d in %s", offsetX, offsetY, filepath.c_str());
      continue;
    }

    int absX = sector_x * 32 + offsetX;
    int absY = sector_y * 32 + offsetY;
    int absZ = floor_z;

    // Parse flags and content
    uint16_t tileFlags = 0;
    std::vector<ParsedItem> items;
    bool hasContent = false;

    while(pos < line.size()) {
      skipWhitespace(line, pos);
      if(pos >= line.size()) break;

      // Check for Content=
      std::string token = readIdentifier(line, pos);
      if(token.empty()) break;

      if(iequals(token, "Refresh")) {
        tileFlags |= TILESTATE_REFRESH;
      } else if(iequals(token, "NoLogout")) {
        tileFlags |= TILESTATE_NOLOGOUT;
      } else if(iequals(token, "ProtectionZone")) {
        tileFlags |= TILESTATE_PROTECTIONZONE;
      } else if(iequals(token, "Content")) {
        skipWhitespace(line, pos);
        if(pos < line.size() && line[pos] == '=') {
          ++pos;
          skipWhitespace(line, pos);
          if(pos < line.size() && line[pos] == '{') {
            ++pos;
            parseItemList(line, pos, items);
            hasContent = true;
          }
        }
      }

      // Skip comma separator between flags
      skipWhitespace(line, pos);
      if(pos < line.size() && line[pos] == ',') {
        ++pos;
        skipWhitespace(line, pos);
      }
    }

    // Create tile if we have flags or content
    if(tileFlags == 0 && !hasContent) continue;

    Position tilePos(absX, absY, absZ);
    Tile* tile = map.allocator(map.createTileL(tilePos));

    if(tileFlags != 0) tile->setMapFlags(tileFlags);

    for(const auto& parsed : items) {
      Item* item = createItemFromParsed(parsed);
      if(!item) continue;
      if(item->isGroundTile()) { delete tile->ground; tile->ground = item; }
      else { tile->items.push_back(item); }
    }

    tile->update();
    map.setTile(tilePos, tile);
  }

  return true;
}

void IOMapSec::loadDisguises(const std::string& dataDir) {
  if(disguisesLoaded) return;
  disguisesLoaded = true;

  std::string filepath = dataDir + "objects.srv";
  std::ifstream file(filepath);
  if(!file.is_open()) return;

  // Parse objects.srv for disguise entries
  // Format: TypeID=N, Flags={..., Disguise, ...}, Attributes={..., DisguiseTarget=N, ...}
  int currentTypeID = -1;
  bool hasDisguise = false;
  int disguiseTarget = -1;

  std::string line;
  while(std::getline(file, line)) {
    if(line.empty() || line[0] == '#') continue;

    size_t eqPos = line.find('=');
    if(eqPos == std::string::npos) continue;

    std::string key;
    for(size_t i = 0; i < eqPos; ++i) {
      if(!std::isspace((unsigned char)line[i])) key += line[i];
    }

    std::string value = line.substr(eqPos + 1);

    if(key == "TypeID") {
      // Save previous entry
      if(currentTypeID >= 0 && hasDisguise && disguiseTarget >= 0) {
        disguiseMap[(uint16_t)currentTypeID] = (uint16_t)disguiseTarget;
      }
      currentTypeID = std::atoi(value.c_str());
      hasDisguise = false;
      disguiseTarget = -1;
    } else if(key == "Flags") {
      std::string lower;
      for(char c : value) lower += std::tolower((unsigned char)c);
      if(lower.find("disguise") != std::string::npos) {
        hasDisguise = true;
      }
    } else if(key == "Attributes") {
      std::string lower;
      for(char c : value) lower += std::tolower((unsigned char)c);
      size_t pos = lower.find("disguisetarget=");
      if(pos != std::string::npos) {
        disguiseTarget = std::atoi(value.c_str() + pos + 15);
      }
    }
  }
  // Last entry
  if(currentTypeID >= 0 && hasDisguise && disguiseTarget >= 0) {
    disguiseMap[(uint16_t)currentTypeID] = (uint16_t)disguiseTarget;
  }
}

bool IOMapSec::loadMap(Map& map, const FileName& identifier) {
  wxFileName fn(identifier);
  std::string dirPath = nstr(fn.GetPath(wxPATH_GET_SEPARATOR | wxPATH_GET_VOLUME));

  if(dirPath.empty()) {
    error("Could not determine directory from file: %s", nstr(identifier.GetFullPath()).c_str());
    return false;
  }

  // Scan directory for .sec files
  int fileCount = 0;
  int maxSectorX = 0, maxSectorY = 0;

  struct SectorInfo {
    std::string filepath;
    int sx, sy, sz;
  };
  std::vector<SectorInfo> sectors;

  try {
    for(const auto& entry : fs::directory_iterator(dirPath)) {
      if(!entry.is_regular_file()) continue;
      std::string fname = entry.path().filename().string();
      int sx, sy, sz;
      if(sscanf(fname.c_str(), "%d-%d-%d.sec", &sx, &sy, &sz) == 3) {
        sectors.push_back({entry.path().string(), sx, sy, sz});
        if(sx > maxSectorX) maxSectorX = sx;
        if(sy > maxSectorY) maxSectorY = sy;
        ++fileCount;
      }
    }
  } catch(const std::exception& e) {
    error("Error scanning directory: %s", e.what());
    return false;
  }

  if(fileCount == 0) {
    error("No .sec files found in directory: %s", dirPath.c_str());
    return false;
  }

  // Set map dimensions
  map.width = (maxSectorX + 1) * 32;
  map.height = (maxSectorY + 1) * 32;

  // Build clientID -> serverID reverse map for .sec TypeID translation
  // .sec files use CipSoft's raw TypeIDs which are client/sprite IDs,
  // but RME's Item::Create() expects OTB server IDs.
  if(g_items.clientToServer.empty()) {
    for(uint16_t sid = g_items.getMinID(); sid <= g_items.getMaxID(); ++sid) {
      const ItemType& t = g_items.getItemType(sid);
      if(t.id != 0 && t.clientID != 0) {
        g_items.clientToServer[t.clientID] = sid;
      }
    }
  }

  // Load disguise info from objects.srv to fix rendering of disguised items
  // Try common relative paths from the .sec directory
  std::string datDir;
  for(const auto& rel : {"../dat/", "../data/dat/", "dat/", "../"}) {
    std::string candidate = dirPath + rel;
    if(fs::exists(candidate + "objects.srv")) { datDir = candidate; break; }
  }
  if(!datDir.empty()) loadDisguises(datDir);

  g_gui.SetLoadDone(0, "Loading sector files...");

  int loaded = 0;
  for(const auto& sec : sectors) {
    if(!loadSectorFile(map, sec.filepath, sec.sx, sec.sy, sec.sz)) {
      warning("Failed to load sector file: %s", sec.filepath.c_str());
    }
    ++loaded;
    g_gui.SetLoadDone((int)(100 * loaded / fileCount));
  }

  return true;
}

// =============================================================================
// Writer helpers
// =============================================================================

static void appendInt(std::string& buf, int val) {
  char tmp[16];
  snprintf(tmp, sizeof(tmp), "%d", val);
  buf += tmp;
}

static void escapeString(std::string& buf, const std::string& str) {
  buf += '"';
  for(char c : str) {
    if(c == '"') { buf += "\\\""; }
    else if(c == '\\') { buf += "\\\\"; }
    else if(c == '\n') { buf += "\\n"; }
    else buf += c;
  }
  buf += '"';
}

void IOMapSec::writeItemAttributes(std::string& buf, Item* item) {
  const ItemType& type = g_items.getItemType(item->getID());

  if(type.stackable && item->getSubtype() > 0) { buf += " Amount="; appendInt(buf, item->getSubtype()); }
  if(item->isCharged() && item->getSubtype() > 0 && !type.stackable) { buf += " Charges="; appendInt(buf, item->getSubtype()); }
  if(type.isSplash() && item->getSubtype() > 0) { buf += " PoolLiquidType="; appendInt(buf, item->getSubtype()); }
  else if(type.isFluidContainer() && item->getSubtype() > 0) { buf += " ContainerLiquidType="; appendInt(buf, item->getSubtype()); }

  uint16_t aid = item->getActionID();
  if(aid > 0) { buf += " KeyNumber="; appendInt(buf, aid); }

  const int32_t* keyholeId = item->getIntegerAttribute("keyholeid");
  if(keyholeId && *keyholeId > 0) { buf += " KeyholeNumber="; appendInt(buf, *keyholeId); }

  const int32_t* chestQuest = item->getIntegerAttribute("chestquestnumber");
  if(chestQuest && *chestQuest > 0) { buf += " ChestQuestNumber="; appendInt(buf, *chestQuest); }

  Door* door = item->getDoor();
  if(door && door->getDoorID() > 0) { buf += " Level="; appendInt(buf, door->getDoorID()); }
  const int32_t* doorQuest = item->getIntegerAttribute("doorquestnumber");
  if(doorQuest && *doorQuest > 0) { buf += " DoorQuestNumber="; appendInt(buf, *doorQuest); }
  const int32_t* doorQuestVal = item->getIntegerAttribute("doorquestvalue");
  if(doorQuestVal && *doorQuestVal > 0) { buf += " DoorQuestValue="; appendInt(buf, *doorQuestVal); }

  std::string text = item->getText();
  if(!text.empty()) { buf += " String="; escapeString(buf, text); }

  std::string desc = item->getDescription();
  if(!desc.empty()) { buf += " Editor="; escapeString(buf, desc); }

  Teleport* teleport = item->getTeleport();
  if(teleport && teleport->hasDestination()) {
    const Position& dest = teleport->getDestination();
    buf += " AbsTeleportDestination=";
    appendInt(buf, PackAbsoluteCoordinate(dest.x, dest.y, dest.z));
  }

  const int32_t* resp = item->getIntegerAttribute("responsible");
  if(resp && *resp > 0) { buf += " Responsible="; appendInt(buf, *resp); }
  const int32_t* expTime = item->getIntegerAttribute("remainingexpiretime");
  if(expTime && *expTime > 0) { buf += " RemainingExpireTime="; appendInt(buf, *expTime); }
  const int32_t* savedExp = item->getIntegerAttribute("savedexpiretime");
  if(savedExp && *savedExp > 0) { buf += " SavedExpireTime="; appendInt(buf, *savedExp); }
  const int32_t* remUses = item->getIntegerAttribute("remaininguses");
  if(remUses && *remUses > 0) { buf += " RemainingUses="; appendInt(buf, *remUses); }
}

void IOMapSec::writeItem(std::string& buf, Item* item, bool first) {
  if(!first) buf += ", ";
  const int32_t* secTypeId = item->getIntegerAttribute("sec_typeid");
  if(secTypeId) { appendInt(buf, *secTypeId); }
  else { appendInt(buf, item->getClientID()); }
  writeItemAttributes(buf, item);

  Container* container = item->getContainer();
  if(container && container->getItemCount() > 0) {
    buf += " Content={";
    bool childFirst = true;
    for(Item* child : container->getVector()) {
      writeItem(buf, child, childFirst);
      childFirst = false;
    }
    buf += "}";
  }
}

// =============================================================================
// Save
// =============================================================================

static void writeTile(std::string& buf, IOMapSec* saver, Tile* tile, int x, int y) {
  uint16_t flags = tile->getMapFlags() & (TILESTATE_REFRESH | TILESTATE_NOLOGOUT | TILESTATE_PROTECTIONZONE);
  bool hasItems = tile->ground || !tile->items.empty();
  if(flags == 0 && !hasItems) return;

  appendInt(buf, x);
  buf += '-';
  appendInt(buf, y);
  buf += ": ";

  if(flags & TILESTATE_REFRESH) { buf += "Refresh, "; }
  if(flags & TILESTATE_NOLOGOUT) { buf += "NoLogout, "; }
  if(flags & TILESTATE_PROTECTIONZONE) { buf += "ProtectionZone, "; }

  if(hasItems) {
    buf += "Content={";
    bool first = true;
    if(tile->ground) { saver->writeItem(buf, tile->ground, first); first = false; }
    for(Item* item : tile->items) { saver->writeItem(buf, item, first); first = false; }
    buf += "}";
  }

  buf += '\n';
}

bool IOMapSec::saveMap(Map& map, const FileName& identifier) {
  wxFileName fn(identifier);
  std::string dirPath = nstr(fn.GetPath(wxPATH_GET_SEPARATOR | wxPATH_GET_VOLUME));

  if(dirPath.empty()) {
    error("Could not determine directory from file: %s", nstr(identifier.GetFullPath()).c_str());
    return false;
  }

  // Collect all tiles into a flat vector, then sort by sector
  struct TileEntry {
    uint16_t sx, sy, ox, oy;
    uint8_t sz;
    Tile* tile;
  };
  std::vector<TileEntry> entries;
  entries.reserve(map.getTileCount() > 0 ? map.getTileCount() : 500000);

  MapIterator it = map.begin();
  MapIterator end = map.end();
  while(it != end) {
    Tile* tile = (*it)->get();
    if(tile) {
      const Position& pos = tile->getPosition();
      entries.push_back({(uint16_t)(pos.x / 32), (uint16_t)(pos.y / 32), (uint16_t)(pos.x % 32), (uint16_t)(pos.y % 32), (uint8_t)pos.z, tile});
    }
    ++it;
  }

  if(entries.empty()) {
    error("Map is empty, nothing to save.");
    return false;
  }

  // Sort by (sx, sy, sz, ox, oy) so tiles in the same sector are contiguous and ordered
  std::sort(entries.begin(), entries.end(), [](const TileEntry& a, const TileEntry& b) {
    if(a.sx != b.sx) return a.sx < b.sx;
    if(a.sy != b.sy) return a.sy < b.sy;
    if(a.sz != b.sz) return a.sz < b.sz;
    if(a.ox != b.ox) return a.ox < b.ox;
    return a.oy < b.oy;
  });

  std::string buf;
  buf.reserve(64 * 1024);
  char filename[256];
  size_t i = 0;
  size_t total = entries.size();

  while(i < total) {
    uint16_t sx = entries[i].sx, sy = entries[i].sy;
    uint8_t sz = entries[i].sz;

    snprintf(filename, sizeof(filename), "%s%04d-%04d-%02d.sec", dirPath.c_str(), sx, sy, sz);

    buf.clear();

    while(i < total && entries[i].sx == sx && entries[i].sy == sy && entries[i].sz == sz) {
      writeTile(buf, this, entries[i].tile, entries[i].ox, entries[i].oy);
      ++i;
    }

    FILE* f = fopen(filename, "wb");
    if(f) { fwrite(buf.data(), 1, buf.size(), f); fclose(f); }
  }

  return true;
}
