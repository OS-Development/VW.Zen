/** 
 * @file llteleporthistory.cpp
 * @brief Teleport history
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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

#include "llteleporthistory.h"

#include "llparcel.h"
#include "llsdserialize.h"

#include "llagent.h"
#include "llslurl.h"
#include "llurlsimstring.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llworldmap.h"

//////////////////////////////////////////////////////////////////////////////
// LLTeleportHistoryItem
//////////////////////////////////////////////////////////////////////////////

LLTeleportHistoryItem::LLTeleportHistoryItem(const LLSD& val)
{
	mTitle = val["title"].asString();
	mGlobalPos.setValue(val["global_pos"]);
}

LLSD LLTeleportHistoryItem::toLLSD() const
{
	LLSD val;

	val["title"]		= mTitle;
	val["global_pos"]	= mGlobalPos.getValue();
	
	return val;
}

//////////////////////////////////////////////////////////////////////////////
// LLTeleportHistory
//////////////////////////////////////////////////////////////////////////////

LLTeleportHistory::LLTeleportHistory():
	mCurrentItem(-1),
	mRequestedItem(-1),
	mGotInitialUpdate(false)
{
	mTeleportFinishedConn = LLViewerParcelMgr::getInstance()->
		setTeleportFinishedCallback(boost::bind(&LLTeleportHistory::updateCurrentLocation, this));
	mTeleportFailedConn = LLViewerParcelMgr::getInstance()->
		setTeleportFailedCallback(boost::bind(&LLTeleportHistory::onTeleportFailed, this));
}

LLTeleportHistory::~LLTeleportHistory()
{
	mTeleportFinishedConn.disconnect();
	mTeleportFailedConn.disconnect();
}

void LLTeleportHistory::goToItem(int idx)

{
	// Validate specified index.
	if (idx < 0 || idx >= (int)mItems.size())
	{
		llwarns << "Invalid teleport history index (" << idx << ") specified" << llendl;
		dump();
		return;
	}
	
	if (idx == mCurrentItem)
	{
		llwarns << "Will not teleport to the same location." << llendl;
		dump();
		return;
	}

	// Attempt to teleport to the requested item.
	gAgent.teleportViaLocation(mItems[idx].mGlobalPos);
	mRequestedItem = idx;
}

void LLTeleportHistory::onTeleportFailed()
{
	// Are we trying to teleport within the history?
	if (mRequestedItem != -1)
	{
		// Not anymore.
		mRequestedItem = -1;
	}
}

void LLTeleportHistory::updateCurrentLocation()
{
	if (mRequestedItem != -1) // teleport within the history in progress?
	{
		mCurrentItem = mRequestedItem;
		mRequestedItem = -1;
	}
	else
	{
		// If we're getting the initial location update
		// while we already have a (loaded) non-empty history,
		// there's no need to purge forward items or add a new item.

		if (mGotInitialUpdate || mItems.size() == 0)
		{
			// Purge forward items (if any).
			if(mItems.size())
				mItems.erase (mItems.begin() + mCurrentItem + 1, mItems.end());
			
			// Append an empty item to the history and make it current.
			mItems.push_back(LLTeleportHistoryItem("", LLVector3d()));
			mCurrentItem++;
		}

		// Update current history item.
		if (mCurrentItem < 0 || mCurrentItem >= (int) mItems.size()) // sanity check
		{
			llwarns << "Invalid current item. (this should not happen)" << llendl;
			return;
		}
		mItems[mCurrentItem].mTitle = getCurrentLocationTitle();
		mItems[mCurrentItem].mGlobalPos	= gAgent.getPositionGlobal();
		mItems[mCurrentItem].mRegionID = gAgent.getRegion()->getRegionID();
	}

	dump();
	
	if (!mGotInitialUpdate)
		mGotInitialUpdate = true;

	// Signal the interesting party that we've changed. 
	onHistoryChanged();
}

boost::signals2::connection LLTeleportHistory::setHistoryChangedCallback(history_callback_t cb)
{
	return mHistoryChangedSignal.connect(cb);
}

void LLTeleportHistory::onHistoryChanged()
{
	mHistoryChangedSignal();
}

// static
std::string LLTeleportHistory::getCurrentLocationTitle()
{
	std::string location_name;

	if (!gAgent.buildLocationString(location_name, LLAgent::LOCATION_FORMAT_NORMAL))
		location_name = "Unknown";

	return location_name;
}

void LLTeleportHistory::dump() const
{
	llinfos << "Teleport history dump (" << mItems.size() << " items):" << llendl;
	
	for (size_t i=0; i<mItems.size(); i++)
	{
		std::stringstream line;
		line << ((i == mCurrentItem) ? " * " : "   ");
		line << i << ": " << mItems[i].mTitle;
		line << " REGION_ID: " << mItems[i].mRegionID;
		llinfos << line.str() << llendl;
	}
}