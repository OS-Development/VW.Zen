/**
 * @file llpaneloutfitedit.cpp
 * @brief Displays outfit edit information in Side Tray.
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#include "llpaneloutfitedit.h"

// *TODO: reorder includes to match the coding standard
#include "llagent.h"
#include "llagentcamera.h"
#include "llagentwearables.h"
#include "llappearancemgr.h"
#include "lloutfitobserver.h"
#include "llcofwearables.h"
#include "llfilteredwearablelist.h"
#include "llinventory.h"
#include "llinventoryitemslist.h"
#include "llviewercontrol.h"
#include "llui.h"
#include "llfloater.h"
#include "llfloaterreg.h"
#include "llinventoryfunctions.h"
#include "llinventorypanel.h"
#include "llviewermenu.h"
#include "llviewerwindow.h"
#include "llviewerinventory.h"
#include "llbutton.h"
#include "llcombobox.h"
#include "llfiltereditor.h"
#include "llfloaterinventory.h"
#include "llinventorybridge.h"
#include "llinventorymodel.h"
#include "llinventorymodelbackgroundfetch.h"
#include "llpaneloutfitsinventory.h"
#include "lluiconstants.h"
#include "llsaveoutfitcombobtn.h"
#include "llscrolllistctrl.h"
#include "lltextbox.h"
#include "lluictrlfactory.h"
#include "llsdutil.h"
#include "llsidepanelappearance.h"
#include "lltoggleablemenu.h"
#include "llwearablelist.h"
#include "llwearableitemslist.h"

static LLRegisterPanelClassWrapper<LLPanelOutfitEdit> t_outfit_edit("panel_outfit_edit");

const U64 WEARABLE_MASK = (1LL << LLInventoryType::IT_WEARABLE);
const U64 ATTACHMENT_MASK = (1LL << LLInventoryType::IT_ATTACHMENT) | (1LL << LLInventoryType::IT_OBJECT);
const U64 ALL_ITEMS_MASK = WEARABLE_MASK | ATTACHMENT_MASK;

static const std::string REVERT_BTN("revert_btn");

class LLPanelOutfitEditGearMenu
{
public:
	static LLMenuGL* create()
	{
		LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;

		registrar.add("Wearable.Create", boost::bind(onCreate, _2));

		LLMenuGL* menu = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>(
			"menu_cof_gear.xml", LLMenuGL::sMenuContainer, LLViewerMenuHolderGL::child_registry_t::instance());
		llassert(menu);
		if (menu)
		{
			populateCreateWearableSubmenus(menu);
			menu->buildDrawLabels();
		}

		return menu;
	}

private:
	static void onCreate(const LLSD& param)
	{
		LLWearableType::EType type = LLWearableType::typeNameToType(param.asString());
		if (type == LLWearableType::WT_NONE)
		{
			llwarns << "Invalid wearable type" << llendl;
			return;
		}

		LLAgentWearables::createWearable(type, true);
	}

	// Populate the menu with items like "New Skin", "New Pants", etc.
	static void populateCreateWearableSubmenus(LLMenuGL* menu)
	{
		LLView* menu_clothes	= gMenuHolder->findChildView("COF.Gear.New_Clothes", FALSE);
		LLView* menu_bp			= gMenuHolder->findChildView("COF.Geear.New_Body_Parts", FALSE);

		if (!menu_clothes || !menu_bp)
		{
			llassert(menu_clothes && menu_bp);
			return;
		}

		for (U8 i = LLWearableType::WT_SHAPE; i != (U8) LLWearableType::WT_COUNT; ++i)
		{
			LLWearableType::EType type = (LLWearableType::EType) i;
			const std::string& type_name = LLWearableType::getTypeName(type);

			LLMenuItemCallGL::Params p;
			p.name = type_name;
			p.label = LLWearableType::getTypeDefaultNewName(type);
			p.on_click.function_name = "Wearable.Create";
			p.on_click.parameter = LLSD(type_name);

			LLView* parent = LLWearableType::getAssetType(type) == LLAssetType::AT_CLOTHING ?
				menu_clothes : menu_bp;
			LLUICtrlFactory::create<LLMenuItemCallGL>(p, parent);
		}
	}
};

class LLCOFDragAndDropObserver : public LLInventoryAddItemByAssetObserver
{
public:
	LLCOFDragAndDropObserver(LLInventoryModel* model);

	virtual ~LLCOFDragAndDropObserver();

	virtual void done();

private:
	LLInventoryModel* mModel;
};

inline LLCOFDragAndDropObserver::LLCOFDragAndDropObserver(LLInventoryModel* model):
		mModel(model)
{
	if (model != NULL)
	{
		model->addObserver(this);
	}
}

inline LLCOFDragAndDropObserver::~LLCOFDragAndDropObserver()
{
	if (mModel != NULL && mModel->containsObserver(this))
	{
		mModel->removeObserver(this);
	}
}

void LLCOFDragAndDropObserver::done()
{
	LLAppearanceMgr::instance().updateAppearanceFromCOF();
}

LLPanelOutfitEdit::LLPanelOutfitEdit()
:	LLPanel(), 
	mSearchFilter(NULL),
	mCOFWearables(NULL),
	mInventoryItemsPanel(NULL),
	mGearMenu(NULL),
	mCOFDragAndDropObserver(NULL),
	mInitialized(false),
	mAddWearablesPanel(NULL),
	mWearableListMaskCollector(NULL),
	mWearableListTypeCollector(NULL)
{
	mSavedFolderState = new LLSaveFolderState();
	mSavedFolderState->setApply(FALSE);
	

	LLOutfitObserver& observer = LLOutfitObserver::instance();
	observer.addBOFReplacedCallback(boost::bind(&LLPanelOutfitEdit::updateCurrentOutfitName, this));
	observer.addBOFChangedCallback(boost::bind(&LLPanelOutfitEdit::updateVerbs, this));
	observer.addOutfitLockChangedCallback(boost::bind(&LLPanelOutfitEdit::updateVerbs, this));
	observer.addCOFChangedCallback(boost::bind(&LLPanelOutfitEdit::update, this));
	
	mLookItemTypes.reserve(NUM_LOOK_ITEM_TYPES);
	for (U32 i = 0; i < NUM_LOOK_ITEM_TYPES; i++)
	{
		mLookItemTypes.push_back(LLLookItemType());
	}
	

}

LLPanelOutfitEdit::~LLPanelOutfitEdit()
{
	delete mSavedFolderState;

	delete mCOFDragAndDropObserver;

	delete mWearableListMaskCollector;
	delete mWearableListTypeCollector;
}

BOOL LLPanelOutfitEdit::postBuild()
{
	// gInventory.isInventoryUsable() no longer needs to be tested per Richard's fix for race conditions between inventory and panels

	mLookItemTypes[LIT_ALL] = LLLookItemType(getString("Filter.All"), ALL_ITEMS_MASK);
	mLookItemTypes[LIT_WEARABLE] = LLLookItemType(getString("Filter.Clothes/Body"), WEARABLE_MASK);
	mLookItemTypes[LIT_ATTACHMENT] = LLLookItemType(getString("Filter.Objects"), ATTACHMENT_MASK);

	mCurrentOutfitName = getChild<LLTextBox>("curr_outfit_name"); 
	mStatus = getChild<LLTextBox>("status");

	mFolderViewBtn = getChild<LLButton>("folder_view_btn");
	mListViewBtn = getChild<LLButton>("list_view_btn");

	childSetCommitCallback("filter_button", boost::bind(&LLPanelOutfitEdit::showWearablesFilter, this), NULL);
	childSetCommitCallback("folder_view_btn", boost::bind(&LLPanelOutfitEdit::showFilteredFolderWearablesPanel, this), NULL);
	childSetCommitCallback("list_view_btn", boost::bind(&LLPanelOutfitEdit::showFilteredWearablesPanel, this), NULL);
	childSetCommitCallback("wearables_gear_menu_btn", boost::bind(&LLPanelOutfitEdit::onGearButtonClick, this, _1), NULL);

	mCOFWearables = getChild<LLCOFWearables>("cof_wearables_list");
	mCOFWearables->setCommitCallback(boost::bind(&LLPanelOutfitEdit::onOutfitItemSelectionChange, this));

	mCOFWearables->getCOFCallbacks().mAddWearable = boost::bind(&LLPanelOutfitEdit::onAddWearableClicked, this);
	mCOFWearables->getCOFCallbacks().mEditWearable = boost::bind(&LLPanelOutfitEdit::onEditWearableClicked, this);
	mCOFWearables->getCOFCallbacks().mDeleteWearable = boost::bind(&LLPanelOutfitEdit::onRemoveFromOutfitClicked, this);
	mCOFWearables->getCOFCallbacks().mMoveWearableCloser = boost::bind(&LLPanelOutfitEdit::moveWearable, this, true);
	mCOFWearables->getCOFCallbacks().mMoveWearableFurther = boost::bind(&LLPanelOutfitEdit::moveWearable, this, false);

	mAddWearablesPanel = getChild<LLPanel>("add_wearables_panel");

	mInventoryItemsPanel = getChild<LLInventoryPanel>("inventory_items");
	mInventoryItemsPanel->setFilterTypes(ALL_ITEMS_MASK);
	mInventoryItemsPanel->setShowFolderState(LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);
	mInventoryItemsPanel->setSelectCallback(boost::bind(&LLPanelOutfitEdit::onInventorySelectionChange, this, _1, _2));
	mInventoryItemsPanel->getRootFolder()->setReshapeCallback(boost::bind(&LLPanelOutfitEdit::onInventorySelectionChange, this, _1, _2));
	
	mCOFDragAndDropObserver = new LLCOFDragAndDropObserver(mInventoryItemsPanel->getModel());

	LLComboBox* type_filter = getChild<LLComboBox>("filter_wearables_combobox");
	type_filter->setCommitCallback(boost::bind(&LLPanelOutfitEdit::onTypeFilterChanged, this, _1));
	type_filter->removeall();
	for (U32 i = 0; i < mLookItemTypes.size(); ++i)
	{
		type_filter->add(mLookItemTypes[i].displayName);
	}
	type_filter->setCurrentByIndex(LIT_ALL);
	
	mSearchFilter = getChild<LLFilterEditor>("look_item_filter");
	mSearchFilter->setCommitCallback(boost::bind(&LLPanelOutfitEdit::onSearchEdit, this, _2));

	childSetAction("show_add_wearables_btn", boost::bind(&LLPanelOutfitEdit::toggleAddWearablesPanel, this));
	childSetAction("add_to_outfit_btn", boost::bind(&LLPanelOutfitEdit::onAddToOutfitClicked, this));
	
	mEditWearableBtn = getChild<LLButton>("edit_wearable_btn");
	mEditWearableBtn->setEnabled(FALSE);
	mEditWearableBtn->setVisible(FALSE);
	mEditWearableBtn->setCommitCallback(boost::bind(&LLPanelOutfitEdit::onEditWearableClicked, this));

	childSetAction(REVERT_BTN, boost::bind(&LLAppearanceMgr::wearBaseOutfit, LLAppearanceMgr::getInstance()));

	mWearableListMaskCollector = new LLFindNonLinksByMask(ALL_ITEMS_MASK);
	mWearableListTypeCollector = new LLFindActualWearablesOfType(LLWearableType::WT_NONE);

	mWearableItemsPanel = getChild<LLPanel>("filtered_wearables_panel");
	mWearableItemsList = getChild<LLInventoryItemsList>("filtered_wearables_list");

	mSaveComboBtn.reset(new LLSaveOutfitComboBtn(this));
	return TRUE;
}

// virtual
void LLPanelOutfitEdit::onOpen(const LLSD& key)
{
	if (!mInitialized)
	{
		// *TODO: this method is called even panel is not visible to user because its parent layout panel is hidden.
		// So, we can defer initializing a bit.
		mWearableListManager = new LLFilteredWearableListManager(mWearableItemsList, mWearableListMaskCollector);
		mWearableListManager->populateList();
		displayCurrentOutfit();
		mInitialized = true;
	}
}

void LLPanelOutfitEdit::moveWearable(bool closer_to_body)
{
	LLUUID item_id = mCOFWearables->getSelectedUUID();
	if (item_id.isNull()) return;
	
	LLViewerInventoryItem* wearable_to_move = gInventory.getItem(item_id);
	LLAppearanceMgr::getInstance()->moveWearable(wearable_to_move, closer_to_body);
}

void LLPanelOutfitEdit::toggleAddWearablesPanel()
{
	BOOL current_visibility = mAddWearablesPanel->getVisible();
	showAddWearablesPanel(!current_visibility);
}

void LLPanelOutfitEdit::showAddWearablesPanel(bool show_add_wearables)
{
	mAddWearablesPanel->setVisible(show_add_wearables);
	
	childSetValue("show_add_wearables_btn", show_add_wearables);

	childSetVisible("filter_wearables_combobox", show_add_wearables);
	childSetVisible("filter_button", show_add_wearables);

	//search filter should be disabled
	if (!show_add_wearables)
	{
		childSetValue("filter_button", false);
		showWearablesFilter();
	}

	//switching button bars
	childSetVisible("no_add_wearables_button_bar", !show_add_wearables);
	childSetVisible("add_wearables_button_bar", show_add_wearables);
}

void LLPanelOutfitEdit::showWearablesFilter()
{
	bool filter_visible = childGetValue("filter_button");

	childSetVisible("filter_panel", filter_visible);

	if(!filter_visible)
	{
		mSearchFilter->clear();
		onSearchEdit(LLStringUtil::null);
	}
}

void LLPanelOutfitEdit::showFilteredWearablesPanel()
{
	if(switchPanels(mInventoryItemsPanel, mWearableItemsPanel))
	{
		mFolderViewBtn->setToggleState(FALSE);
		mFolderViewBtn->setImageOverlay(getString("folder_view_off"), mFolderViewBtn->getImageOverlayHAlign());
		mListViewBtn->setImageOverlay(getString("list_view_on"), mListViewBtn->getImageOverlayHAlign());
	}
	mListViewBtn->setToggleState(TRUE);
}

void LLPanelOutfitEdit::showFilteredFolderWearablesPanel()
{
	if(switchPanels(mWearableItemsPanel, mInventoryItemsPanel))
	{
		mListViewBtn->setToggleState(FALSE);
		mListViewBtn->setImageOverlay(getString("list_view_off"), mListViewBtn->getImageOverlayHAlign());
		mFolderViewBtn->setImageOverlay(getString("folder_view_on"), mFolderViewBtn->getImageOverlayHAlign());
	}
	mFolderViewBtn->setToggleState(TRUE);
}

void LLPanelOutfitEdit::onTypeFilterChanged(LLUICtrl* ctrl)
{
	LLComboBox* type_filter = dynamic_cast<LLComboBox*>(ctrl);
	llassert(type_filter);
	if (type_filter)
	{
		U32 curr_filter_type = type_filter->getCurrentIndex();
		mInventoryItemsPanel->setFilterTypes(mLookItemTypes[curr_filter_type].inventoryMask);

		mWearableListMaskCollector->setFilterMask(mLookItemTypes[curr_filter_type].inventoryMask);
		mWearableListManager->setFilterCollector(mWearableListMaskCollector);
	}
	
	mSavedFolderState->setApply(TRUE);
	mInventoryItemsPanel->getRootFolder()->applyFunctorRecursively(*mSavedFolderState);
	
	LLOpenFoldersWithSelection opener;
	mInventoryItemsPanel->getRootFolder()->applyFunctorRecursively(opener);
	mInventoryItemsPanel->getRootFolder()->scrollToShowSelection();
	
	LLInventoryModelBackgroundFetch::instance().start();
}

void LLPanelOutfitEdit::onSearchEdit(const std::string& string)
{
	if (mSearchString != string)
	{
		mSearchString = string;
		
		// Searches are case-insensitive
		LLStringUtil::toUpper(mSearchString);
		LLStringUtil::trimHead(mSearchString);
	}
	
	if (mSearchString == "")
	{
		mInventoryItemsPanel->setFilterSubString(LLStringUtil::null);
		mWearableItemsList->setFilterSubString(LLStringUtil::null);
		// re-open folders that were initially open
		mSavedFolderState->setApply(TRUE);
		mInventoryItemsPanel->getRootFolder()->applyFunctorRecursively(*mSavedFolderState);
		LLOpenFoldersWithSelection opener;
		mInventoryItemsPanel->getRootFolder()->applyFunctorRecursively(opener);
		mInventoryItemsPanel->getRootFolder()->scrollToShowSelection();
	}
	
	LLInventoryModelBackgroundFetch::instance().start();
	
	if (mInventoryItemsPanel->getFilterSubString().empty() && mSearchString.empty())
	{
		// current filter and new filter empty, do nothing
		return;
	}
	
	// save current folder open state if no filter currently applied
	if (mInventoryItemsPanel->getRootFolder()->getFilterSubString().empty())
	{
		mSavedFolderState->setApply(FALSE);
		mInventoryItemsPanel->getRootFolder()->applyFunctorRecursively(*mSavedFolderState);
	}
	
	// set new filter string
	mInventoryItemsPanel->setFilterSubString(mSearchString);
	mWearableItemsList->setFilterSubString(mSearchString);

}

void LLPanelOutfitEdit::onAddToOutfitClicked(void)
{
	LLUUID selected_id;
	if (mInventoryItemsPanel->getVisible())
	{
		LLFolderViewItem* curr_item = mInventoryItemsPanel->getRootFolder()->getCurSelectedItem();
		if (!curr_item) return;

		LLFolderViewEventListener* listenerp  = curr_item->getListener();
		if (!listenerp) return;

		selected_id = listenerp->getUUID();
	}
	else if (mWearableItemsPanel->getVisible())
	{
		selected_id = mWearableItemsList->getSelectedUUID();
	}

	if (selected_id.isNull()) return;

	LLAppearanceMgr::getInstance()->wearItemOnAvatar(selected_id);
}

void LLPanelOutfitEdit::onAddWearableClicked(void)
{
	LLPanelDummyClothingListItem* item = dynamic_cast<LLPanelDummyClothingListItem*>(mCOFWearables->getSelectedItem());

	if(item)
	{
		showFilteredWearableItemsList(item->getWearableType());
	}
}

void LLPanelOutfitEdit::onReplaceBodyPartMenuItemClicked(LLUUID selected_item_id)
{
	LLViewerInventoryItem* item = gInventory.getLinkedItem(selected_item_id);

	if (item && item->getType() == LLAssetType::AT_BODYPART)
	{
		showFilteredWearableItemsList(item->getWearableType());
	}
}

void LLPanelOutfitEdit::onRemoveFromOutfitClicked(void)
{
	LLUUID id_to_remove = mCOFWearables->getSelectedUUID();
	
	LLAppearanceMgr::getInstance()->removeItemFromAvatar(id_to_remove);
}


void LLPanelOutfitEdit::onEditWearableClicked(void)
{
	LLUUID selected_item_id = mCOFWearables->getSelectedUUID();
	if (selected_item_id.notNull())
	{
		gAgentWearables.editWearable(selected_item_id);
	}
}

void LLPanelOutfitEdit::onInventorySelectionChange(const std::deque<LLFolderViewItem*> &items, BOOL user_action)
{
	LLFolderViewItem* current_item = mInventoryItemsPanel->getRootFolder()->getCurSelectedItem();
	if (!current_item)
	{
		return;
	}

	LLViewerInventoryItem* item = current_item->getInventoryItem();
	if (!item) return;

	switch (item->getType())
	{
	case LLAssetType::AT_CLOTHING:
	case LLAssetType::AT_BODYPART:
	case LLAssetType::AT_OBJECT:
	default:
		break;
	}
	
	/* Removing add to look inline button (not part of mvp for viewer 2)
	LLRect btn_rect(current_item->getLocalRect().mRight - 50,
					current_item->getLocalRect().mTop,
					current_item->getLocalRect().mRight - 30,
					current_item->getLocalRect().mBottom);
	
	mAddToLookBtn->setRect(btn_rect);
	mAddToLookBtn->setEnabled(TRUE);
	if (!mAddToLookBtn->getVisible())
	{
		mAddToLookBtn->setVisible(TRUE);
	}
	
	current_item->addChild(mAddToLookBtn); */
}

void LLPanelOutfitEdit::onOutfitItemSelectionChange(void)
{	
	LLUUID item_id = mCOFWearables->getSelectedUUID();

	//*TODO show Edit Wearable Button

	LLViewerInventoryItem* item_to_remove = gInventory.getItem(item_id);
	if (!item_to_remove) return;

	switch (item_to_remove->getType())
	{
	case LLAssetType::AT_CLOTHING:
	case LLAssetType::AT_OBJECT:
	default:
		break;
	}
}

void LLPanelOutfitEdit::update()
{
	mCOFWearables->refresh();

	updateVerbs();
}

BOOL LLPanelOutfitEdit::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
										  EDragAndDropType cargo_type,
										  void* cargo_data,
										  EAcceptance* accept,
										  std::string& tooltip_msg)
{
	if (cargo_data == NULL)
	{
		llwarns << "cargo_data is NULL" << llendl;
		return TRUE;
	}

	switch (cargo_type)
	{
	case DAD_BODYPART:
	case DAD_CLOTHING:
	case DAD_OBJECT:
	case DAD_LINK:
		*accept = ACCEPT_YES_MULTI;
		break;
	default:
		*accept = ACCEPT_NO;
	}

	if (drop)
	{
		LLInventoryItem* item = static_cast<LLInventoryItem*>(cargo_data);

		if (LLAssetType::lookupIsAssetIDKnowable(item->getType()))
		{
			mCOFDragAndDropObserver->watchAsset(item->getAssetUUID());

			/*
			 * Adding request to wear item. If the item is a link, then getLinkedUUID() will
			 * return the ID of the linked item. Otherwise it will return the item's ID. The
			 * second argument is used to delay the appearance update until all dragged items
			 * are added to optimize user experience.
			 */
			LLAppearanceMgr::instance().addCOFItemLink(item->getLinkedUUID(), false);
		}
		else
		{
			// if asset id is not available for the item we must wear it immediately (attachments only)
			LLAppearanceMgr::instance().addCOFItemLink(item->getLinkedUUID(), true);
		}
	}

	return TRUE;
}

void LLPanelOutfitEdit::displayCurrentOutfit()
{
	if (!getVisible())
	{
		setVisible(TRUE);
	}

	updateCurrentOutfitName();

	update();
}

void LLPanelOutfitEdit::updateCurrentOutfitName()
{
	std::string current_outfit_name;
	if (LLAppearanceMgr::getInstance()->getBaseOutfitName(current_outfit_name))
	{
		mCurrentOutfitName->setText(current_outfit_name);
	}
	else
	{
		mCurrentOutfitName->setText(getString("No Outfit"));
	}
}

//private
void LLPanelOutfitEdit::updateVerbs()
{
	bool outfit_is_dirty = LLAppearanceMgr::getInstance()->isOutfitDirty();
	bool outfit_locked = LLAppearanceMgr::getInstance()->isOutfitLocked();
	bool has_baseoutfit = LLAppearanceMgr::getInstance()->getBaseOutfitUUID().notNull();

	mSaveComboBtn->setSaveBtnEnabled(!outfit_locked && outfit_is_dirty);
	childSetEnabled(REVERT_BTN, outfit_is_dirty && has_baseoutfit);

	mSaveComboBtn->setMenuItemEnabled("save_outfit", !outfit_locked && outfit_is_dirty);

	mStatus->setText(outfit_is_dirty ? getString("unsaved_changes") : getString("now_editing"));

}

bool LLPanelOutfitEdit::switchPanels(LLPanel* switch_from_panel, LLPanel* switch_to_panel)
{
	if(switch_from_panel && switch_to_panel && !switch_to_panel->getVisible())
	{
		switch_from_panel->setVisible(FALSE);
		switch_to_panel->setVisible(TRUE);
		return true;
	}
	return false;
}

void LLPanelOutfitEdit::onGearButtonClick(LLUICtrl* clicked_button)
{
	if(!mGearMenu)
	{
		mGearMenu = LLPanelOutfitEditGearMenu::create();
	}

	S32 menu_y = mGearMenu->getRect().getHeight() + clicked_button->getRect().getHeight();
	LLMenuGL::showPopup(clicked_button, mGearMenu, 0, menu_y);
}

void LLPanelOutfitEdit::showFilteredWearableItemsList(LLWearableType::EType type)
{
	mWearableListTypeCollector->setType(type);
	mWearableListManager->setFilterCollector(mWearableListTypeCollector);
	showAddWearablesPanel(true);
	showFilteredWearablesPanel();
}

// EOF
