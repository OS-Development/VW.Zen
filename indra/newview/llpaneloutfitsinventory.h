/**
 * @file llpaneloutfitsinventory.h
 * @brief Outfits inventory panel
 * class definition
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

#ifndef LL_LLPANELOUTFITSINVENTORY_H
#define LL_LLPANELOUTFITSINVENTORY_H

#include "llpanel.h"
#include "llinventoryobserver.h"

class LLFolderView;
class LLFolderViewItem;
class LLFolderViewEventListener;
class LLInventoryPanel;
class LLSaveFolderState;
class LLButton;
class LLMenuGL;
class LLSidepanelAppearance;

class LLPanelOutfitsInventory : public LLPanel
{
public:
	LLPanelOutfitsInventory();
	virtual ~LLPanelOutfitsInventory();

	/*virtual*/ BOOL postBuild();
	
	void onSearchEdit(const std::string& string);
	void onWear();
	void onEdit();
	void onNew();

	void onSelectionChange(const std::deque<LLFolderViewItem*> &items, BOOL user_action);
	void onSelectorButtonClicked();

	// If a compatible listener type is selected, then return a pointer to that.
	// Otherwise, return NULL.
	LLFolderViewEventListener* getCorrectListenerForAction();
	void setParent(LLSidepanelAppearance *parent);
protected:
	void updateParent();
	bool getIsCorrectType(const LLFolderViewEventListener *listenerp) const;
	LLFolderView* getRootFolder();

private:
	LLSidepanelAppearance*      mParent;
	LLSaveFolderState*			mSavedFolderState;

public:
	//////////////////////////////////////////////////////////////////////////////////
	// Accordion                                                                    //
	LLInventoryPanel* 	getActivePanel();
	bool isAccordionPanel(LLInventoryPanel *panel);
	
protected:
	void 				initAccordionPanels();
	void 				onAccordionSelectionChange(LLInventoryPanel* accordion_panel, const std::deque<LLFolderViewItem*> &items, BOOL user_action);
	
private:
	LLInventoryPanel* 	mActivePanel;
	typedef std::vector<LLInventoryPanel *> accordionpanels_vec_t;
	accordionpanels_vec_t 		mAccordionPanels;

	// Accordion                                                                  //
	////////////////////////////////////////////////////////////////////////////////
	

	//////////////////////////////////////////////////////////////////////////////////
	// List Commands                                                                //
protected:
	void initListCommandsHandlers();
	void updateListCommands();
	void onGearButtonClick();
	void onAddButtonClick();
	void showActionMenu(LLMenuGL* menu, std::string spawning_view_name);
	void onTrashButtonClick();
	void onClipboardAction(const LLSD& userdata);
	BOOL isActionEnabled(const LLSD& command_name);
	void onCustomAction(const LLSD& command_name);
	bool handleDragAndDropToTrash(BOOL drop, EDragAndDropType cargo_type, EAcceptance* accept);
private:
	LLPanel*					mListCommands;
	LLMenuGL*					mMenuGearDefault;
	LLMenuGL*					mMenuAdd;
	// List Commands                                                              //
	////////////////////////////////////////////////////////////////////////////////
};

#endif //LL_LLPANELOUTFITSINVENTORY_H