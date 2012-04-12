/** 
 * @file llnearbychatbar.h
 * @brief LLNearbyChatBar class definition
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

#ifndef LL_LLNEARBYCHATBAR_H
#define LL_LLNEARBYCHATBAR_H

#include "llfloater.h"
#include "llcombobox.h"
#include "llgesturemgr.h"
#include "llchat.h"
#include "llvoiceclient.h"
#include "lloutputmonitorctrl.h"
#include "llspeakers.h"
#include "llnearbychatbarbase.h"

class LLNearbyChat;

class LLNearbyChatBarSingle 
	: public LLPanel
	, public LLNearbyChatBarBase
{
public:
	LLNearbyChatBarSingle();
	/*virtual*/ ~LLNearbyChatBarSingle() {}

public:
	/*virtual*/ void draw();
	/*virtual*/ BOOL postBuild();
protected:
	void displaySpeakingIndicator();
	void onChatBoxCommit();
	void onChatFontChange(LLFontGL* fontp);

	// LLNearbyChatBarBase overrides
public:
	/*virtual*/ LLUICtrl* getChatBoxCtrl()								{ return mChatBox; }
	/*virtual*/ LLWString getChatBoxText()								{ return mChatBox->getConvertedText(); }
	/*virtual*/ void      setChatBoxText(const LLStringExplicit& text)	{ mChatBox->setText(text); }
	/*virtual*/ void	  setChatBoxCursorToEnd()						{ mChatBox->setCursorToEnd(); }

protected:
	LLLineEditor*		 mChatBox;
	LLOutputMonitorCtrl* mOutputMonitor;
	LLLocalSpeakerMgr*	 mSpeakerMgr;
};

class LLNearbyChatBar :	public LLFloater
{
public:
	// constructor for inline chat-bars (e.g. hosted in chat history window)
	LLNearbyChatBar(const LLSD& key);
	~LLNearbyChatBar() {}

	virtual BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& key);

	static LLNearbyChatBar* getInstance();


	LLNearbyChatBarBase* getChatBarImpl() const { return mChatBarImpl; }


	std::string getCurrentChat();
	virtual BOOL handleKeyHere( KEY key, MASK mask );

	static void startChat(const char* line);
	static void stopChat();

	void showHistory();
	void enableTranslationCheckbox(BOOL enable);

	/*virtual*/ BOOL canClose();
	
	void updateUseNearbyChatConsole(const LLSD &data);


protected:
	bool onNewNearbyChatMsg(const LLSD& sdEvent);
	void onTearOff(const LLSD& sdData);

	/* virtual */ bool applyRectControl();

	void onToggleNearbyChatPanel();

public:
	static const std::string&	getFloaterXMLFile();
	static bool					isTabbedNearbyChat();
	static void					processFloaterTypeChanged();
protected:
	static void* createChatBarSingle(void*);
	static void* createChatBarMulti(void*);


	LLPanel*			 mNearbyChatContainer;		// "panel_nearby_chat" is the parent panel containing "nearby_chat"
	LLNearbyChat*		 mNearbyChat;				// "nearby_chat"
	LLNearbyChatBarBase* mChatBarImpl;

	S32 mExpandedHeight;
	S32 mExpandedHeightMin;
	
private:
	BOOL UseNearbyChatConsole;	
	
};

#endif
