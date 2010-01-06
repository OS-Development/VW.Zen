/**
 * @file llpaneloutfitsinventory.cpp
 * @brief Outfits inventory panel
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 *
 * Copyright (c) 2001-2009, Linden Research, Inc.
 *
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 *
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/flossexception
 *
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 *
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llpaneloutfitsinventory.h"

#include "llagent.h"
#include "llagentwearables.h"

#include "llbutton.h"
#include "llfloaterreg.h"
#include "llfloaterworldmap.h"
#include "llfloaterinventory.h"
#include "llfoldervieweventlistener.h"
#include "llinventoryfunctions.h"
#include "llinventorypanel.h"
#include "lllandmark.h"
#include "llsidepanelappearance.h"
#include "llsidetray.h"
#include "lltabcontainer.h"
#include "llviewerfoldertype.h"
#include "llviewerjointattachment.h"
#include "llvoavatarself.h"

// List Commands
#include "lldndbutton.h"
#include "llmenugl.h"
#include "llviewermenu.h"

#include "llviewercontrol.h"

static LLRegisterPanelClassWrapper<LLPanelOutfitsInventory> t_inventory("panel_outfits_inventory");
bool LLPanelOutfitsInventory::sShowDebugEditor = false;

LLPanelOutfitsInventory::LLPanelOutfitsInventory() :
	mActivePanel(NULL),
	mParent(NULL)
{
	mSavedFolderState = new LLSaveFolderState();
	mSavedFolderState->setApply(FALSE);
}

LLPanelOutfitsInventory::~LLPanelOutfitsInventory()
{
	delete mSavedFolderState;
}

// virtual
BOOL LLPanelOutfitsInventory::postBuild()
{
	sShowDebugEditor = gSavedSettings.getBOOL("ShowDebugAppearanceEditor");
	initTabPanels();
	initListCommandsHandlers();

	return TRUE;
}

// virtual
void LLPanelOutfitsInventory::onOpen(const LLSD& key)
{
	// Make sure we know which tab is selected, update the filter,
	// and update verbs.
	onTabChange();
}

void LLPanelOutfitsInventory::updateVerbs()
{
	if (mParent)
	{
		mParent->updateVerbs();
	}

	if (mListCommands)
	{
		mListCommands->childSetVisible("look_edit_btn",sShowDebugEditor);
		updateListCommands();
	}
}

void LLPanelOutfitsInventory::setParent(LLSidepanelAppearance* parent)
{
	mParent = parent;
}

// virtual
void LLPanelOutfitsInventory::onSearchEdit(const std::string& string)
{
	mFilterSubString = string;
	if (string == "")
	{
		mActivePanel->setFilterSubString(LLStringUtil::null);

		// re-open folders that were initially open
		mSavedFolderState->setApply(TRUE);
		getRootFolder()->applyFunctorRecursively(*mSavedFolderState);
		LLOpenFoldersWithSelection opener;
		getRootFolder()->applyFunctorRecursively(opener);
		getRootFolder()->scrollToShowSelection();
	}

	gInventory.startBackgroundFetch();

	if (mActivePanel->getFilterSubString().empty() && string.empty())
	{
		// current filter and new filter empty, do nothing
		return;
	}

	// save current folder open state if no filter currently applied
	if (getRootFolder()->getFilterSubString().empty())
	{
		mSavedFolderState->setApply(FALSE);
		getRootFolder()->applyFunctorRecursively(*mSavedFolderState);
	}

	// set new filter string
	mActivePanel->setFilterSubString(string);
}

void LLPanelOutfitsInventory::onWearButtonClick()
{
	LLFolderViewEventListener* listenerp = getCorrectListenerForAction();
	if (listenerp)
	{
		listenerp->performAction(NULL, NULL,"replaceoutfit");
	}
}

void LLPanelOutfitsInventory::onAdd()
{
	LLFolderViewEventListener* listenerp = getCorrectListenerForAction();
	if (listenerp)
	{
		listenerp->performAction(NULL, NULL,"addtooutfit");
	}
}

void LLPanelOutfitsInventory::onRemove()
{
	LLFolderViewEventListener* listenerp = getCorrectListenerForAction();
	if (listenerp)
	{
		listenerp->performAction(NULL, NULL,"removefromoutfit");
	}
}

void LLPanelOutfitsInventory::onEdit()
{
}

void LLPanelOutfitsInventory::onNew()
{
	const std::string& outfit_name = LLViewerFolderType::lookupNewCategoryName(LLFolderType::FT_OUTFIT);
	LLUUID outfit_folder = gAgentWearables.makeNewOutfitLinks(outfit_name);
}

void LLPanelOutfitsInventory::onSelectionChange(const std::deque<LLFolderViewItem*> &items, BOOL user_action)
{
	updateVerbs();
	if (getRootFolder()->needsAutoRename() && items.size())
	{
		getRootFolder()->startRenamingSelectedItem();
		getRootFolder()->setNeedsAutoRename(FALSE);
	}
}

void LLPanelOutfitsInventory::onSelectorButtonClicked()
{
	  LLFolderViewItem* cur_item = getRootFolder()->getCurSelectedItem();

	  LLFolderViewEventListener* listenerp = cur_item->getListener();
	  if (getIsCorrectType(listenerp))
	  {
	  LLSD key;
	  key["type"] = "look";
	  key["id"] = listenerp->getUUID();

	  LLSideTray::getInstance()->showPanel("sidepanel_appearance", key);
	  } 
}

LLFolderViewEventListener *LLPanelOutfitsInventory::getCorrectListenerForAction()
{
	LLFolderViewItem* current_item = getRootFolder()->getCurSelectedItem();
	if (!current_item)
		return NULL;

	LLFolderViewEventListener* listenerp = current_item->getListener();
	if (getIsCorrectType(listenerp))
	{
		return listenerp;
	}
	return NULL;
}

bool LLPanelOutfitsInventory::getIsCorrectType(const LLFolderViewEventListener *listenerp) const
{
	if (listenerp->getInventoryType() == LLInventoryType::IT_CATEGORY)
	{
		LLViewerInventoryCategory *cat = gInventory.getCategory(listenerp->getUUID());
		if (cat && cat->getPreferredType() == LLFolderType::FT_OUTFIT)
		{
			return true;
		}
	}
	return false;
}

LLFolderView *LLPanelOutfitsInventory::getRootFolder()
{
	return mActivePanel->getRootFolder();
}

//////////////////////////////////////////////////////////////////////////////////
// List Commands                                                                //

void LLPanelOutfitsInventory::initListCommandsHandlers()
{
	mListCommands = getChild<LLPanel>("bottom_panel");

	mListCommands->childSetAction("options_gear_btn", boost::bind(&LLPanelOutfitsInventory::onGearButtonClick, this));
	mListCommands->childSetAction("trash_btn", boost::bind(&LLPanelOutfitsInventory::onTrashButtonClick, this));
	mListCommands->childSetAction("make_outfit_btn", boost::bind(&LLPanelOutfitsInventory::onAddButtonClick, this));
	mListCommands->childSetAction("wear_btn", boost::bind(&LLPanelOutfitsInventory::onWearButtonClick, this));

	mListCommands->childSetAction("look_edit_btn", boost::bind(&LLPanelOutfitsInventory::onSelectorButtonClicked, this));

	LLDragAndDropButton* trash_btn = mListCommands->getChild<LLDragAndDropButton>("trash_btn");
	trash_btn->setDragAndDropHandler(boost::bind(&LLPanelOutfitsInventory::handleDragAndDropToTrash, this
				   ,       _4 // BOOL drop
				   ,       _5 // EDragAndDropType cargo_type
				   ,       _7 // EAcceptance* accept
				   ));

	mCommitCallbackRegistrar.add("panel_outfits_inventory_gear_default.Custom.Action",
								 boost::bind(&LLPanelOutfitsInventory::onCustomAction, this, _2));
	mEnableCallbackRegistrar.add("panel_outfits_inventory_gear_default.Enable",
								 boost::bind(&LLPanelOutfitsInventory::isActionEnabled, this, _2));
	mMenuGearDefault = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>("panel_outfits_inventory_gear_default.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
}

void LLPanelOutfitsInventory::updateListCommands()
{
	bool trash_enabled = isActionEnabled("delete");
	bool wear_enabled = isActionEnabled("wear");
	bool make_outfit_enabled = isActionEnabled("make_outfit");

	mListCommands->childSetEnabled("trash_btn", trash_enabled);
	mListCommands->childSetEnabled("wear_btn", wear_enabled);
	mListCommands->childSetEnabled("make_outfit_btn", make_outfit_enabled);
}

void LLPanelOutfitsInventory::onGearButtonClick()
{
	showActionMenu(mMenuGearDefault,"options_gear_btn");
}

void LLPanelOutfitsInventory::onAddButtonClick()
{
	onNew();
}

void LLPanelOutfitsInventory::showActionMenu(LLMenuGL* menu, std::string spawning_view_name)
{
	if (menu)
	{
		menu->buildDrawLabels();
		menu->updateParent(LLMenuGL::sMenuContainer);
		LLView* spawning_view = getChild<LLView> (spawning_view_name);
		S32 menu_x, menu_y;
		//show menu in co-ordinates of panel
		spawning_view->localPointToOtherView(0, spawning_view->getRect().getHeight(), &menu_x, &menu_y, this);
		menu_y += menu->getRect().getHeight();
		LLMenuGL::showPopup(this, menu, menu_x, menu_y);
	}
}

void LLPanelOutfitsInventory::onTrashButtonClick()
{
	onClipboardAction("delete");
}

void LLPanelOutfitsInventory::onClipboardAction(const LLSD& userdata)
{
	std::string command_name = userdata.asString();
	getActivePanel()->getRootFolder()->doToSelected(getActivePanel()->getModel(),command_name);
}

void LLPanelOutfitsInventory::onCustomAction(const LLSD& userdata)
{
	if (!isActionEnabled(userdata))
		return;

	const std::string command_name = userdata.asString();
	if (command_name == "new")
	{
		onNew();
	}
	if (command_name == "edit")
	{
		onEdit();
	}
	if (command_name == "wear")
	{
		onWearButtonClick();
	}
	// Note: This option has been removed from the gear menu.
	if (command_name == "add")
	{
		onAdd();
	}
	if (command_name == "remove")
	{
		onRemove();
	}
	if (command_name == "rename")
	{
		onClipboardAction("rename");
	}
	if (command_name == "remove_link")
	{
		onClipboardAction("delete");
	}
	if (command_name == "delete")
	{
		onClipboardAction("delete");
	}
}

BOOL LLPanelOutfitsInventory::isActionEnabled(const LLSD& userdata)
{
	const std::string command_name = userdata.asString();
	if (command_name == "delete")
	{
		BOOL can_delete = FALSE;
		LLFolderView *folder = getActivePanel()->getRootFolder();
		if (folder)
		{
			can_delete = TRUE;
			std::set<LLUUID> selection_set;
			folder->getSelectionList(selection_set);
			for (std::set<LLUUID>::iterator iter = selection_set.begin();
				 iter != selection_set.end();
				 ++iter)
			{
				const LLUUID &item_id = (*iter);
				LLFolderViewItem *item = folder->getItemByID(item_id);
				can_delete &= item->getListener()->isItemRemovable();
			}
			return can_delete;
		}
		return FALSE;
	}
	if (command_name == "remove_link")
	{
		BOOL can_delete = FALSE;
		LLFolderView *folder = getActivePanel()->getRootFolder();
		if (folder)
		{
			can_delete = TRUE;
			std::set<LLUUID> selection_set;
			folder->getSelectionList(selection_set);
			for (std::set<LLUUID>::iterator iter = selection_set.begin();
				 iter != selection_set.end();
				 ++iter)
			{
				const LLUUID &item_id = (*iter);
				LLViewerInventoryItem *item = gInventory.getItem(item_id);
				if (!item || !item->getIsLinkType())
					return FALSE;
			}
			return can_delete;
		}
		return FALSE;
	}
	if (command_name == "wear" ||
		command_name == "make_outfit")
	{
		const BOOL is_my_outfits = (mActivePanel->getName() == "outfitslist_tab");
		if (!is_my_outfits)
		{
			return FALSE;
		}
	}
   
	if (command_name == "edit" || 
		command_name == "wear" ||
		command_name == "add" ||
		command_name == "remove"
		)
	{
		return (getCorrectListenerForAction() != NULL);
	}
	return TRUE;
}

bool LLPanelOutfitsInventory::handleDragAndDropToTrash(BOOL drop, EDragAndDropType cargo_type, EAcceptance* accept)
{
	*accept = ACCEPT_NO;

	const bool is_enabled = isActionEnabled("delete");
	if (is_enabled) *accept = ACCEPT_YES_MULTI;

	if (is_enabled && drop)
	{
		onClipboardAction("delete");
	}
	return true;
}

// List Commands                                                              //
////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Tab panels                                                                    //

void LLPanelOutfitsInventory::initTabPanels()
{
	mTabPanels.resize(2);

	LLInventoryPanel *cof_panel = getChild<LLInventoryPanel>("cof_tab");
	cof_panel->setShowFolderState(LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);
	mTabPanels[0] = cof_panel;
	
	LLInventoryPanel *myoutfits_panel = getChild<LLInventoryPanel>("outfitslist_tab");
	myoutfits_panel->setFilterTypes(1LL << LLFolderType::FT_OUTFIT, LLInventoryFilter::FILTERTYPE_CATEGORY);
	myoutfits_panel->setShowFolderState(LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);
	mTabPanels[1] = myoutfits_panel;

	mActivePanel = mTabPanels[0];

	for (tabpanels_vec_t::iterator iter = mTabPanels.begin();
		 iter != mTabPanels.end();
		 ++iter)
	{
		LLInventoryPanel *panel = (*iter);
		panel->setSelectCallback(boost::bind(&LLPanelOutfitsInventory::onTabSelectionChange, this, panel, _1, _2));
	}

	mAppearanceTabs = getChild<LLTabContainer>("appearance_tabs");
	mAppearanceTabs->setCommitCallback(boost::bind(&LLPanelOutfitsInventory::onTabChange, this));
}

void LLPanelOutfitsInventory::onTabSelectionChange(LLInventoryPanel* tab_panel, const std::deque<LLFolderViewItem*> &items, BOOL user_action)
{
	if (user_action && items.size() > 0)
	{
		for (tabpanels_vec_t::iterator iter = mTabPanels.begin();
			 iter != mTabPanels.end();
			 ++iter)
		{
			LLInventoryPanel *panel = (*iter);
			if (panel == tab_panel)
			{
				mActivePanel = panel;
			}
			else
			{
				panel->getRootFolder()->clearSelection();
			}
		}
	}
	onSelectionChange(items, user_action);
}

void LLPanelOutfitsInventory::onTabChange()
{
	mActivePanel = (LLInventoryPanel*)childGetVisibleTab("appearance_tabs");
	if (!mActivePanel)
	{
		return;
	}
	mActivePanel->setFilterSubString(mFilterSubString);
	updateVerbs();
}

LLInventoryPanel* LLPanelOutfitsInventory::getActivePanel()
{
	return mActivePanel;
}

bool LLPanelOutfitsInventory::isTabPanel(LLInventoryPanel *panel)
{
	for(tabpanels_vec_t::iterator it = mTabPanels.begin();
		it != mTabPanels.end();
		++it)
	{
		if (*it == panel)
			return true;
	}
	return false;
}
