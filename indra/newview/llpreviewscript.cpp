/** 
 * @file llpreviewscript.cpp
 * @brief LLPreviewScript class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#include "llpreviewscript.h"

#include "llassetstorage.h"
#include "llassetuploadresponders.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "lldir.h"
#include "llfloaterreg.h"
#include "llinventorymodel.h"
#include "llkeyboard.h"
#include "lllineeditor.h"

#include "llresmgr.h"
#include "llscrollbar.h"
#include "llscrollcontainer.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "llscrolllistcell.h"
#include "llslider.h"
#include "lscript_rt_interface.h"
#include "lscript_export.h"
#include "lltextbox.h"
#include "lltooldraganddrop.h"
#include "llvfile.h"

#include "llagent.h"
#include "llnotify.h"
#include "llmenugl.h"
#include "roles_constants.h"
#include "llselectmgr.h"
#include "llviewerinventory.h"
#include "llviewermenu.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llkeyboard.h"
#include "llscrollcontainer.h"
#include "llcheckboxctrl.h"
#include "llselectmgr.h"
#include "lltooldraganddrop.h"
#include "llscrolllistctrl.h"
#include "lltextbox.h"
#include "llslider.h"
#include "lldir.h"
#include "llcombobox.h"
//#include "llfloaterchat.h"
#include "llviewerstats.h"
#include "llviewertexteditor.h"
#include "llviewerwindow.h"
#include "lluictrlfactory.h"
#include "llmediactrl.h"
#include "lluictrlfactory.h"

#include "llviewercontrol.h"
#include "llappviewer.h"

#include "llpanelinventory.h"
#include "lltrans.h"

const std::string HELLO_LSL =
	"default\n"
	"{\n"
	"    state_entry()\n"
    "    {\n"
    "        llSay(0, \"Hello, Avatar!\");\n"
    "    }\n"
	"\n"
	"    touch_start(integer total_number)\n"
	"    {\n"
	"        llSay(0, \"Touched.\");\n"
	"    }\n"
	"}\n";
const std::string HELP_LSL_URL = "http://wiki.secondlife.com/wiki/LSL_Portal";

const std::string DEFAULT_SCRIPT_NAME = "New Script"; // *TODO:Translate?
const std::string DEFAULT_SCRIPT_DESC = "(No Description)"; // *TODO:Translate?

// Description and header information

const S32 MAX_EXPORT_SIZE = 1000;

const S32 MAX_HISTORY_COUNT = 10;
const F32 LIVE_HELP_REFRESH_TIME = 1.f;

static bool have_script_upload_cap(LLUUID& object_id)
{
	LLViewerObject* object = gObjectList.findObject(object_id);
	return object && (! object->getRegion()->getCapability("UpdateScriptTask").empty());
}

/// ---------------------------------------------------------------------------
/// LLFloaterScriptSearch
/// ---------------------------------------------------------------------------
class LLFloaterScriptSearch : public LLFloater
{
public:
	LLFloaterScriptSearch(LLScriptEdCore* editor_core);
	~LLFloaterScriptSearch();

	/*virtual*/	BOOL	postBuild();
	static void show(LLScriptEdCore* editor_core);
	static void onBtnSearch(void* userdata);
	void handleBtnSearch();

	static void onBtnReplace(void* userdata);
	void handleBtnReplace();

	static void onBtnReplaceAll(void* userdata);
	void handleBtnReplaceAll();

	LLScriptEdCore* getEditorCore() { return mEditorCore; }
	static LLFloaterScriptSearch* getInstance() { return sInstance; }

private:

	LLScriptEdCore* mEditorCore;

	static LLFloaterScriptSearch*	sInstance;
};

LLFloaterScriptSearch* LLFloaterScriptSearch::sInstance = NULL;

LLFloaterScriptSearch::LLFloaterScriptSearch(LLScriptEdCore* editor_core)
:	LLFloater(LLSD()),
	mEditorCore(editor_core)
{
	LLUICtrlFactory::getInstance()->buildFloater(this,"floater_script_search.xml", NULL);

	sInstance = this;
	
	// find floater in which script panel is embedded
	LLView* viewp = (LLView*)editor_core;
	while(viewp)
	{
		LLFloater* floaterp = dynamic_cast<LLFloater*>(viewp);
		if (floaterp)
		{
			floaterp->addDependentFloater(this);
			break;
		}
		viewp = viewp->getParent();
	}
}

BOOL LLFloaterScriptSearch::postBuild()
{
	childSetAction("search_btn", onBtnSearch,this);
	childSetAction("replace_btn", onBtnReplace,this);
	childSetAction("replace_all_btn", onBtnReplaceAll,this);

	setDefaultBtn("search_btn");

	return TRUE;
}

//static 
void LLFloaterScriptSearch::show(LLScriptEdCore* editor_core)
{
	if (sInstance && sInstance->mEditorCore && sInstance->mEditorCore != editor_core)
	{
		sInstance->closeFloater();
		delete sInstance;
	}

	if (!sInstance)
	{
		// sInstance will be assigned in the constructor.
		new LLFloaterScriptSearch(editor_core);
	}

	sInstance->openFloater();
}

LLFloaterScriptSearch::~LLFloaterScriptSearch()
{
	sInstance = NULL;
}

// static 
void LLFloaterScriptSearch::onBtnSearch(void *userdata)
{
	LLFloaterScriptSearch* self = (LLFloaterScriptSearch*)userdata;
	self->handleBtnSearch();
}

void LLFloaterScriptSearch::handleBtnSearch()
{
	LLCheckBoxCtrl* caseChk = getChild<LLCheckBoxCtrl>("case_text");
	mEditorCore->mEditor->selectNext(childGetText("search_text"), caseChk->get());
}

// static 
void LLFloaterScriptSearch::onBtnReplace(void *userdata)
{
	LLFloaterScriptSearch* self = (LLFloaterScriptSearch*)userdata;
	self->handleBtnReplace();
}

void LLFloaterScriptSearch::handleBtnReplace()
{
	LLCheckBoxCtrl* caseChk = getChild<LLCheckBoxCtrl>("case_text");
	mEditorCore->mEditor->replaceText(childGetText("search_text"), childGetText("replace_text"), caseChk->get());
}

// static 
void LLFloaterScriptSearch::onBtnReplaceAll(void *userdata)
{
	LLFloaterScriptSearch* self = (LLFloaterScriptSearch*)userdata;
	self->handleBtnReplaceAll();
}

void LLFloaterScriptSearch::handleBtnReplaceAll()
{
	LLCheckBoxCtrl* caseChk = getChild<LLCheckBoxCtrl>("case_text");
	mEditorCore->mEditor->replaceTextAll(childGetText("search_text"), childGetText("replace_text"), caseChk->get());
}

/// ---------------------------------------------------------------------------
/// LLScriptEdCore
/// ---------------------------------------------------------------------------

LLScriptEdCore::LLScriptEdCore(
	const std::string& sample,
	const std::string& help_url,
	const LLHandle<LLFloater>& floater_handle,
	void (*load_callback)(void*),
	void (*save_callback)(void*, BOOL),
	void (*search_replace_callback) (void* userdata),
	void* userdata,
	S32 bottom_pad)
	:
	LLPanel(),
	mSampleText(sample),
	mHelpURL(help_url),
	mEditor( NULL ),
	mLoadCallback( load_callback ),
	mSaveCallback( save_callback ),
	mSearchReplaceCallback( search_replace_callback ),
	mUserdata( userdata ),
	mForceClose( FALSE ),
	mLastHelpToken(NULL),
	mLiveHelpHistorySize(0),
	mEnableSave(FALSE),
	mHasScriptData(FALSE)
{
	setFollowsAll();
	setBorderVisible(FALSE);

	
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_script_ed.xml");

	std::vector<std::string> funcs;
	std::vector<std::string> tooltips;
	for (S32 i = 0; i < gScriptLibrary.mNextNumber; i++)
	{
		// Make sure this isn't a god only function, or the agent is a god.
		if (!gScriptLibrary.mFunctions[i]->mGodOnly || gAgent.isGodlike())
		{
			funcs.push_back(ll_safe_string(gScriptLibrary.mFunctions[i]->mName));
			tooltips.push_back(ll_safe_string(gScriptLibrary.mFunctions[i]->mDesc));
		}
	}
	LLColor3 color(0.5f, 0.0f, 0.15f);
		
	mEditor->loadKeywords(gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS,"keywords.ini"), funcs, tooltips, color);

	
	LLKeywordToken *token;
	LLKeywords::keyword_iterator_t token_it;
	for (token_it = mEditor->keywordsBegin(); token_it != mEditor->keywordsEnd(); ++token_it)
	{
		token = token_it->second;
		if (token->getColor() == color)
			mFunctions->add(wstring_to_utf8str(token->getToken()));
	}

	for (token_it = mEditor->keywordsBegin(); token_it != mEditor->keywordsEnd(); ++token_it)
	{
		token = token_it->second;
		if (token->getColor() != color)
			mFunctions->add(wstring_to_utf8str(token->getToken()));
	}
}

LLScriptEdCore::~LLScriptEdCore()
{
	deleteBridges();

	// If the search window is up for this editor, close it.
	LLFloaterScriptSearch* script_search = LLFloaterScriptSearch::getInstance();
	if (script_search && script_search->getEditorCore() == this)
	{
		script_search->closeFloater();
		delete script_search;
	}
}

BOOL LLScriptEdCore::postBuild()
{

	mErrorList = getChild<LLScrollListCtrl>("lsl errors");

	mFunctions = getChild<LLComboBox>( "Insert...");

	childSetCommitCallback("Insert...", &LLScriptEdCore::onBtnInsertFunction, this);

	mEditor = getChild<LLViewerTextEditor>("Script Editor");

	childSetCommitCallback("lsl errors", &LLScriptEdCore::onErrorList, this);
	childSetAction("Save_btn", boost::bind(&LLScriptEdCore::doSave,this,FALSE));

	initMenu();
	return TRUE;
}

void LLScriptEdCore::initMenu()
{
	// *TODO: Skinning - make these callbacks data driven
	LLMenuItemCallGL* menuItem;
	
	menuItem = getChild<LLMenuItemCallGL>("Save");
	menuItem->setClickCallback(boost::bind(&LLScriptEdCore::doSave, this, FALSE));
	menuItem->setEnableCallback(boost::bind(&LLScriptEdCore::hasChanged, this));
	
	menuItem = getChild<LLMenuItemCallGL>("Revert All Changes");
	menuItem->setClickCallback(boost::bind(&LLScriptEdCore::onBtnUndoChanges, this));
	menuItem->setEnableCallback(boost::bind(&LLScriptEdCore::hasChanged, this));

	menuItem = getChild<LLMenuItemCallGL>("Undo");
	menuItem->setClickCallback(boost::bind(&LLTextEditor::undo, mEditor));
	menuItem->setEnableCallback(boost::bind(&LLTextEditor::canUndo, mEditor));

	menuItem = getChild<LLMenuItemCallGL>("Redo");
	menuItem->setClickCallback(boost::bind(&LLTextEditor::redo, mEditor));
	menuItem->setEnableCallback(boost::bind(&LLTextEditor::canRedo, mEditor));

	menuItem = getChild<LLMenuItemCallGL>("Cut");
	menuItem->setClickCallback(boost::bind(&LLTextEditor::cut, mEditor));
	menuItem->setEnableCallback(boost::bind(&LLTextEditor::canCut, mEditor));

	menuItem = getChild<LLMenuItemCallGL>("Copy");
	menuItem->setClickCallback(boost::bind(&LLTextEditor::copy, mEditor));
	menuItem->setEnableCallback(boost::bind(&LLTextEditor::canCopy, mEditor));

	menuItem = getChild<LLMenuItemCallGL>("Paste");
	menuItem->setClickCallback(boost::bind(&LLTextEditor::paste, mEditor));
	menuItem->setEnableCallback(boost::bind(&LLTextEditor::canPaste, mEditor));

	menuItem = getChild<LLMenuItemCallGL>("Select All");
	menuItem->setClickCallback(boost::bind(&LLTextEditor::selectAll, mEditor));
	menuItem->setEnableCallback(boost::bind(&LLTextEditor::canSelectAll, mEditor));

	menuItem = getChild<LLMenuItemCallGL>("Search / Replace...");
	menuItem->setClickCallback(boost::bind(&LLFloaterScriptSearch::show, this));

	menuItem = getChild<LLMenuItemCallGL>("Help...");
	menuItem->setClickCallback(boost::bind(&LLScriptEdCore::onBtnHelp, this));

	menuItem = getChild<LLMenuItemCallGL>("LSL Wiki Help...");
	menuItem->setClickCallback(boost::bind(&LLScriptEdCore::onBtnDynamicHelp, this));
}

void LLScriptEdCore::setScriptText(const std::string& text, BOOL is_valid)
{
	if (mEditor)
	{
		mEditor->setText(text);
		mHasScriptData = is_valid;
	}
}

bool LLScriptEdCore::hasChanged()
{
	if (!mEditor) return false;

	return !mEditor->isPristine();
}

void LLScriptEdCore::draw()
{
	BOOL script_changed	= hasChanged();
	childSetEnabled("Save_btn",	script_changed);

	if( mEditor->hasFocus() )
	{
		S32 line = 0;
		S32 column = 0;
		mEditor->getCurrentLineAndColumn( &line, &column, FALSE );  // don't include wordwrap
		LLStringUtil::format_map_t args;
		std::string cursor_pos;
		args["[LINE]"] = llformat ("%d", line);
		args["[COLUMN]"] = llformat ("%d", column);
		cursor_pos = LLTrans::getString("CursorPos", args);
		childSetText("line_col", cursor_pos);
	}
	else
	{
		childSetText("line_col", LLStringUtil::null);
	}

	updateDynamicHelp();

	LLPanel::draw();
}

void LLScriptEdCore::updateDynamicHelp(BOOL immediate)
{
	LLFloater* help_floater = mLiveHelpHandle.get();
	if (!help_floater) return;

	// update back and forward buttons
	LLButton* fwd_button = help_floater->getChild<LLButton>("fwd_btn");
	LLButton* back_button = help_floater->getChild<LLButton>("back_btn");
	LLMediaCtrl* browser = help_floater->getChild<LLMediaCtrl>("lsl_guide_html");
	back_button->setEnabled(browser->canNavigateBack());
	fwd_button->setEnabled(browser->canNavigateForward());

	if (!immediate && !gSavedSettings.getBOOL("ScriptHelpFollowsCursor"))
	{
		return;
	}

	LLTextSegmentPtr segment = NULL;
	std::vector<LLTextSegmentPtr> selected_segments;
	mEditor->getSelectedSegments(selected_segments);

	// try segments in selection range first
	std::vector<LLTextSegmentPtr>::iterator segment_iter;
	for (segment_iter = selected_segments.begin(); segment_iter != selected_segments.end(); ++segment_iter)
	{
		if((*segment_iter)->getToken() && (*segment_iter)->getToken()->getType() == LLKeywordToken::WORD)
		{
			segment = *segment_iter;
			break;
		}
	}

	// then try previous segment in case we just typed it
	if (!segment)
	{
		const LLTextSegmentPtr test_segment = mEditor->getPreviousSegment();
		if(test_segment->getToken() && test_segment->getToken()->getType() == LLKeywordToken::WORD)
		{
			segment = test_segment;
		}
	}

	if (segment)
	{
		if (segment->getToken() != mLastHelpToken)
		{
			mLastHelpToken = segment->getToken();
			mLiveHelpTimer.start();
		}
		if (immediate || (mLiveHelpTimer.getStarted() && mLiveHelpTimer.getElapsedTimeF32() > LIVE_HELP_REFRESH_TIME))
		{
			std::string help_string = mEditor->getText().substr(segment->getStart(), segment->getEnd() - segment->getStart());
			setHelpPage(help_string);
			mLiveHelpTimer.stop();
		}
	}
	else if (immediate)
	{
		setHelpPage(LLStringUtil::null);
	}
}

void LLScriptEdCore::setHelpPage(const std::string& help_string)
{
	LLFloater* help_floater = mLiveHelpHandle.get();
	if (!help_floater) return;
	
	LLMediaCtrl* web_browser = help_floater->getChild<LLMediaCtrl>("lsl_guide_html");
	if (!web_browser) return;

	LLComboBox* history_combo = help_floater->getChild<LLComboBox>("history_combo");
	if (!history_combo) return;

	LLUIString url_string = gSavedSettings.getString("LSLHelpURL");
	url_string.setArg("[LSL_STRING]", help_string);

	addHelpItemToHistory(help_string);

	web_browser->navigateTo(url_string);

}


void LLScriptEdCore::addHelpItemToHistory(const std::string& help_string)
{
	if (help_string.empty()) return;

	LLFloater* help_floater = mLiveHelpHandle.get();
	if (!help_floater) return;

	LLComboBox* history_combo = help_floater->getChild<LLComboBox>("history_combo");
	if (!history_combo) return;

	// separate history items from full item list
	if (mLiveHelpHistorySize == 0)
	{
		history_combo->addSeparator(ADD_TOP);
	}
	// delete all history items over history limit
	while(mLiveHelpHistorySize > MAX_HISTORY_COUNT - 1)
	{
		history_combo->remove(mLiveHelpHistorySize - 1);
		mLiveHelpHistorySize--;
	}

	history_combo->setSimple(help_string);
	S32 index = history_combo->getCurrentIndex();

	// if help string exists in the combo box
	if (index >= 0)
	{
		S32 cur_index = history_combo->getCurrentIndex();
		if (cur_index < mLiveHelpHistorySize)
		{
			// item found in history, bubble up to top
			history_combo->remove(history_combo->getCurrentIndex());
			mLiveHelpHistorySize--;
		}
	}
	history_combo->add(help_string, LLSD(help_string), ADD_TOP);
	history_combo->selectFirstItem();
	mLiveHelpHistorySize++;
}

BOOL LLScriptEdCore::canClose()
{
	if(mForceClose || !hasChanged())
	{
		return TRUE;
	}
	else
	{
		// Bring up view-modal dialog: Save changes? Yes, No, Cancel
		LLNotifications::instance().add("SaveChanges", LLSD(), LLSD(), boost::bind(&LLScriptEdCore::handleSaveChangesDialog, this, _1, _2));
		return FALSE;
	}
}

bool LLScriptEdCore::handleSaveChangesDialog(const LLSD& notification, const LLSD& response )
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	switch( option )
	{
	case 0:  // "Yes"
		// close after saving
			doSave( TRUE );
		break;

	case 1:  // "No"
		mForceClose = TRUE;
		// This will close immediately because mForceClose is true, so we won't
		// infinite loop with these dialogs. JC
		((LLFloater*) getParent())->closeFloater();
		break;

	case 2: // "Cancel"
	default:
		// If we were quitting, we didn't really mean it.
        LLAppViewer::instance()->abortQuit();
		break;
	}
	return false;
}

// static 
bool LLScriptEdCore::onHelpWebDialog(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);

	switch(option)
	{
	case 0:
		LLWeb::loadURL(notification["payload"]["help_url"]);
		break;
	default:
		break;
	}
	return false;
}

void LLScriptEdCore::onBtnHelp()
{
	LLSD payload;
	payload["help_url"] = mHelpURL;
	LLNotifications::instance().add("WebLaunchLSLGuide", LLSD(), payload, onHelpWebDialog);
}

void LLScriptEdCore::onBtnDynamicHelp()
{
	LLFloater* live_help_floater = mLiveHelpHandle.get();
	if (live_help_floater)
	{
		live_help_floater->setFocus(TRUE);
		updateDynamicHelp(TRUE);

		return;
	}

	live_help_floater = new LLFloater(LLSD());
	LLUICtrlFactory::getInstance()->buildFloater(live_help_floater, "floater_lsl_guide.xml", NULL);
	LLFloater* parent = dynamic_cast<LLFloater*>(getParent());
	parent->addDependentFloater(live_help_floater, TRUE);
	live_help_floater->childSetCommitCallback("lock_check", onCheckLock, this);
	live_help_floater->childSetValue("lock_check", gSavedSettings.getBOOL("ScriptHelpFollowsCursor"));
	live_help_floater->childSetCommitCallback("history_combo", onHelpComboCommit, this);
	live_help_floater->childSetAction("back_btn", onClickBack, this);
	live_help_floater->childSetAction("fwd_btn", onClickForward, this);

	LLMediaCtrl* browser = live_help_floater->getChild<LLMediaCtrl>("lsl_guide_html");
	browser->setAlwaysRefresh(TRUE);

	LLComboBox* help_combo = live_help_floater->getChild<LLComboBox>("history_combo");
	LLKeywordToken *token;
	LLKeywords::keyword_iterator_t token_it;
	for (token_it = mEditor->keywordsBegin(); 
		token_it != mEditor->keywordsEnd(); 
		++token_it)
	{
		token = token_it->second;
		help_combo->add(wstring_to_utf8str(token->getToken()));
	}
	help_combo->sortByName();

	// re-initialize help variables
	mLastHelpToken = NULL;
	mLiveHelpHandle = live_help_floater->getHandle();
	mLiveHelpHistorySize = 0;
	updateDynamicHelp(TRUE);
}

//static 
void LLScriptEdCore::onClickBack(void* userdata)
{
	LLScriptEdCore* corep = (LLScriptEdCore*)userdata;
	LLFloater* live_help_floater = corep->mLiveHelpHandle.get();
	if (live_help_floater)
	{
		LLMediaCtrl* browserp = live_help_floater->getChild<LLMediaCtrl>("lsl_guide_html");
		if (browserp)
		{
			browserp->navigateBack();
		}
	}
}

//static 
void LLScriptEdCore::onClickForward(void* userdata)
{
	LLScriptEdCore* corep = (LLScriptEdCore*)userdata;
	LLFloater* live_help_floater = corep->mLiveHelpHandle.get();
	if (live_help_floater)
	{
		LLMediaCtrl* browserp = live_help_floater->getChild<LLMediaCtrl>("lsl_guide_html");
		if (browserp)
		{
			browserp->navigateForward();
		}
	}
}

// static
void LLScriptEdCore::onCheckLock(LLUICtrl* ctrl, void* userdata)
{
	LLScriptEdCore* corep = (LLScriptEdCore*)userdata;

	// clear out token any time we lock the frame, so we will refresh web page immediately when unlocked
	gSavedSettings.setBOOL("ScriptHelpFollowsCursor", ctrl->getValue().asBoolean());

	corep->mLastHelpToken = NULL;
}

// static 
void LLScriptEdCore::onBtnInsertSample(void* userdata)
{
	LLScriptEdCore* self = (LLScriptEdCore*) userdata;

	// Insert sample code
	self->mEditor->selectAll();
	self->mEditor->cut();
	self->mEditor->insertText(self->mSampleText);
}

// static 
void LLScriptEdCore::onHelpComboCommit(LLUICtrl* ctrl, void* userdata)
{
	LLScriptEdCore* corep = (LLScriptEdCore*)userdata;

	LLFloater* live_help_floater = corep->mLiveHelpHandle.get();
	if (live_help_floater)
	{
		std::string help_string = ctrl->getValue().asString();

		corep->addHelpItemToHistory(help_string);

		LLMediaCtrl* web_browser = live_help_floater->getChild<LLMediaCtrl>("lsl_guide_html");
		LLUIString url_string = gSavedSettings.getString("LSLHelpURL");
		url_string.setArg("[LSL_STRING]", help_string);
		web_browser->navigateTo(url_string);
	}
}

// static 
void LLScriptEdCore::onBtnInsertFunction(LLUICtrl *ui, void* userdata)
{
	LLScriptEdCore* self = (LLScriptEdCore*) userdata;

	// Insert sample code
	if(self->mEditor->getEnabled())
	{
		self->mEditor->insertText(self->mFunctions->getSimple());
	}
	self->mEditor->setFocus(TRUE);
	self->setHelpPage(self->mFunctions->getSimple());
}

void LLScriptEdCore::doSave( BOOL close_after_save )
{
	LLViewerStats::getInstance()->incStat( LLViewerStats::ST_LSL_SAVE_COUNT );

	if( mSaveCallback )
	{
		mSaveCallback( mUserdata, close_after_save );
	}
}


void LLScriptEdCore::onBtnUndoChanges()
{
	if( !mEditor->tryToRevertToPristineState() )
	{
		LLNotifications::instance().add("ScriptCannotUndo", LLSD(), LLSD(), boost::bind(&LLScriptEdCore::handleReloadFromServerDialog, this, _1, _2));
	}
}

// static
void LLScriptEdCore::onErrorList(LLUICtrl*, void* user_data)
{
	LLScriptEdCore* self = (LLScriptEdCore*)user_data;
	LLScrollListItem* item = self->mErrorList->getFirstSelected();
	if(item)
	{
		// *FIX: replace with boost grep
		S32 row = 0;
		S32 column = 0;
		const LLScrollListCell* cell = item->getColumn(0);
		std::string line(cell->getValue().asString());
		line.erase(0, 1);
		LLStringUtil::replaceChar(line, ',',' ');
		LLStringUtil::replaceChar(line, ')',' ');
		sscanf(line.c_str(), "%d %d", &row, &column);
		//llinfos << "LLScriptEdCore::onErrorList() - " << row << ", "
		//<< column << llendl;
		self->mEditor->setCursor(row, column);
		self->mEditor->setFocus(TRUE);
	}
}

bool LLScriptEdCore::handleReloadFromServerDialog(const LLSD& notification, const LLSD& response )
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	switch( option )
	{
	case 0: // "Yes"
		if( mLoadCallback )
		{
			setScriptText(getString("loading"), FALSE);
			mLoadCallback(mUserdata);
		}
		break;

	case 1: // "No"
		break;

	default:
		llassert(0);
		break;
	}
	return false;
}

void LLScriptEdCore::selectFirstError()
{
	// Select the first item;
	mErrorList->selectFirstItem();
	onErrorList(mErrorList, this);
}


struct LLEntryAndEdCore
{
	LLScriptEdCore* mCore;
	LLEntryAndEdCore(LLScriptEdCore* core) :
		mCore(core)
		{}
};

void LLScriptEdCore::deleteBridges()
{
	S32 count = mBridges.count();
	LLEntryAndEdCore* eandc;
	for(S32 i = 0; i < count; i++)
	{
		eandc = mBridges.get(i);
		delete eandc;
		mBridges[i] = NULL;
	}
	mBridges.reset();
}

// virtual
BOOL LLScriptEdCore::handleKeyHere(KEY key, MASK mask)
{
	bool just_control = MASK_CONTROL == (mask & MASK_MODIFIERS);

	if(('S' == key) && just_control)
	{
		if(mSaveCallback)
		{
			// don't close after saving
			mSaveCallback(mUserdata, FALSE);
		}

		return TRUE;
	}

	if(('F' == key) && just_control)
	{
		if(mSearchReplaceCallback)
		{
			mSearchReplaceCallback(mUserdata);
		}

		return TRUE;
	}

	return FALSE;
}

/// ---------------------------------------------------------------------------
/// LLPreviewLSL
/// ---------------------------------------------------------------------------

struct LLScriptSaveInfo
{
	LLUUID mItemUUID;
	std::string mDescription;
	LLTransactionID mTransactionID;

	LLScriptSaveInfo(const LLUUID& uuid, const std::string& desc, LLTransactionID tid) :
		mItemUUID(uuid), mDescription(desc),  mTransactionID(tid) {}
};



//static
void* LLPreviewLSL::createScriptEdPanel(void* userdata)
{
	
	LLPreviewLSL *self = (LLPreviewLSL*)userdata;

	self->mScriptEd =  new LLScriptEdCore(
								   HELLO_LSL,
								   HELP_LSL_URL,
								   self->getHandle(),
								   LLPreviewLSL::onLoad,
								   LLPreviewLSL::onSave,
								   LLPreviewLSL::onSearchReplace,
								   self,
								   0);

	return self->mScriptEd;
}


LLPreviewLSL::LLPreviewLSL(const LLSD& key )
  : LLPreview( key ),
	mPendingUploads(0)
{
	mFactoryMap["script panel"] = LLCallbackMap(LLPreviewLSL::createScriptEdPanel, this);
	//Called from floater reg: LLUICtrlFactory::getInstance()->buildFloater(this,"floater_script_preview.xml", FALSE);
}

// virtual
BOOL LLPreviewLSL::postBuild()
{
	const LLInventoryItem* item = getItem();	

	childSetCommitCallback("desc", LLPreview::onText, this);
	childSetText("desc", item->getDescription());
	childSetPrevalidate("desc", &LLLineEditor::prevalidatePrintableNotPipe);

	return LLPreview::postBuild();
}

// virtual
void LLPreviewLSL::callbackLSLCompileSucceeded()
{
	llinfos << "LSL Bytecode saved" << llendl;
	mScriptEd->mErrorList->setCommentText(LLTrans::getString("CompileSuccessful"));
	mScriptEd->mErrorList->setCommentText(LLTrans::getString("SaveComplete"));
	closeIfNeeded();
}

// virtual
void LLPreviewLSL::callbackLSLCompileFailed(const LLSD& compile_errors)
{
	llinfos << "Compile failed!" << llendl;

	for(LLSD::array_const_iterator line = compile_errors.beginArray();
		line < compile_errors.endArray();
		line++)
	{
		LLSD row;
		std::string error_message = line->asString();
		LLStringUtil::stripNonprintable(error_message);
		row["columns"][0]["value"] = error_message;
		row["columns"][0]["font"] = "OCRA";
		mScriptEd->mErrorList->addElement(row);
	}
	mScriptEd->selectFirstError();
	closeIfNeeded();
}

void LLPreviewLSL::loadAsset()
{
	// *HACK: we poke into inventory to see if it's there, and if so,
	// then it might be part of the inventory library. If it's in the
	// library, then you can see the script, but not modify it.
	const LLInventoryItem* item = gInventory.getItem(mItemUUID);
	BOOL is_library = item
		&& !gInventory.isObjectDescendentOf(mItemUUID,
											gInventory.getRootFolderID());
	if(!item)
	{
		// do the more generic search.
		getItem();
	}
	if(item)
	{
		BOOL is_copyable = gAgent.allowOperation(PERM_COPY, 
								item->getPermissions(), GP_OBJECT_MANIPULATE);
		BOOL is_modifiable = gAgent.allowOperation(PERM_MODIFY,
								item->getPermissions(), GP_OBJECT_MANIPULATE);
		if (gAgent.isGodlike() || (is_copyable && (is_modifiable || is_library)))
		{
			LLUUID* new_uuid = new LLUUID(mItemUUID);
			gAssetStorage->getInvItemAsset(LLHost::invalid,
										gAgent.getID(),
										gAgent.getSessionID(),
										item->getPermissions().getOwner(),
										LLUUID::null,
										item->getUUID(),
										item->getAssetUUID(),
										item->getType(),
										&LLPreviewLSL::onLoadComplete,
										(void*)new_uuid,
										TRUE);
			mAssetStatus = PREVIEW_ASSET_LOADING;
		}
		else
		{
			mScriptEd->setScriptText(mScriptEd->getString("can_not_view"), FALSE);
			mScriptEd->mEditor->makePristine();
			mScriptEd->mEditor->setEnabled(FALSE);
			mScriptEd->mFunctions->setEnabled(FALSE);
			mAssetStatus = PREVIEW_ASSET_LOADED;
		}
		childSetVisible("lock", !is_modifiable);
		mScriptEd->childSetEnabled("Insert...", is_modifiable);
	}
	else
	{
		mScriptEd->setScriptText(std::string(HELLO_LSL), TRUE);
		mAssetStatus = PREVIEW_ASSET_LOADED;
	}
}


BOOL LLPreviewLSL::canClose()
{
	return mScriptEd->canClose();
}

void LLPreviewLSL::closeIfNeeded()
{
	// Find our window and close it if requested.
	getWindow()->decBusyCount();
	mPendingUploads--;
	if (mPendingUploads <= 0 && mCloseAfterSave)
	{
		closeFloater();
	}
}

void LLPreviewLSL::onSearchReplace(void* userdata)
{
	LLPreviewLSL* self = (LLPreviewLSL*)userdata;
	LLScriptEdCore* sec = self->mScriptEd; 
	LLFloaterScriptSearch::show(sec);
}

// static
void LLPreviewLSL::onLoad(void* userdata)
{
	LLPreviewLSL* self = (LLPreviewLSL*)userdata;
	self->loadAsset();
}

// static
void LLPreviewLSL::onSave(void* userdata, BOOL close_after_save)
{
	LLPreviewLSL* self = (LLPreviewLSL*)userdata;
	self->mCloseAfterSave = close_after_save;
	self->saveIfNeeded();
}

// Save needs to compile the text in the buffer. If the compile
// succeeds, then save both assets out to the database. If the compile
// fails, go ahead and save the text anyway so that the user doesn't
// get too fucked.
void LLPreviewLSL::saveIfNeeded()
{
	// llinfos << "LLPreviewLSL::saveIfNeeded()" << llendl;
	if(!mScriptEd->hasChanged())
	{
		return;
	}

	mPendingUploads = 0;
	mScriptEd->mErrorList->deleteAllItems();
	mScriptEd->mEditor->makePristine();

	// save off asset into file
	LLTransactionID tid;
	tid.generate();
	LLAssetID asset_id = tid.makeAssetID(gAgent.getSecureSessionID());
	std::string filepath = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,asset_id.asString());
	std::string filename = filepath + ".lsl";

	LLFILE* fp = LLFile::fopen(filename, "wb");
	if(!fp)
	{
		llwarns << "Unable to write to " << filename << llendl;

		LLSD row;
		row["columns"][0]["value"] = "Error writing to local file. Is your hard drive full?";
		row["columns"][0]["font"] = "SANSSERIF_SMALL";
		mScriptEd->mErrorList->addElement(row);
		return;
	}

	std::string utf8text = mScriptEd->mEditor->getText();
	fputs(utf8text.c_str(), fp);
	fclose(fp);
	fp = NULL;

	const LLInventoryItem *inv_item = getItem();
	// save it out to asset server
	std::string url = gAgent.getRegion()->getCapability("UpdateScriptAgent");
	if(inv_item)
	{
		getWindow()->incBusyCount();
		mPendingUploads++;
		if (!url.empty())
		{
			uploadAssetViaCaps(url, filename, mItemUUID);
		}
		else if (gAssetStorage)
		{
			uploadAssetLegacy(filename, mItemUUID, tid);
		}
	}
}

void LLPreviewLSL::uploadAssetViaCaps(const std::string& url,
									  const std::string& filename,
									  const LLUUID& item_id)
{
	llinfos << "Update Agent Inventory via capability" << llendl;
	LLSD body;
	body["item_id"] = item_id;
	body["target"] = "lsl2";
	LLHTTPClient::post(url, body, new LLUpdateAgentInventoryResponder(body, filename, LLAssetType::AT_LSL_TEXT));
}

void LLPreviewLSL::uploadAssetLegacy(const std::string& filename,
									  const LLUUID& item_id,
									  const LLTransactionID& tid)
{
	LLLineEditor* descEditor = getChild<LLLineEditor>("desc");
	LLScriptSaveInfo* info = new LLScriptSaveInfo(item_id,
								descEditor->getText(),
								tid);
	gAssetStorage->storeAssetData(filename,	tid,
								  LLAssetType::AT_LSL_TEXT,
								  &LLPreviewLSL::onSaveComplete,
								  info);

	LLAssetID asset_id = tid.makeAssetID(gAgent.getSecureSessionID());
	std::string filepath = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,asset_id.asString());
	std::string dst_filename = llformat("%s.lso", filepath.c_str());
	std::string err_filename = llformat("%s.out", filepath.c_str());

	const BOOL compile_to_mono = FALSE;
	if(!lscript_compile(filename.c_str(),
						dst_filename.c_str(),
						err_filename.c_str(),
						compile_to_mono,
						asset_id.asString().c_str(),
						gAgent.isGodlike()))
	{
		llinfos << "Compile failed!" << llendl;
		//char command[256];
		//sprintf(command, "type %s\n", err_filename.c_str());
		//system(command);

		// load the error file into the error scrolllist
		LLFILE* fp = LLFile::fopen(err_filename, "r");
		if(fp)
		{
			char buffer[MAX_STRING];		/*Flawfinder: ignore*/
			std::string line;
			while(!feof(fp)) 
			{
				if (fgets(buffer, MAX_STRING, fp) == NULL)
				{
					buffer[0] = '\0';
				}
				if(feof(fp))
				{
					break;
				}
				else
				{
					line.assign(buffer);
					LLStringUtil::stripNonprintable(line);

					LLSD row;
					row["columns"][0]["value"] = line;
					row["columns"][0]["font"] = "OCRA";
					mScriptEd->mErrorList->addElement(row);
				}
			}
			fclose(fp);
			mScriptEd->selectFirstError();
		}
	}
	else
	{
		llinfos << "Compile worked!" << llendl;
		if(gAssetStorage)
		{
			getWindow()->incBusyCount();
			mPendingUploads++;
			LLUUID* this_uuid = new LLUUID(mItemUUID);
			gAssetStorage->storeAssetData(dst_filename,
										  tid,
										  LLAssetType::AT_LSL_BYTECODE,
										  &LLPreviewLSL::onSaveBytecodeComplete,
										  (void**)this_uuid);
		}
	}

	// get rid of any temp files left lying around
	LLFile::remove(filename);
	LLFile::remove(err_filename);
	LLFile::remove(dst_filename);
}


// static
void LLPreviewLSL::onSaveComplete(const LLUUID& asset_uuid, void* user_data, S32 status, LLExtStat ext_status) // StoreAssetData callback (fixed)
{
	LLScriptSaveInfo* info = reinterpret_cast<LLScriptSaveInfo*>(user_data);
	if(0 == status)
	{
		if (info)
		{
			const LLViewerInventoryItem* item;
			item = (const LLViewerInventoryItem*)gInventory.getItem(info->mItemUUID);
			if(item)
			{
				LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
				new_item->setAssetUUID(asset_uuid);
				new_item->setTransactionID(info->mTransactionID);
				new_item->updateServer(FALSE);
				gInventory.updateItem(new_item);
				gInventory.notifyObservers();
			}
			else
			{
				llwarns << "Inventory item for script " << info->mItemUUID
					<< " is no longer in agent inventory." << llendl
			}

			// Find our window and close it if requested.
			LLPreviewLSL* self = LLFloaterReg::findTypedInstance<LLPreviewLSL>("preview_script", info->mItemUUID);
			if (self)
			{
				getWindow()->decBusyCount();
				self->mPendingUploads--;
				if (self->mPendingUploads <= 0
					&& self->mCloseAfterSave)
				{
					self->closeFloater();
				}
			}
		}
	}
	else
	{
		llwarns << "Problem saving script: " << status << llendl;
		LLSD args;
		args["REASON"] = std::string(LLAssetStorage::getErrorString(status));
		LLNotifications::instance().add("SaveScriptFailReason", args);
	}
	delete info;
}

// static
void LLPreviewLSL::onSaveBytecodeComplete(const LLUUID& asset_uuid, void* user_data, S32 status, LLExtStat ext_status) // StoreAssetData callback (fixed)
{
	LLUUID* instance_uuid = (LLUUID*)user_data;
	LLPreviewLSL* self = NULL;
	if(instance_uuid)
	{
		self = LLFloaterReg::findTypedInstance<LLPreviewLSL>("preview_script", *instance_uuid);
	}
	if (0 == status)
	{
		if (self)
		{
			LLSD row;
			row["columns"][0]["value"] = "Compile successful!";
			row["columns"][0]["font"] = "SANSSERIF_SMALL";
			self->mScriptEd->mErrorList->addElement(row);

			// Find our window and close it if requested.
			self->getWindow()->decBusyCount();
			self->mPendingUploads--;
			if (self->mPendingUploads <= 0
				&& self->mCloseAfterSave)
			{
				self->closeFloater();
			}
		}
	}
	else
	{
		llwarns << "Problem saving LSL Bytecode (Preview)" << llendl;
		LLSD args;
		args["REASON"] = std::string(LLAssetStorage::getErrorString(status));
		LLNotifications::instance().add("SaveBytecodeFailReason", args);
	}
	delete instance_uuid;
}

// static
void LLPreviewLSL::onLoadComplete( LLVFS *vfs, const LLUUID& asset_uuid, LLAssetType::EType type,
								   void* user_data, S32 status, LLExtStat ext_status)
{
	lldebugs << "LLPreviewLSL::onLoadComplete: got uuid " << asset_uuid
		 << llendl;
	LLUUID* item_uuid = (LLUUID*)user_data;
	LLPreviewLSL* preview = LLFloaterReg::findTypedInstance<LLPreviewLSL>("preview_script", *item_uuid);
	if( preview )
	{
		if(0 == status)
		{
			LLVFile file(vfs, asset_uuid, type);
			S32 file_length = file.getSize();

			std::vector<char> buffer(file_length+1);
			file.read((U8*)&buffer[0], file_length);

			// put a EOS at the end
			buffer[file_length] = 0;
			preview->mScriptEd->setScriptText(LLStringExplicit(&buffer[0]), TRUE);
			preview->mScriptEd->mEditor->makePristine();
			LLInventoryItem* item = gInventory.getItem(*item_uuid);
			BOOL is_modifiable = FALSE;
			if(item
			   && gAgent.allowOperation(PERM_MODIFY, item->getPermissions(),
				   					GP_OBJECT_MANIPULATE))
			{
				is_modifiable = TRUE;		
			}
			preview->mScriptEd->mEditor->setEnabled(is_modifiable);
			preview->mAssetStatus = PREVIEW_ASSET_LOADED;
		}
		else
		{
			LLViewerStats::getInstance()->incStat( LLViewerStats::ST_DOWNLOAD_FAILED );

			if( LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE == status ||
				LL_ERR_FILE_EMPTY == status)
			{
				LLNotifications::instance().add("ScriptMissing");
			}
			else if (LL_ERR_INSUFFICIENT_PERMISSIONS == status)
			{
				LLNotifications::instance().add("ScriptNoPermissions");
			}
			else
			{
				LLNotifications::instance().add("UnableToLoadScript");
			}

			preview->mAssetStatus = PREVIEW_ASSET_ERROR;
			llwarns << "Problem loading script: " << status << llendl;
		}
	}
	delete item_uuid;
}

/// ---------------------------------------------------------------------------
/// LLLiveLSLEditor
/// ---------------------------------------------------------------------------


//static 
void* LLLiveLSLEditor::createScriptEdPanel(void* userdata)
{
	
	LLLiveLSLEditor *self = (LLLiveLSLEditor*)userdata;

	self->mScriptEd =  new LLScriptEdCore(
								   HELLO_LSL,
								   HELP_LSL_URL,
								   self->getHandle(),
								   &LLLiveLSLEditor::onLoad,
								   &LLLiveLSLEditor::onSave,
								   &LLLiveLSLEditor::onSearchReplace,
								   self,
								   0);

	return self->mScriptEd;
}


LLLiveLSLEditor::LLLiveLSLEditor(const LLSD& key) :
	LLPreview(key),
	mScriptEd(NULL),
	mAskedForRunningInfo(FALSE),
	mHaveRunningInfo(FALSE),
	mCloseAfterSave(FALSE),
	mPendingUploads(0),
	mIsModifiable(FALSE),
	mIsNew(false)
{
	mFactoryMap["script ed panel"] = LLCallbackMap(LLLiveLSLEditor::createScriptEdPanel, this);
	//Called from floater reg: LLUICtrlFactory::getInstance()->buildFloater(this,"floater_live_lsleditor.xml", FALSE);
}

BOOL LLLiveLSLEditor::postBuild()
{
	childSetCommitCallback("running", LLLiveLSLEditor::onRunningCheckboxClicked, this);
	childSetEnabled("running", FALSE);

	childSetAction("Reset",&LLLiveLSLEditor::onReset,this);
	childSetEnabled("Reset", TRUE);

	mMonoCheckbox =	getChild<LLCheckBoxCtrl>("mono");
	childSetCommitCallback("mono", &LLLiveLSLEditor::onMonoCheckboxClicked, this);
	childSetEnabled("mono", FALSE);

	mScriptEd->mEditor->makePristine();
	mScriptEd->mEditor->setFocus(TRUE);

	return LLPreview::postBuild();
}

LLLiveLSLEditor::~LLLiveLSLEditor()
{
}

// virtual
void LLLiveLSLEditor::callbackLSLCompileSucceeded(const LLUUID& task_id,
												  const LLUUID& item_id,
												  bool is_script_running)
{
	lldebugs << "LSL Bytecode saved" << llendl;
	mScriptEd->mErrorList->setCommentText(LLTrans::getString("CompileSuccessful"));
	mScriptEd->mErrorList->setCommentText(LLTrans::getString("SaveComplete"));
	closeIfNeeded();
}

// virtual
void LLLiveLSLEditor::callbackLSLCompileFailed(const LLSD& compile_errors)
{
	lldebugs << "Compile failed!" << llendl;
	for(LLSD::array_const_iterator line = compile_errors.beginArray();
		line < compile_errors.endArray();
		line++)
	{
		LLSD row;
		std::string error_message = line->asString();
		LLStringUtil::stripNonprintable(error_message);
		row["columns"][0]["value"] = error_message;
		// *TODO: change to "MONOSPACE" and change llfontgl.cpp?
		row["columns"][0]["font"] = "OCRA";
		mScriptEd->mErrorList->addElement(row);
	}
	mScriptEd->selectFirstError();
	closeIfNeeded();
}

void LLLiveLSLEditor::loadAsset()
{
	//llinfos << "LLLiveLSLEditor::loadAsset()" << llendl;
	if(!mIsNew)
	{
		LLViewerObject* object = gObjectList.findObject(mObjectUUID);
		if(object)
		{
			LLViewerInventoryItem* item = dynamic_cast<LLViewerInventoryItem*>(object->getInventoryObject(mItemUUID));
			if(item 
				&& (gAgent.allowOperation(PERM_COPY, item->getPermissions(), GP_OBJECT_MANIPULATE)
				   || gAgent.isGodlike()))
			{
				mItem = new LLViewerInventoryItem(item);
				//llinfos << "asset id " << mItem->getAssetUUID() << llendl;
			}

			if(!gAgent.isGodlike()
			   && (item
				   && (!gAgent.allowOperation(PERM_COPY, item->getPermissions(), GP_OBJECT_MANIPULATE)
					   || !gAgent.allowOperation(PERM_MODIFY, item->getPermissions(), GP_OBJECT_MANIPULATE))))
			{
				mItem = new LLViewerInventoryItem(item);
				mScriptEd->setScriptText(getString("not_allowed"), FALSE);
				mScriptEd->mEditor->makePristine();
				mScriptEd->mEditor->setEnabled(FALSE);
				mScriptEd->enableSave(FALSE);
				mAssetStatus = PREVIEW_ASSET_LOADED;
			}
			else if(item && mItem.notNull())
			{
				// request the text from the object
				LLUUID* user_data = new LLUUID(mItemUUID); //  ^ mObjectUUID
				gAssetStorage->getInvItemAsset(object->getRegion()->getHost(),
											   gAgent.getID(),
											   gAgent.getSessionID(),
											   item->getPermissions().getOwner(),
											   object->getID(),
											   item->getUUID(),
											   item->getAssetUUID(),
											   item->getType(),
											   &LLLiveLSLEditor::onLoadComplete,
											   (void*)user_data,
											   TRUE);
				LLMessageSystem* msg = gMessageSystem;
				msg->newMessageFast(_PREHASH_GetScriptRunning);
				msg->nextBlockFast(_PREHASH_Script);
				msg->addUUIDFast(_PREHASH_ObjectID, mObjectUUID);
				msg->addUUIDFast(_PREHASH_ItemID, mItemUUID);
				msg->sendReliable(object->getRegion()->getHost());
				mAskedForRunningInfo = TRUE;
				mAssetStatus = PREVIEW_ASSET_LOADING;
			}
			else
			{
				mScriptEd->setScriptText(LLStringUtil::null, FALSE);
				mScriptEd->mEditor->makePristine();
				mAssetStatus = PREVIEW_ASSET_LOADED;
			}

			mIsModifiable = item && gAgent.allowOperation(PERM_MODIFY, 
										item->getPermissions(),
				   						GP_OBJECT_MANIPULATE);
			if(!mIsModifiable)
			{
				mScriptEd->mEditor->setEnabled(FALSE);
			}
			
			// This is commented out, because we don't completely
			// handle script exports yet.
			/*
			// request the exports from the object
			gMessageSystem->newMessage("GetScriptExports");
			gMessageSystem->nextBlock("ScriptBlock");
			gMessageSystem->addUUID("AgentID", gAgent.getID());
			U32 local_id = object->getLocalID();
			gMessageSystem->addData("LocalID", &local_id);
			gMessageSystem->addUUID("ItemID", mItemUUID);
			LLHost host(object->getRegion()->getIP(),
						object->getRegion()->getPort());
			gMessageSystem->sendReliable(host);
			*/
		}
	}
	else
	{
		mScriptEd->setScriptText(std::string(HELLO_LSL), TRUE);
		mScriptEd->enableSave(FALSE);
		LLPermissions perm;
		perm.init(gAgent.getID(), gAgent.getID(), LLUUID::null, gAgent.getGroupID());
		perm.initMasks(PERM_ALL, PERM_ALL, PERM_NONE, PERM_NONE, PERM_MOVE | PERM_TRANSFER);
		mItem = new LLViewerInventoryItem(mItemUUID,
										  mObjectUUID,
										  perm,
										  LLUUID::null,
										  LLAssetType::AT_LSL_TEXT,
										  LLInventoryType::IT_LSL,
										  DEFAULT_SCRIPT_NAME,
										  DEFAULT_SCRIPT_DESC,
										  LLSaleInfo::DEFAULT,
										  LLInventoryItem::II_FLAGS_NONE,
										  time_corrected());
		mAssetStatus = PREVIEW_ASSET_LOADED;
	}
}

// static
void LLLiveLSLEditor::onLoadComplete(LLVFS *vfs, const LLUUID& asset_id,
									 LLAssetType::EType type,
									 void* user_data, S32 status, LLExtStat ext_status)
{
	lldebugs << "LLLiveLSLEditor::onLoadComplete: got uuid " << asset_id
		 << llendl;
	LLUUID* xored_id = (LLUUID*)user_data;
	
	LLLiveLSLEditor* instance = LLFloaterReg::findTypedInstance<LLLiveLSLEditor>("preview_scriptedit", *xored_id);
	
	if(instance )
	{
		if( LL_ERR_NOERR == status )
		{
			instance->loadScriptText(vfs, asset_id, type);
			instance->mAssetStatus = PREVIEW_ASSET_LOADED;
		}
		else
		{
			LLViewerStats::getInstance()->incStat( LLViewerStats::ST_DOWNLOAD_FAILED );

			if( LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE == status ||
				LL_ERR_FILE_EMPTY == status)
			{
				LLNotifications::instance().add("ScriptMissing");
			}
			else if (LL_ERR_INSUFFICIENT_PERMISSIONS == status)
			{
				LLNotifications::instance().add("ScriptNoPermissions");
			}
			else
			{
				LLNotifications::instance().add("UnableToLoadScript");
			}
			instance->mAssetStatus = PREVIEW_ASSET_ERROR;
		}
	}

	delete xored_id;
}

// unused
// void LLLiveLSLEditor::loadScriptText(const std::string& filename)
// {
// 	if(!filename)
// 	{
// 		llerrs << "Filename is Empty!" << llendl;
// 		return;
// 	}
// 	LLFILE* file = LLFile::fopen(filename, "rb");		/*Flawfinder: ignore*/
// 	if(file)
// 	{
// 		// read in the whole file
// 		fseek(file, 0L, SEEK_END);
// 		long file_length = ftell(file);
// 		fseek(file, 0L, SEEK_SET);
// 		char* buffer = new char[file_length+1];
// 		size_t nread = fread(buffer, 1, file_length, file);
// 		if (nread < (size_t) file_length)
// 		{
// 			llwarns << "Short read" << llendl;
// 		}
// 		buffer[nread] = '\0';
// 		fclose(file);
// 		mScriptEd->mEditor->setText(LLStringExplicit(buffer));
// 		mScriptEd->mEditor->makePristine();
// 		delete[] buffer;
// 	}
// 	else
// 	{
// 		llwarns << "Error opening " << filename << llendl;
// 	}
// }

void LLLiveLSLEditor::loadScriptText(LLVFS *vfs, const LLUUID &uuid, LLAssetType::EType type)
{
	LLVFile file(vfs, uuid, type);
	S32 file_length = file.getSize();
	std::vector<char> buffer(file_length + 1);
	file.read((U8*)&buffer[0], file_length);

	if (file.getLastBytesRead() != file_length ||
		file_length <= 0)
	{
		llwarns << "Error reading " << uuid << ":" << type << llendl;
	}

	buffer[file_length] = '\0';

	mScriptEd->setScriptText(LLStringExplicit(&buffer[0]), TRUE);
	mScriptEd->mEditor->makePristine();
}


void LLLiveLSLEditor::onRunningCheckboxClicked( LLUICtrl*, void* userdata )
{
	LLLiveLSLEditor* self = (LLLiveLSLEditor*) userdata;
	LLViewerObject* object = gObjectList.findObject( self->mObjectUUID );
	LLCheckBoxCtrl* runningCheckbox = self->getChild<LLCheckBoxCtrl>("running");
	BOOL running =  runningCheckbox->get();
	//self->mRunningCheckbox->get();
	if( object )
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_SetScriptRunning);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_Script);
		msg->addUUIDFast(_PREHASH_ObjectID, self->mObjectUUID);
		msg->addUUIDFast(_PREHASH_ItemID, self->mItemUUID);
		msg->addBOOLFast(_PREHASH_Running, running);
		msg->sendReliable(object->getRegion()->getHost());
	}
	else
	{
		runningCheckbox->set(!running);
		LLNotifications::instance().add("CouldNotStartStopScript");
	}
}

void LLLiveLSLEditor::onReset(void *userdata)
{
	LLLiveLSLEditor* self = (LLLiveLSLEditor*) userdata;

	LLViewerObject* object = gObjectList.findObject( self->mObjectUUID );
	if(object)
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_ScriptReset);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_Script);
		msg->addUUIDFast(_PREHASH_ObjectID, self->mObjectUUID);
		msg->addUUIDFast(_PREHASH_ItemID, self->mItemUUID);
		msg->sendReliable(object->getRegion()->getHost());
	}
	else
	{
		LLNotifications::instance().add("CouldNotStartStopScript"); 
	}
}

void LLLiveLSLEditor::draw()
{
	LLViewerObject* object = gObjectList.findObject(mObjectUUID);
	LLCheckBoxCtrl* runningCheckbox = getChild<LLCheckBoxCtrl>( "running");
	if(object && mAskedForRunningInfo && mHaveRunningInfo)
	{
		if(object->permAnyOwner())
		{
			runningCheckbox->setLabel(getString("script_running"));
			runningCheckbox->setEnabled(TRUE);

			if(object->permAnyOwner())
			{
				runningCheckbox->setLabel(getString("script_running"));
				runningCheckbox->setEnabled(TRUE);
			}
			else
			{
				runningCheckbox->setLabel(getString("public_objects_can_not_run"));
				runningCheckbox->setEnabled(FALSE);
				// *FIX: Set it to false so that the ui is correct for
				// a box that is released to public. It could be
				// incorrect after a release/claim cycle, but will be
				// correct after clicking on it.
				runningCheckbox->set(FALSE);
				mMonoCheckbox->set(FALSE);
			}
		}
		else
		{
			runningCheckbox->setLabel(getString("public_objects_can_not_run"));
			runningCheckbox->setEnabled(FALSE);

			// *FIX: Set it to false so that the ui is correct for
			// a box that is released to public. It could be
			// incorrect after a release/claim cycle, but will be
			// correct after clicking on it.
			runningCheckbox->set(FALSE);
			mMonoCheckbox->setEnabled(FALSE);
			// object may have fallen out of range.
			mHaveRunningInfo = FALSE;
		}
	}
	else if(!object)
	{
		// HACK: Display this information in the title bar.
		// Really ought to put in main window.
		setTitle(LLTrans::getString("ObjectOutOfRange"));
		runningCheckbox->setEnabled(FALSE);
		// object may have fallen out of range.
		mHaveRunningInfo = FALSE;
	}

	LLPreview::draw();
}


void LLLiveLSLEditor::onSearchReplace(void* userdata)
{
	LLLiveLSLEditor* self = (LLLiveLSLEditor*)userdata;

	LLScriptEdCore* sec = self->mScriptEd; 
	LLFloaterScriptSearch::show(sec);
}

struct LLLiveLSLSaveData
{
	LLLiveLSLSaveData(const LLUUID& id, const LLViewerInventoryItem* item, BOOL active);
	LLUUID mSaveObjectID;
	LLPointer<LLViewerInventoryItem> mItem;
	BOOL mActive;
};

LLLiveLSLSaveData::LLLiveLSLSaveData(const LLUUID& id,
									 const LLViewerInventoryItem* item,
									 BOOL active) :
	mSaveObjectID(id),
	mActive(active)
{
	llassert(item);
	mItem = new LLViewerInventoryItem(item);
}

void LLLiveLSLEditor::saveIfNeeded()
{
	llinfos << "LLLiveLSLEditor::saveIfNeeded()" << llendl;
	LLViewerObject* object = gObjectList.findObject(mObjectUUID);
	if(!object)
	{
		LLNotifications::instance().add("SaveScriptFailObjectNotFound");
		return;
	}

	if(mItem.isNull() || !mItem->isComplete())
	{
		// $NOTE: While the error message may not be exactly correct,
		// it's pretty close.
		LLNotifications::instance().add("SaveScriptFailObjectNotFound");
		return;
	}

	// get the latest info about it. We used to be losing the script
	// name on save, because the viewer object version of the item,
	// and the editor version would get out of synch. Here's a good
	// place to synch them back up.
	LLInventoryItem* inv_item = dynamic_cast<LLInventoryItem*>(object->getInventoryObject(mItemUUID));
	if(inv_item)
	{
		mItem->copyItem(inv_item);
	}

	// Don't need to save if we're pristine
	if(!mScriptEd->hasChanged())
	{
		return;
	}

	mPendingUploads = 0;

	// save the script
	mScriptEd->enableSave(FALSE);
	mScriptEd->mEditor->makePristine();
	mScriptEd->mErrorList->deleteAllItems();

	// set up the save on the local machine.
	mScriptEd->mEditor->makePristine();
	LLTransactionID tid;
	tid.generate();
	LLAssetID asset_id = tid.makeAssetID(gAgent.getSecureSessionID());
	std::string filepath = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,asset_id.asString());
	std::string filename = llformat("%s.lsl", filepath.c_str());

	mItem->setAssetUUID(asset_id);
	mItem->setTransactionID(tid);

	// write out the data, and store it in the asset database
	LLFILE* fp = LLFile::fopen(filename, "wb");
	if(!fp)
	{
		llwarns << "Unable to write to " << filename << llendl;

		LLSD row;
		row["columns"][0]["value"] = "Error writing to local file. Is your hard drive full?";
		row["columns"][0]["font"] = "SANSSERIF_SMALL";
		mScriptEd->mErrorList->addElement(row);
		return;
	}
	std::string utf8text = mScriptEd->mEditor->getText();

	// Special case for a completely empty script - stuff in one space so it can store properly.  See SL-46889
	if ( utf8text.size() == 0 )
	{
		utf8text = " ";
	}

	fputs(utf8text.c_str(), fp);
	fclose(fp);
	fp = NULL;
	
	// save it out to asset server
	std::string url = object->getRegion()->getCapability("UpdateScriptTask");
	getWindow()->incBusyCount();
	mPendingUploads++;
	BOOL is_running = getChild<LLCheckBoxCtrl>( "running")->get();
	if (!url.empty())
	{
		uploadAssetViaCaps(url, filename, mObjectUUID, mItemUUID, is_running);
	}
	else if (gAssetStorage)
	{
		uploadAssetLegacy(filename, object, tid, is_running);
	}
}

void LLLiveLSLEditor::uploadAssetViaCaps(const std::string& url,
										 const std::string& filename,
										 const LLUUID& task_id,
										 const LLUUID& item_id,
										 BOOL is_running)
{
	llinfos << "Update Task Inventory via capability" << llendl;
	LLSD body;
	body["task_id"] = task_id;
	body["item_id"] = item_id;
	body["is_script_running"] = is_running;
	body["target"] = monoChecked() ? "mono" : "lsl2";
	LLHTTPClient::post(url, body,
		new LLUpdateTaskInventoryResponder(body, filename, LLAssetType::AT_LSL_TEXT));
}

void LLLiveLSLEditor::uploadAssetLegacy(const std::string& filename,
										LLViewerObject* object,
										const LLTransactionID& tid,
										BOOL is_running)
{
	LLLiveLSLSaveData* data = new LLLiveLSLSaveData(mObjectUUID,
													mItem,
													is_running);
	gAssetStorage->storeAssetData(filename, tid,
								  LLAssetType::AT_LSL_TEXT,
								  &onSaveTextComplete,
								  (void*)data,
								  FALSE);

	LLAssetID asset_id = tid.makeAssetID(gAgent.getSecureSessionID());
	std::string filepath = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,asset_id.asString());
	std::string dst_filename = llformat("%s.lso", filepath.c_str());
	std::string err_filename = llformat("%s.out", filepath.c_str());

	LLFILE *fp;
	const BOOL compile_to_mono = FALSE;
	if(!lscript_compile(filename.c_str(),
						dst_filename.c_str(),
						err_filename.c_str(),
						compile_to_mono,
						asset_id.asString().c_str(),
						gAgent.isGodlike()))
	{
		// load the error file into the error scrolllist
		llinfos << "Compile failed!" << llendl;
		if(NULL != (fp = LLFile::fopen(err_filename, "r")))
		{
			char buffer[MAX_STRING];		/*Flawfinder: ignore*/
			std::string line;
			while(!feof(fp)) 
			{
				
				if (fgets(buffer, MAX_STRING, fp) == NULL)
				{
					buffer[0] = '\0';
				}
				if(feof(fp))
				{
					break;
				}
				else
				{
					line.assign(buffer);
					LLStringUtil::stripNonprintable(line);
				
					LLSD row;
					row["columns"][0]["value"] = line;
					row["columns"][0]["font"] = "OCRA";
					mScriptEd->mErrorList->addElement(row);
				}
			}
			fclose(fp);
			mScriptEd->selectFirstError();
			// don't set the asset id, because we want to save the
			// script, even though the compile failed.
			//mItem->setAssetUUID(LLUUID::null);
			object->saveScript(mItem, FALSE, false);
			dialog_refresh_all();
		}
	}
	else
	{
		llinfos << "Compile worked!" << llendl;
		mScriptEd->mErrorList->setCommentText(LLTrans::getString("CompileSuccessfulSaving"));
		if(gAssetStorage)
		{
			llinfos << "LLLiveLSLEditor::saveAsset "
					<< mItem->getAssetUUID() << llendl;
			getWindow()->incBusyCount();
			mPendingUploads++;
			LLLiveLSLSaveData* data = NULL;
			data = new LLLiveLSLSaveData(mObjectUUID,
										 mItem,
										 is_running);
			gAssetStorage->storeAssetData(dst_filename,
										  tid,
										  LLAssetType::AT_LSL_BYTECODE,
										  &LLLiveLSLEditor::onSaveBytecodeComplete,
										  (void*)data);
			dialog_refresh_all();
		}
	}

	// get rid of any temp files left lying around
	LLFile::remove(filename);
	LLFile::remove(err_filename);
	LLFile::remove(dst_filename);

	// If we successfully saved it, then we should be able to check/uncheck the running box!
	LLCheckBoxCtrl* runningCheckbox = getChild<LLCheckBoxCtrl>( "running");
	runningCheckbox->setLabel(getString("script_running"));
	runningCheckbox->setEnabled(TRUE);
}

void LLLiveLSLEditor::onSaveTextComplete(const LLUUID& asset_uuid, void* user_data, S32 status, LLExtStat ext_status) // StoreAssetData callback (fixed)
{
	LLLiveLSLSaveData* data = (LLLiveLSLSaveData*)user_data;

	if (status)
	{
		llwarns << "Unable to save text for a script." << llendl;
		LLSD args;
		args["REASON"] = std::string(LLAssetStorage::getErrorString(status));
		LLNotifications::instance().add("CompileQueueSaveText", args);
	}
	else
	{
		LLLiveLSLEditor* self = LLFloaterReg::findTypedInstance<LLLiveLSLEditor>("preview_scriptedit", data->mItem->getUUID()); //  ^ data->mSaveObjectID
		if (self)
		{
			self->getWindow()->decBusyCount();
			self->mPendingUploads--;
			if (self->mPendingUploads <= 0
				&& self->mCloseAfterSave)
			{
				self->closeFloater();
			}
		}
	}
	delete data;
	data = NULL;
}


void LLLiveLSLEditor::onSaveBytecodeComplete(const LLUUID& asset_uuid, void* user_data, S32 status, LLExtStat ext_status) // StoreAssetData callback (fixed)
{
	LLLiveLSLSaveData* data = (LLLiveLSLSaveData*)user_data;
	if(!data)
		return;
	if(0 ==status)
	{
		llinfos << "LSL Bytecode saved" << llendl;
		LLLiveLSLEditor* self = LLFloaterReg::findTypedInstance<LLLiveLSLEditor>("preview_scriptedit", data->mItem->getUUID()); //  ^ data->mSaveObjectID
		if (self)
		{
			// Tell the user that the compile worked.
			self->mScriptEd->mErrorList->setCommentText(LLTrans::getString("SaveComplete"));
			// close the window if this completes both uploads
			self->getWindow()->decBusyCount();
			self->mPendingUploads--;
			if (self->mPendingUploads <= 0
				&& self->mCloseAfterSave)
			{
				self->closeFloater();
			}
		}
		LLViewerObject* object = gObjectList.findObject(data->mSaveObjectID);
		if(object)
		{
			object->saveScript(data->mItem, data->mActive, false);
			dialog_refresh_all();
			//LLToolDragAndDrop::dropScript(object, ids->first,
			//						  LLAssetType::AT_LSL_TEXT, FALSE);
		}
	}
	else
	{
		llinfos << "Problem saving LSL Bytecode (Live Editor)" << llendl;
		llwarns << "Unable to save a compiled script." << llendl;

		LLSD args;
		args["REASON"] = std::string(LLAssetStorage::getErrorString(status));
		LLNotifications::instance().add("CompileQueueSaveBytecode", args);
	}

	std::string filepath = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,asset_uuid.asString());
	std::string dst_filename = llformat("%s.lso", filepath.c_str());
	LLFile::remove(dst_filename);
	delete data;
}

BOOL LLLiveLSLEditor::canClose()
{
	return (mScriptEd->canClose());
}

void LLLiveLSLEditor::closeIfNeeded()
{
	getWindow()->decBusyCount();
	mPendingUploads--;
	if (mPendingUploads <= 0 && mCloseAfterSave)
	{
		closeFloater();
	}
}

// static
void LLLiveLSLEditor::onLoad(void* userdata)
{
	LLLiveLSLEditor* self = (LLLiveLSLEditor*)userdata;
	self->loadAsset();
}

// static
void LLLiveLSLEditor::onSave(void* userdata, BOOL close_after_save)
{
	LLLiveLSLEditor* self = (LLLiveLSLEditor*)userdata;
	self->mCloseAfterSave = close_after_save;
	self->saveIfNeeded();
}

// static
void LLLiveLSLEditor::processScriptRunningReply(LLMessageSystem* msg, void**)
{
	LLUUID item_id;
	LLUUID object_id;
	msg->getUUIDFast(_PREHASH_Script, _PREHASH_ObjectID, object_id);
	msg->getUUIDFast(_PREHASH_Script, _PREHASH_ItemID, item_id);

	LLLiveLSLEditor* instance = LLFloaterReg::findTypedInstance<LLLiveLSLEditor>("preview_scriptedit", item_id); //  ^ object_id
	if(instance)
	{
		instance->mHaveRunningInfo = TRUE;
		BOOL running;
		msg->getBOOLFast(_PREHASH_Script, _PREHASH_Running, running);
		LLCheckBoxCtrl* runningCheckbox = instance->getChild<LLCheckBoxCtrl>("running");
		runningCheckbox->set(running);
		BOOL mono;
		msg->getBOOLFast(_PREHASH_Script, "Mono", mono);
		LLCheckBoxCtrl* monoCheckbox = instance->getChild<LLCheckBoxCtrl>("mono");
		monoCheckbox->setEnabled(instance->getIsModifiable() && have_script_upload_cap(object_id));
		monoCheckbox->set(mono);
	}
}

void LLLiveLSLEditor::onMonoCheckboxClicked(LLUICtrl*, void* userdata)
{
	LLLiveLSLEditor* self = static_cast<LLLiveLSLEditor*>(userdata);
	self->mMonoCheckbox->setEnabled(have_script_upload_cap(self->mObjectUUID));
	self->mScriptEd->enableSave(self->getIsModifiable());
}

BOOL LLLiveLSLEditor::monoChecked() const
{
	if(NULL != mMonoCheckbox)
	{
		return mMonoCheckbox->getValue()? TRUE : FALSE;
	}
	return FALSE;
}
