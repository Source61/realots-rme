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

#include "monster_editor_dialog.h"
#include "gui.h"
#include "gui_ids.h"
#include "graphics.h"

#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/statline.h>
#include <wx/textdlg.h>
#include <wx/dcbuffer.h>
#include <filesystem>

namespace fs = std::filesystem;

// ============================================================================
// SpritePreviewPanel
// ============================================================================

BEGIN_EVENT_TABLE(SpritePreviewPanel, wxPanel)
  EVT_PAINT(SpritePreviewPanel::OnPaint)
END_EVENT_TABLE()

SpritePreviewPanel::SpritePreviewPanel(wxWindow* parent, wxWindowID id) :
  wxPanel(parent, id, wxDefaultPosition, wxSize(36, 36))
{
  SetMinSize(wxSize(36, 36));
  SetMaxSize(wxSize(36, 36));
  SetBackgroundStyle(wxBG_STYLE_PAINT);
}

void SpritePreviewPanel::SetOutfit(int lookType, int head, int body, int legs, int feet) {
  mode = OUTFIT_MODE;
  outfit.lookType = lookType;
  outfit.lookHead = head;
  outfit.lookBody = body;
  outfit.lookLegs = legs;
  outfit.lookFeet = feet;
  outfit.lookItem = 0;
  outfit.lookMount = 0;
  outfit.lookAddon = 0;
  Refresh();
}

void SpritePreviewPanel::SetItemSprite(int clientItemId) {
  mode = ITEM_MODE;
  itemSpriteId = clientItemId;
  Refresh();
}

void SpritePreviewPanel::ClearSprite() {
  mode = NONE;
  Refresh();
}

void SpritePreviewPanel::OnPaint(wxPaintEvent&) {
  wxBufferedPaintDC dc(this);

  dc.SetBrush(wxBrush(wxColour(0, 0, 0)));
  dc.SetPen(*wxBLACK_PEN);
  dc.DrawRectangle(0, 0, 36, 36);

  if(g_gui.gfx.isUnloaded()) return;

  if(mode == OUTFIT_MODE && outfit.lookType > 0) {
    GameSprite* spr = g_gui.gfx.getCreatureSprite(outfit.lookType);
    if(spr) {
      wxRect rect(2, 2, 32, 32);
      spr->DrawTo(&dc, rect, outfit);
      return;
    }
  }

  if(mode == ITEM_MODE && itemSpriteId > 0) {
    Sprite* spr = g_gui.gfx.getSprite(itemSpriteId);
    if(spr) {
      spr->DrawTo(&dc, SPRITE_SIZE_32x32, 2, 2);
      return;
    }
  }
}

// ============================================================================
// Flag names table (order matches SecMonsterFlag bits)
// ============================================================================
static const struct { uint32_t bit; const char* name; } kFlagTable[] = {
  {IOMapSec::FLAG_KICK_BOXES, "KickBoxes"},
  {IOMapSec::FLAG_KICK_CREATURES, "KickCreatures"},
  {IOMapSec::FLAG_SEE_INVISIBLE, "SeeInvisible"},
  {IOMapSec::FLAG_UNPUSHABLE, "Unpushable"},
  {IOMapSec::FLAG_DISTANCE_FIGHTING, "DistanceFighting"},
  {IOMapSec::FLAG_NO_SUMMON, "NoSummon"},
  {IOMapSec::FLAG_NO_ILLUSION, "NoIllusion"},
  {IOMapSec::FLAG_NO_CONVINCE, "NoConvince"},
  {IOMapSec::FLAG_NO_BURNING, "NoBurning"},
  {IOMapSec::FLAG_NO_POISON, "NoPoison"},
  {IOMapSec::FLAG_NO_ENERGY, "NoEnergy"},
  {IOMapSec::FLAG_NO_HIT, "NoHit"},
  {IOMapSec::FLAG_NO_LIFE_DRAIN, "NoLifeDrain"},
  {IOMapSec::FLAG_NO_PARALYZE, "NoParalyze"},
};

static const char* kSkillNames[] = {
  "Level", "MagicLevel", "HitPoints", "Mana", "GoStrength", "CarryStrength",
  "Shielding", "DistanceFighting", "SwordFighting", "ClubFighting",
  "AxeFighting", "FistFighting", "MagicDefense", "Fishing", "Fed",
};
static const int kSkillNameCount = sizeof(kSkillNames) / sizeof(kSkillNames[0]);

static const char* kShapeNames[] = {"Actor", "Victim", "Origin", "Destination", "Angle"};
static const char* kImpactNames[] = {"Damage", "Field", "Healing", "Speed", "Drunken", "Strength", "Outfit", "Summon"};
static const char* kBloodNames[] = {"Blood", "Slime", "Bones", "Fire", "Energy"};

// Shape param counts and labels
static const int kShapeParamCounts[] = {1, 3, 2, 4, 3};
static const char* kShapeParamLabels[][4] = {
  {"Effect", "", "", ""},                            // Actor
  {"Range", "Shoot Effect", "Target Effect", ""},    // Victim
  {"Radius", "Effect", "", ""},                      // Origin
  {"Range", "Shoot Effect", "Radius", "Effect"},     // Destination
  {"Effect", "Range", "Length", ""},                  // Angle
};

// Impact param counts and labels
static const int kImpactParamCounts[] = {3, 1, 2, 3, 3, 4, 3, 2};
static const char* kImpactParamLabels[][4] = {
  {"Damage Type", "Max Damage", "Min Damage", ""},                 // Damage
  {"Field Type (1=Fire,2=Poison,3=Energy,4=MWall,5=Wild)", "", "", ""},  // Field
  {"Max Amount", "Variation", "", ""},                              // Healing
  {"Percent (+/-)", "Variation", "Duration", ""},                   // Speed
  {"Strength", "Variation", "Duration", ""},                        // Drunken
  {"Skills Bitmask", "Percent (+/-)", "Variation", "Duration"},     // Strength
  {"Outfit ID", "Color/ItemType", "Duration", ""},                  // Outfit
  {"Race Number", "Max Count", "", ""},                             // Summon
};

// Widget IDs for list buttons (offset from base)
enum {
  ID_ADD_SKILL = wxID_HIGHEST + 500,
  ID_EDIT_SKILL,
  ID_REMOVE_SKILL,
  ID_ADD_SPELL,
  ID_EDIT_SPELL,
  ID_REMOVE_SPELL,
  ID_ADD_LOOT,
  ID_EDIT_LOOT,
  ID_REMOVE_LOOT,
  ID_ADD_TALK,
  ID_EDIT_TALK,
  ID_REMOVE_TALK,
  ID_SKILL_LIST,
  ID_SPELL_LIST,
  ID_INVENTORY_LIST,
  ID_TALK_LIST,
  ID_OUTFIT_ID_SPIN,
  ID_OUTFIT_HEAD_SPIN,
  ID_OUTFIT_BODY_SPIN,
  ID_OUTFIT_LEGS_SPIN,
  ID_OUTFIT_FEET_SPIN,
  ID_OUTFIT_ITEM_SPIN,
  ID_CORPSE_SPIN,
  ID_LOOT_ITEM_SPIN,
};

// ============================================================================
// EditSpellDialog
// ============================================================================

BEGIN_EVENT_TABLE(EditSpellDialog, wxDialog)
  EVT_CHOICE(EDIT_SPELL_SHAPE_CHOICE, EditSpellDialog::OnShapeChanged)
  EVT_CHOICE(EDIT_SPELL_IMPACT_CHOICE, EditSpellDialog::OnImpactChanged)
  EVT_BUTTON(wxID_OK, EditSpellDialog::OnClickOK)
  EVT_BUTTON(wxID_CANCEL, EditSpellDialog::OnClickCancel)
END_EVENT_TABLE()

EditSpellDialog::EditSpellDialog(wxWindow* parent, IOMapSec::SecMonsterSpell& spell) :
  wxDialog(parent, wxID_ANY, "Edit Spell", wxDefaultPosition, wxSize(500, 450)),
  spell(spell)
{
  wxBoxSizer* topSizer = newd wxBoxSizer(wxVERTICAL);

  // Shape type
  wxFlexGridSizer* shapeGrid = newd wxFlexGridSizer(2, 5, 10);
  shapeGrid->AddGrowableCol(1);
  shapeGrid->Add(newd wxStaticText(this, wxID_ANY, "Shape Type"), wxSizerFlags().CenterVertical());
  wxArrayString shapeChoices;
  for(int i = 0; i < 5; ++i) shapeChoices.Add(kShapeNames[i]);
  shapeChoice = newd wxChoice(this, EDIT_SPELL_SHAPE_CHOICE, wxDefaultPosition, wxDefaultSize, shapeChoices);
  shapeChoice->SetSelection(spell.shape);
  shapeGrid->Add(shapeChoice, wxSizerFlags(1).Expand());
  topSizer->Add(shapeGrid, wxSizerFlags(0).Expand().Border(wxALL, 10));

  // Shape params panel
  shapeParamPanel = newd wxPanel(this);
  wxFlexGridSizer* spGrid = newd wxFlexGridSizer(2, 5, 10);
  spGrid->AddGrowableCol(1);
  for(int i = 0; i < 4; ++i) {
    shapeLabels[i] = newd wxStaticText(shapeParamPanel, wxID_ANY, "");
    spGrid->Add(shapeLabels[i], wxSizerFlags().CenterVertical());
    shapeParamCtrls[i] = newd wxSpinCtrl(shapeParamPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -100000, 100000, spell.shapeParams[i]);
    spGrid->Add(shapeParamCtrls[i], wxSizerFlags(1).Expand());
  }
  shapeParamPanel->SetSizer(spGrid);
  topSizer->Add(shapeParamPanel, wxSizerFlags(0).Expand().Border(wxLEFT | wxRIGHT, 10));

  topSizer->Add(newd wxStaticLine(this), wxSizerFlags(0).Expand().Border(wxALL, 5));

  // Impact type
  wxFlexGridSizer* impactGrid = newd wxFlexGridSizer(2, 5, 10);
  impactGrid->AddGrowableCol(1);
  impactGrid->Add(newd wxStaticText(this, wxID_ANY, "Impact Type"), wxSizerFlags().CenterVertical());
  wxArrayString impactChoices;
  for(int i = 0; i < 8; ++i) impactChoices.Add(kImpactNames[i]);
  impactChoice = newd wxChoice(this, EDIT_SPELL_IMPACT_CHOICE, wxDefaultPosition, wxDefaultSize, impactChoices);
  impactChoice->SetSelection(spell.impact);
  impactGrid->Add(impactChoice, wxSizerFlags(1).Expand());
  topSizer->Add(impactGrid, wxSizerFlags(0).Expand().Border(wxALL, 10));

  // Impact params panel
  impactParamPanel = newd wxPanel(this);
  wxFlexGridSizer* ipGrid = newd wxFlexGridSizer(2, 5, 10);
  ipGrid->AddGrowableCol(1);
  for(int i = 0; i < 4; ++i) {
    impactLabels[i] = newd wxStaticText(impactParamPanel, wxID_ANY, "");
    ipGrid->Add(impactLabels[i], wxSizerFlags().CenterVertical());
    impactParamCtrls[i] = newd wxSpinCtrl(impactParamPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -100000, 100000, spell.impactParams[i]);
    ipGrid->Add(impactParamCtrls[i], wxSizerFlags(1).Expand());
  }
  impactParamPanel->SetSizer(ipGrid);
  topSizer->Add(impactParamPanel, wxSizerFlags(0).Expand().Border(wxLEFT | wxRIGHT, 10));

  topSizer->Add(newd wxStaticLine(this), wxSizerFlags(0).Expand().Border(wxALL, 5));

  // Delay
  wxFlexGridSizer* delayGrid = newd wxFlexGridSizer(2, 5, 10);
  delayGrid->AddGrowableCol(1);
  delayGrid->Add(newd wxStaticText(this, wxID_ANY, "Chance (1/N)"), wxSizerFlags().CenterVertical());
  delayCtrl = newd wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100, spell.delay);
  delayGrid->Add(delayCtrl, wxSizerFlags(1).Expand());
  topSizer->Add(delayGrid, wxSizerFlags(0).Expand().Border(wxALL, 10));

  // Buttons
  wxSizer* btnSizer = newd wxBoxSizer(wxHORIZONTAL);
  btnSizer->Add(newd wxButton(this, wxID_OK, "OK"), wxSizerFlags(1).Center());
  btnSizer->Add(newd wxButton(this, wxID_CANCEL, "Cancel"), wxSizerFlags(1).Center());
  topSizer->Add(btnSizer, wxSizerFlags(0).Center().Border(wxALL, 10));

  SetSizer(topSizer);
  UpdateShapeParams();
  UpdateImpactParams();
}

void EditSpellDialog::UpdateShapeParams() {
  int sel = shapeChoice->GetSelection();
  if(sel < 0 || sel > 4) sel = 0;
  int count = kShapeParamCounts[sel];
  for(int i = 0; i < 4; ++i) {
    bool vis = (i < count);
    shapeLabels[i]->Show(vis);
    shapeParamCtrls[i]->Show(vis);
    if(vis) shapeLabels[i]->SetLabel(kShapeParamLabels[sel][i]);
  }
  shapeParamPanel->Layout();
  GetSizer()->Layout();
  Fit();
}

void EditSpellDialog::UpdateImpactParams() {
  int sel = impactChoice->GetSelection();
  if(sel < 0 || sel > 7) sel = 0;
  int count = kImpactParamCounts[sel];
  for(int i = 0; i < 4; ++i) {
    bool vis = (i < count);
    impactLabels[i]->Show(vis);
    impactParamCtrls[i]->Show(vis);
    if(vis) impactLabels[i]->SetLabel(kImpactParamLabels[sel][i]);
  }
  impactParamPanel->Layout();
  GetSizer()->Layout();
  Fit();
}

void EditSpellDialog::OnShapeChanged(wxCommandEvent&) { UpdateShapeParams(); }
void EditSpellDialog::OnImpactChanged(wxCommandEvent&) { UpdateImpactParams(); }

void EditSpellDialog::OnClickOK(wxCommandEvent&) {
  int shSel = shapeChoice->GetSelection();
  int imSel = impactChoice->GetSelection();
  spell.shape = (IOMapSec::SecSpellShape)shSel;
  spell.shapeParamCount = kShapeParamCounts[shSel];
  for(int i = 0; i < 4; ++i) spell.shapeParams[i] = shapeParamCtrls[i]->GetValue();
  spell.impact = (IOMapSec::SecSpellImpact)imSel;
  spell.impactParamCount = kImpactParamCounts[imSel];
  for(int i = 0; i < 4; ++i) spell.impactParams[i] = impactParamCtrls[i]->GetValue();
  spell.delay = delayCtrl->GetValue();
  EndModal(wxID_OK);
}

void EditSpellDialog::OnClickCancel(wxCommandEvent&) { EndModal(wxID_CANCEL); }

// ============================================================================
// EditSkillDialog
// ============================================================================

BEGIN_EVENT_TABLE(EditSkillDialog, wxDialog)
  EVT_BUTTON(wxID_OK, EditSkillDialog::OnClickOK)
  EVT_BUTTON(wxID_CANCEL, EditSkillDialog::OnClickCancel)
END_EVENT_TABLE()

EditSkillDialog::EditSkillDialog(wxWindow* parent, IOMapSec::SecMonsterSkill& skill) :
  wxDialog(parent, wxID_ANY, "Edit Skill", wxDefaultPosition, wxSize(400, 350)),
  skill(skill)
{
  wxBoxSizer* topSizer = newd wxBoxSizer(wxVERTICAL);
  wxFlexGridSizer* grid = newd wxFlexGridSizer(2, 5, 10);
  grid->AddGrowableCol(1);

  // Skill name choice
  grid->Add(newd wxStaticText(this, wxID_ANY, "Skill"), wxSizerFlags().CenterVertical());
  wxArrayString choices;
  int selIdx = 0;
  for(int i = 0; i < kSkillNameCount; ++i) {
    choices.Add(kSkillNames[i]);
    if(skill.name == kSkillNames[i]) selIdx = i;
  }
  nameChoice = newd wxChoice(this, EDIT_SKILL_NAME_CHOICE, wxDefaultPosition, wxDefaultSize, choices);
  nameChoice->SetSelection(selIdx);
  grid->Add(nameChoice, wxSizerFlags(1).Expand());

  grid->Add(newd wxStaticText(this, wxID_ANY, "Actual"), wxSizerFlags().CenterVertical());
  actualCtrl = newd wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10000000, skill.actual);
  grid->Add(actualCtrl, wxSizerFlags(1).Expand());

  grid->Add(newd wxStaticText(this, wxID_ANY, "Minimum"), wxSizerFlags().CenterVertical());
  minimumCtrl = newd wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10000000, skill.minimum);
  grid->Add(minimumCtrl, wxSizerFlags(1).Expand());

  grid->Add(newd wxStaticText(this, wxID_ANY, "Maximum"), wxSizerFlags().CenterVertical());
  maximumCtrl = newd wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10000000, skill.maximum);
  grid->Add(maximumCtrl, wxSizerFlags(1).Expand());

  grid->Add(newd wxStaticText(this, wxID_ANY, "Next Level"), wxSizerFlags().CenterVertical());
  nextLevelCtrl = newd wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10000000, skill.nextLevel);
  grid->Add(nextLevelCtrl, wxSizerFlags(1).Expand());

  grid->Add(newd wxStaticText(this, wxID_ANY, "Factor %"), wxSizerFlags().CenterVertical());
  factorPercentCtrl = newd wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10000000, skill.factorPercent);
  grid->Add(factorPercentCtrl, wxSizerFlags(1).Expand());

  grid->Add(newd wxStaticText(this, wxID_ANY, "Add Level"), wxSizerFlags().CenterVertical());
  addLevelCtrl = newd wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10000000, skill.addLevel);
  grid->Add(addLevelCtrl, wxSizerFlags(1).Expand());

  topSizer->Add(grid, wxSizerFlags(1).Expand().Border(wxALL, 10));

  wxSizer* btnSizer = newd wxBoxSizer(wxHORIZONTAL);
  btnSizer->Add(newd wxButton(this, wxID_OK, "OK"), wxSizerFlags(1).Center());
  btnSizer->Add(newd wxButton(this, wxID_CANCEL, "Cancel"), wxSizerFlags(1).Center());
  topSizer->Add(btnSizer, wxSizerFlags(0).Center().Border(wxALL, 10));

  SetSizer(topSizer);
}

void EditSkillDialog::OnClickOK(wxCommandEvent&) {
  int sel = nameChoice->GetSelection();
  if(sel >= 0 && sel < kSkillNameCount) skill.name = kSkillNames[sel];
  skill.actual = actualCtrl->GetValue();
  skill.minimum = minimumCtrl->GetValue();
  skill.maximum = maximumCtrl->GetValue();
  skill.nextLevel = nextLevelCtrl->GetValue();
  skill.factorPercent = factorPercentCtrl->GetValue();
  skill.addLevel = addLevelCtrl->GetValue();
  EndModal(wxID_OK);
}

void EditSkillDialog::OnClickCancel(wxCommandEvent&) { EndModal(wxID_CANCEL); }

// ============================================================================
// EditLootDialog
// ============================================================================

BEGIN_EVENT_TABLE(EditLootDialog, wxDialog)
  EVT_BUTTON(wxID_OK, EditLootDialog::OnClickOK)
  EVT_BUTTON(wxID_CANCEL, EditLootDialog::OnClickCancel)
  EVT_SPINCTRL(ID_LOOT_ITEM_SPIN, EditLootDialog::OnItemIdChanged)
END_EVENT_TABLE()

EditLootDialog::EditLootDialog(wxWindow* parent, IOMapSec::SecMonsterLoot& loot) :
  wxDialog(parent, wxID_ANY, "Edit Loot", wxDefaultPosition, wxSize(350, 250)),
  loot(loot)
{
  wxBoxSizer* topSizer = newd wxBoxSizer(wxVERTICAL);
  wxFlexGridSizer* grid = newd wxFlexGridSizer(2, 5, 10);
  grid->AddGrowableCol(1);

  grid->Add(newd wxStaticText(this, wxID_ANY, "Item ID"), wxSizerFlags().CenterVertical());
  {
    wxBoxSizer* itemRow = newd wxBoxSizer(wxHORIZONTAL);
    itemPreview = newd SpritePreviewPanel(this);
    itemRow->Add(itemPreview, wxSizerFlags(0).Border(wxRIGHT, 5));
    itemIdCtrl = newd wxSpinCtrl(this, ID_LOOT_ITEM_SPIN, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 99999, loot.itemId);
    itemRow->Add(itemIdCtrl, wxSizerFlags(1).Expand());
    grid->Add(itemRow, wxSizerFlags(1).Expand());
  }

  grid->Add(newd wxStaticText(this, wxID_ANY, "Max Quantity"), wxSizerFlags().CenterVertical());
  quantityCtrl = newd wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100, loot.maxQuantity);
  grid->Add(quantityCtrl, wxSizerFlags(1).Expand());

  grid->Add(newd wxStaticText(this, wxID_ANY, "Probability (per mille)"), wxSizerFlags().CenterVertical());
  probabilityCtrl = newd wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 1000, loot.probabilityPerMille);
  grid->Add(probabilityCtrl, wxSizerFlags(1).Expand());

  topSizer->Add(grid, wxSizerFlags(1).Expand().Border(wxALL, 10));

  wxSizer* btnSizer = newd wxBoxSizer(wxHORIZONTAL);
  btnSizer->Add(newd wxButton(this, wxID_OK, "OK"), wxSizerFlags(1).Center());
  btnSizer->Add(newd wxButton(this, wxID_CANCEL, "Cancel"), wxSizerFlags(1).Center());
  topSizer->Add(btnSizer, wxSizerFlags(0).Center().Border(wxALL, 10));

  SetSizer(topSizer);

  // Initialize preview
  if(loot.itemId > 0) itemPreview->SetItemSprite(loot.itemId);
}

void EditLootDialog::OnClickOK(wxCommandEvent&) {
  loot.itemId = itemIdCtrl->GetValue();
  loot.maxQuantity = quantityCtrl->GetValue();
  loot.probabilityPerMille = probabilityCtrl->GetValue();
  EndModal(wxID_OK);
}

void EditLootDialog::OnClickCancel(wxCommandEvent&) { EndModal(wxID_CANCEL); }

void EditLootDialog::OnItemIdChanged(wxSpinEvent&) {
  int id = itemIdCtrl->GetValue();
  if(id > 0) itemPreview->SetItemSprite(id);
  else itemPreview->ClearSprite();
}

// ============================================================================
// EditMonstersDialog
// ============================================================================

BEGIN_EVENT_TABLE(EditMonstersDialog, wxDialog)
  EVT_LISTBOX(EDIT_MONSTERS_LISTBOX, EditMonstersDialog::OnListSelect)
  EVT_TEXT(EDIT_MONSTERS_FILTER, EditMonstersDialog::OnFilterChanged)
  EVT_BUTTON(EDIT_MONSTERS_ADD, EditMonstersDialog::OnAddMonster)
  EVT_BUTTON(EDIT_MONSTERS_REMOVE, EditMonstersDialog::OnRemoveMonster)
  EVT_BUTTON(EDIT_MONSTERS_SAVE_ALL, EditMonstersDialog::OnSaveAll)
  EVT_BUTTON(wxID_OK, EditMonstersDialog::OnClickOK)
  EVT_BUTTON(wxID_CANCEL, EditMonstersDialog::OnClickCancel)
  EVT_BUTTON(ID_ADD_SKILL, EditMonstersDialog::OnAddSkill)
  EVT_BUTTON(ID_EDIT_SKILL, EditMonstersDialog::OnEditSkill)
  EVT_BUTTON(ID_REMOVE_SKILL, EditMonstersDialog::OnRemoveSkill)
  EVT_BUTTON(ID_ADD_SPELL, EditMonstersDialog::OnAddSpell)
  EVT_BUTTON(ID_EDIT_SPELL, EditMonstersDialog::OnEditSpell)
  EVT_BUTTON(ID_REMOVE_SPELL, EditMonstersDialog::OnRemoveSpell)
  EVT_BUTTON(ID_ADD_LOOT, EditMonstersDialog::OnAddLoot)
  EVT_BUTTON(ID_EDIT_LOOT, EditMonstersDialog::OnEditLoot)
  EVT_BUTTON(ID_REMOVE_LOOT, EditMonstersDialog::OnRemoveLoot)
  EVT_BUTTON(ID_ADD_TALK, EditMonstersDialog::OnAddTalk)
  EVT_BUTTON(ID_EDIT_TALK, EditMonstersDialog::OnEditTalk)
  EVT_BUTTON(ID_REMOVE_TALK, EditMonstersDialog::OnRemoveTalk)
  EVT_SPINCTRL(ID_OUTFIT_ID_SPIN, EditMonstersDialog::OnOutfitChanged)
  EVT_SPINCTRL(ID_OUTFIT_HEAD_SPIN, EditMonstersDialog::OnOutfitChanged)
  EVT_SPINCTRL(ID_OUTFIT_BODY_SPIN, EditMonstersDialog::OnOutfitChanged)
  EVT_SPINCTRL(ID_OUTFIT_LEGS_SPIN, EditMonstersDialog::OnOutfitChanged)
  EVT_SPINCTRL(ID_OUTFIT_FEET_SPIN, EditMonstersDialog::OnOutfitChanged)
  EVT_SPINCTRL(ID_OUTFIT_ITEM_SPIN, EditMonstersDialog::OnOutfitChanged)
  EVT_SPINCTRL(ID_CORPSE_SPIN, EditMonstersDialog::OnCorpseChanged)
END_EVENT_TABLE()

EditMonstersDialog::EditMonstersDialog(wxWindow* parent, const std::string& monDir) :
  wxDialog(parent, wxID_ANY, "Edit Monsters", wxDefaultPosition, wxSize(950, 700)),
  monDir(monDir)
{
  // Make a working copy
  workingCopy = IOMapSec::monsterTypes;

  wxBoxSizer* mainSizer = newd wxBoxSizer(wxHORIZONTAL);

  // ---- Left panel: monster list ----
  wxBoxSizer* leftSizer = newd wxBoxSizer(wxVERTICAL);

  leftSizer->Add(newd wxStaticText(this, wxID_ANY, "Filter:"), wxSizerFlags(0).Border(wxLEFT | wxTOP, 5));
  filterCtrl = newd wxTextCtrl(this, EDIT_MONSTERS_FILTER, "", wxDefaultPosition, wxSize(200, -1));
  leftSizer->Add(filterCtrl, wxSizerFlags(0).Expand().Border(wxALL, 5));

  monsterList = newd wxListBox(this, EDIT_MONSTERS_LISTBOX, wxDefaultPosition, wxSize(200, -1));
  leftSizer->Add(monsterList, wxSizerFlags(1).Expand().Border(wxLEFT | wxRIGHT | wxBOTTOM, 5));

  wxBoxSizer* listBtnSizer = newd wxBoxSizer(wxHORIZONTAL);
  listBtnSizer->Add(newd wxButton(this, EDIT_MONSTERS_ADD, "Add"), wxSizerFlags(1));
  listBtnSizer->Add(newd wxButton(this, EDIT_MONSTERS_REMOVE, "Remove"), wxSizerFlags(1));
  leftSizer->Add(listBtnSizer, wxSizerFlags(0).Expand().Border(wxLEFT | wxRIGHT | wxBOTTOM, 5));

  mainSizer->Add(leftSizer, wxSizerFlags(0).Expand());

  // ---- Right panel: notebook with tabs ----
  wxNotebook* notebook = newd wxNotebook(this, EDIT_MONSTERS_NOTEBOOK);

  // == Tab 1: Basic ==
  wxScrolledWindow* basicPanel = newd wxScrolledWindow(notebook, wxID_ANY);
  basicPanel->SetScrollRate(0, 10);
  wxFlexGridSizer* basicGrid = newd wxFlexGridSizer(2, 5, 10);
  basicGrid->AddGrowableCol(1);

  basicGrid->Add(newd wxStaticText(basicPanel, wxID_ANY, "Race Number"), wxSizerFlags().CenterVertical());
  raceNumberCtrl = newd wxSpinCtrl(basicPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 99999, 0);
  raceNumberCtrl->Enable(false);
  basicGrid->Add(raceNumberCtrl, wxSizerFlags(1).Expand());

  basicGrid->Add(newd wxStaticText(basicPanel, wxID_ANY, "Name"), wxSizerFlags().CenterVertical());
  nameCtrl = newd wxTextCtrl(basicPanel, wxID_ANY, "");
  basicGrid->Add(nameCtrl, wxSizerFlags(1).Expand());

  basicGrid->Add(newd wxStaticText(basicPanel, wxID_ANY, "Article"), wxSizerFlags().CenterVertical());
  articleCtrl = newd wxTextCtrl(basicPanel, wxID_ANY, "");
  basicGrid->Add(articleCtrl, wxSizerFlags(1).Expand());

  // Outfit ID + preview
  basicGrid->Add(newd wxStaticText(basicPanel, wxID_ANY, "Outfit"), wxSizerFlags().CenterVertical());
  {
    wxBoxSizer* outfitRow = newd wxBoxSizer(wxHORIZONTAL);
    outfitIdCtrl = newd wxSpinCtrl(basicPanel, ID_OUTFIT_ID_SPIN, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 99999, 0);
    outfitPreview = newd SpritePreviewPanel(basicPanel);
    outfitRow->Add(outfitPreview, wxSizerFlags(0).Border(wxRIGHT, 5));
    outfitRow->Add(outfitIdCtrl, wxSizerFlags(1).Expand());
    basicGrid->Add(outfitRow, wxSizerFlags(1).Expand());
  }

  const char* colorLabels[] = {"Head", "Body", "Legs", "Feet"};
  const int colorIds[] = {ID_OUTFIT_HEAD_SPIN, ID_OUTFIT_BODY_SPIN, ID_OUTFIT_LEGS_SPIN, ID_OUTFIT_FEET_SPIN};
  for(int i = 0; i < 4; ++i) {
    basicGrid->Add(newd wxStaticText(basicPanel, wxID_ANY, wxString("  ") + colorLabels[i]), wxSizerFlags().CenterVertical());
    outfitColorCtrls[i] = newd wxSpinCtrl(basicPanel, colorIds[i], "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 255, 0);
    basicGrid->Add(outfitColorCtrls[i], wxSizerFlags(1).Expand());
  }

  basicGrid->Add(newd wxStaticText(basicPanel, wxID_ANY, "  ItemType"), wxSizerFlags().CenterVertical());
  outfitItemTypeCtrl = newd wxSpinCtrl(basicPanel, ID_OUTFIT_ITEM_SPIN, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 99999, 0);
  basicGrid->Add(outfitItemTypeCtrl, wxSizerFlags(1).Expand());

  // Corpse + preview
  basicGrid->Add(newd wxStaticText(basicPanel, wxID_ANY, "Corpse"), wxSizerFlags().CenterVertical());
  {
    wxBoxSizer* corpseRow = newd wxBoxSizer(wxHORIZONTAL);
    corpseCtrl = newd wxSpinCtrl(basicPanel, ID_CORPSE_SPIN, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 99999, 0);
    corpsePreview = newd SpritePreviewPanel(basicPanel);
    corpseRow->Add(corpsePreview, wxSizerFlags(0).Border(wxRIGHT, 5));
    corpseRow->Add(corpseCtrl, wxSizerFlags(1).Expand());
    basicGrid->Add(corpseRow, wxSizerFlags(1).Expand());
  }

  basicGrid->Add(newd wxStaticText(basicPanel, wxID_ANY, "Blood"), wxSizerFlags().CenterVertical());
  wxArrayString bloodChoices;
  for(int i = 0; i < 5; ++i) bloodChoices.Add(kBloodNames[i]);
  bloodCtrl = newd wxChoice(basicPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, bloodChoices);
  basicGrid->Add(bloodCtrl, wxSizerFlags(1).Expand());

  basicGrid->Add(newd wxStaticText(basicPanel, wxID_ANY, "Experience"), wxSizerFlags().CenterVertical());
  experienceCtrl = newd wxSpinCtrl(basicPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10000000, 0);
  basicGrid->Add(experienceCtrl, wxSizerFlags(1).Expand());

  basicGrid->Add(newd wxStaticText(basicPanel, wxID_ANY, "Summon Cost"), wxSizerFlags().CenterVertical());
  summonCostCtrl = newd wxSpinCtrl(basicPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10000000, 0);
  basicGrid->Add(summonCostCtrl, wxSizerFlags(1).Expand());

  basicGrid->Add(newd wxStaticText(basicPanel, wxID_ANY, "Flee Threshold"), wxSizerFlags().CenterVertical());
  fleeThresholdCtrl = newd wxSpinCtrl(basicPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10000000, 0);
  basicGrid->Add(fleeThresholdCtrl, wxSizerFlags(1).Expand());

  basicGrid->Add(newd wxStaticText(basicPanel, wxID_ANY, "Attack"), wxSizerFlags().CenterVertical());
  attackCtrl = newd wxSpinCtrl(basicPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10000000, 0);
  basicGrid->Add(attackCtrl, wxSizerFlags(1).Expand());

  basicGrid->Add(newd wxStaticText(basicPanel, wxID_ANY, "Defend"), wxSizerFlags().CenterVertical());
  defendCtrl = newd wxSpinCtrl(basicPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10000000, 0);
  basicGrid->Add(defendCtrl, wxSizerFlags(1).Expand());

  basicGrid->Add(newd wxStaticText(basicPanel, wxID_ANY, "Armor"), wxSizerFlags().CenterVertical());
  armorCtrl = newd wxSpinCtrl(basicPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10000000, 0);
  basicGrid->Add(armorCtrl, wxSizerFlags(1).Expand());

  basicGrid->Add(newd wxStaticText(basicPanel, wxID_ANY, "Poison"), wxSizerFlags().CenterVertical());
  poisonCtrl = newd wxSpinCtrl(basicPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10000000, 0);
  basicGrid->Add(poisonCtrl, wxSizerFlags(1).Expand());

  basicGrid->Add(newd wxStaticText(basicPanel, wxID_ANY, "Lose Target"), wxSizerFlags().CenterVertical());
  loseTargetCtrl = newd wxSpinCtrl(basicPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10000000, 0);
  basicGrid->Add(loseTargetCtrl, wxSizerFlags(1).Expand());

  const char* stratLabels[] = {"Strategy Chase%", "Strategy Cast%", "Strategy Combat%", "Strategy Defend%"};
  for(int i = 0; i < 4; ++i) {
    basicGrid->Add(newd wxStaticText(basicPanel, wxID_ANY, stratLabels[i]), wxSizerFlags().CenterVertical());
    strategyCtrls[i] = newd wxSpinCtrl(basicPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, 0);
    basicGrid->Add(strategyCtrls[i], wxSizerFlags(1).Expand());
  }

  basicPanel->SetSizer(basicGrid);
  notebook->AddPage(basicPanel, "Basic");

  // == Tab 2: Flags ==
  wxPanel* flagsPanel = newd wxPanel(notebook, wxID_ANY);
  wxBoxSizer* flagsSizer = newd wxBoxSizer(wxVERTICAL);
  for(int i = 0; i < 14; ++i) {
    flagChecks[i] = newd wxCheckBox(flagsPanel, wxID_ANY, kFlagTable[i].name);
    flagsSizer->Add(flagChecks[i], wxSizerFlags(0).Border(wxALL, 3));
  }
  flagsPanel->SetSizer(flagsSizer);
  notebook->AddPage(flagsPanel, "Flags");

  // == Tab 3: Skills ==
  wxPanel* skillsPanel = newd wxPanel(notebook, wxID_ANY);
  wxBoxSizer* skillsSizer = newd wxBoxSizer(wxVERTICAL);
  skillList = newd wxListCtrl(skillsPanel, ID_SKILL_LIST, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
  skillList->InsertColumn(0, "Skill", wxLIST_FORMAT_LEFT, 100);
  skillList->InsertColumn(1, "Actual", wxLIST_FORMAT_RIGHT, 70);
  skillList->InsertColumn(2, "Min", wxLIST_FORMAT_RIGHT, 60);
  skillList->InsertColumn(3, "Max", wxLIST_FORMAT_RIGHT, 70);
  skillList->InsertColumn(4, "NextLvl", wxLIST_FORMAT_RIGHT, 65);
  skillList->InsertColumn(5, "Factor%", wxLIST_FORMAT_RIGHT, 65);
  skillList->InsertColumn(6, "AddLvl", wxLIST_FORMAT_RIGHT, 60);
  skillsSizer->Add(skillList, wxSizerFlags(1).Expand().Border(wxALL, 5));
  wxBoxSizer* skillBtnSizer = newd wxBoxSizer(wxHORIZONTAL);
  skillBtnSizer->Add(newd wxButton(skillsPanel, ID_ADD_SKILL, "Add"), wxSizerFlags(1));
  skillBtnSizer->Add(newd wxButton(skillsPanel, ID_EDIT_SKILL, "Edit"), wxSizerFlags(1));
  skillBtnSizer->Add(newd wxButton(skillsPanel, ID_REMOVE_SKILL, "Remove"), wxSizerFlags(1));
  skillsSizer->Add(skillBtnSizer, wxSizerFlags(0).Expand().Border(wxLEFT | wxRIGHT | wxBOTTOM, 5));
  skillsPanel->SetSizer(skillsSizer);
  notebook->AddPage(skillsPanel, "Skills");

  // == Tab 4: Spells ==
  wxPanel* spellsPanel = newd wxPanel(notebook, wxID_ANY);
  wxBoxSizer* spellsSizer = newd wxBoxSizer(wxVERTICAL);
  spellList = newd wxListCtrl(spellsPanel, ID_SPELL_LIST, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
  spellList->InsertColumn(0, "Shape", wxLIST_FORMAT_LEFT, 180);
  spellList->InsertColumn(1, "Impact", wxLIST_FORMAT_LEFT, 220);
  spellList->InsertColumn(2, "Chance (1/N)", wxLIST_FORMAT_RIGHT, 90);
  spellsSizer->Add(spellList, wxSizerFlags(1).Expand().Border(wxALL, 5));
  wxBoxSizer* spellBtnSizer = newd wxBoxSizer(wxHORIZONTAL);
  spellBtnSizer->Add(newd wxButton(spellsPanel, ID_ADD_SPELL, "Add"), wxSizerFlags(1));
  spellBtnSizer->Add(newd wxButton(spellsPanel, ID_EDIT_SPELL, "Edit"), wxSizerFlags(1));
  spellBtnSizer->Add(newd wxButton(spellsPanel, ID_REMOVE_SPELL, "Remove"), wxSizerFlags(1));
  spellsSizer->Add(spellBtnSizer, wxSizerFlags(0).Expand().Border(wxLEFT | wxRIGHT | wxBOTTOM, 5));
  spellsPanel->SetSizer(spellsSizer);
  notebook->AddPage(spellsPanel, "Spells");

  // == Tab 5: Inventory ==
  wxPanel* invPanel = newd wxPanel(notebook, wxID_ANY);
  wxBoxSizer* invSizer = newd wxBoxSizer(wxVERTICAL);
  inventoryList = newd wxListCtrl(invPanel, ID_INVENTORY_LIST, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
  inventoryList->InsertColumn(0, "Item ID", wxLIST_FORMAT_LEFT, 100);
  inventoryList->InsertColumn(1, "Quantity", wxLIST_FORMAT_RIGHT, 80);
  inventoryList->InsertColumn(2, "Probability", wxLIST_FORMAT_RIGHT, 100);
  invSizer->Add(inventoryList, wxSizerFlags(1).Expand().Border(wxALL, 5));
  wxBoxSizer* invBtnSizer = newd wxBoxSizer(wxHORIZONTAL);
  invBtnSizer->Add(newd wxButton(invPanel, ID_ADD_LOOT, "Add"), wxSizerFlags(1));
  invBtnSizer->Add(newd wxButton(invPanel, ID_EDIT_LOOT, "Edit"), wxSizerFlags(1));
  invBtnSizer->Add(newd wxButton(invPanel, ID_REMOVE_LOOT, "Remove"), wxSizerFlags(1));
  invSizer->Add(invBtnSizer, wxSizerFlags(0).Expand().Border(wxLEFT | wxRIGHT | wxBOTTOM, 5));
  invPanel->SetSizer(invSizer);
  notebook->AddPage(invPanel, "Inventory");

  // == Tab 6: Talk ==
  wxPanel* talkPanel = newd wxPanel(notebook, wxID_ANY);
  wxBoxSizer* talkSizer = newd wxBoxSizer(wxVERTICAL);
  talkList = newd wxListBox(talkPanel, ID_TALK_LIST, wxDefaultPosition, wxDefaultSize);
  talkSizer->Add(talkList, wxSizerFlags(1).Expand().Border(wxALL, 5));
  wxBoxSizer* talkBtnSizer = newd wxBoxSizer(wxHORIZONTAL);
  talkBtnSizer->Add(newd wxButton(talkPanel, ID_ADD_TALK, "Add"), wxSizerFlags(1));
  talkBtnSizer->Add(newd wxButton(talkPanel, ID_EDIT_TALK, "Edit"), wxSizerFlags(1));
  talkBtnSizer->Add(newd wxButton(talkPanel, ID_REMOVE_TALK, "Remove"), wxSizerFlags(1));
  talkSizer->Add(talkBtnSizer, wxSizerFlags(0).Expand().Border(wxLEFT | wxRIGHT | wxBOTTOM, 5));
  talkPanel->SetSizer(talkSizer);
  notebook->AddPage(talkPanel, "Talk");

  mainSizer->Add(notebook, wxSizerFlags(1).Expand().Border(wxALL, 5));

  // ---- Bottom buttons ----
  wxBoxSizer* outerSizer = newd wxBoxSizer(wxVERTICAL);
  outerSizer->Add(mainSizer, wxSizerFlags(1).Expand());

  wxBoxSizer* bottomSizer = newd wxBoxSizer(wxHORIZONTAL);
  bottomSizer->Add(newd wxButton(this, EDIT_MONSTERS_SAVE_ALL, "Save All"), wxSizerFlags(0).Border(wxRIGHT, 20));
  bottomSizer->AddStretchSpacer();
  bottomSizer->Add(newd wxButton(this, wxID_OK, "OK"), wxSizerFlags(0).Border(wxRIGHT, 5));
  bottomSizer->Add(newd wxButton(this, wxID_CANCEL, "Cancel"), wxSizerFlags(0));
  outerSizer->Add(bottomSizer, wxSizerFlags(0).Expand().Border(wxALL, 10));

  SetSizer(outerSizer);
  SetMinSize(wxSize(800, 500));

  BuildMonsterList();
  if(!sortedRaceNumbers.empty()) {
    monsterList->SetSelection(0);
    LoadMonster(sortedRaceNumbers[0]);
  }

  Centre(wxBOTH);
}

EditMonstersDialog::~EditMonstersDialog() {}

void EditMonstersDialog::BuildMonsterList() {
  monsterList->Clear();
  sortedRaceNumbers.clear();

  std::string filter = filterCtrl->GetValue().ToStdString();
  std::string filterLower;
  for(char c : filter) filterLower += std::tolower((unsigned char)c);

  // Collect and sort by name
  std::vector<std::pair<std::string, int>> nameRace;
  for(const auto& pair : workingCopy) {
    std::string nameLower;
    for(char c : pair.second.name) nameLower += std::tolower((unsigned char)c);
    if(!filterLower.empty() && nameLower.find(filterLower) == std::string::npos) continue;
    nameRace.push_back({pair.second.name, pair.first});
  }
  std::sort(nameRace.begin(), nameRace.end());

  for(const auto& nr : nameRace) {
    monsterList->Append(wxString(nr.first) + wxString::Format(" (#%d)", nr.second));
    sortedRaceNumbers.push_back(nr.second);
  }
}

void EditMonstersDialog::SaveCurrentMonster() {
  if(currentRace < 0) return;
  auto it = workingCopy.find(currentRace);
  if(it == workingCopy.end()) return;

  IOMapSec::SecMonsterType& mon = it->second;
  mon.name = nameCtrl->GetValue().ToStdString();
  mon.article = articleCtrl->GetValue().ToStdString();
  mon.outfitId = outfitIdCtrl->GetValue();
  for(int i = 0; i < 4; ++i) mon.outfitColors[i] = outfitColorCtrls[i]->GetValue();
  mon.outfitItemType = outfitItemTypeCtrl->GetValue();
  mon.corpse = corpseCtrl->GetValue();
  mon.blood = (IOMapSec::SecBloodType)bloodCtrl->GetSelection();
  mon.experience = experienceCtrl->GetValue();
  mon.summonCost = summonCostCtrl->GetValue();
  mon.fleeThreshold = fleeThresholdCtrl->GetValue();
  mon.attack = attackCtrl->GetValue();
  mon.defend = defendCtrl->GetValue();
  mon.armor = armorCtrl->GetValue();
  mon.poison = poisonCtrl->GetValue();
  mon.loseTarget = loseTargetCtrl->GetValue();
  for(int i = 0; i < 4; ++i) mon.strategy[i] = strategyCtrls[i]->GetValue();

  // Flags
  mon.flags = 0;
  for(int i = 0; i < 14; ++i) {
    if(flagChecks[i]->GetValue()) mon.flags |= kFlagTable[i].bit;
  }

  // Skills, spells, inventory, talk are already updated in-place via the sub-dialogs
  // Update hitpoints from skills
  for(const auto& sk : mon.skills) {
    std::string lower;
    for(char c : sk.name) lower += std::tolower((unsigned char)c);
    if(lower == "hitpoints") { mon.hitpoints = sk.actual; break; }
  }
}

void EditMonstersDialog::LoadMonster(int raceNumber) {
  currentRace = raceNumber;
  auto it = workingCopy.find(raceNumber);
  if(it == workingCopy.end()) return;

  const IOMapSec::SecMonsterType& mon = it->second;
  raceNumberCtrl->SetValue(mon.raceNumber);
  nameCtrl->SetValue(mon.name);
  articleCtrl->SetValue(mon.article);
  outfitIdCtrl->SetValue(mon.outfitId);
  for(int i = 0; i < 4; ++i) outfitColorCtrls[i]->SetValue(mon.outfitColors[i]);
  outfitItemTypeCtrl->SetValue(mon.outfitItemType);
  corpseCtrl->SetValue(mon.corpse);
  bloodCtrl->SetSelection(mon.blood);
  experienceCtrl->SetValue(mon.experience);
  summonCostCtrl->SetValue(mon.summonCost);
  fleeThresholdCtrl->SetValue(mon.fleeThreshold);
  attackCtrl->SetValue(mon.attack);
  defendCtrl->SetValue(mon.defend);
  armorCtrl->SetValue(mon.armor);
  poisonCtrl->SetValue(mon.poison);
  loseTargetCtrl->SetValue(mon.loseTarget);
  for(int i = 0; i < 4; ++i) strategyCtrls[i]->SetValue(mon.strategy[i]);

  // Flags
  for(int i = 0; i < 14; ++i) {
    flagChecks[i]->SetValue((mon.flags & kFlagTable[i].bit) != 0);
  }

  RefreshSkillList();
  RefreshSpellList();
  RefreshInventoryList();
  RefreshTalkList();
  RefreshPreviews();
}

void EditMonstersDialog::OnListSelect(wxCommandEvent&) {
  int sel = monsterList->GetSelection();
  if(sel < 0 || sel >= (int)sortedRaceNumbers.size()) return;
  SaveCurrentMonster();
  LoadMonster(sortedRaceNumbers[sel]);
}

void EditMonstersDialog::OnFilterChanged(wxCommandEvent&) {
  SaveCurrentMonster();
  BuildMonsterList();
  currentRace = -1;
  if(!sortedRaceNumbers.empty()) {
    monsterList->SetSelection(0);
    LoadMonster(sortedRaceNumbers[0]);
  }
}

void EditMonstersDialog::OnAddMonster(wxCommandEvent&) {
  // Find next available race number
  int nextRace = 1;
  for(const auto& pair : workingCopy) {
    if(pair.first >= nextRace) nextRace = pair.first + 1;
  }

  IOMapSec::SecMonsterType mon;
  mon.raceNumber = nextRace;
  mon.name = "New Monster";
  mon.article = "a";
  mon.blood = IOMapSec::BLOOD_BLOOD;
  mon.strategy[0] = 100;
  // Generate filename
  std::string filename = monDir + "newmonster" + std::to_string(nextRace) + ".mon";
  mon.sourceFilePath = filename;

  workingCopy[nextRace] = mon;
  BuildMonsterList();

  // Select the new monster
  for(int i = 0; i < (int)sortedRaceNumbers.size(); ++i) {
    if(sortedRaceNumbers[i] == nextRace) {
      monsterList->SetSelection(i);
      LoadMonster(nextRace);
      break;
    }
  }
}

void EditMonstersDialog::OnRemoveMonster(wxCommandEvent&) {
  if(currentRace < 0) return;
  auto it = workingCopy.find(currentRace);
  if(it == workingCopy.end()) return;

  int ret = wxMessageBox(wxString("Delete monster '") + wxString(it->second.name) + "'? This will also delete the .mon file on save.", "Confirm", wxYES_NO | wxICON_WARNING, this);
  if(ret != wxYES) return;

  // Delete the file if it exists
  if(!it->second.sourceFilePath.empty() && fs::exists(it->second.sourceFilePath)) {
    fs::remove(it->second.sourceFilePath);
  }

  workingCopy.erase(it);
  currentRace = -1;
  BuildMonsterList();
  if(!sortedRaceNumbers.empty()) {
    monsterList->SetSelection(0);
    LoadMonster(sortedRaceNumbers[0]);
  }
}

void EditMonstersDialog::OnSaveAll(wxCommandEvent&) {
  SaveCurrentMonster();
  // Write all to static map and save to files
  IOMapSec::monsterTypes = workingCopy;
  for(const auto& pair : workingCopy) {
    IOMapSec::saveMonsterType(pair.second);
  }
  wxMessageBox("All monsters saved.", "Save", wxOK | wxICON_INFORMATION, this);
}

void EditMonstersDialog::OnClickOK(wxCommandEvent&) {
  SaveCurrentMonster();
  IOMapSec::monsterTypes = workingCopy;
  for(const auto& pair : workingCopy) {
    IOMapSec::saveMonsterType(pair.second);
  }
  EndModal(wxID_OK);
}

void EditMonstersDialog::OnClickCancel(wxCommandEvent&) {
  EndModal(wxID_CANCEL);
}

// ---- Refresh helpers for lists ----

static std::string formatSpellShape(const IOMapSec::SecMonsterSpell& sp) {
  std::string r = kShapeNames[sp.shape];
  r += " (";
  for(int i = 0; i < sp.shapeParamCount; ++i) {
    if(i > 0) r += ", ";
    r += std::to_string(sp.shapeParams[i]);
  }
  r += ")";
  return r;
}

static std::string formatSpellImpact(const IOMapSec::SecMonsterSpell& sp) {
  std::string r = kImpactNames[sp.impact];
  r += " (";
  if(sp.impact == IOMapSec::IMPACT_OUTFIT) {
    r += "(" + std::to_string(sp.impactParams[0]) + ", " + std::to_string(sp.impactParams[1]) + "), " + std::to_string(sp.impactParams[2]);
  } else {
    for(int i = 0; i < sp.impactParamCount; ++i) {
      if(i > 0) r += ", ";
      r += std::to_string(sp.impactParams[i]);
    }
  }
  r += ")";
  return r;
}

void EditMonstersDialog::RefreshSkillList() {
  skillList->DeleteAllItems();
  if(currentRace < 0) return;
  auto it = workingCopy.find(currentRace);
  if(it == workingCopy.end()) return;
  for(int i = 0; i < (int)it->second.skills.size(); ++i) {
    const auto& sk = it->second.skills[i];
    long idx = skillList->InsertItem(i, sk.name);
    skillList->SetItem(idx, 1, std::to_string(sk.actual));
    skillList->SetItem(idx, 2, std::to_string(sk.minimum));
    skillList->SetItem(idx, 3, std::to_string(sk.maximum));
    skillList->SetItem(idx, 4, std::to_string(sk.nextLevel));
    skillList->SetItem(idx, 5, std::to_string(sk.factorPercent));
    skillList->SetItem(idx, 6, std::to_string(sk.addLevel));
  }
}

void EditMonstersDialog::RefreshSpellList() {
  spellList->DeleteAllItems();
  if(currentRace < 0) return;
  auto it = workingCopy.find(currentRace);
  if(it == workingCopy.end()) return;
  for(int i = 0; i < (int)it->second.spells.size(); ++i) {
    const auto& sp = it->second.spells[i];
    long idx = spellList->InsertItem(i, formatSpellShape(sp));
    spellList->SetItem(idx, 1, formatSpellImpact(sp));
    spellList->SetItem(idx, 2, std::to_string(sp.delay));
  }
}

static wxBitmap MakeItemBitmap(int clientItemId) {
  wxBitmap bmp(32, 32, 24);
  wxMemoryDC dc(bmp);
  dc.SetBrush(*wxWHITE_BRUSH);
  dc.SetPen(*wxWHITE_PEN);
  dc.DrawRectangle(0, 0, 32, 32);
  if(!g_gui.gfx.isUnloaded() && clientItemId > 0) {
    Sprite* spr = g_gui.gfx.getSprite(clientItemId);
    if(spr) spr->DrawTo(&dc, SPRITE_SIZE_32x32, 0, 0);
  }
  dc.SelectObject(wxNullBitmap);
  return bmp;
}

void EditMonstersDialog::RefreshInventoryList() {
  inventoryList->DeleteAllItems();
  if(currentRace < 0) return;
  auto it = workingCopy.find(currentRace);
  if(it == workingCopy.end()) return;

  wxImageList* imgList = newd wxImageList(32, 32, false);
  for(int i = 0; i < (int)it->second.inventory.size(); ++i) {
    const auto& loot = it->second.inventory[i];
    imgList->Add(MakeItemBitmap(loot.itemId));
  }
  inventoryList->AssignImageList(imgList, wxIMAGE_LIST_SMALL);

  for(int i = 0; i < (int)it->second.inventory.size(); ++i) {
    const auto& loot = it->second.inventory[i];
    long idx = inventoryList->InsertItem(i, std::to_string(loot.itemId), i);
    inventoryList->SetItem(idx, 1, std::to_string(loot.maxQuantity));
    inventoryList->SetItem(idx, 2, std::to_string(loot.probabilityPerMille));
  }
}

void EditMonstersDialog::RefreshTalkList() {
  talkList->Clear();
  if(currentRace < 0) return;
  auto it = workingCopy.find(currentRace);
  if(it == workingCopy.end()) return;
  for(const auto& t : it->second.talk) {
    talkList->Append(t);
  }
}

// ---- Skill events ----

void EditMonstersDialog::OnAddSkill(wxCommandEvent&) {
  if(currentRace < 0) return;
  auto it = workingCopy.find(currentRace);
  if(it == workingCopy.end()) return;
  IOMapSec::SecMonsterSkill skill;
  skill.name = "HitPoints";
  EditSkillDialog dlg(this, skill);
  if(dlg.ShowModal() == wxID_OK) {
    it->second.skills.push_back(skill);
    RefreshSkillList();
  }
}

void EditMonstersDialog::OnEditSkill(wxCommandEvent&) {
  if(currentRace < 0) return;
  long sel = skillList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
  if(sel < 0) return;
  auto it = workingCopy.find(currentRace);
  if(it == workingCopy.end() || sel >= (int)it->second.skills.size()) return;
  EditSkillDialog dlg(this, it->second.skills[sel]);
  if(dlg.ShowModal() == wxID_OK) RefreshSkillList();
}

void EditMonstersDialog::OnRemoveSkill(wxCommandEvent&) {
  if(currentRace < 0) return;
  long sel = skillList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
  if(sel < 0) return;
  auto it = workingCopy.find(currentRace);
  if(it == workingCopy.end() || sel >= (int)it->second.skills.size()) return;
  it->second.skills.erase(it->second.skills.begin() + sel);
  RefreshSkillList();
}

// ---- Spell events ----

void EditMonstersDialog::OnAddSpell(wxCommandEvent&) {
  if(currentRace < 0) return;
  auto it = workingCopy.find(currentRace);
  if(it == workingCopy.end()) return;
  IOMapSec::SecMonsterSpell spell;
  spell.shapeParamCount = 1;
  spell.impactParamCount = 3;
  spell.delay = 10;
  EditSpellDialog dlg(this, spell);
  if(dlg.ShowModal() == wxID_OK) {
    it->second.spells.push_back(spell);
    RefreshSpellList();
  }
}

void EditMonstersDialog::OnEditSpell(wxCommandEvent&) {
  if(currentRace < 0) return;
  long sel = spellList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
  if(sel < 0) return;
  auto it = workingCopy.find(currentRace);
  if(it == workingCopy.end() || sel >= (int)it->second.spells.size()) return;
  EditSpellDialog dlg(this, it->second.spells[sel]);
  if(dlg.ShowModal() == wxID_OK) RefreshSpellList();
}

void EditMonstersDialog::OnRemoveSpell(wxCommandEvent&) {
  if(currentRace < 0) return;
  long sel = spellList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
  if(sel < 0) return;
  auto it = workingCopy.find(currentRace);
  if(it == workingCopy.end() || sel >= (int)it->second.spells.size()) return;
  it->second.spells.erase(it->second.spells.begin() + sel);
  RefreshSpellList();
}

// ---- Loot events ----

void EditMonstersDialog::OnAddLoot(wxCommandEvent&) {
  if(currentRace < 0) return;
  auto it = workingCopy.find(currentRace);
  if(it == workingCopy.end()) return;
  IOMapSec::SecMonsterLoot loot;
  loot.maxQuantity = 1;
  loot.probabilityPerMille = 100;
  EditLootDialog dlg(this, loot);
  if(dlg.ShowModal() == wxID_OK) {
    it->second.inventory.push_back(loot);
    RefreshInventoryList();
  }
}

void EditMonstersDialog::OnEditLoot(wxCommandEvent&) {
  if(currentRace < 0) return;
  long sel = inventoryList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
  if(sel < 0) return;
  auto it = workingCopy.find(currentRace);
  if(it == workingCopy.end() || sel >= (int)it->second.inventory.size()) return;
  EditLootDialog dlg(this, it->second.inventory[sel]);
  if(dlg.ShowModal() == wxID_OK) RefreshInventoryList();
}

void EditMonstersDialog::OnRemoveLoot(wxCommandEvent&) {
  if(currentRace < 0) return;
  long sel = inventoryList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
  if(sel < 0) return;
  auto it = workingCopy.find(currentRace);
  if(it == workingCopy.end() || sel >= (int)it->second.inventory.size()) return;
  it->second.inventory.erase(it->second.inventory.begin() + sel);
  RefreshInventoryList();
}

// ---- Talk events ----

void EditMonstersDialog::OnAddTalk(wxCommandEvent&) {
  if(currentRace < 0) return;
  auto it = workingCopy.find(currentRace);
  if(it == workingCopy.end()) return;
  wxTextEntryDialog dlg(this, "Enter talk text (use #Y prefix for yell):", "Add Talk");
  if(dlg.ShowModal() == wxID_OK) {
    it->second.talk.push_back(dlg.GetValue().ToStdString());
    RefreshTalkList();
  }
}

void EditMonstersDialog::OnEditTalk(wxCommandEvent&) {
  if(currentRace < 0) return;
  int sel = talkList->GetSelection();
  if(sel < 0) return;
  auto it = workingCopy.find(currentRace);
  if(it == workingCopy.end() || sel >= (int)it->second.talk.size()) return;
  wxTextEntryDialog dlg(this, "Edit talk text (use #Y prefix for yell):", "Edit Talk", it->second.talk[sel]);
  if(dlg.ShowModal() == wxID_OK) {
    it->second.talk[sel] = dlg.GetValue().ToStdString();
    RefreshTalkList();
  }
}

void EditMonstersDialog::OnRemoveTalk(wxCommandEvent&) {
  if(currentRace < 0) return;
  int sel = talkList->GetSelection();
  if(sel < 0) return;
  auto it = workingCopy.find(currentRace);
  if(it == workingCopy.end() || sel >= (int)it->second.talk.size()) return;
  it->second.talk.erase(it->second.talk.begin() + sel);
  RefreshTalkList();
}

// ---- Sprite previews ----

void EditMonstersDialog::RefreshPreviews() {
  if(currentRace < 0) {
    outfitPreview->ClearSprite();
    corpsePreview->ClearSprite();
    return;
  }
  outfitPreview->SetOutfit(outfitIdCtrl->GetValue(), outfitColorCtrls[0]->GetValue(), outfitColorCtrls[1]->GetValue(), outfitColorCtrls[2]->GetValue(), outfitColorCtrls[3]->GetValue());
  corpsePreview->SetItemSprite(corpseCtrl->GetValue());
}

void EditMonstersDialog::OnOutfitChanged(wxSpinEvent&) {
  RefreshPreviews();
}

void EditMonstersDialog::OnCorpseChanged(wxSpinEvent&) {
  corpsePreview->SetItemSprite(corpseCtrl->GetValue());
}
