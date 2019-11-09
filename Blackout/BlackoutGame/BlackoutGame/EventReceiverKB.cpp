#include "EventReceiverKB.h"

EventReceiverKB::EventReceiverKB()
{
     for (u32 i=0; i<KEY_KEY_CODES_COUNT; ++i)
          KeyIsDown[i] = false;
}

bool EventReceiverKB::OnEvent(const SEvent& event)
{
     // Remember whether each key is down or up
     if (event.EventType == irr::EET_KEY_INPUT_EVENT)
     KeyIsDown[event.KeyInput.Key] = event.KeyInput.PressedDown;

     return false;
}

bool EventReceiverKB::IsKeyDown(EKEY_CODE keyCode) const
{
     return KeyIsDown[keyCode];
}