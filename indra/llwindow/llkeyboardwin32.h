/** 
 * @file llkeyboardwin32.h
 * @brief Handler for assignable key bindings
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

#ifndef LL_LLKEYBOARDWIN32_H
#define LL_LLKEYBOARDWIN32_H

#include "llkeyboard.h"

// this mask distinguishes extended keys, which include non-numpad arrow keys 
// (and, curiously, the num lock and numpad '/')
const MASK MASK_EXTENDED =  0x0100;

class LLKeyboardWin32 : public LLKeyboard
{
public:
	LLKeyboardWin32();
	/*virtual*/ ~LLKeyboardWin32() {};

	/*virtual*/ BOOL	handleKeyUp(const U16 key, MASK mask);
	/*virtual*/ BOOL	handleKeyDown(const U16 key, MASK mask);
	/*virtual*/ void	resetMaskKeys();
	/*virtual*/ MASK	currentMask(BOOL for_mouse_event);
	/*virtual*/ void	scanKeyboard();
	BOOL				translateExtendedKey(const U16 os_key, const MASK mask, KEY *translated_key);
	U16					inverseTranslateExtendedKey(const KEY translated_key);

protected:
	MASK	updateModifiers();
	//void	setModifierKeyLevel( KEY key, BOOL new_state );
private:
	std::map<U16, KEY> mTranslateNumpadMap;
	std::map<KEY, U16> mInvTranslateNumpadMap;
};

#endif
