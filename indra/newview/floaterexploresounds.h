// <edit>

#ifndef LL_LLFLOATEREXPLORESOUNDS_H
#define LL_LLFLOATEREXPLORESOUNDS_H

#include "llfloater.h"
#include "lleventtimer.h"
#include "llaudioengine.h"

class FloaterExploreSounds
: public LLFloater, public LLEventTimer
{
	friend class LLFloaterReg;
public:
	FloaterExploreSounds(const LLSD& key);
	BOOL postBuild(void);
	void close(bool app_quitting);

	BOOL tick();

	LLSoundHistoryItem getItem(LLUUID itemID);

	static void handle_play_locally(void* user_data);
	static void handle_play_in_world(void* user_data);
	static void handle_look_at(void* user_data);
	static void handle_open(void* user_data);
	static void handle_copy_uuid(void* user_data);
	static void handle_stop(void* user_data);

private:
	virtual ~FloaterExploreSounds();
	std::list<LLSoundHistoryItem> mLastHistory;

// static stuff!
public:
	static FloaterExploreSounds* sInstance;

	static void toggle();
};

#endif
// </edit>
