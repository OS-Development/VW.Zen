/** 
 * @file llnearbychatbar.cpp
 * @brief LLNearbyChatBar class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "message.h"

#include "llappviewer.h"
#include "llfloaterreg.h"
#include "lltrans.h"

#include "llfirstuse.h"
#include "llnearbychatbar.h"
#include "llagent.h"
#include "llgesturemgr.h"
#include "llmultigesture.h"
#include "llkeyboard.h"
#include "llanimationstates.h"
#include "llviewerstats.h"
#include "llcommandhandler.h"
#include "llviewercontrol.h"
#include "llnavigationbar.h"
#include "llwindow.h"
#include "llviewerwindow.h"
#include "llrootview.h"
#include "llviewerchat.h"
#include "llnearbychat.h"
#include "lltranslate.h"
#include "llevents.h"
#include "llimfloater.h"
#include "llimfloatercontainer.h"
#include "llmenugl.h"
#include "llmultifloater.h"
#include "llnearbychatbarmulti.h"

#include "llresizehandle.h"

S32 LLNearbyChatBarBase::sLastSpecialChatChannel = 0;

const S32 EXPANDED_HEIGHT = 300;
const S32 EXPANDED_MIN_HEIGHT = 150;
S32 COLLAPSED_HEIGHT = 60;

// legacy callback glue
void send_chat_from_viewer(const std::string& utf8_out_text, EChatType type, S32 channel);

struct LLChatTypeTrigger {
	std::string name;
	EChatType type;
};

static LLChatTypeTrigger sChatTypeTriggers[] = {
	{ "/whisper"	, CHAT_TYPE_WHISPER},
	{ "/shout"	, CHAT_TYPE_SHOUT}
};

LLNearbyChatBarSingle::LLNearbyChatBarSingle() 
	: LLPanel()
	, mChatBox(NULL)
	, mOutputMonitor(NULL)
	, mSpeakerMgr(LLLocalSpeakerMgr::getInstance())
{
}

LLNearbyChatBar::LLNearbyChatBar(const LLSD& key)
:	LLFloater(key),
mExpandedHeight(COLLAPSED_HEIGHT + EXPANDED_HEIGHT),
	mExpandedHeightMin(EXPANDED_MIN_HEIGHT),
	mNearbyChatContainer(NULL),
	mNearbyChat(NULL),
	mChatBarImpl(NULL)
{
	mFactoryMap["panel_chat_bar_single"] = LLCallbackMap(createChatBarSingle, NULL);
	mFactoryMap["panel_chat_bar_multi"] = LLCallbackMap(createChatBarMulti, NULL);
}

//virtual
BOOL LLNearbyChatBarSingle::postBuild()
{
	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
	LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;
	registrar.add("NearbyChat.Action", boost::bind(&LLNearbyChat::onNearbyChatAction, _2));
	enable_registrar.add("NearbyChat.Check", boost::bind(&LLNearbyChat::onNearbyChatCheck, _2));
	registrar.add("NearbyChat.SetChatBarType", boost::bind(&LLNearbyChat::onSetChatBarType, _2));
	enable_registrar.add("NearbyChat.CheckChatBarType", boost::bind(&LLNearbyChat::onCheckChatBarType, _2));
	registrar.add("NearbyChat.SetFontSize", boost::bind(&LLNearbyChat::onSetFontSize, _2));
	enable_registrar.add("NearbyChat.CheckFontSize", boost::bind(&LLNearbyChat::onCheckFontSize, _2));

	mChatBox = getChild<LLLineEditor>("chat_box");

	mChatBox->setContextMenu(LLUICtrlFactory::instance().createFromFile<LLContextMenu>("menu_chat_bar.xml", 
																					   LLMenuGL::sMenuContainer, 
																					   LLMenuHolderGL::child_registry_t::instance()));
	
	mChatBox->setCommitCallback(boost::bind(&LLNearbyChatBarSingle::onChatBoxCommit, this));
	mChatBox->setKeystrokeCallback(boost::bind(&LLNearbyChatBarSingle::onChatBoxKeystroke, this), NULL);
	mChatBox->setFocusLostCallback(boost::bind(&LLNearbyChatBarSingle::onChatBoxFocusLost, this));
	mChatBox->setFocusReceivedCallback(boost::bind(&LLNearbyChatBarSingle::onChatBoxFocusReceived, this));

	mChatBox->setIgnoreArrowKeys( FALSE ); 
	mChatBox->setCommitOnFocusLost( FALSE );
	mChatBox->setRevertOnEsc( FALSE );
	mChatBox->setIgnoreTab(TRUE);
	mChatBox->setPassDelete(TRUE);
	mChatBox->setReplaceNewlinesWithSpaces(FALSE);
	mChatBox->setEnableLineHistory(TRUE);
	mChatBox->setFont(LLViewerChat::getChatFont());

	mOutputMonitor = getChild<LLOutputMonitorCtrl>("chat_zone_indicator");
	mOutputMonitor->setVisible(FALSE);
	
	// Register for font change notifications
	LLViewerChat::setFontChangedCallback(boost::bind(&LLNearbyChatBarSingle::onChatFontChange, this, _1));

	return TRUE;
}

BOOL LLNearbyChatBar::postBuild()
{
	mNearbyChatContainer = findChild<LLPanel>("panel_nearby_chat");
	mNearbyChat = findChild<LLNearbyChat>("nearby_chat");

	gSavedSettings.declareBOOL("nearbychat_history_visibility", mNearbyChatContainer->getVisible(), "Visibility state of nearby chat history", TRUE);
	mNearbyChatContainer->setVisible(gSavedSettings.getBOOL("nearbychat_history_visibility"));
	
	mChatBarImpl = (hasChild("panel_chat_bar_multi", TRUE)) ? findChild<LLNearbyChatBarBase>("panel_chat_bar_multi", TRUE) : findChild<LLNearbyChatBarBase>("panel_chat_bar_single", TRUE);

	LLUICtrl* show_btn = getChild<LLUICtrl>("show_nearby_chat");
	show_btn->setCommitCallback(boost::bind(&LLNearbyChatBar::onToggleNearbyChatPanel, this));

	// The collpased height differs between single-line and multi-line so dynamically calculate it from the default sizes
	COLLAPSED_HEIGHT = getRect().getHeight() - mNearbyChatContainer->getRect().getHeight();
	mExpandedHeightMin = llmax(getMinHeight(), mExpandedHeightMin);

	enableResizeCtrls(true, true, false);

	// Initalize certain parameters depending on default vs embedded state
	bool fTabbedNearbyChat = isTabbedNearbyChat();
	setCanTearOff(fTabbedNearbyChat);

	if (fTabbedNearbyChat)
	{
		LLIMFloaterContainer* pConvFloater = LLIMFloaterContainer::getInstance();
		if (pConvFloater)
		{
			if (!mNearbyChatContainer->getVisible())
				onToggleNearbyChatPanel();
			pConvFloater->addFloater(this, TRUE, LLTabContainer::START);

			LLEventPump& pumpNearbyChat = LLEventPumps::instance().obtain("LLChat");
			pumpNearbyChat.listen("nearby_chat_bar", boost::bind(&LLNearbyChatBar::onNewNearbyChatMsg, this, _1));
		}

		onTearOff(LLSD(isTornOff()));
		setTearOffCallback(boost::bind(&LLNearbyChatBar::onTearOff, this, _2));
	}

	return TRUE;
}

// virtual
void LLNearbyChatBar::onOpen(const LLSD& key)
{
	enableTranslationCheckbox(LLTranslate::isTranslationConfigured());

	// When opening the floater with nearby chat visible, go ahead and kill off screen chats
	if (mNearbyChatContainer->getVisible())
	{
		mNearbyChat->removeScreenChat();
	}
}

bool LLNearbyChatBar::onNewNearbyChatMsg(const LLSD& sdEvent)
{
	if ( (!isTornOff()) && (!isInVisibleChain()) )
	{
		LLIMFloaterContainer* pConvFloater = LLIMFloaterContainer::getInstance();
		if (pConvFloater)
		{
			if (pConvFloater->isFloaterFlashing(this))
				pConvFloater->setFloaterFlashing(this, FALSE);
			pConvFloater->setFloaterFlashing(this, TRUE);
		}
	}
	return false;
}

void LLNearbyChatBar::onTearOff(const LLSD& sdData)
{
	LLUICtrl* pTogglePanel = findChild<LLUICtrl>("panel_nearby_chat_toggle");
	if (sdData.asBoolean())		// Tearing off
	{
		pTogglePanel->setVisible(TRUE);
	}
	else						// Attaching
	{
		showHistory();
		pTogglePanel->setVisible(FALSE);
	}

	// Don't allow closing the nearby chat floater while it's attached
	setCanClose(sdData.asBoolean());
}

bool LLNearbyChatBar::applyRectControl()
{
	if (getHost())
	{
		return true;
	}

	bool rect_controlled = LLFloater::applyRectControl();

	if (!mNearbyChatContainer->getVisible())
	{
		if (!isMinimized())
			mExpandedHeight = llmax(EXPANDED_MIN_HEIGHT, getRect().getHeight());
		setResizeLimits(getMinWidth(), COLLAPSED_HEIGHT);
		reshape(getRect().getWidth(), COLLAPSED_HEIGHT);
		enableResizeCtrls(true, true, false);
	}
	else
	{
		enableResizeCtrls(true);
		setResizeLimits(getMinWidth(), mExpandedHeightMin);
	}
	
	return rect_controlled;
}

void LLNearbyChatBarSingle::onChatFontChange(LLFontGL* fontp)
{
	// Update things with the new font whohoo
	if (mChatBox)
	{
		mChatBox->setFont(fontp);
	}
}

//static
LLNearbyChatBar* LLNearbyChatBar::getInstance()
{
	return LLFloaterReg::getTypedInstance<LLNearbyChatBar>("chat_bar");
}

void LLNearbyChatBar::showHistory()
{
	openFloater();

	if (!mNearbyChatContainer->getVisible())
	{
		onToggleNearbyChatPanel();
	}
}

void LLNearbyChatBar::enableTranslationCheckbox(BOOL enable)
{
	getChild<LLUICtrl>("translate_chat_checkbox")->setEnabled(enable);
}

void LLNearbyChatBarSingle::draw()
{
	displaySpeakingIndicator();
	LLPanel::draw();
}

// virtual
BOOL LLNearbyChatBar::handleKeyHere( KEY key, MASK mask )
{
	BOOL handled = FALSE;

	if( KEY_RETURN == key && mask == MASK_CONTROL)
	{
		// shout
		mChatBarImpl->sendChat(CHAT_TYPE_SHOUT);
		handled = TRUE;
	}
	if ( (KEY_RETURN == key) && (mask == MASK_SHIFT) )
	{
		// Whisper
		mChatBarImpl->sendChat(CHAT_TYPE_WHISPER);
		handled = TRUE;
	}

	return handled;
}

BOOL LLNearbyChatBarBase::matchChatTypeTrigger(const std::string& in_str, std::string* out_str)
{
	U32 in_len = in_str.length();
	S32 cnt = sizeof(sChatTypeTriggers) / sizeof(*sChatTypeTriggers);
	
	for (S32 n = 0; n < cnt; n++)
	{
		if (in_len > sChatTypeTriggers[n].name.length())
			continue;

		std::string trigger_trunc = sChatTypeTriggers[n].name;
		LLStringUtil::truncate(trigger_trunc, in_len);

		if (!LLStringUtil::compareInsensitive(in_str, trigger_trunc))
		{
			*out_str = sChatTypeTriggers[n].name;
			return TRUE;
		}
	}

	return FALSE;
}

void LLNearbyChatBarBase::onChatBoxKeystroke()
{
	LLFirstUse::otherAvatarChatFirst(false);

	LLLineEditor* pLineEditor = dynamic_cast<LLLineEditor*>(getChatBoxCtrl());
	LLTextEditor* pTextEditor = dynamic_cast<LLTextEditor*>(getChatBoxCtrl());

	LLWString raw_text = (pLineEditor) ? pLineEditor->getWText() : pTextEditor->getWText();

	// Can't trim the end, because that will cause autocompletion
	// to eat trailing spaces that might be part of a gesture.
	LLWStringUtil::trimHead(raw_text);

	S32 length = raw_text.length();

	if( (length > 0) && (raw_text[0] != '/') )  // forward slash is used for escape (eg. emote) sequences
	{
		gAgent.startTyping();
	}
	else
	{
		gAgent.stopTyping();
	}

	/* Doesn't work -- can't tell the difference between a backspace
	   that killed the selection vs. backspace at the end of line.
	if (length > 1 
		&& text[0] == '/'
		&& key == KEY_BACKSPACE)
	{
		// the selection will already be deleted, but we need to trim
		// off the character before
		std::string new_text = raw_text.substr(0, length-1);
		self->mInputEditor->setText( new_text );
		self->mInputEditor->setCursorToEnd();
		length = length - 1;
	}
	*/

	KEY key = gKeyboard->currentKey();

	// Ignore "special" keys, like backspace, arrows, etc.
	if (length > 1 
		&& raw_text[0] == '/'
		&& key < KEY_SPECIAL)
	{
		// we're starting a gesture, attempt to autocomplete

		std::string utf8_trigger = wstring_to_utf8str(raw_text);
		std::string utf8_out_str(utf8_trigger);

		if (LLGestureMgr::instance().matchPrefix(utf8_trigger, &utf8_out_str))
		{
			std::string rest_of_match = utf8_out_str.substr(utf8_trigger.size());
			if (pLineEditor)
			{
				pLineEditor->setText(utf8_trigger + rest_of_match);
				pLineEditor->setSelection(length, pLineEditor->getLength());
			}
			else
			{
				pTextEditor->setText(utf8_trigger + rest_of_match);
				pTextEditor->setSelection(length, pTextEditor->getLength());
			}
		}
		else if (matchChatTypeTrigger(utf8_trigger, &utf8_out_str))
		{
			std::string rest_of_match = utf8_out_str.substr(utf8_trigger.size());
			if (pLineEditor)
			{
				pLineEditor->setText(utf8_trigger + rest_of_match + " ");
				pLineEditor->setCursorToEnd();
			}
			else
			{
				pTextEditor->setText(utf8_trigger + rest_of_match + " ");
				pTextEditor->endOfDoc();
			}
		}
	}
}

// static
void LLNearbyChatBarBase::onChatBoxFocusLost()
{
	// stop typing animation
	gAgent.stopTyping();
}

void LLNearbyChatBarBase::onChatBoxFocusReceived()
{
	getChatBoxCtrl()->setEnabled(!gDisconnected);
}

EChatType LLNearbyChatBarBase::processChatTypeTriggers(EChatType type, std::string &str)
{
	U32 length = str.length();
	S32 cnt = sizeof(sChatTypeTriggers) / sizeof(*sChatTypeTriggers);
	
	for (S32 n = 0; n < cnt; n++)
	{
		if (length >= sChatTypeTriggers[n].name.length())
		{
			std::string trigger = str.substr(0, sChatTypeTriggers[n].name.length());

			if (!LLStringUtil::compareInsensitive(trigger, sChatTypeTriggers[n].name))
			{
				U32 trigger_length = sChatTypeTriggers[n].name.length();

				// It's to remove space after trigger name
				if (length > trigger_length && str[trigger_length] == ' ')
					trigger_length++;

				str = str.substr(trigger_length, length);

				if (CHAT_TYPE_NORMAL == type)
					return sChatTypeTriggers[n].type;
				else
					break;
			}
		}
	}

	return type;
}

void LLNearbyChatBarBase::sendChat(EChatType type)
{
	LLLineEditor* pLineEditor = dynamic_cast<LLLineEditor*>(getChatBoxCtrl());
	LLTextEditor* pTextEditor = dynamic_cast<LLTextEditor*>(getChatBoxCtrl());
	if ( (pLineEditor) || (pTextEditor) )
	{
		LLWString text = getChatBoxText();
		LLWStringUtil::trim(text);
		if (!text.empty())
		{
			// store sent line in history, duplicates will get filtered
			if (pLineEditor)
				pLineEditor->updateHistory();
			// Check if this is destined for another channel
			S32 channel = 0;
			stripChannelNumber(text, &channel);
			
			std::string utf8text = wstring_to_utf8str(text);
			// Try to trigger a gesture, if not chat to a script.
			std::string utf8_revised_text;
			if (0 == channel)
			{
				// discard returned "found" boolean
				LLGestureMgr::instance().triggerAndReviseString(utf8text, &utf8_revised_text);
			}
			else
			{
				utf8_revised_text = utf8text;
			}

			utf8_revised_text = utf8str_trim(utf8_revised_text);

			type = processChatTypeTriggers(type, utf8_revised_text);

			if (!utf8_revised_text.empty())
			{
				// Chat with animation
				sendChatFromViewer(utf8_revised_text, type, TRUE);
			}
		}

		setChatBoxText(LLStringExplicit(""));
	}

	gAgent.stopTyping();

	// If the user wants to stop chatting on hitting return, lose focus
	// and go out of chat mode.
	if (gSavedSettings.getBOOL("CloseChatOnReturn"))
	{
		getChatBoxCtrl()->setFocus(FALSE);
	}
}


void LLNearbyChatBar::onToggleNearbyChatPanel()
{
	LLView* nearby_chat = mNearbyChatContainer;

	if (nearby_chat->getVisible())
	{
		if (!isMinimized())
		{
			mExpandedHeight = getRect().getHeight();
		}
		setResizeLimits(getMinWidth(), COLLAPSED_HEIGHT);
		nearby_chat->setVisible(FALSE);
		reshape(getRect().getWidth(), COLLAPSED_HEIGHT);
		enableResizeCtrls(true, true, false);
		storeRectControl();
	}
	else
	{
		nearby_chat->setVisible(TRUE);
		setResizeLimits(getMinWidth(), mExpandedHeightMin);
		reshape(getRect().getWidth(), mExpandedHeight);
		enableResizeCtrls(true);
		storeRectControl();
	}

	gSavedSettings.setBOOL("nearbychat_history_visibility", mNearbyChatContainer->getVisible());
}

BOOL LLNearbyChatBar::canClose()
{
	// The added getVisible() check is a little hack for LLNearbyChatBar::processFloaterTypeChanged() to make LLFloater::closeFloater() not skip over clean-up
	if ( (getHost()) && (getVisible()) )
		return false;
	return LLFloater::canClose();
}

void LLNearbyChatBarSingle::onChatBoxCommit()

{
	if (mChatBox->getText().length() > 0)
	{
		sendChat(CHAT_TYPE_NORMAL);
	}
	else if (gSavedSettings.getBOOL("CloseChatOnEmptyReturn"))
	{
		// Close if we're the child of a floater
		LLFloater* pFloater = getParentByType<LLFloater>();
		if (pFloater)
			pFloater->closeFloater();
	}

	gAgent.stopTyping();
}

void LLNearbyChatBarSingle::displaySpeakingIndicator()
{
	LLSpeakerMgr::speaker_list_t speaker_list;
	LLUUID id;

	id.setNull();
	mSpeakerMgr->update(TRUE);
	mSpeakerMgr->getSpeakerList(&speaker_list, FALSE);

	for (LLSpeakerMgr::speaker_list_t::iterator i = speaker_list.begin(); i != speaker_list.end(); ++i)
	{
		LLPointer<LLSpeaker> s = *i;
		if (s->mSpeechVolume > 0 || s->mStatus == LLSpeaker::STATUS_SPEAKING)
		{
			id = s->mID;
			break;
		}
	}

	if (!id.isNull())
	{
		mOutputMonitor->setVisible(TRUE);
		mOutputMonitor->setSpeakerId(id);
	}
	else
	{
		mOutputMonitor->setVisible(FALSE);
	}
}

void LLNearbyChatBarBase::sendChatFromViewer(const std::string &utf8text, EChatType type, BOOL animate)
{
	sendChatFromViewer(utf8str_to_wstring(utf8text), type, animate);
}

void LLNearbyChatBarBase::sendChatFromViewer(const LLWString &wtext, EChatType type, BOOL animate)
{
	// Look for "/20 foo" channel chats.
	S32 channel = 0;
	LLWString out_text = stripChannelNumber(wtext, &channel);
	std::string utf8_out_text = wstring_to_utf8str(out_text);
	std::string utf8_text = wstring_to_utf8str(wtext);

	utf8_text = utf8str_trim(utf8_text);
	if (!utf8_text.empty())
	{
		utf8_text = utf8str_truncate(utf8_text, MAX_STRING - 1);
	}

	// Don't animate for chats people can't hear (chat to scripts)
	if (animate && (channel == 0))
	{
		if (type == CHAT_TYPE_WHISPER)
		{
			lldebugs << "You whisper " << utf8_text << llendl;
			gAgent.sendAnimationRequest(ANIM_AGENT_WHISPER, ANIM_REQUEST_START);
		}
		else if (type == CHAT_TYPE_NORMAL)
		{
			lldebugs << "You say " << utf8_text << llendl;
			gAgent.sendAnimationRequest(ANIM_AGENT_TALK, ANIM_REQUEST_START);
		}
		else if (type == CHAT_TYPE_SHOUT)
		{
			lldebugs << "You shout " << utf8_text << llendl;
			gAgent.sendAnimationRequest(ANIM_AGENT_SHOUT, ANIM_REQUEST_START);
		}
		else
		{
			llinfos << "send_chat_from_viewer() - invalid volume" << llendl;
			return;
		}
	}
	else
	{
		if (type != CHAT_TYPE_START && type != CHAT_TYPE_STOP)
		{
			lldebugs << "Channel chat: " << utf8_text << llendl;
		}
	}

	send_chat_from_viewer(utf8_out_text, type, channel);
}

// static 
void LLNearbyChatBar::startChat(const char* line)
{
	LLNearbyChatBar* pSelf = getInstance();
	if (!pSelf)
		return;

	pSelf->openFloater(LLSD());
	pSelf->setFocus(TRUE);
	pSelf->mChatBarImpl->getChatBoxCtrl()->setFocus(TRUE);

	if (line)
	{
		LLStringExplicit line_string(line);
		pSelf->mChatBarImpl->setChatBoxText(line_string);
	}

	pSelf->mChatBarImpl->setChatBoxCursorToEnd();
}

// Exit "chat mode" and do the appropriate focus changes
// static
void LLNearbyChatBar::stopChat()
{
	LLNearbyChatBar* pSelf = getInstance();
	if (!pSelf)
		return;

	pSelf->getChatBarImpl()->getChatBoxCtrl()->setFocus(FALSE);

 	// stop typing animation
 	gAgent.stopTyping();
}

// If input of the form "/20foo" or "/20 foo", returns "foo" and channel 20.
// Otherwise returns input and channel 0.
LLWString LLNearbyChatBarBase::stripChannelNumber(const LLWString &mesg, S32* channel)
{
	if (mesg[0] == '/'
		&& mesg[1] == '/')
	{
		// This is a "repeat channel send"
		*channel = sLastSpecialChatChannel;
		return mesg.substr(2, mesg.length() - 2);
	}
	else if (mesg[0] == '/'
			 && mesg[1]
			 && LLStringOps::isDigit(mesg[1]))
	{
		// This a special "/20" speak on a channel
		S32 pos = 0;

		// Copy the channel number into a string
		LLWString channel_string;
		llwchar c;
		do
		{
			c = mesg[pos+1];
			channel_string.push_back(c);
			pos++;
		}
		while(c && pos < 64 && LLStringOps::isDigit(c));
		
		// Move the pointer forward to the first non-whitespace char
		// Check isspace before looping, so we can handle "/33foo"
		// as well as "/33 foo"
		while(c && iswspace(c))
		{
			c = mesg[pos+1];
			pos++;
		}
		
		sLastSpecialChatChannel = strtol(wstring_to_utf8str(channel_string).c_str(), NULL, 10);
		*channel = sLastSpecialChatChannel;
		return mesg.substr(pos, mesg.length() - pos);
	}
	else
	{
		// This is normal chat.
		*channel = 0;
		return mesg;
	}
}

void* LLNearbyChatBar::createChatBarSingle(void*)
{
	return new LLNearbyChatBarSingle();
}

void* LLNearbyChatBar::createChatBarMulti(void*)
{
	return new LLNearbyChatBarMulti();
}

const std::string& LLNearbyChatBar::getFloaterXMLFile()
{
	static std::string strFile;
	switch (gSavedSettings.getS32("NearbyChatFloaterBarType"))
	{
		case 2:		// Multi-line
			strFile = "floater_chat_bar_multi.xml";
			break;
		case 0:		// None (default)
		case 1:		// Single-line
		default:
			strFile = "floater_chat_bar.xml";
			break;
	}
	return strFile;
}

bool LLNearbyChatBar::isTabbedNearbyChat()
{
	return (LLIMFloater::isChatMultiTab()) && (gSavedSettings.getBOOL("NearbyChatFloaterWindow"));
}

void LLNearbyChatBar::processFloaterTypeChanged()
{
	// We only need to do anything if an instance of the nearby chat floater already exists
	LLNearbyChatBar* pNearbyChat = LLFloaterReg::findTypedInstance<LLNearbyChatBar>("chat_bar");
	if (pNearbyChat)
	{
		LLIMFloaterContainer* pConvFloater = LLIMFloaterContainer::findInstance();
		
		bool fConvVisible = pConvFloater->getVisible();
		bool fNearbyVisible = pNearbyChat->getVisible();
		
		if (pNearbyChat->getHost())
			pNearbyChat->setVisible(FALSE);	// See LLNearbyChatBar::canClose()
		std::vector<LLChat> msgArchive = pNearbyChat->mNearbyChat->getHistory();

		// NOTE: * LLFloater::closeFloater() won't call LLFloater::destroy() since the nearby chat floater is single instanced
		//       * we can't call LLFloater::destroy() since it will call LLMortician::die() which defers destruction until a later time
		//   => we'll have created a new instance and the delayed destructor calling LLFloaterReg::removeInstance() will make all future
		//      LLFloaterReg::getTypedInstance() calls return NULL so we need to destruct manually [see LLFloaterReg::destroyInstance()]
		pNearbyChat->closeFloater();
		LLFloaterReg::destroyInstance("chat_bar", LLSD());

		if ((pNearbyChat = LLFloaterReg::getTypedInstance<LLNearbyChatBar>("chat_bar", LLSD())) != NULL)
		{
			pNearbyChat->mNearbyChat->setHistory(msgArchive);
			pNearbyChat->mNearbyChat->updateChatHistoryStyle();
			if (fNearbyVisible)
				pNearbyChat->openFloater(LLSD());
			if ( (pConvFloater) && (pConvFloater->getVisible()) && (!fConvVisible) )
				pConvFloater->closeFloater();
		}
	}
}

void send_chat_from_viewer(const std::string& utf8_out_text, EChatType type, S32 channel)
{
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_ChatFromViewer);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_ChatData);
	msg->addStringFast(_PREHASH_Message, utf8_out_text);
	msg->addU8Fast(_PREHASH_Type, type);
	msg->addS32("Channel", channel);

	gAgent.sendReliableMessage();

	LLViewerStats::getInstance()->incStat(LLViewerStats::ST_CHAT_COUNT);
}

class LLChatCommandHandler : public LLCommandHandler
{
public:
	// not allowed from outside the app
	LLChatCommandHandler() : LLCommandHandler("chat", UNTRUSTED_BLOCK) { }

    // Your code here
	bool handle(const LLSD& tokens, const LLSD& query_map,
				LLMediaCtrl* web)
	{
		bool retval = false;
		// Need at least 2 tokens to have a valid message.
		if (tokens.size() < 2)
		{
			retval = false;
		}
		else
		{
		S32 channel = tokens[0].asInteger();
			// VWR-19499 Restrict function to chat channels greater than 0.
			if ((channel > 0) && (channel < CHAT_CHANNEL_DEBUG))
			{
				retval = true;
		// Send unescaped message, see EXT-6353.
		std::string unescaped_mesg (LLURI::unescape(tokens[1].asString()));
		send_chat_from_viewer(unescaped_mesg, CHAT_TYPE_NORMAL, channel);
			}
			else
			{
				retval = false;
				// Tell us this is an unsupported SLurl.
			}
		}
		return retval;
	}
};

// Creating the object registers with the dispatcher.
LLChatCommandHandler gChatHandler;


