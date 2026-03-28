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
#include "materials.h"
#include "palette_window.h"
#include "creatures.h"
#include "creature.h"
#include "spawn.h"

#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <cmath>

namespace fs = std::filesystem;

bool IOMapSec::objectInfoLoaded = false;
std::map<uint16_t, IOMapSec::SecObjectInfo> IOMapSec::objectInfo;
bool IOMapSec::monsterTypesLoaded = false;
std::map<int, IOMapSec::SecMonsterType> IOMapSec::monsterTypes;
std::vector<IOMapSec::SecSpawnEntry> IOMapSec::spawnEntries;

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

  // Use objects.srv for disguise resolution instead of separate disguiseMap
  auto infoIt = objectInfo.find(clientId);
  if(infoIt != objectInfo.end() && infoIt->second.isDisguise) {
    lookupId = infoIt->second.disguiseTarget;
  }

  // Find OTB server ID from client ID
  uint16_t serverId = lookupId;
  auto it = g_items.clientToServer.find(lookupId);
  if(it != g_items.clientToServer.end()) {
    serverId = it->second;
  }

  // Use objects.srv flags to decide item subclass instead of OTB
  bool srvContainer = (infoIt != objectInfo.end()) ? infoIt->second.isContainer : g_items.getItemType(serverId).isContainer();
  bool srvDoor = (infoIt != objectInfo.end()) ? infoIt->second.isDoor : g_items.getItemType(serverId).isDoor();
  bool srvTeleport = (infoIt != objectInfo.end()) ? infoIt->second.isTeleport : g_items.getItemType(serverId).isTeleport();

  Item* item;
  if(srvContainer || !parsed.children.empty()) {
    item = new Container(serverId);
  } else if(srvDoor) {
    item = new Door(serverId);
  } else if(srvTeleport) {
    item = new Teleport(serverId);
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

  // Door attributes — store as int32_t attribute to avoid uint8_t truncation
  if(parsed.doorLevel >= 0) {
    Door* door = item->getDoor();
    if(door) door->setDoorID(parsed.doorLevel);
    if(parsed.doorLevel > 255) item->setAttribute("sec_doorlevel", parsed.doorLevel);
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
      // Use objects.srv Bank flag to identify ground, not OTB's isGroundTile()
      auto infoIt = objectInfo.find(parsed.id);
      bool isBank = (infoIt != objectInfo.end()) ? infoIt->second.isBank : item->isGroundTile();
      if(isBank) { delete tile->ground; tile->ground = item; }
      else { tile->items.push_back(item); }
    }

    tile->update();
    map.setTile(tilePos, tile);
  }

  return true;
}

// Check if a comma-separated flag list contains a specific flag (case-insensitive)
static bool hasFlag(const std::string& flagStr, const char* flag) {
  std::string lower;
  lower.reserve(flagStr.size());
  for(char c : flagStr) lower += std::tolower((unsigned char)c);
  std::string lowerFlag;
  for(const char* p = flag; *p; ++p) lowerFlag += std::tolower((unsigned char)*p);
  size_t pos = lower.find(lowerFlag);
  if(pos == std::string::npos) return false;
  // Ensure it's a whole word (not a substring of another flag)
  if(pos > 0 && std::isalpha((unsigned char)lower[pos - 1])) return false;
  size_t end = pos + lowerFlag.size();
  if(end < lower.size() && std::isalpha((unsigned char)lower[end])) return false;
  return true;
}

void IOMapSec::loadObjectsSrv(const std::string& dataDir) {
  if(!objectInfo.empty()) return;

  std::string filepath = dataDir + "objects.srv";
  std::ifstream file(filepath);
  if(!file.is_open()) return;

  int currentTypeID = -1;
  SecObjectInfo currentInfo;

  auto commitEntry = [&]() {
    if(currentTypeID >= 0) objectInfo[(uint16_t)currentTypeID] = currentInfo;
  };

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
      commitEntry();
      currentTypeID = std::atoi(value.c_str());
      currentInfo = SecObjectInfo{};
    } else if(key == "Flags") {
      if(hasFlag(value, "Bank")) currentInfo.isBank = true;
      if(hasFlag(value, "Bottom")) currentInfo.isBottom = true;
      if(hasFlag(value, "Top")) currentInfo.isTop = true;
      if(hasFlag(value, "Container") || hasFlag(value, "Chest")) currentInfo.isContainer = true;
      if(hasFlag(value, "KeyDoor") || hasFlag(value, "NameDoor") || hasFlag(value, "LevelDoor") || hasFlag(value, "QuestDoor")) currentInfo.isDoor = true;
      if(hasFlag(value, "TeleportAbsolute") || hasFlag(value, "TeleportRelative")) currentInfo.isTeleport = true;
      if(hasFlag(value, "Disguise")) currentInfo.isDisguise = true;
    } else if(key == "Attributes") {
      if(currentInfo.isDisguise) {
        std::string lower;
        for(char c : value) lower += std::tolower((unsigned char)c);
        size_t pos = lower.find("disguisetarget=");
        if(pos != std::string::npos) currentInfo.disguiseTarget = (uint16_t)std::atoi(value.c_str() + pos + 15);
      }
    }
  }
  commitEntry();
}

void IOMapSec::loadMonsterTypes(const std::string& monDir) {
  if(monsterTypesLoaded) return;
  monsterTypesLoaded = true;

  for(const auto& entry : fs::directory_iterator(monDir)) {
    if(!entry.is_regular_file()) continue;
    std::string ext = entry.path().extension().string();
    for(char& c : ext) c = std::tolower((unsigned char)c);
    if(ext != ".mon") continue;

    std::ifstream file(entry.path().string());
    if(!file.is_open()) continue;

    SecMonsterType mon;
    std::string line;
    while(std::getline(file, line)) {
      if(line.empty() || line[0] == '#') continue;
      size_t eqPos = line.find('=');
      if(eqPos == std::string::npos) continue;

      std::string key;
      for(size_t i = 0; i < eqPos; ++i) {
        if(!std::isspace((unsigned char)line[i])) key += std::tolower((unsigned char)line[i]);
      }
      std::string value = line.substr(eqPos + 1);
      // Trim leading whitespace from value
      size_t vstart = 0;
      while(vstart < value.size() && std::isspace((unsigned char)value[vstart])) ++vstart;
      value = value.substr(vstart);

      if(key == "racenumber") {
        mon.raceNumber = std::atoi(value.c_str());
      } else if(key == "name") {
        // Extract quoted string
        size_t q1 = value.find('"');
        size_t q2 = value.find('"', q1 + 1);
        if(q1 != std::string::npos && q2 != std::string::npos) mon.name = value.substr(q1 + 1, q2 - q1 - 1);
      } else if(key == "article") {
        size_t q1 = value.find('"');
        size_t q2 = value.find('"', q1 + 1);
        if(q1 != std::string::npos && q2 != std::string::npos) mon.article = value.substr(q1 + 1, q2 - q1 - 1);
      } else if(key == "outfit") {
        // Format: (outfitId, head-body-legs-feet) or (0, objectTypeId)
        size_t p1 = value.find('(');
        if(p1 != std::string::npos) {
          mon.outfitId = std::atoi(value.c_str() + p1 + 1);
          size_t comma = value.find(',', p1);
          if(comma != std::string::npos) {
            std::string rest = value.substr(comma + 1);
            // Trim
            size_t rs = 0;
            while(rs < rest.size() && std::isspace((unsigned char)rest[rs])) ++rs;
            rest = rest.substr(rs);
            if(mon.outfitId == 0) {
              mon.outfitItemType = std::atoi(rest.c_str());
            } else {
              // Parse head-body-legs-feet byte sequence
              int ci = 0;
              size_t pos = 0;
              while(ci < 4 && pos < rest.size()) {
                mon.outfitColors[ci] = std::atoi(rest.c_str() + pos);
                ++ci;
                size_t dash = rest.find('-', pos);
                if(dash == std::string::npos) break;
                pos = dash + 1;
              }
            }
          }
        }
      } else if(key == "corpse") {
        mon.corpse = std::atoi(value.c_str());
      } else if(key == "experience") {
        mon.experience = std::atoi(value.c_str());
      } else if(key == "attack") {
        mon.attack = std::atoi(value.c_str());
      } else if(key == "defend") {
        mon.defend = std::atoi(value.c_str());
      } else if(key == "armor") {
        mon.armor = std::atoi(value.c_str());
      } else if(key == "skills") {
        // Extract HitPoints from Skills list: (HitPoints, max, ...)
        std::string lower;
        for(char c : value) lower += std::tolower((unsigned char)c);
        size_t hp = lower.find("hitpoints");
        if(hp != std::string::npos) {
          size_t comma1 = value.find(',', hp);
          if(comma1 != std::string::npos) {
            mon.hitpoints = std::atoi(value.c_str() + comma1 + 1);
          }
        }
      }
      // Other fields (flags, spells, inventory, talk) stored as raw strings if needed later
    }

    if(mon.raceNumber > 0 && !mon.name.empty()) {
      monsterTypes[mon.raceNumber] = mon;
    }
  }
}

void IOMapSec::loadMonsterDb(const std::string& filepath) {
  spawnEntries.clear();
  std::ifstream file(filepath);
  if(!file.is_open()) return;

  std::string line;
  while(std::getline(file, line)) {
    if(line.empty()) continue;
    // Skip comment lines
    size_t first = line.find_first_not_of(" \t");
    if(first == std::string::npos || line[first] == '#') continue;

    // Data line: Race X Y Z Radius Amount Regen
    SecSpawnEntry entry;
    if(sscanf(line.c_str(), " %d %d %d %d %d %d %d", &entry.raceNumber, &entry.x, &entry.y, &entry.z, &entry.radius, &entry.amount, &entry.regen) == 7) {
      spawnEntries.push_back(entry);
    }
  }
}

bool IOMapSec::loadMap(Map& map, const FileName& identifier) {
  wxFileName fn(identifier);
  std::string dirPath = nstr(fn.GetPath(wxPATH_GET_SEPARATOR | wxPATH_GET_VOLUME));

  if(dirPath.empty()) {
    error("Could not determine directory from file: %s", nstr(identifier.GetFullPath()).c_str());
    return false;
  }

  // Derive root directory: the .sec file is in root/map/
  // Root is the parent of the directory containing the .sec file
  fs::path mapDirPath = fs::path(dirPath).parent_path();
  std::string rootDir = mapDirPath.parent_path().string() + "/";
  std::string mapDir = rootDir + "map/";
  std::string datDir = rootDir + "dat/";
  std::string monDir = rootDir + "mon/";

  // If root/map/ doesn't exist, fall back to using dirPath directly as the map dir
  if(!fs::is_directory(mapDir)) {
    mapDir = dirPath;
    rootDir = dirPath;
    datDir = "";
    monDir = "";
  }

  // Scan map directory for .sec files
  int fileCount = 0;
  int maxSectorX = 0, maxSectorY = 0;

  struct SectorInfo {
    std::string filepath;
    int sx, sy, sz;
  };
  std::vector<SectorInfo> sectors;

  try {
    for(const auto& entry : fs::directory_iterator(mapDir)) {
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
    error("No .sec files found in directory: %s", mapDir.c_str());
    return false;
  }

  // Set map dimensions
  map.width = (maxSectorX + 1) * 32;
  map.height = (maxSectorY + 1) * 32;

  // Build clientID -> serverID reverse map for .sec TypeID translation
  if(g_items.clientToServer.empty()) {
    for(uint16_t sid = g_items.getMinID(); sid <= g_items.getMaxID(); ++sid) {
      const ItemType& t = g_items.getItemType(sid);
      if(t.id != 0 && t.clientID != 0) {
        g_items.clientToServer[t.clientID] = sid;
      }
    }
  }

  // Load objects.srv from root/dat/
  if(!datDir.empty() && fs::exists(datDir + "objects.srv")) {
    loadObjectsSrv(datDir);
    g_materials.createOtherTileset();
    Tileset* dt = g_materials.tilesets["Disguise"];
    if(dt) {
      const TilesetCategory* dtc = dt->getCategory(TILESET_RAW);
      if(dtc && dtc->size() > 0) {
        PaletteWindow* pal = g_gui.GetPalette();
        if(pal) pal->AddRAWTilesetPage("Disguise", dtc);
      }
    }
  }
  // Load monster types from root/mon/
  if(!monDir.empty() && fs::is_directory(monDir)) {
    loadMonsterTypes(monDir);
    // Register monsters in the creature database
    for(auto& pair : monsterTypes) {
      const SecMonsterType& mon = pair.second;
      if(g_creatures[mon.name] != nullptr) continue;
      Outfit outfit;
      outfit.lookType = mon.outfitId;
      if(mon.outfitId == 0) {
        outfit.lookItem = mon.outfitItemType;
      } else {
        outfit.lookHead = mon.outfitColors[0];
        outfit.lookBody = mon.outfitColors[1];
        outfit.lookLegs = mon.outfitColors[2];
        outfit.lookFeet = mon.outfitColors[3];
      }
      g_creatures.addCreatureType(mon.name, false, outfit);
    }
    g_materials.createOtherTileset();
    g_gui.RebuildPalettes();
  }

  // Load monster.db spawn database
  if(!datDir.empty()) {
    std::string monsterDbPath = datDir + "monster.db";
    if(fs::exists(monsterDbPath)) loadMonsterDb(monsterDbPath);
  }

  g_gui.SetLoadDone(0, "Loading sector files...");

  int loaded = 0;
  for(const auto& sec : sectors) {
    if(!loadSectorFile(map, sec.filepath, sec.sx, sec.sy, sec.sz)) {
      warning("Failed to load sector file: %s", sec.filepath.c_str());
    }
    ++loaded;
    g_gui.SetLoadDone((int)(100 * loaded / fileCount));
  }

  // Place monster spawns on the map from monster.db
  if(!spawnEntries.empty()) {
    int placedSpawns = 0;
    for(const auto& entry : spawnEntries) {
      // Look up creature name from race number
      auto monIt = monsterTypes.find(entry.raceNumber);
      if(monIt == monsterTypes.end()) continue;
      const std::string& creatureName = monIt->second.name;

      Position pos(entry.x, entry.y, entry.z);

      // Get or create tile at spawn position
      Tile* tile = map.getTile(pos);
      if(!tile) {
        tile = map.allocator(map.createTileL(pos));
        map.setTile(pos, tile);
      }

      // Create spawn on tile if none exists
      if(!tile->spawn) {
        // Visual radius = sqrt(SEC radius), SEC radius stored for round-trip
        int visualRadius = (int)std::sqrt((double)entry.radius);
        if(visualRadius < 1) visualRadius = 1;
        Spawn* spawn = newd Spawn(visualRadius);
        spawn->setSecRadius(entry.radius);
        spawn->setAmount(entry.amount);
        tile->spawn = spawn;
        map.addSpawn(tile);
      }

      // Place creature if tile doesn't already have one
      if(!tile->creature) {
        CreatureType* type = g_creatures[creatureName];
        if(!type) type = g_creatures.addMissingCreatureType(creatureName, false);
        Creature* creature = newd Creature(type);
        creature->setSpawnTime(entry.regen);
        creature->setRaceID(entry.raceNumber);
        tile->creature = creature;
        ++placedSpawns;
      }
    }
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

  const int32_t* secDoorLevel = item->getIntegerAttribute("sec_doorlevel");
  Door* door = item->getDoor();
  if(secDoorLevel && *secDoorLevel > 0) { buf += " Level="; appendInt(buf, *secDoorLevel); }
  else if(door && door->getDoorID() > 0) { buf += " Level="; appendInt(buf, door->getDoorID()); }
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

  // Save monster.db — derive root/dat/ from save path
  fs::path saveDirPath = fs::path(dirPath).parent_path();
  std::string saveRootDir = saveDirPath.parent_path().string() + "/";
  std::string saveDatDir = saveRootDir + "dat/";
  if(fs::is_directory(saveDatDir)) {
    // Collect all spawn entries from the map
    struct SaveSpawnEntry {
      int race, x, y, z, radius, amount, regen;
      uint16_t sx, sy;
      uint8_t sz;
    };
    std::vector<SaveSpawnEntry> saveEntries;

    MapIterator sit = map.begin();
    MapIterator send = map.end();
    while(sit != send) {
      Tile* tile = (*sit)->get();
      if(tile && tile->spawn && tile->creature && tile->creature->getRaceID() > 0) {
        const Position& pos = tile->getPosition();
        int radius = tile->spawn->getSecRadius() > 0 ? tile->spawn->getSecRadius() : tile->spawn->getSize();
        int amount = tile->spawn->getAmount();
        int regen = tile->creature->getSpawnTime();
        saveEntries.push_back({tile->creature->getRaceID(), pos.x, pos.y, pos.z, radius, amount, regen, (uint16_t)(pos.x / 32), (uint16_t)(pos.y / 32), (uint8_t)pos.z});
      }
      ++sit;
    }

    // Sort by sector
    std::sort(saveEntries.begin(), saveEntries.end(), [](const SaveSpawnEntry& a, const SaveSpawnEntry& b) {
      if(a.sx != b.sx) return a.sx < b.sx;
      if(a.sy != b.sy) return a.sy < b.sy;
      return a.sz < b.sz;
    });

    // Write monster.db
    std::string dbBuf;
    dbBuf += "# Race     X     Y  Z Radius Amount Regen.\n";
    uint16_t lastSx = 0xFFFF, lastSy = 0xFFFF;
    uint8_t lastSz = 0xFF;
    char line[256];
    for(auto& e : saveEntries) {
      if(e.sx != lastSx || e.sy != lastSy || e.sz != lastSz) {
        snprintf(line, sizeof(line), "\n# ====== %04d,%04d,%02d ====================\n", e.sx, e.sy, e.sz);
        dbBuf += line;
        lastSx = e.sx; lastSy = e.sy; lastSz = e.sz;
      }
      snprintf(line, sizeof(line), "%5d %5d %5d %2d %5d %6d %4d\n", e.race, e.x, e.y, e.z, e.radius, e.amount, e.regen);
      dbBuf += line;
    }

    std::string monsterDbPath = saveDatDir + "monster.db";
    FILE* dbFile = fopen(monsterDbPath.c_str(), "wb");
    if(dbFile) { fwrite(dbBuf.data(), 1, dbBuf.size(), dbFile); fclose(dbFile); }
  }

  return true;
}
