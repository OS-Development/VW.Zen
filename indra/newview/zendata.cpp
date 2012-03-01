/* Copyright (c) 2010
 *
 * Modular Systems All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *   3. Neither the name Modular Systems nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MODULAR SYSTEMS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MODULAR SYSTEMS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "llviewerprecompiledheaders.h"
#include "zendata.h"
#include "llbufferstream.h"

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>

#include "llsdserialize.h"
#include "llversionviewer.h"
#include "llviewercontrol.h"
#include "llviewermedia.h"
#include "llversioninfo.h"
#include "llpanellogin.h"

#include "llprimitive.h"
#include "llagent.h"
#include "llsys.h"
#include "llagentui.h"
#include "lltrans.h"
#include "llweb.h"

static const std::string install_version_str = LLVersionInfo::getVersion();
static const std::string install_version_int = llformat("%d%d%d%d", LL_VERSION_MAJOR, LL_VERSION_MINOR, LL_VERSION_PATCH, LL_VERSION_BUILD);
static const std::string versionid = llformat("%s %d.%d.%d (%d)", LL_CHANNEL, LL_VERSION_MAJOR, LL_VERSION_MINOR, LL_VERSION_PATCH, LL_VERSION_BUILD);
static const std::string zendata_url = "https://bitbucket.org/Zena_Juran/zendataxml/downloads/zendata.xml";

class ZenDownloader : public LLHTTPClient::Responder
{
	LOG_CLASS(ZenDownloader);
public:
	ZenDownloader(std::string url) :
		mURL(url)
	{
	}
	
	virtual void completedRaw(U32 status,
				const std::string& reason,
				const LLChannelDescriptors& channels,
				const LLIOPipe::buffer_ptr_t& buffer)
	{
		if (!isGoodStatus(status))
		{
			gSavedSettings.setString("InstalledVersion", install_version_str);
			gSavedSettings.setString("AvailableVersion", install_version_str);
			LLPanelLogin::setVersion();
			return;
		}
	  
		LLSD content;
		LLBufferStream istr(channels, buffer.get());
		if (!LLSDSerialize::fromXML(content, istr))
		{
			gSavedSettings.setString("InstalledVersion", install_version_str);
			gSavedSettings.setString("AvailableVersion", install_version_str);
			LLPanelLogin::setVersion();
			return;
		}
		
		ZenData::getInstance()->processData(content);
	}
	
private:
	std::string mURL;
};

void ZenData::startDownload()
{
	LLSD headers;
	headers.insert("User-Agent", LLViewerMedia::getCurrentUserAgent());
	headers.insert("viewer-version", versionid);
	LL_INFOS("Data") << "Downloading zendata.xml" << LL_ENDL;
	LLHTTPClient::get(zendata_url,new ZenDownloader(zendata_url),headers);
}

void ZenData::processData(const LLSD& zenData)
{
	// Set Message Of The Day if present
	if(zenData.has("MOTD"))
	{
		gAgent.mMOTD.assign(zenData["MOTD"]);
	}
	
	if (!(zenData["AvailVersionInt"].asInteger() == 0))
	{
		const int InstallVersionInt = atoi(install_version_int.c_str());

		if (InstallVersionInt < zenData["AvailVersionInt"].asInteger())
		{
			std::string avail_version_str = zenData["AvailVersionStr"].asString();
			gSavedSettings.setString("AvailableVersion", avail_version_str);
		}
		else
		{
			gSavedSettings.setString("AvailableVersion", install_version_str);
		}
		
		gSavedSettings.setString("InstalledVersion", install_version_str);
	}
	else
	{
		gSavedSettings.setString("InstalledVersion", install_version_str);
		gSavedSettings.setString("AvailableVersion", install_version_str);
	}
	
	LLPanelLogin::setVersion();
}


