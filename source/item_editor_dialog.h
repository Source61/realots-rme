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

#ifndef RME_ITEM_EDITOR_DIALOG_H_
#define RME_ITEM_EDITOR_DIALOG_H_

#include "main.h"
#include "iomap_sec.h"
#include <wx/listctrl.h>
#include <wx/notebook.h>
#include <wx/spinctrl.h>

class SpritePreviewPanel;

class EditItemsDialog : public wxDialog
{
public:
  EditItemsDialog(wxWindow* parent);
  ~EditItemsDialog();

  void OnListSelect(wxListEvent& event);
  void OnFilterChanged(wxCommandEvent& event);
  void OnAddItem(wxCommandEvent& event);
  void OnRemoveItem(wxCommandEvent& event);
  void OnDuplicateItem(wxCommandEvent& event);
  void OnSaveAll(wxCommandEvent& event);
  void OnClickOK(wxCommandEvent& event);
  void OnClickCancel(wxCommandEvent& event);

  // Attribute list events
  void OnAddAttribute(wxCommandEvent& event);
  void OnEditAttribute(wxCommandEvent& event);
  void OnRemoveAttribute(wxCommandEvent& event);

  // Config tab events
  void OnAddFlag(wxCommandEvent& event);
  void OnRemoveFlag(wxCommandEvent& event);
  void OnAddAttrKey(wxCommandEvent& event);
  void OnRemoveAttrKey(wxCommandEvent& event);

private:
  void BuildItemList();
  void SaveCurrentItem();
  void LoadItem(int typeId);
  void RefreshFlagChecks();
  void RefreshAttributeList();
  void RefreshConfigLists();
  void RebuildFlagCheckboxes();

  std::map<int, IOMapSec::SecObjectType> workingCopy;
  IOMapSec::SecObjectConfig workingConfig;
  std::vector<int> sortedTypeIds;
  int currentTypeId = -1;

  // Left panel
  wxTextCtrl* filterCtrl;
  wxListCtrl* itemList;

  // Basic tab
  wxSpinCtrl* typeIdCtrl;
  wxTextCtrl* nameCtrl;
  wxTextCtrl* descCtrl;

  // Flags tab
  wxPanel* flagsPanel;
  wxBoxSizer* flagsSizer;
  std::vector<wxCheckBox*> flagChecks;

  // Attributes tab
  wxListCtrl* attrList;

  // Config tab
  wxListBox* configFlagList;
  wxListBox* configAttrList;

  DECLARE_EVENT_TABLE()
};

#endif // RME_ITEM_EDITOR_DIALOG_H_
