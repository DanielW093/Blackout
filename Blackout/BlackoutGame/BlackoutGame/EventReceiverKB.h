#include <irrlicht.h>

using namespace irr;

#pragma once
class EventReceiverKB : public IEventReceiver
{
public:
	EventReceiverKB();

	virtual bool OnEvent(const SEvent& event);
	virtual bool IsKeyDown(EKEY_CODE keyCode) const;

private:
	bool KeyIsDown[KEY_KEY_CODES_COUNT];
};