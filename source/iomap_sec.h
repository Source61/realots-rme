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

#ifndef RME_IOMAP_SEC_H_
#define RME_IOMAP_SEC_H_

#include "iomap.h"

class IOMapSec : public IOMap
{
public:
  IOMapSec(MapVersion ver) { version = ver; }
  ~IOMapSec() {}

  virtual bool loadMap(Map& map, const FileName& identifier);
  virtual bool saveMap(Map& map, const FileName& identifier);

  // Coordinate packing matching CipSoft server format (serversrc/moveuse.cc:23-35)
  // Domain: [24576, 40959] x [24576, 40959] x [0, 15]
  static int32_t PackAbsoluteCoordinate(int x, int y, int z) {
    return (((x - 24576) & 0x3FFF) << 18) | (((y - 24576) & 0x3FFF) << 4) | (z & 0xF);
  }
  static void UnpackAbsoluteCoordinate(int32_t packed, int& x, int& y, int& z) {
    x = ((packed >> 18) & 0x3FFF) + 24576;
    y = ((packed >> 4) & 0x3FFF) + 24576;
    z = (packed >> 0) & 0xF;
  }

private:
  bool loadSectorFile(Map& map, const std::string& filepath, int sector_x, int sector_y, int floor_z);
  bool saveSectorFile(Map& map, const std::string& dirpath, int sector_x, int sector_y, int floor_z);

  // Parser helpers
  struct ParsedItem {
    uint16_t id;
    // Attributes
    int32_t amount; // -1 = not set
    int32_t charges; // -1 = not set
    int32_t keyNumber; // -1 = not set
    int32_t keyholeNumber; // -1 = not set
    int32_t doorLevel; // -1 = not set
    int32_t doorQuestNumber; // -1 = not set
    int32_t doorQuestValue; // -1 = not set
    int32_t chestQuestNumber; // -1 = not set
    int32_t containerLiquidType; // -1 = not set
    int32_t poolLiquidType; // -1 = not set
    int32_t absTeleportDest; // INT32_MIN = not set
    int32_t responsible; // -1 = not set
    int32_t remainingExpireTime; // -1 = not set
    int32_t savedExpireTime; // -1 = not set
    int32_t remainingUses; // -1 = not set
    std::string text;
    std::string editor;
    bool hasText;
    bool hasEditor;
    std::vector<ParsedItem> children;

    ParsedItem() : id(0), amount(-1), charges(-1), keyNumber(-1), keyholeNumber(-1),
      doorLevel(-1), doorQuestNumber(-1), doorQuestValue(-1), chestQuestNumber(-1),
      containerLiquidType(-1), poolLiquidType(-1), absTeleportDest(INT32_MIN),
      responsible(-1), remainingExpireTime(-1), savedExpireTime(-1), remainingUses(-1),
      hasText(false), hasEditor(false) {}
  };

  Item* createItemFromParsed(const ParsedItem& parsed);
  bool parseItemList(const std::string& line, size_t& pos, std::vector<ParsedItem>& items);
  bool parseItem(const std::string& line, size_t& pos, ParsedItem& item);
  bool parseAttributes(const std::string& line, size_t& pos, ParsedItem& item);
  std::string readQuotedString(const std::string& line, size_t& pos);
  int32_t readNumber(const std::string& line, size_t& pos);
  std::string readIdentifier(const std::string& line, size_t& pos);
  void skipWhitespace(const std::string& line, size_t& pos);

  // Writer helpers
  void writeItem(std::ofstream& out, Item* item, bool first);
  void writeItemAttributes(std::ofstream& out, Item* item);
};

#endif // RME_IOMAP_SEC_H_
