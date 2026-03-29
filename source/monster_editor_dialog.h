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

#ifndef RME_MONSTER_EDITOR_DIALOG_H_
#define RME_MONSTER_EDITOR_DIALOG_H_

#include "main.h"
#include "iomap_sec.h"
#include "outfit.h"
#include <wx/listctrl.h>
#include <wx/notebook.h>
#include <wx/spinctrl.h>

class GameSprite;

// Panel that renders a 32x32 sprite preview (outfit or item)
class SpritePreviewPanel : public wxPanel
{
public:
  SpritePreviewPanel(wxWindow* parent, wxWindowID id = wxID_ANY);
  void SetOutfit(int lookType, int head, int body, int legs, int feet);
  void SetItemSprite(int serverItemId);
  void ClearSprite();
  void OnPaint(wxPaintEvent& event);

private:
  enum Mode { NONE, OUTFIT_MODE, ITEM_MODE };
  Mode mode = NONE;
  Outfit outfit;
  int itemSpriteId = 0; // clientID for item mode

  DECLARE_EVENT_TABLE()
};

// Sub-dialog for editing a single spell
class EditSpellDialog : public wxDialog
{
public:
  EditSpellDialog(wxWindow* parent, IOMapSec::SecMonsterSpell& spell);

  void OnShapeChanged(wxCommandEvent& event);
  void OnImpactChanged(wxCommandEvent& event);
  void OnClickOK(wxCommandEvent& event);
  void OnClickCancel(wxCommandEvent& event);

private:
  void UpdateShapeParams();
  void UpdateImpactParams();

  IOMapSec::SecMonsterSpell& spell;
  wxChoice* shapeChoice;
  wxChoice* impactChoice;
  wxSpinCtrl* delayCtrl;

  // Shape param controls (max 4)
  wxPanel* shapeParamPanel;
  wxStaticText* shapeLabels[4];
  wxSpinCtrl* shapeParamCtrls[4];

  // Impact param controls (max 4)
  wxPanel* impactParamPanel;
  wxStaticText* impactLabels[4];
  wxSpinCtrl* impactParamCtrls[4];

  DECLARE_EVENT_TABLE()
};

// Sub-dialog for editing a single skill
class EditSkillDialog : public wxDialog
{
public:
  EditSkillDialog(wxWindow* parent, IOMapSec::SecMonsterSkill& skill);
  void OnClickOK(wxCommandEvent& event);
  void OnClickCancel(wxCommandEvent& event);

private:
  IOMapSec::SecMonsterSkill& skill;
  wxChoice* nameChoice;
  wxSpinCtrl* actualCtrl;
  wxSpinCtrl* minimumCtrl;
  wxSpinCtrl* maximumCtrl;
  wxSpinCtrl* nextLevelCtrl;
  wxSpinCtrl* factorPercentCtrl;
  wxSpinCtrl* addLevelCtrl;

  DECLARE_EVENT_TABLE()
};

// Sub-dialog for editing a single loot entry
class EditLootDialog : public wxDialog
{
public:
  EditLootDialog(wxWindow* parent, IOMapSec::SecMonsterLoot& loot);
  void OnClickOK(wxCommandEvent& event);
  void OnClickCancel(wxCommandEvent& event);
  void OnItemIdChanged(wxSpinEvent& event);

private:
  IOMapSec::SecMonsterLoot& loot;
  wxSpinCtrl* itemIdCtrl;
  wxSpinCtrl* quantityCtrl;
  wxSpinCtrl* probabilityCtrl;
  SpritePreviewPanel* itemPreview;

  DECLARE_EVENT_TABLE()
};

// Main monster editor dialog
class EditMonstersDialog : public wxDialog
{
public:
  EditMonstersDialog(wxWindow* parent, const std::string& monDir);
  ~EditMonstersDialog();

  void OnListSelect(wxCommandEvent& event);
  void OnFilterChanged(wxCommandEvent& event);
  void OnAddMonster(wxCommandEvent& event);
  void OnRemoveMonster(wxCommandEvent& event);
  void OnSaveAll(wxCommandEvent& event);
  void OnClickOK(wxCommandEvent& event);
  void OnClickCancel(wxCommandEvent& event);

  // Skill list events
  void OnAddSkill(wxCommandEvent& event);
  void OnEditSkill(wxCommandEvent& event);
  void OnRemoveSkill(wxCommandEvent& event);

  // Spell list events
  void OnAddSpell(wxCommandEvent& event);
  void OnEditSpell(wxCommandEvent& event);
  void OnRemoveSpell(wxCommandEvent& event);

  // Inventory list events
  void OnAddLoot(wxCommandEvent& event);
  void OnEditLoot(wxCommandEvent& event);
  void OnRemoveLoot(wxCommandEvent& event);

  // Talk list events
  void OnAddTalk(wxCommandEvent& event);
  void OnEditTalk(wxCommandEvent& event);
  void OnRemoveTalk(wxCommandEvent& event);

  // Sprite preview updates
  void OnOutfitChanged(wxSpinEvent& event);
  void OnCorpseChanged(wxSpinEvent& event);

private:
  void BuildMonsterList();
  void SaveCurrentMonster();
  void LoadMonster(int raceNumber);
  void RefreshSkillList();
  void RefreshSpellList();
  void RefreshInventoryList();
  void RefreshTalkList();
  void RefreshPreviews();

  std::string monDir;
  std::map<int, IOMapSec::SecMonsterType> workingCopy;
  std::vector<int> sortedRaceNumbers; // filtered and sorted
  int currentRace = -1;

  // Left panel
  wxTextCtrl* filterCtrl;
  wxListBox* monsterList;

  // Basic tab
  wxSpinCtrl* raceNumberCtrl;
  wxTextCtrl* nameCtrl;
  wxTextCtrl* articleCtrl;
  wxSpinCtrl* outfitIdCtrl;
  wxSpinCtrl* outfitColorCtrls[4];
  wxSpinCtrl* outfitItemTypeCtrl;
  wxSpinCtrl* corpseCtrl;
  wxChoice* bloodCtrl;
  wxSpinCtrl* experienceCtrl;
  wxSpinCtrl* summonCostCtrl;
  wxSpinCtrl* fleeThresholdCtrl;
  wxSpinCtrl* attackCtrl;
  wxSpinCtrl* defendCtrl;
  wxSpinCtrl* armorCtrl;
  wxSpinCtrl* poisonCtrl;
  wxSpinCtrl* loseTargetCtrl;
  wxSpinCtrl* strategyCtrls[4];

  // Sprite previews
  SpritePreviewPanel* outfitPreview;
  SpritePreviewPanel* corpsePreview;

  // Flags tab
  wxCheckBox* flagChecks[14];

  // Skills tab
  wxListCtrl* skillList;

  // Spells tab
  wxListCtrl* spellList;

  // Inventory tab
  wxListCtrl* inventoryList;

  // Talk tab
  wxListBox* talkList;

  DECLARE_EVENT_TABLE()
};

#endif // RME_MONSTER_EDITOR_DIALOG_H_
