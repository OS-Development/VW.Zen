/** 
 * @file llpanelmarketplaceoutboxinventory.h
 * @brief LLOutboxInventoryPanel class declaration
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#ifndef LL_OUTBOXINVENTORYPANEL_H
#define LL_OUTBOXINVENTORYPANEL_H


#include "llbadgeowner.h"
#include "llinventorypanel.h"
#include "llfolderviewitem.h"


class LLOutboxInventoryPanel : public LLInventoryPanel
{
public:
	struct Params : public LLInitParam::Block<Params, LLInventoryPanel::Params>
	{
		Params() {}
	};
	
	LLOutboxInventoryPanel(const Params& p);
	~LLOutboxInventoryPanel();

	// virtual
	void buildFolderView(const LLInventoryPanel::Params& params);

	// virtual
	class LLFolderViewFolder*	createFolderViewFolder(LLInvFVBridge * bridge);
};


class LLOutboxFolderViewFolder : public LLFolderViewFolder, public LLBadgeOwner
{
public:
	struct Params : public LLInitParam::Block<Params, LLFolderViewFolder::Params>
	{
		Optional<LLBadge::Params>	error_badge;
		
		Params()
		: error_badge("error_badge")
		{
		}
	};
	
	LLOutboxFolderViewFolder(const Params& p);
	~LLOutboxFolderViewFolder();

	void draw();
	
	bool hasError() const { return mError; }

protected:
	void setCreationDate(time_t creation_date_utc) const;
	
	bool mError;
};


#endif //LL_OUTBOXINVENTORYPANEL_H
