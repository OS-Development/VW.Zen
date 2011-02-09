/** 
 * @file llagentpilot.cpp
 * @brief LLAgentPilot class implementation
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

#include <iostream>
#include <fstream>
#include <iomanip>

#include "llagentpilot.h"
#include "llagent.h"
#include "llappviewer.h"
#include "llviewercontrol.h"
#include "llviewercamera.h"
#include "llviewerjoystick.h"
#include "llsdserialize.h"
#include "llsdutil_math.h"

LLAgentPilot gAgentPilot;

BOOL LLAgentPilot::sLoop = TRUE;
BOOL LLAgentPilot::sReplaySession = FALSE;

LLAgentPilot::LLAgentPilot() :
	mNumRuns(-1),
	mQuitAfterRuns(FALSE),
	mRecording(FALSE),
	mLastRecordTime(0.f),
	mStarted(FALSE),
	mPlaying(FALSE),
	mCurrentAction(0),
	mOverrideCamera(FALSE)
{
}

LLAgentPilot::~LLAgentPilot()
{
}

#define CAM_FIELDS 0

void LLAgentPilot::load()
{
	std::string txt_filename = gSavedSettings.getString("StatsPilotFile");
	std::string xml_filename = gSavedSettings.getString("StatsPilotXMLFile");
	if (LLFile::isfile(xml_filename))
	{
		loadXML(xml_filename);
	}
	else if (LLFile::isfile(txt_filename))
	{
		loadTxt(txt_filename);
	}
	else
	{
		lldebugs << "no autopilot file found" << llendl;
		return;
	}
}

void LLAgentPilot::loadTxt(const std::string& filename)
{
	if(filename.empty())
	{
		return;
	}
	
	llifstream file(filename);

	if (!file)
	{
		lldebugs << "Couldn't open " << filename
			<< ", aborting agentpilot load!" << llendl;
		return;
	}
	else
	{
		llinfos << "Opening pilot file " << filename << llendl;
	}

	S32 num_actions;

	file >> num_actions;

	for (S32 i = 0; i < num_actions; i++)
	{
		S32 action_type;
		Action new_action;
		file >> new_action.mTime >> action_type;
		file >> new_action.mTarget.mdV[VX] >> new_action.mTarget.mdV[VY] >> new_action.mTarget.mdV[VZ];
#if CAM_FIELDS
		file >> new_action.mCameraView;
		file >> new_action.mCameraOrigin.mV[VX]
			 >> new_action.mCameraOrigin.mV[VY]
			 >> new_action.mCameraOrigin.mV[VZ];

		file >> new_action.mCameraXAxis.mV[VX]
			 >> new_action.mCameraXAxis.mV[VY]
			 >> new_action.mCameraXAxis.mV[VZ];

		file >> new_action.mCameraYAxis.mV[VX]
			 >> new_action.mCameraYAxis.mV[VY]
			 >> new_action.mCameraYAxis.mV[VZ];

		file >> new_action.mCameraZAxis.mV[VX]
			 >> new_action.mCameraZAxis.mV[VY]
			 >> new_action.mCameraZAxis.mV[VZ];
#endif
		new_action.mType = (EActionType)action_type;
		mActions.put(new_action);
	}

	mOverrideCamera = false;
	
	file.close();
}

void LLAgentPilot::loadXML(const std::string& filename)
{
	if(filename.empty())
	{
		return;
	}
	
	llifstream file(filename);

	if (!file)
	{
		lldebugs << "Couldn't open " << filename
			<< ", aborting agentpilot load!" << llendl;
		return;
	}
	else
	{
		llinfos << "Opening pilot file " << filename << llendl;
	}

	LLSD record;
	while (!file.eof() && LLSDSerialize::fromXML(record, file))
	{
		Action action;
		action.mTime = record["time"].asReal();
		action.mType = (EActionType)record["type"].asInteger();
		action.mCameraView = record["camera_view"].asReal();
		action.mTarget = ll_vector3d_from_sd(record["target"]);
		action.mCameraOrigin = ll_vector3_from_sd(record["camera_origin"]);
		action.mCameraXAxis = ll_vector3_from_sd(record["camera_xaxis"]);
		action.mCameraYAxis = ll_vector3_from_sd(record["camera_yaxis"]);
		action.mCameraZAxis = ll_vector3_from_sd(record["camera_zaxis"]);
		mActions.put(action);
	}
	mOverrideCamera = true;
	file.close();
}

void LLAgentPilot::save()
{
	std::string txt_filename = gSavedSettings.getString("StatsPilotFile");
	std::string xml_filename = gSavedSettings.getString("StatsPilotXMLFile");
	saveTxt(txt_filename);
	saveXML(xml_filename);
}

void LLAgentPilot::saveTxt(const std::string& filename)
{
	llofstream file;
	file.open(filename);

	if (!file)
	{
		llinfos << "Couldn't open " << filename << ", aborting agentpilot save!" << llendl;
	}

	file << mActions.count() << '\n';

	S32 i;
	for (i = 0; i < mActions.count(); i++)
	{
		file << mActions[i].mTime << "\t" << mActions[i].mType << "\t";
		file << std::setprecision(32) << mActions[i].mTarget.mdV[VX] << "\t" << mActions[i].mTarget.mdV[VY] << "\t" << mActions[i].mTarget.mdV[VZ];
#if CAM_FIELDS
		file << "\t" << mActions[i].mCameraView;

		file << "\t" << mActions[i].mCameraOrigin[VX]
			 << "\t" << mActions[i].mCameraOrigin[VY]
			 << "\t" << mActions[i].mCameraOrigin[VZ];

		file << "\t" << mActions[i].mCameraXAxis[VX]
			 << "\t" << mActions[i].mCameraXAxis[VY]
			 << "\t" << mActions[i].mCameraXAxis[VZ];

		file << "\t" << mActions[i].mCameraYAxis[VX]
			 << "\t" << mActions[i].mCameraYAxis[VY]
			 << "\t" << mActions[i].mCameraYAxis[VZ];

		file << "\t" << mActions[i].mCameraZAxis[VX]
			 << "\t" << mActions[i].mCameraZAxis[VY]
			 << "\t" << mActions[i].mCameraZAxis[VZ];
#endif
		file << '\n';
	}

	file.close();
}

void LLAgentPilot::saveXML(const std::string& filename)
{
	llofstream file;
	file.open(filename);

	if (!file)
	{
		llinfos << "Couldn't open " << filename << ", aborting agentpilot save!" << llendl;
	}

	S32 i;
	for (i = 0; i < mActions.count(); i++)
	{
		Action& action = mActions[i];
		LLSD record;
		record["time"] = (LLSD::Real)action.mTime;
		record["type"] = (LLSD::Integer)action.mType;
		record["camera_view"] = (LLSD::Real)action.mCameraView;
		record["target"] = ll_sd_from_vector3d(action.mTarget);
		record["camera_origin"] = ll_sd_from_vector3(action.mCameraOrigin);
		record["camera_xaxis"] = ll_sd_from_vector3(action.mCameraXAxis);
		record["camera_yaxis"] = ll_sd_from_vector3(action.mCameraYAxis);
		record["camera_zaxis"] = ll_sd_from_vector3(action.mCameraZAxis);
		LLSDSerialize::toXML(record, file);
	}
	file.close();
}

void LLAgentPilot::startRecord()
{
	mActions.reset();
	mTimer.reset();
	addAction(STRAIGHT);
	mRecording = TRUE;
}

void LLAgentPilot::stopRecord()
{
	gAgentPilot.addAction(STRAIGHT);
	gAgentPilot.save();
	mRecording = FALSE;
}

void LLAgentPilot::addAction(enum EActionType action_type)
{
	llinfos << "Adding waypoint: " << gAgent.getPositionGlobal() << llendl;
	Action action;
	action.mType = action_type;
	action.mTarget = gAgent.getPositionGlobal();
	action.mTime = mTimer.getElapsedTimeF32();
	LLViewerCamera *cam = LLViewerCamera::getInstance();
	action.mCameraView = cam->getView();
	action.mCameraOrigin = cam->getOrigin();
	action.mCameraXAxis = cam->getXAxis();
	action.mCameraYAxis = cam->getYAxis();
	action.mCameraZAxis = cam->getZAxis();
	mLastRecordTime = (F32)action.mTime;
	mActions.put(action);
}

void LLAgentPilot::startPlayback()
{
	if (!mPlaying)
	{
		mPlaying = TRUE;
		mCurrentAction = 0;
		mTimer.reset();

		if (mActions.count())
		{
			llinfos << "Starting playback, moving to waypoint 0" << llendl;
			if (getOverrideCamera() &&
				!LLViewerJoystick::getInstance()->getOverrideCamera())	
			{
				LLViewerJoystick::getInstance()->toggleFlycam();
			}
			gAgent.startAutoPilotGlobal(mActions[0].mTarget);
			moveCamera(mActions[0]);
			mStarted = FALSE;
		}
		else
		{
			llinfos << "No autopilot data, cancelling!" << llendl;
			mPlaying = FALSE;
		}
	}
}

void LLAgentPilot::stopPlayback()
{
	if (mPlaying)
	{
		mPlaying = FALSE;
		mCurrentAction = 0;
		mTimer.reset();
		gAgent.stopAutoPilot();
	}

	if (sReplaySession)
	{
		LLAppViewer::instance()->forceQuit();
	}
}

void LLAgentPilot::moveCamera(Action& action)
{
	if (!getOverrideCamera())
		return;
	
	LLViewerCamera::getInstance()->setView(action.mCameraView);
	LLViewerCamera::getInstance()->setOrigin(action.mCameraOrigin);
	LLViewerCamera::getInstance()->mXAxis = LLVector3(action.mCameraXAxis);
	LLViewerCamera::getInstance()->mYAxis = LLVector3(action.mCameraYAxis);
	LLViewerCamera::getInstance()->mZAxis = LLVector3(action.mCameraZAxis);
}

void LLAgentPilot::updateTarget()
{
	if (mPlaying)
	{
		if (mCurrentAction < mActions.count())
		{
			if (0 == mCurrentAction)
			{
				if (gAgent.getAutoPilot())
				{
					// Wait until we get to the first location before starting.
					return;
				}
				else
				{
					if (!mStarted)
					{
						llinfos << "At start, beginning playback" << llendl;
						mTimer.reset();
						mStarted = TRUE;
					}
				}
			}
			if (mTimer.getElapsedTimeF32() > mActions[mCurrentAction].mTime)
			{
				//gAgent.stopAutoPilot();
				mCurrentAction++;

				if (mCurrentAction < mActions.count())
				{
					gAgent.startAutoPilotGlobal(mActions[mCurrentAction].mTarget);
					moveCamera(mActions[mCurrentAction]);
				}
				else
				{
					stopPlayback();
					mNumRuns--;
					if (sLoop)
					{
						if ((mNumRuns < 0) || (mNumRuns > 0))
						{
							llinfos << "Looping, restarting playback" << llendl;
							startPlayback();
						}
						else if (mQuitAfterRuns)
						{
							llinfos << "Done with all runs, quitting viewer!" << llendl;
							LLAppViewer::instance()->forceQuit();
						}
						else
						{
							llinfos << "Done with all runs, disabling pilot" << llendl;
							stopPlayback();
						}
					}
				}
			}
		}
		else
		{
			stopPlayback();
		}
	}
	else if (mRecording)
	{
		if (mTimer.getElapsedTimeF32() - mLastRecordTime > 1.f)
		{
			addAction(STRAIGHT);
		}
	}
}

// static
void LLAgentPilot::startRecord(void *)
{
	gAgentPilot.startRecord();
}

void LLAgentPilot::saveRecord(void *)
{
	gAgentPilot.stopRecord();
}

void LLAgentPilot::addWaypoint(void *)
{
	gAgentPilot.addAction(STRAIGHT);
}

void LLAgentPilot::startPlayback(void *)
{
	gAgentPilot.mNumRuns = -1;
	gAgentPilot.startPlayback();
}

void LLAgentPilot::stopPlayback(void *)
{
	gAgentPilot.stopPlayback();
}
