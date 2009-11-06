/** 
 * @file llpanelprimmediacontrols.h
 * @brief Pop-up media controls panel
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#ifndef LL_PANELPRIMMEDIACONTROLS_H
#define LL_PANELPRIMMEDIACONTROLS_H

#include "llpanel.h"
#include "llviewermedia.h"

class LLCoordWindow;
class LLViewerMediaImpl;

class LLPanelPrimMediaControls : public LLPanel
{
public:
	LLPanelPrimMediaControls();
	virtual ~LLPanelPrimMediaControls();
	/*virtual*/ BOOL postBuild();
	virtual void draw();
	virtual BOOL handleScrollWheel(S32 x, S32 y, S32 clicks);

	virtual BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL handleKeyHere(KEY key, MASK mask);
	
	void updateShape();
	bool isMouseOver();
	void nextZoomLevel();
	void resetZoomLevel();
	void close();

	LLHandle<LLPanelPrimMediaControls>	getHandle() const { return mPanelHandle; }
	void setMediaFace(LLPointer<LLViewerObject> objectp, S32 face, viewer_media_t media_impl, LLVector3 pick_normal = LLVector3::zero);


	enum EZoomLevel
	{
		ZOOM_NONE = 0,
		ZOOM_FAR,
		ZOOM_MEDIUM,
		ZOOM_NEAR
	};
	static const EZoomLevel kZoomLevels[];
	static const int kNumZoomLevels;
	
	enum EScrollDir
	{
		SCROLL_UP = 0,
		SCROLL_DOWN,
		SCROLL_LEFT,
		SCROLL_RIGHT,
		SCROLL_NONE
	};

private:
	void onClickClose();
	void onClickBack();
	void onClickForward();
	void onClickHome();
	void onClickOpen();
	void onClickReload();
	void onClickPlay();
	void onClickPause();
	void onClickStop();
	void onClickZoom();
	void onCommitURL();
	
	void updateZoom();
	void setCurrentURL();
	void onCommitSlider();
	
	void onCommitVolumeUp();
	void onCommitVolumeDown();
	void onToggleMute();
	
	static void onScrollUp(void* user_data);
	static void onScrollUpHeld(void* user_data);
	static void onScrollLeft(void* user_data);
	static void onScrollLeftHeld(void* user_data);
	static void onScrollRight(void* user_data);
	static void onScrollRightHeld(void* user_data);
	static void onScrollDown(void* user_data);
	static void onScrollDownHeld(void* user_data);
	static void onScrollStop(void* user_data);
	
	static void onInputURL(LLFocusableElement* caller, void *userdata);
	static bool hasControlsPermission(LLViewerObject *obj, const LLMediaEntry *media_entry);
	
	void focusOnTarget();
	
	LLViewerMediaImpl* getTargetMediaImpl();
	LLViewerObject* getTargetObject();
	LLPluginClassMedia* getTargetMediaPlugin();
	bool mPauseFadeout;
	bool mUpdateSlider;
	bool mClearFaceOnFade;

	LLMatrix4 mLastCameraMat;
	EZoomLevel mCurrentZoom;
	EScrollDir mScrollState;
	LLCoordWindow mLastCursorPos;
	LLFrameTimer mInactivityTimer;
	LLFrameTimer mFadeTimer;
	F32 mInactiveTimeout;
	F32 mControlFadeTime;
	LLRootHandle<LLPanelPrimMediaControls> mPanelHandle;
	F32 mAlpha;
	std::string mCurrentURL;
	std::string mPreviousURL;
	F64 mCurrentRate;
	F64 mMovieDuration;
	int mUpdatePercent;

	LLUUID mTargetObjectID;
	S32 mTargetObjectFace;
	LLUUID mTargetImplID;
	LLVector3 mTargetObjectNormal;
};

#endif // LL_PANELPRIMMEDIACONTROLS_H
