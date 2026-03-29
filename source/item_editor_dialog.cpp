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
  ID_ITEM_ADD,
  ID_ITEM_REMOVE,
  ID_ITEM_DUPLICATE,
};

static wxBitmap MakeItemBitmapForList(int clientItemId) {
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

BEGIN_EVENT_TABLE(EditItemsDialog, wxDialog)
  EVT_LIST_ITEM_SELECTED(EDIT_ITEMS_LISTBOX, EditItemsDialog::OnListSelect)
  EVT_TEXT(EDIT_ITEMS_FILTER, EditItemsDialog::OnFilterChanged)
  EVT_BUTTON(ID_ITEM_ADD, EditItemsDialog::OnAddItem)
  EVT_BUTTON(ID_ITEM_REMOVE, EditItemsDialog::OnRemoveItem)
  EVT_BUTTON(ID_ITEM_DUPLICATE, EditItemsDialog::OnDuplicateItem)
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

  // ---- Left panel: item list with sprites ----
  wxBoxSizer* leftSizer = newd wxBoxSizer(wxVERTICAL);

  leftSizer->Add(newd wxStaticText(this, wxID_ANY, "Filter:"), wxSizerFlags(0).Border(wxLEFT | wxTOP, 5));
  filterCtrl = newd wxTextCtrl(this, EDIT_ITEMS_FILTER, "", wxDefaultPosition, wxSize(250, -1));
  leftSizer->Add(filterCtrl, wxSizerFlags(0).Expand().Border(wxALL, 5));

  itemList = newd wxListCtrl(this, EDIT_ITEMS_LISTBOX, wxDefaultPosition, wxSize(250, -1), wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_NO_HEADER);
  itemList->InsertColumn(0, "", wxLIST_FORMAT_LEFT, 240);
  leftSizer->Add(itemList, wxSizerFlags(1).Expand().Border(wxLEFT | wxRIGHT | wxBOTTOM, 5));

  wxBoxSizer* listBtnSizer = newd wxBoxSizer(wxHORIZONTAL);
  listBtnSizer->Add(newd wxButton(this, ID_ITEM_ADD, "Add"), wxSizerFlags(1));
  listBtnSizer->Add(newd wxButton(this, ID_ITEM_DUPLICATE, "Duplicate"), wxSizerFlags(1));
  listBtnSizer->Add(newd wxButton(this, ID_ITEM_REMOVE, "Remove"), wxSizerFlags(1));
  leftSizer->Add(listBtnSizer, wxSizerFlags(0).Expand().Border(wxLEFT | wxRIGHT | wxBOTTOM, 5));

  mainSizer->Add(leftSizer, wxSizerFlags(0).Expand());

  // ---- Right panel: notebook with tabs ----
  wxNotebook* notebook = newd wxNotebook(this, EDIT_ITEMS_NOTEBOOK);

  // == Tab 1: Basic ==
  wxPanel* basicPanel = newd wxPanel(notebook, wxID_ANY);
  wxFlexGridSizer* basicGrid = newd wxFlexGridSizer(2, 5, 10);
  basicGrid->AddGrowableCol(1);

  basicGrid->Add(newd wxStaticText(basicPanel, wxID_ANY, "TypeID"), wxSizerFlags().CenterVertical());
  typeIdCtrl = newd wxSpinCtrl(basicPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 99999, 0);
  typeIdCtrl->Enable(false);
  basicGrid->Add(typeIdCtrl, wxSizerFlags(1).Expand());

  basicGrid->Add(newd wxStaticText(basicPanel, wxID_ANY, "Name"), wxSizerFlags().CenterVertical());
  nameCtrl = newd wxTextCtrl(basicPanel, wxID_ANY, "");
  basicGrid->Add(nameCtrl, wxSizerFlags(1).Expand());

  basicGrid->Add(newd wxStaticText(basicPanel, wxID_ANY, "Description"), wxSizerFlags().CenterVertical());
  descCtrl = newd wxTextCtrl(basicPanel, wxID_ANY, "");
  basicGrid->Add(descCtrl, wxSizerFlags(1).Expand());

  basicPanel->SetSizer(basicGrid);
  notebook->AddPage(basicPanel, "Basic");

  // == Tab 2: Flags (sorted: checked first, then alphabetical) ==
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

  wxBoxSizer* cfgFlagSizer = newd wxBoxSizer(wxVERTICAL);
  cfgFlagSizer->Add(newd wxStaticText(configPanel, wxID_ANY, "Available Flags:"), wxSizerFlags(0).Border(wxALL, 5));
  configFlagList = newd wxListBox(configPanel, ID_ITEM_CONFIG_FLAG_LIST, wxDefaultPosition, wxDefaultSize);
  cfgFlagSizer->Add(configFlagList, wxSizerFlags(1).Expand().Border(wxLEFT | wxRIGHT, 5));
  wxBoxSizer* cfgFlagBtnSizer = newd wxBoxSizer(wxHORIZONTAL);
  cfgFlagBtnSizer->Add(newd wxButton(configPanel, ID_ITEM_ADD_FLAG, "Add Flag"), wxSizerFlags(1));
  cfgFlagBtnSizer->Add(newd wxButton(configPanel, ID_ITEM_REMOVE_FLAG, "Remove"), wxSizerFlags(1));
  cfgFlagSizer->Add(cfgFlagBtnSizer, wxSizerFlags(0).Expand().Border(wxALL, 5));
  configSizer->Add(cfgFlagSizer, wxSizerFlags(1).Expand());

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
    itemList->SetItemState(0, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
    LoadItem(sortedTypeIds[0]);
  }

  Centre(wxBOTH);
}

EditItemsDialog::~EditItemsDialog() {}

void EditItemsDialog::RebuildFlagCheckboxes() {
  flagsSizer->Clear(true);
  flagChecks.clear();
  for(const auto& fname : workingConfig.flagNames) {
    wxCheckBox* cb = newd wxCheckBox(flagsPanel, wxID_ANY, fname);
    flagsSizer->Add(cb, wxSizerFlags(0).Border(wxALL, 3));
    flagChecks.push_back(cb);
  }
  flagsPanel->Layout();
  flagsPanel->FitInside();
}

void EditItemsDialog::BuildItemList() {
  itemList->DeleteAllItems();
  sortedTypeIds.clear();

  std::string filter = filterCtrl->GetValue().ToStdString();
  std::string filterLower;
  for(char c : filter) filterLower += std::tolower((unsigned char)c);

  std::vector<std::pair<int, std::string>> items;
  for(const auto& pair : workingCopy) {
    if(pair.first < 100) continue;
    std::string nameLower;
    for(char c : pair.second.name) nameLower += std::tolower((unsigned char)c);
    std::string idStr = std::to_string(pair.first);
    if(!filterLower.empty() && nameLower.find(filterLower) == std::string::npos && idStr.find(filterLower) == std::string::npos) continue;
    items.push_back({pair.first, pair.second.name});
  }
  std::sort(items.begin(), items.end());

  // Build image list with item sprites (use DisguiseTarget sprite if present)
  wxImageList* imgList = newd wxImageList(32, 32, false);
  for(const auto& it : items) {
    int spriteId = it.first;
    auto objIt = workingCopy.find(it.first);
    if(objIt != workingCopy.end()) {
      for(const auto& attr : objIt->second.attributes) {
        std::string ak;
        for(char c : attr.first) ak += std::tolower((unsigned char)c);
        if(ak == "disguisetarget" && attr.second > 0) { spriteId = attr.second; break; }
      }
    }
    imgList->Add(MakeItemBitmapForList(spriteId));
  }
  itemList->AssignImageList(imgList, wxIMAGE_LIST_SMALL);

  for(int i = 0; i < (int)items.size(); ++i) {
    wxString label = wxString::Format("%d", items[i].first);
    if(!items[i].second.empty()) label += wxString(" - ") + wxString(items[i].second);
    itemList->InsertItem(i, label, i);
    sortedTypeIds.push_back(items[i].first);
  }
}

void EditItemsDialog::SaveCurrentItem() {
  if(currentTypeId < 0) return;
  auto it = workingCopy.find(currentTypeId);
  if(it == workingCopy.end()) return;

  IOMapSec::SecObjectType& obj = it->second;
  obj.name = nameCtrl->GetValue().ToStdString();
  obj.description = descCtrl->GetValue().ToStdString();

  obj.flags.clear();
  for(size_t i = 0; i < flagChecks.size(); ++i) {
    if(flagChecks[i]->GetValue()) obj.flags.push_back(flagChecks[i]->GetLabel().ToStdString());
  }
}

void EditItemsDialog::LoadItem(int typeId) {
  currentTypeId = typeId;
  auto it = workingCopy.find(typeId);
  if(it == workingCopy.end()) return;

  const IOMapSec::SecObjectType& obj = it->second;
  typeIdCtrl->SetValue(obj.typeId);
  nameCtrl->SetValue(obj.name);
  descCtrl->SetValue(obj.description);

  RefreshFlagChecks();
  RefreshAttributeList();
}

void EditItemsDialog::RefreshFlagChecks() {
  if(currentTypeId < 0) return;
  auto it = workingCopy.find(currentTypeId);
  if(it == workingCopy.end()) return;

  // Determine which flags are checked for this item
  std::set<std::string> checkedLower;
  for(const auto& f : it->second.flags) {
    std::string fl;
    for(char c : f) fl += std::tolower((unsigned char)c);
    checkedLower.insert(fl);
  }

  // Build sorted order: checked first (alphabetical), then unchecked (alphabetical)
  std::vector<std::pair<std::string, bool>> sorted;
  for(const auto& fname : workingConfig.flagNames) {
    std::string fl;
    for(char c : fname) fl += std::tolower((unsigned char)c);
    sorted.push_back({fname, checkedLower.count(fl) > 0});
  }
  std::stable_sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
    if(a.second != b.second) return a.second > b.second; // checked first
    return false; // preserve alphabetical order from config
  });

  // Rebuild checkboxes in sorted order
  flagsSizer->Clear(true);
  flagChecks.clear();
  for(const auto& entry : sorted) {
    wxCheckBox* cb = newd wxCheckBox(flagsPanel, wxID_ANY, entry.first);
    cb->SetValue(entry.second);
    flagsSizer->Add(cb, wxSizerFlags(0).Border(wxALL, 3));
    flagChecks.push_back(cb);
  }
  flagsPanel->Layout();
  flagsPanel->FitInside();
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

void EditItemsDialog::OnListSelect(wxListEvent& event) {
  int sel = event.GetIndex();
  if(sel < 0 || sel >= (int)sortedTypeIds.size()) return;
  SaveCurrentItem();
  LoadItem(sortedTypeIds[sel]);
}

void EditItemsDialog::OnFilterChanged(wxCommandEvent&) {
  SaveCurrentItem();
  BuildItemList();
  currentTypeId = -1;
  if(!sortedTypeIds.empty()) {
    itemList->SetItemState(0, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
    LoadItem(sortedTypeIds[0]);
  }
}

void EditItemsDialog::OnAddItem(wxCommandEvent&) {
  wxTextEntryDialog dlg(this, "New TypeID:", "Add Item");
  if(dlg.ShowModal() != wxID_OK) return;
  int newId = std::atoi(dlg.GetValue().ToStdString().c_str());
  if(newId < 100) {
    wxMessageBox("TypeID must be >= 100.", "Error", wxOK, this);
    return;
  }
  if(workingCopy.count(newId)) {
    wxMessageBox("TypeID already exists.", "Error", wxOK, this);
    return;
  }
  IOMapSec::SecObjectType obj;
  obj.typeId = newId;
  obj.name = "";
  workingCopy[newId] = obj;
  BuildItemList();
  // Select it
  for(int i = 0; i < (int)sortedTypeIds.size(); ++i) {
    if(sortedTypeIds[i] == newId) {
      itemList->SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
      itemList->EnsureVisible(i);
      LoadItem(newId);
      break;
    }
  }
}

void EditItemsDialog::OnRemoveItem(wxCommandEvent&) {
  if(currentTypeId < 0) return;
  int ret = wxMessageBox(wxString::Format("Delete item %d?", currentTypeId), "Confirm", wxYES_NO | wxICON_WARNING, this);
  if(ret != wxYES) return;
  workingCopy.erase(currentTypeId);
  currentTypeId = -1;
  BuildItemList();
  if(!sortedTypeIds.empty()) {
    itemList->SetItemState(0, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
    LoadItem(sortedTypeIds[0]);
  }
}

void EditItemsDialog::OnDuplicateItem(wxCommandEvent&) {
  if(currentTypeId < 0) return;
  SaveCurrentItem();
  auto it = workingCopy.find(currentTypeId);
  if(it == workingCopy.end()) return;

  // Find next available ID
  int newId = currentTypeId + 1;
  while(workingCopy.count(newId)) ++newId;

  IOMapSec::SecObjectType obj = it->second;
  obj.typeId = newId;
  workingCopy[newId] = obj;
  BuildItemList();
  for(int i = 0; i < (int)sortedTypeIds.size(); ++i) {
    if(sortedTypeIds[i] == newId) {
      itemList->SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
      itemList->EnsureVisible(i);
      LoadItem(newId);
      break;
    }
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
  UpdateCurrentItemSprite();
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
    UpdateCurrentItemSprite();
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
  UpdateCurrentItemSprite();
}

void EditItemsDialog::UpdateCurrentItemSprite() {
  if(currentTypeId < 0) return;
  // Find which index in the list this item is
  for(int i = 0; i < (int)sortedTypeIds.size(); ++i) {
    if(sortedTypeIds[i] == currentTypeId) {
      int spriteId = currentTypeId;
      auto objIt = workingCopy.find(currentTypeId);
      if(objIt != workingCopy.end()) {
        for(const auto& attr : objIt->second.attributes) {
          std::string ak;
          for(char c : attr.first) ak += std::tolower((unsigned char)c);
          if(ak == "disguisetarget" && attr.second > 0) { spriteId = attr.second; break; }
        }
      }
      wxImageList* imgList = itemList->GetImageList(wxIMAGE_LIST_SMALL);
      if(imgList) imgList->Replace(i, MakeItemBitmapForList(spriteId));
      itemList->RefreshItem(i);
      break;
    }
  }
}

// ---- Config events ----

void EditItemsDialog::OnAddFlag(wxCommandEvent&) {
  wxTextEntryDialog dlg(this, "New flag name:", "Add Flag");
  if(dlg.ShowModal() == wxID_OK) {
    std::string name = dlg.GetValue().ToStdString();
    if(name.empty()) return;
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
