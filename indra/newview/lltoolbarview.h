/** 
 * @file lltoolbarview.h
 * @author Merov Linden
 * @brief User customizable toolbar class
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#ifndef LL_LLTOOLBARVIEW_H
#define LL_LLTOOLBARVIEW_H

#include "lluictrl.h"
#include "lltoolbar.h"
#include "llcommandmanager.h"

class LLUICtrlFactory;

// Parent of all LLToolBar

class LLToolBarView : public LLUICtrl
{
public:
	// Xui structure of the toolbar panel
	struct Params : public LLInitParam::Block<Params, LLUICtrl::Params> {};

	// Note: valid children for LLToolBarView are stored in this registry
	typedef LLDefaultChildRegistry child_registry_t;
	
	// Xml structure of the toolbars.xml setting
	// Those live in a toolbars.xml found in app_settings (for the default) and in
	// the user folder for the user specific (saved) settings
	struct Toolbar : public LLInitParam::Block<Toolbar>
	{
		Mandatory<LLToolBarEnums::ButtonType>	button_display_mode;
		Multiple<LLCommandId::Params>	commands;
		Toolbar();
	};
	struct ToolbarSet : public LLInitParam::Block<ToolbarSet>
	{
		Optional<Toolbar>	left_toolbar,
							right_toolbar,
							bottom_toolbar;
		ToolbarSet();
	};

	// Derived methods
	virtual ~LLToolBarView();
	virtual BOOL postBuild();
	virtual void draw();
	// Toolbar view interface with the rest of the world
	// Checks if the commandId is being used somewhere in one of the toolbars
	bool hasCommand(const LLCommandId& commandId) const;
	// Loads the toolbars from the existing user or default settings
	bool loadToolbars(bool force_default = false);	// return false if load fails
	
	static bool loadDefaultToolbars();
	
	static void startDragItem( S32 x, S32 y, const LLUUID& uuid);
	static BOOL handleDragItem( S32 x, S32 y, const LLUUID& uuid, LLAssetType::EType type);
	static BOOL handleDrop(	void* cargo_data, S32 x, S32 y, LLToolBar* toolbar);
	
protected:
	friend class LLUICtrlFactory;
	LLToolBarView(const Params&);

	void initFromParams(const Params&);

private:
	void	saveToolbars() const;
	bool	addCommand(const LLCommandId& commandId, LLToolBar*	toolbar);
	void	addToToolset(command_id_list_t& command_list, Toolbar& toolbar) const;

	// Pointers to the toolbars handled by the toolbar view
	LLToolBar*	mToolbarLeft;
	LLToolBar*	mToolbarRight;
	LLToolBar*	mToolbarBottom;
	bool		mDragging;
	LLToolBarButton* mDragButton;
	S32			mMouseX;
	S32			mMouseY;
	
	static bool			sDragStarted;
};

extern LLToolBarView* gToolBarView;

#endif  // LL_LLTOOLBARVIEW_H