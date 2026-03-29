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

#include "item_editor_dialog.h"
#include "monster_editor_dialog.h" // for SpritePreviewPanel
#include "gui.h"
#include "gui_ids.h"
#include "graphics.h"

#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/textdlg.h>
#include <wx/choicdlg.h>
#include <wx/dcbuffer.h>
#include <wx/notebook.h>

enum {
  ID_ITEM_ADD_ATTR = wxID_HIGHEST + 700,
  ID_ITEM_EDIT_ATTR,
  ID_ITEM_REMOVE_ATTR,
  ID_ITEM_ATTR_LIST,
  ID_ITEM_ADD_FLAG,
  ID_ITEM_REMOVE_FLAG,
  ID_ITEM_ADD_ATTR_KEY,
  ID_ITEM_REMOVE_ATTR_KEY,
  ID_ITEM_CONFIG_FLAG_LIST,
  ID_ITEM_CONFIG_ATTR_LIST,
};

BEGIN_EVENT_TABLE(EditItemsDialog, wxDialog)
  EVT_LISTBOX(EDIT_ITEMS_LISTBOX, EditItemsDialog::OnListSelect)
  EVT_TEXT(EDIT_ITEMS_FILTER, EditItemsDialog::OnFilterChanged)
  EVT_BUTTON(EDIT_ITEMS_SAVE_ALL, EditItemsDialog::OnSaveAll)
  EVT_BUTTON(wxID_OK, EditItemsDialog::OnClickOK)
  EVT_BUTTON(wxID_CANCEL, EditItemsDialog::OnClickCancel)
  EVT_BUTTON(ID_ITEM_ADD_ATTR, EditItemsDialog::OnAddAttribute)
  EVT_BUTTON(ID_ITEM_EDIT_ATTR, EditItemsDialog::OnEditAttribute)
  EVT_BUTTON(ID_ITEM_REMOVE_ATTR, EditItemsDialog::OnRemoveAttribute)
  EVT_BUTTON(ID_ITEM_ADD_FLAG, EditItemsDialog::OnAddFlag)
  EVT_BUTTON(ID_ITEM_REMOVE_FLAG, EditItemsDialog::OnRemoveFlag)
  EVT_BUTTON(ID_ITEM_ADD_ATTR_KEY, EditItemsDialog::OnAddAttrKey)
  EVT_BUTTON(ID_ITEM_REMOVE_ATTR_KEY, EditItemsDialog::OnRemoveAttrKey)
END_EVENT_TABLE()

EditItemsDialog::EditItemsDialog(wxWindow* parent) :
  wxDialog(parent, wxID_ANY, "Edit Items (objects.srv)", wxDefaultPosition, wxSize(950, 700))
{
  workingCopy = IOMapSec::objectTypes;
  workingConfig = IOMapSec::objectConfig;

  wxBoxSizer* mainSizer = newd wxBoxSizer(wxHORIZONTAL);

  // ---- Left panel: item list ----
  wxBoxSizer* leftSizer = newd wxBoxSizer(wxVERTICAL);

  leftSizer->Add(newd wxStaticText(this, wxID_ANY, "Filter:"), wxSizerFlags(0).Border(wxLEFT | wxTOP, 5));
  filterCtrl = newd wxTextCtrl(this, EDIT_ITEMS_FILTER, "", wxDefaultPosition, wxSize(200, -1));
  leftSizer->Add(filterCtrl, wxSizerFlags(0).Expand().Border(wxALL, 5));

  itemList = newd wxListBox(this, EDIT_ITEMS_LISTBOX, wxDefaultPosition, wxSize(200, -1));
  leftSizer->Add(itemList, wxSizerFlags(1).Expand().Border(wxLEFT | wxRIGHT | wxBOTTOM, 5));

  mainSizer->Add(leftSizer, wxSizerFlags(0).Expand());

  // ---- Right panel: notebook with tabs ----
  wxNotebook* notebook = newd wxNotebook(this, EDIT_ITEMS_NOTEBOOK);

  // == Tab 1: Basic ==
  wxPanel* basicPanel = newd wxPanel(notebook, wxID_ANY);
  wxFlexGridSizer* basicGrid = newd wxFlexGridSizer(2, 5, 10);
  basicGrid->AddGrowableCol(1);

  basicGrid->Add(newd wxStaticText(basicPanel, wxID_ANY, "TypeID"), wxSizerFlags().CenterVertical());
  {
    wxBoxSizer* idRow = newd wxBoxSizer(wxHORIZONTAL);
    spritePreview = newd SpritePreviewPanel(basicPanel);
    idRow->Add(spritePreview, wxSizerFlags(0).Border(wxRIGHT, 5));
    typeIdCtrl = newd wxSpinCtrl(basicPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 99999, 0);
    typeIdCtrl->Enable(false);
    idRow->Add(typeIdCtrl, wxSizerFlags(1).Expand());
    basicGrid->Add(idRow, wxSizerFlags(1).Expand());
  }

  basicGrid->Add(newd wxStaticText(basicPanel, wxID_ANY, "Name"), wxSizerFlags().CenterVertical());
  nameCtrl = newd wxTextCtrl(basicPanel, wxID_ANY, "");
  basicGrid->Add(nameCtrl, wxSizerFlags(1).Expand());

  basicGrid->Add(newd wxStaticText(basicPanel, wxID_ANY, "Description"), wxSizerFlags().CenterVertical());
  descCtrl = newd wxTextCtrl(basicPanel, wxID_ANY, "");
  basicGrid->Add(descCtrl, wxSizerFlags(1).Expand());

  basicPanel->SetSizer(basicGrid);
  notebook->AddPage(basicPanel, "Basic");

  // == Tab 2: Flags ==
  wxScrolledWindow* flagsScroll = newd wxScrolledWindow(notebook, wxID_ANY);
  flagsScroll->SetScrollRate(0, 10);
  flagsPanel = flagsScroll;
  flagsSizer = newd wxBoxSizer(wxVERTICAL);
  flagsPanel->SetSizer(flagsSizer);
  RebuildFlagCheckboxes();
  notebook->AddPage(flagsScroll, "Flags");

  // == Tab 3: Attributes ==
  wxPanel* attrPanel = newd wxPanel(notebook, wxID_ANY);
  wxBoxSizer* attrSizer = newd wxBoxSizer(wxVERTICAL);
  attrList = newd wxListCtrl(attrPanel, ID_ITEM_ATTR_LIST, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
  attrList->InsertColumn(0, "Attribute", wxLIST_FORMAT_LEFT, 200);
  attrList->InsertColumn(1, "Value", wxLIST_FORMAT_RIGHT, 150);
  attrSizer->Add(attrList, wxSizerFlags(1).Expand().Border(wxALL, 5));
  wxBoxSizer* attrBtnSizer = newd wxBoxSizer(wxHORIZONTAL);
  attrBtnSizer->Add(newd wxButton(attrPanel, ID_ITEM_ADD_ATTR, "Add"), wxSizerFlags(1));
  attrBtnSizer->Add(newd wxButton(attrPanel, ID_ITEM_EDIT_ATTR, "Edit"), wxSizerFlags(1));
  attrBtnSizer->Add(newd wxButton(attrPanel, ID_ITEM_REMOVE_ATTR, "Remove"), wxSizerFlags(1));
  attrSizer->Add(attrBtnSizer, wxSizerFlags(0).Expand().Border(wxLEFT | wxRIGHT | wxBOTTOM, 5));
  attrPanel->SetSizer(attrSizer);
  notebook->AddPage(attrPanel, "Attributes");

  // == Tab 4: Config (manage available flags/attribute keys) ==
  wxPanel* configPanel = newd wxPanel(notebook, wxID_ANY);
  wxBoxSizer* configSizer = newd wxBoxSizer(wxHORIZONTAL);

  // Left: flags config
  wxBoxSizer* cfgFlagSizer = newd wxBoxSizer(wxVERTICAL);
  cfgFlagSizer->Add(newd wxStaticText(configPanel, wxID_ANY, "Available Flags:"), wxSizerFlags(0).Border(wxALL, 5));
  configFlagList = newd wxListBox(configPanel, ID_ITEM_CONFIG_FLAG_LIST, wxDefaultPosition, wxDefaultSize);
  cfgFlagSizer->Add(configFlagList, wxSizerFlags(1).Expand().Border(wxLEFT | wxRIGHT, 5));
  wxBoxSizer* cfgFlagBtnSizer = newd wxBoxSizer(wxHORIZONTAL);
  cfgFlagBtnSizer->Add(newd wxButton(configPanel, ID_ITEM_ADD_FLAG, "Add Flag"), wxSizerFlags(1));
  cfgFlagBtnSizer->Add(newd wxButton(configPanel, ID_ITEM_REMOVE_FLAG, "Remove"), wxSizerFlags(1));
  cfgFlagSizer->Add(cfgFlagBtnSizer, wxSizerFlags(0).Expand().Border(wxALL, 5));
  configSizer->Add(cfgFlagSizer, wxSizerFlags(1).Expand());

  // Right: attribute keys config
  wxBoxSizer* cfgAttrSizer = newd wxBoxSizer(wxVERTICAL);
  cfgAttrSizer->Add(newd wxStaticText(configPanel, wxID_ANY, "Available Attribute Keys:"), wxSizerFlags(0).Border(wxALL, 5));
  configAttrList = newd wxListBox(configPanel, ID_ITEM_CONFIG_ATTR_LIST, wxDefaultPosition, wxDefaultSize);
  cfgAttrSizer->Add(configAttrList, wxSizerFlags(1).Expand().Border(wxLEFT | wxRIGHT, 5));
  wxBoxSizer* cfgAttrBtnSizer = newd wxBoxSizer(wxHORIZONTAL);
  cfgAttrBtnSizer->Add(newd wxButton(configPanel, ID_ITEM_ADD_ATTR_KEY, "Add Key"), wxSizerFlags(1));
  cfgAttrBtnSizer->Add(newd wxButton(configPanel, ID_ITEM_REMOVE_ATTR_KEY, "Remove"), wxSizerFlags(1));
  cfgAttrSizer->Add(cfgAttrBtnSizer, wxSizerFlags(0).Expand().Border(wxALL, 5));
  configSizer->Add(cfgAttrSizer, wxSizerFlags(1).Expand());

  configPanel->SetSizer(configSizer);
  notebook->AddPage(configPanel, "Config");

  mainSizer->Add(notebook, wxSizerFlags(1).Expand().Border(wxALL, 5));

  // ---- Bottom buttons ----
  wxBoxSizer* outerSizer = newd wxBoxSizer(wxVERTICAL);
  outerSizer->Add(mainSizer, wxSizerFlags(1).Expand());

  wxBoxSizer* bottomSizer = newd wxBoxSizer(wxHORIZONTAL);
  bottomSizer->Add(newd wxButton(this, EDIT_ITEMS_SAVE_ALL, "Save All"), wxSizerFlags(0).Border(wxRIGHT, 20));
  bottomSizer->AddStretchSpacer();
  bottomSizer->Add(newd wxButton(this, wxID_OK, "OK"), wxSizerFlags(0).Border(wxRIGHT, 5));
  bottomSizer->Add(newd wxButton(this, wxID_CANCEL, "Cancel"), wxSizerFlags(0));
  outerSizer->Add(bottomSizer, wxSizerFlags(0).Expand().Border(wxALL, 10));

  SetSizer(outerSizer);
  SetMinSize(wxSize(800, 500));

  RefreshConfigLists();
  BuildItemList();
  if(!sortedTypeIds.empty()) {
    itemList->SetSelection(0);
    LoadItem(sortedTypeIds[0]);
  }

  Centre(wxBOTH);
}

EditItemsDialog::~EditItemsDialog() {}

void EditItemsDialog::RebuildFlagCheckboxes() {
  // Clear existing checkboxes
  flagsSizer->Clear(true);
  flagChecks.clear();
  // Create one checkbox per available flag
  for(const auto& fname : workingConfig.flagNames) {
    wxCheckBox* cb = newd wxCheckBox(flagsPanel, wxID_ANY, fname);
    flagsSizer->Add(cb, wxSizerFlags(0).Border(wxALL, 3));
    flagChecks.push_back(cb);
  }
  flagsPanel->Layout();
  flagsPanel->FitInside();
}

void EditItemsDialog::BuildItemList() {
  itemList->Clear();
  sortedTypeIds.clear();

  std::string filter = filterCtrl->GetValue().ToStdString();
  std::string filterLower;
  for(char c : filter) filterLower += std::tolower((unsigned char)c);

  // Collect items with ID >= 100
  std::vector<std::pair<int, std::string>> items;
  for(const auto& pair : workingCopy) {
    if(pair.first < 100) continue;
    std::string nameLower;
    for(char c : pair.second.name) nameLower += std::tolower((unsigned char)c);
    std::string idStr = std::to_string(pair.first);
    if(!filterLower.empty() && nameLower.find(filterLower) == std::string::npos && idStr.find(filterLower) == std::string::npos) continue;
    items.push_back({pair.first, pair.second.name});
  }
  // Sort by ID
  std::sort(items.begin(), items.end());

  for(const auto& it : items) {
    wxString label = wxString::Format("%d", it.first);
    if(!it.second.empty()) label += wxString(" - ") + wxString(it.second);
    itemList->Append(label);
    sortedTypeIds.push_back(it.first);
  }
}

void EditItemsDialog::SaveCurrentItem() {
  if(currentTypeId < 0) return;
  auto it = workingCopy.find(currentTypeId);
  if(it == workingCopy.end()) return;

  IOMapSec::SecObjectType& obj = it->second;
  obj.name = nameCtrl->GetValue().ToStdString();
  obj.description = descCtrl->GetValue().ToStdString();

  // Save flags from checkboxes
  obj.flags.clear();
  for(size_t i = 0; i < flagChecks.size() && i < workingConfig.flagNames.size(); ++i) {
    if(flagChecks[i]->GetValue()) obj.flags.push_back(workingConfig.flagNames[i]);
  }
  // Attributes are already updated in-place via sub-dialogs
}

void EditItemsDialog::LoadItem(int typeId) {
  currentTypeId = typeId;
  auto it = workingCopy.find(typeId);
  if(it == workingCopy.end()) return;

  const IOMapSec::SecObjectType& obj = it->second;
  typeIdCtrl->SetValue(obj.typeId);
  nameCtrl->SetValue(obj.name);
  descCtrl->SetValue(obj.description);

  // Update sprite preview
  if(typeId > 0) spritePreview->SetItemSprite(typeId);
  else spritePreview->ClearSprite();

  RefreshFlagChecks();
  RefreshAttributeList();
}

void EditItemsDialog::RefreshFlagChecks() {
  if(currentTypeId < 0) return;
  auto it = workingCopy.find(currentTypeId);
  if(it == workingCopy.end()) return;

  const auto& objFlags = it->second.flags;
  for(size_t i = 0; i < flagChecks.size() && i < workingConfig.flagNames.size(); ++i) {
    bool found = false;
    for(const auto& f : objFlags) {
      // Case-insensitive compare
      std::string a, b;
      for(char c : f) a += std::tolower((unsigned char)c);
      for(char c : workingConfig.flagNames[i]) b += std::tolower((unsigned char)c);
      if(a == b) { found = true; break; }
    }
    flagChecks[i]->SetValue(found);
  }
}

void EditItemsDialog::RefreshAttributeList() {
  attrList->DeleteAllItems();
  if(currentTypeId < 0) return;
  auto it = workingCopy.find(currentTypeId);
  if(it == workingCopy.end()) return;

  for(int i = 0; i < (int)it->second.attributes.size(); ++i) {
    const auto& attr = it->second.attributes[i];
    long idx = attrList->InsertItem(i, attr.first);
    attrList->SetItem(idx, 1, std::to_string(attr.second));
  }
}

void EditItemsDialog::RefreshConfigLists() {
  configFlagList->Clear();
  for(const auto& f : workingConfig.flagNames) configFlagList->Append(f);
  configAttrList->Clear();
  for(const auto& a : workingConfig.attributeNames) configAttrList->Append(a);
}

void EditItemsDialog::OnListSelect(wxCommandEvent&) {
  int sel = itemList->GetSelection();
  if(sel < 0 || sel >= (int)sortedTypeIds.size()) return;
  SaveCurrentItem();
  LoadItem(sortedTypeIds[sel]);
}

void EditItemsDialog::OnFilterChanged(wxCommandEvent&) {
  SaveCurrentItem();
  BuildItemList();
  currentTypeId = -1;
  if(!sortedTypeIds.empty()) {
    itemList->SetSelection(0);
    LoadItem(sortedTypeIds[0]);
  }
}

void EditItemsDialog::OnSaveAll(wxCommandEvent&) {
  SaveCurrentItem();
  IOMapSec::objectTypes = workingCopy;
  IOMapSec::objectConfig = workingConfig;
  IOMapSec::saveObjectTypes();
  IOMapSec::rebuildObjectInfo();
  std::string rmeDataDir = nstr(g_gui.getFoundDataDirectory()) + "/";
  IOMapSec::saveObjectConfig(rmeDataDir);
  wxMessageBox("All items and config saved.", "Save", wxOK | wxICON_INFORMATION, this);
}

void EditItemsDialog::OnClickOK(wxCommandEvent&) {
  SaveCurrentItem();
  IOMapSec::objectTypes = workingCopy;
  IOMapSec::objectConfig = workingConfig;
  IOMapSec::saveObjectTypes();
  IOMapSec::rebuildObjectInfo();
  std::string rmeDataDir = nstr(g_gui.getFoundDataDirectory()) + "/";
  IOMapSec::saveObjectConfig(rmeDataDir);
  EndModal(wxID_OK);
}

void EditItemsDialog::OnClickCancel(wxCommandEvent&) {
  EndModal(wxID_CANCEL);
}

// ---- Attribute events ----

void EditItemsDialog::OnAddAttribute(wxCommandEvent&) {
  if(currentTypeId < 0) return;
  auto it = workingCopy.find(currentTypeId);
  if(it == workingCopy.end()) return;

  // Show choice of available attribute keys
  wxArrayString choices;
  for(const auto& a : workingConfig.attributeNames) choices.Add(a);
  if(choices.IsEmpty()) {
    wxMessageBox("No attribute keys defined. Add them in the Config tab first.", "Info", wxOK, this);
    return;
  }
  wxSingleChoiceDialog keyDlg(this, "Select attribute key:", "Add Attribute", choices);
  if(keyDlg.ShowModal() != wxID_OK) return;
  std::string key = keyDlg.GetStringSelection().ToStdString();

  wxTextEntryDialog valDlg(this, wxString("Value for ") + key + ":", "Attribute Value", "0");
  if(valDlg.ShowModal() != wxID_OK) return;
  int val = std::atoi(valDlg.GetValue().ToStdString().c_str());

  it->second.attributes.push_back({key, val});
  RefreshAttributeList();
}

void EditItemsDialog::OnEditAttribute(wxCommandEvent&) {
  if(currentTypeId < 0) return;
  long sel = attrList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
  if(sel < 0) return;
  auto it = workingCopy.find(currentTypeId);
  if(it == workingCopy.end() || sel >= (int)it->second.attributes.size()) return;

  auto& attr = it->second.attributes[sel];
  wxTextEntryDialog valDlg(this, wxString("Value for ") + attr.first + ":", "Edit Attribute", std::to_string(attr.second));
  if(valDlg.ShowModal() == wxID_OK) {
    attr.second = std::atoi(valDlg.GetValue().ToStdString().c_str());
    RefreshAttributeList();
  }
}

void EditItemsDialog::OnRemoveAttribute(wxCommandEvent&) {
  if(currentTypeId < 0) return;
  long sel = attrList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
  if(sel < 0) return;
  auto it = workingCopy.find(currentTypeId);
  if(it == workingCopy.end() || sel >= (int)it->second.attributes.size()) return;
  it->second.attributes.erase(it->second.attributes.begin() + sel);
  RefreshAttributeList();
}

// ---- Config events ----

void EditItemsDialog::OnAddFlag(wxCommandEvent&) {
  wxTextEntryDialog dlg(this, "New flag name:", "Add Flag");
  if(dlg.ShowModal() == wxID_OK) {
    std::string name = dlg.GetValue().ToStdString();
    if(name.empty()) return;
    // Check for duplicates
    for(const auto& f : workingConfig.flagNames) {
      if(f == name) { wxMessageBox("Flag already exists.", "Error", wxOK, this); return; }
    }
    workingConfig.flagNames.push_back(name);
    std::sort(workingConfig.flagNames.begin(), workingConfig.flagNames.end());
    RefreshConfigLists();
    RebuildFlagCheckboxes();
    if(currentTypeId >= 0) RefreshFlagChecks();
  }
}

void EditItemsDialog::OnRemoveFlag(wxCommandEvent&) {
  int sel = configFlagList->GetSelection();
  if(sel < 0 || sel >= (int)workingConfig.flagNames.size()) return;
  workingConfig.flagNames.erase(workingConfig.flagNames.begin() + sel);
  RefreshConfigLists();
  RebuildFlagCheckboxes();
  if(currentTypeId >= 0) RefreshFlagChecks();
}

void EditItemsDialog::OnAddAttrKey(wxCommandEvent&) {
  wxTextEntryDialog dlg(this, "New attribute key name:", "Add Attribute Key");
  if(dlg.ShowModal() == wxID_OK) {
    std::string name = dlg.GetValue().ToStdString();
    if(name.empty()) return;
    for(const auto& a : workingConfig.attributeNames) {
      if(a == name) { wxMessageBox("Attribute key already exists.", "Error", wxOK, this); return; }
    }
    workingConfig.attributeNames.push_back(name);
    std::sort(workingConfig.attributeNames.begin(), workingConfig.attributeNames.end());
    RefreshConfigLists();
  }
}

void EditItemsDialog::OnRemoveAttrKey(wxCommandEvent&) {
  int sel = configAttrList->GetSelection();
  if(sel < 0 || sel >= (int)workingConfig.attributeNames.size()) return;
  workingConfig.attributeNames.erase(workingConfig.attributeNames.begin() + sel);
  RefreshConfigLists();
}
