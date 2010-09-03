/** 
 * @file lldir_win32.h
 * @brief Definition of directory utilities class for windows
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#ifndef LL_LLDIR_WIN32_H
#define LL_LLDIR_WIN32_H

#include "lldir.h"

class LLDir_Win32 : public LLDir
{
public:
	LLDir_Win32();
	virtual ~LLDir_Win32();

	/*virtual*/ void initAppDirs(const std::string &app_name,
		const std::string& app_read_only_data_dir);

	/*virtual*/ std::string getCurPath();
	/*virtual*/ U32 countFilesInDir(const std::string &dirname, const std::string &mask);
	/*virtual*/ BOOL getNextFileInDir(const std::string &dirname, const std::string &mask, std::string &fname, BOOL wrap);
	/*virtual*/ void getRandomFileInDir(const std::string &dirname, const std::string &mask, std::string &fname);
	/*virtual*/ BOOL fileExists(const std::string &filename) const;

	/*virtual*/ std::string getLLPluginLauncher();
	/*virtual*/ std::string getLLPluginFilename(std::string base_name);

private:
	BOOL LLDir_Win32::getNextFileInDir(const llutf16string &dirname, const std::string &mask, std::string &fname, BOOL wrap);
	
	void* mDirSearch_h;
	llutf16string mCurrentDir;
};

#endif // LL_LLDIR_WIN32_H


