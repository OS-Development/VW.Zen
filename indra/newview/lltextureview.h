/** 
 * @file lltextureview.h
 * @brief LLTextureView class header file
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
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

#ifndef LL_LLTEXTUREVIEW_H
#define LL_LLTEXTUREVIEW_H

#include "llcontainerview.h"

class LLViewerFetchedTexture;
class LLTextureBar;
class LLGLTexMemBar;

class LLTextureView : public LLContainerView
{
	friend class LLTextureBar;
	friend class LLGLTexMemBar;
protected:
	LLTextureView(const Params&);
	friend class LLUICtrlFactory;
public:
	~LLTextureView();

	/*virtual*/ void draw();
	/*virtual*/ BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL handleMouseUp(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL handleKey(KEY key, MASK mask, BOOL called_from_parent);

	static void addDebugImage(LLViewerFetchedTexture* image) { sDebugImages.insert(image); }
	static void removeDebugImage(LLViewerFetchedTexture* image) { sDebugImages.insert(image); }
	static void clearDebugImages() { sDebugImages.clear(); }

private:
	BOOL addBar(LLViewerFetchedTexture *image, BOOL hilight = FALSE);
	void removeAllBars();

private:
	BOOL mFreezeView;
	BOOL mOrderFetch;
	BOOL mPrintList;
	
	LLTextBox *mInfoTextp;

	std::vector<LLTextureBar*> mTextureBars;
	U32 mNumTextureBars;

	LLGLTexMemBar* mGLTexMemBar;
	
public:
	static std::set<LLViewerFetchedTexture*> sDebugImages;
};

class LLGLTexSizeBar;

class LLTextureSizeView : public LLView
{
public:
	LLTextureSizeView(const Params&);
	~LLTextureSizeView();

	/*virtual*/ void draw();
	/*virtual*/ BOOL handleHover(S32 x, S32 y, MASK mask) ;
	
private:
	F32 drawTextureSizeDistributionGraph() ;
	
private:
	std::vector<LLGLTexSizeBar*> mTextureSizeBar ;
	LLRect mTextureSizeBarRect ;
	S32    mTextureSizeBarWidth ;
};
extern LLTextureView *gTextureView;
extern LLTextureSizeView *gTextureSizeView;
#endif // LL_TEXTURE_VIEW_H
