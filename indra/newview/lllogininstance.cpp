/** 
 * @file lllogininstance.cpp
 * @brief Viewer's host for a login connection.
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "lllogininstance.h"

// llcommon
#include "llevents.h"
#include "llmd5.h"
#include "stringize.h"

// llmessage (!)
#include "llfiltersd2xmlrpc.h" // for xml_escape_string()

// login
#include "lllogin.h"

// newview
#include "llviewernetwork.h"
#include "llviewercontrol.h"
#include "llversioninfo.h"
#include "llslurl.h"
#include "llstartup.h"
#include "llfloaterreg.h"
#include "llnotifications.h"
#include "llwindow.h"
#include "llviewerwindow.h"
#include "llprogressview.h"
#if LL_LINUX || LL_SOLARIS
#include "lltrans.h"
#endif
#include "llsecapi.h"
#include "llstartup.h"
#include "llmachineid.h"


class LLLoginInstance::Disposable {
public:
	virtual ~Disposable() {}
};



static const char * const TOS_REPLY_PUMP = "lllogininstance_tos_callback";
static const char * const TOS_LISTENER_NAME = "lllogininstance_tos";

std::string construct_start_string();




// LLLoginInstance
//-----------------------------------------------------------------------------


LLLoginInstance::LLLoginInstance() :
	mLoginModule(new LLLogin()),
	mNotifications(NULL),
	mLoginState("offline"),
	mAttemptComplete(false),
	mTransferRate(0.0f),
	mDispatcher("LLLoginInstance", "change")
{
	mLoginModule->getEventPump().listen("lllogininstance", 
		boost::bind(&LLLoginInstance::handleLoginEvent, this, _1));
	// This internal use of LLEventDispatcher doesn't really need
	// per-function descriptions.
	mDispatcher.add("fail.login", "", boost::bind(&LLLoginInstance::handleLoginFailure, this, _1));
	mDispatcher.add("connect",    "", boost::bind(&LLLoginInstance::handleLoginSuccess, this, _1));
	mDispatcher.add("disconnect", "", boost::bind(&LLLoginInstance::handleDisconnect, this, _1));
	mDispatcher.add("indeterminate", "", boost::bind(&LLLoginInstance::handleIndeterminate, this, _1));
}

LLLoginInstance::~LLLoginInstance()
{
}

void LLLoginInstance::connect(LLPointer<LLCredential> credentials)
{
	std::vector<std::string> uris;
	LLGridManager::getInstance()->getLoginURIs(uris);
	connect(uris.front(), credentials);
}

void LLLoginInstance::connect(const std::string& uri, LLPointer<LLCredential> credentials)
{
	mAttemptComplete = false; // Reset attempt complete at this point!
	constructAuthParams(credentials);
	mLoginModule->connect(uri, mRequestData);
}

void LLLoginInstance::reconnect()
{
	// Sort of like connect, only using the pre-existing
	// request params.
	std::vector<std::string> uris;
	LLGridManager::getInstance()->getLoginURIs(uris);
	mLoginModule->connect(uris.front(), mRequestData);
	gViewerWindow->setShowProgress(true);
}

void LLLoginInstance::disconnect()
{
	mAttemptComplete = false; // Reset attempt complete at this point!
	mRequestData.clear();
	mLoginModule->disconnect();
}

LLSD LLLoginInstance::getResponse() 
{
	return mResponseData; 
}

void LLLoginInstance::constructAuthParams(LLPointer<LLCredential> user_credential)
{
	// Set up auth request options.
//#define LL_MINIMIAL_REQUESTED_OPTIONS
	LLSD requested_options;
	// *Note: this is where gUserAuth used to be created.
	requested_options.append("inventory-root");
	requested_options.append("inventory-skeleton");
	//requested_options.append("inventory-meat");
	//requested_options.append("inventory-skel-targets");
#if (!defined LL_MINIMIAL_REQUESTED_OPTIONS)
	if(FALSE == gSavedSettings.getBOOL("NoInventoryLibrary"))
	{
		requested_options.append("inventory-lib-root");
		requested_options.append("inventory-lib-owner");
		requested_options.append("inventory-skel-lib");
	//	requested_options.append("inventory-meat-lib");
	}

	requested_options.append("initial-outfit");
	requested_options.append("gestures");
	requested_options.append("display_names");
	requested_options.append("event_categories");
	requested_options.append("event_notifications");
	requested_options.append("classified_categories");
	requested_options.append("adult_compliant"); 
	//requested_options.append("inventory-targets");
	requested_options.append("buddy-list");
	requested_options.append("newuser-config");
	requested_options.append("ui-config");

	//send this info to login.cgi for stats gathering 
	//since viewerstats isn't reliable enough
	requested_options.append("advanced-mode");

#endif
	requested_options.append("max-agent-groups");	
	requested_options.append("map-server-url");	
	requested_options.append("voice-config");
	requested_options.append("tutorial_setting");
	requested_options.append("login-flags");
	requested_options.append("global-textures");
	if(gSavedSettings.getBOOL("ConnectAsGod"))
	{
		gSavedSettings.setBOOL("UseDebugMenus", TRUE);
		requested_options.append("god-connect");
	}
	
	//TODO: make this more flexible
	if (LLGridManager::getInstance()->isInOpenSim())
	{
		requested_options.append("max_groups");
		requested_options.append("profile-server-url");
		requested_options.append("web-profile-url");
	}
	
	// (re)initialize the request params with creds.
	LLSD request_params = user_credential->getLoginParams();

	char hashed_unique_id_string[MD5HEX_STR_SIZE];		/* Flawfinder: ignore */
	LLMD5 hashed_unique_id;
	unsigned char unique_id[MAC_ADDRESS_BYTES];
	if(LLUUID::getNodeID(unique_id) == 0) {
		if(LLMachineID::getUniqueID(unique_id, sizeof(unique_id)) == 0) {
			llerrs << "Failed to get an id; cannot uniquely identify this machine." << llendl;
		}
	}
	hashed_unique_id.update(unique_id, MAC_ADDRESS_BYTES);
	hashed_unique_id.finalize();
	hashed_unique_id.hex_digest(hashed_unique_id_string);
	
	request_params["start"] = construct_start_string();
	request_params["agree_to_tos"] = false; // Always false here. Set true in 
	request_params["read_critical"] = false; // handleTOSResponse
	request_params["last_exec_event"] = mLastExecEvent;
	request_params["mac"] = hashed_unique_id_string;
	request_params["version"] = LLVersionInfo::getChannelAndVersion(); // Includes channel name
	request_params["channel"] = LLVersionInfo::getChannel();
	request_params["id0"] = mSerialNumber;
	request_params["host_id"] = gSavedSettings.getString("HostID");
	request_params["extended_errors"] = true; // request message_id and message_args

	mRequestData.clear();
	mRequestData["method"] = "login_to_simulator";
	mRequestData["params"] = request_params;
	mRequestData["options"] = requested_options;

	mRequestData["cfg_srv_timeout"] = gSavedSettings.getF32("LoginSRVTimeout");
	mRequestData["cfg_srv_pump"] = gSavedSettings.getString("LoginSRVPump");
}

bool LLLoginInstance::handleLoginEvent(const LLSD& event)
{
	LL_DEBUGS("LLLogin") << "LoginListener called!: \n" << event << LL_ENDL;

	if(!(event.has("state") && event.has("change") && event.has("progress")))
	{
		llerrs << "Unknown message from LLLogin: " << event << llendl;
	}

	mLoginState = event["state"].asString();
	mResponseData = event["data"];
	
	if(event.has("transfer_rate"))
	{
		mTransferRate = event["transfer_rate"].asReal();
	}

	

	// Call the method registered in constructor, if any, for more specific
	// handling
	mDispatcher.try_call(event);
	return false;
}

void LLLoginInstance::handleLoginFailure(const LLSD& event)
{
	// Login has failed. 
	// Figure out why and respond...
	LLSD response = event["data"];
	std::string reason_response = response["reason"].asString();
	std::string message_response = response["message"].asString();
	// For the cases of critical message or TOS agreement,
	// start the TOS dialog. The dialog response will be handled
	// by the LLLoginInstance::handleTOSResponse() callback.
	// The callback intiates the login attempt next step, either 
	// to reconnect or to end the attempt in failure.
	if(reason_response == "tos")
	{
		LLSD data(LLSD::emptyMap());
		data["message"] = message_response;
		data["reply_pump"] = TOS_REPLY_PUMP;
		gViewerWindow->setShowProgress(FALSE);
		LLFloaterReg::showInstance("message_tos", data);
		LLEventPumps::instance().obtain(TOS_REPLY_PUMP)
			.listen(TOS_LISTENER_NAME,
					boost::bind(&LLLoginInstance::handleTOSResponse, 
								this, _1, "agree_to_tos"));
	}
	else if(reason_response == "critical")
	{
		LLSD data(LLSD::emptyMap());
		data["message"] = message_response;
		data["reply_pump"] = TOS_REPLY_PUMP;
		if(response.has("error_code"))
		{
			data["error_code"] = response["error_code"];
		}
		if(response.has("certificate"))
		{
			data["certificate"] = response["certificate"];
		}
		
		gViewerWindow->setShowProgress(FALSE);
		LLFloaterReg::showInstance("message_critical", data);
		LLEventPumps::instance().obtain(TOS_REPLY_PUMP)
			.listen(TOS_LISTENER_NAME,
					boost::bind(&LLLoginInstance::handleTOSResponse, 
								this, _1, "read_critical"));
	}
	else
	{	
		attemptComplete();
	}	
}

void LLLoginInstance::handleLoginSuccess(const LLSD& event)
{
	attemptComplete();
}

void LLLoginInstance::handleDisconnect(const LLSD& event)
{
    // placeholder
}

void LLLoginInstance::handleIndeterminate(const LLSD& event)
{
	// The indeterminate response means that the server
	// gave the viewer a new url and params to try.
	// The login module handles the retry, but it gives us the
	// server response so that we may show
	// the user some status.
	LLSD message = event.get("data").get("message");
	if(message.isDefined())
	{
		LLSD progress_update;
		progress_update["desc"] = message;
		LLEventPumps::getInstance()->obtain("LLProgressView").post(progress_update);
	}
}

bool LLLoginInstance::handleTOSResponse(bool accepted, const std::string& key)
{
	if(accepted)
	{	
		// Set the request data to true and retry login.
		mRequestData["params"][key] = true; 
		reconnect();
	}
	else
	{
		attemptComplete();
	}

	LLEventPumps::instance().obtain(TOS_REPLY_PUMP).stopListening(TOS_LISTENER_NAME);
	return true;
}

std::string construct_start_string()
{
	std::string start;
	LLSLURL start_slurl = LLStartUp::getStartSLURL();
	switch(start_slurl.getType())
	{
		case LLSLURL::LOCATION:
		{
			// a startup URL was specified
			LLVector3 position = start_slurl.getPosition();
			std::string unescaped_start = 
			STRINGIZE(  "uri:" 
					  << start_slurl.getRegion() << "&" 
						<< position[VX] << "&" 
						<< position[VY] << "&" 
						<< position[VZ]);
			start = xml_escape_string(unescaped_start);
			break;
		}
		case LLSLURL::HOME_LOCATION:
		{
			start = "home";
			break;
		}
		default:
		{
			start = "last";
		}
	}
	return start;
}

