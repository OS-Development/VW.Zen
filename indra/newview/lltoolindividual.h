/** 
 * @file lltoolindividual.h
 * @brief LLToolIndividual class header file
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

#ifndef LL_LLTOOLINDIVIDUAL_H
#define LL_LLTOOLINDIVIDUAL_H

#include "lltool.h"

class LLPickInfo;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class lltoolindividual
//
// A tool to select individual objects rather than linked sets.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLToolIndividual : public LLTool, public LLSingleton<LLToolIndividual>
{
public:
	LLToolIndividual();
	virtual ~LLToolIndividual();

	virtual BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL handleDoubleClick(S32 x, S32 y, MASK mask);
	virtual void handleSelect();
	//virtual void handleDeselect();
	//virtual void render();

	static void pickCallback(const LLPickInfo& pick_info);

protected:

};


#endif // LL_LLTOOLINDIVIDUAL_H
