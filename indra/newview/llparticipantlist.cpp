/** 
 * @file llparticipantlist.cpp
 * @brief LLParticipantList intended to update view(LLAvatarList) according to incoming messages
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

// common includes
#include "lltrans.h"
#include "llavataractions.h"
#include "llagent.h"

#include "llparticipantlist.h"
#include "llavatarlist.h"
#include "llspeakers.h"

//LLParticipantList retrieves add, clear and remove events and updates view accordingly 
#if LL_MSVC
#pragma warning (disable : 4355) // 'this' used in initializer list: yes, intentionally
#endif
LLParticipantList::LLParticipantList(LLSpeakerMgr* data_source, LLAvatarList* avatar_list):
	mSpeakerMgr(data_source),
	mAvatarList(avatar_list),
	mSortOrder(E_SORT_BY_NAME)
{
	mSpeakerAddListener = new SpeakerAddListener(*this);
	mSpeakerRemoveListener = new SpeakerRemoveListener(*this);
	mSpeakerClearListener = new SpeakerClearListener(*this);

	mSpeakerMgr->addListener(mSpeakerAddListener, "add");
	mSpeakerMgr->addListener(mSpeakerRemoveListener, "remove");
	mSpeakerMgr->addListener(mSpeakerClearListener, "clear");

	mAvatarList->setNoItemsCommentText(LLTrans::getString("LoadingData"));
	mAvatarList->setDoubleClickCallback(boost::bind(&LLParticipantList::onAvatarListDoubleClicked, this, mAvatarList));

	//Lets fill avatarList with existing speakers
	LLAvatarList::uuid_vector_t& group_members = mAvatarList->getIDs();

	LLSpeakerMgr::speaker_list_t speaker_list;
	mSpeakerMgr->getSpeakerList(&speaker_list, true);
	for(LLSpeakerMgr::speaker_list_t::iterator it = speaker_list.begin(); it != speaker_list.end(); it++)
	{
		group_members.push_back((*it)->mID);
	}
	sort();
}

LLParticipantList::~LLParticipantList()
{
}

void LLParticipantList::onAvatarListDoubleClicked(LLAvatarList* list)
{
	LLUUID clicked_id = list->getSelectedUUID();

	if (clicked_id.isNull() || clicked_id == gAgent.getID())
		return;
	
	LLAvatarActions::startIM(clicked_id);
}

void LLParticipantList::setSortOrder(EParticipantSortOrder order)
{
	if ( mSortOrder != order )
	{
		mSortOrder = order;
		sort();
	}
}

bool LLParticipantList::onAddItemEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	LLAvatarList::uuid_vector_t& group_members = mAvatarList->getIDs();
	LLUUID uu_id = event->getValue().asUUID();

	LLAvatarList::uuid_vector_t::iterator found = std::find(group_members.begin(), group_members.end(), uu_id);
	if(found != group_members.end())
	{
		llinfos << "Already got a buddy" << llendl;
		return true;
	}

	group_members.push_back(uu_id);
	sort();
	return true;
}

bool LLParticipantList::onRemoveItemEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	LLAvatarList::uuid_vector_t& group_members = mAvatarList->getIDs();
	LLAvatarList::uuid_vector_t::iterator pos = std::find(group_members.begin(), group_members.end(), event->getValue().asUUID());
	if(pos != group_members.end())
	{
		group_members.erase(pos);
		mAvatarList->setDirty();
	}
	return true;
}

bool LLParticipantList::onClearListEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	LLAvatarList::uuid_vector_t& group_members = mAvatarList->getIDs();
	group_members.clear();
	mAvatarList->setDirty();
	return true;
}

void LLParticipantList::sort()
{
	if ( !mAvatarList )
		return;

	// Mark AvatarList as dirty one
	mAvatarList->setDirty();

	// TODO: Implement more sorting orders after specs updating (EM)
	switch ( mSortOrder ) {
	case E_SORT_BY_NAME :
		mAvatarList->sortByName();
		break;
	default :
		llwarns << "Unrecognized sort order for " << mAvatarList->getName() << llendl;
		return;
	}
}

//
// LLParticipantList::SpeakerAddListener
//
bool LLParticipantList::SpeakerAddListener::handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	return mParent.onAddItemEvent(event, userdata);
}

//
// LLParticipantList::SpeakerRemoveListener
//
bool LLParticipantList::SpeakerRemoveListener::handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	return mParent.onRemoveItemEvent(event, userdata);
}

//
// LLParticipantList::SpeakerClearListener
//
bool LLParticipantList::SpeakerClearListener::handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	return mParent.onClearListEvent(event, userdata);
}
