#include "MyFPSCameraAnimator.h"
#include "IVideoDriver.h"
#include "ISceneManager.h"
#include "Keycodes.h"
#include "ICursorControl.h"
#include "ICameraSceneNode.h"
#include "ISceneNodeAnimatorCollisionResponse.h"

/*
This class was based heavily on the default FPS camera included in the Irrlicht engine.
The reasoning for this was that it seemed like a waste of time to rewrite the fundamental
functionality of the first person camera, considering it already existed in engine.
That said, I was not happy with how it handled, and saw fit to modify it to improve 
the quality of my game. Rather than modify the engine and risk my modifications going
unnoticed, I chose to create my own custom class within my solution.
*/

namespace irr
{
namespace scene
{

MyFPSCameraAnimator::MyFPSCameraAnimator(gui::ICursorControl* cursorControl, f32 rotateSpeed,
											f32 moveSpeed, f32 jumpSpeed, SKeyMap* keyMapArray, u32 keyMapSize, 
											bool noVerticalMovement, bool invertY) 
: CursorControl(cursorControl), MaxVerticalAngle(88.0f), 
WalkMoveSpeed(moveSpeed), RotateSpeed(rotateSpeed), JumpSpeed(jumpSpeed),
MouseYDirection(invertY ? -1.0f : 1.0f), LastAnimationTime(0), 
firstUpdate(true), firstInput(true), NoVerticalMovement(noVerticalMovement)
{
	#ifdef _DEBUG
	setDebugName("MyFPSCameraAnimator");
	#endif

	ar = 0.001f;
	fbMoveSpeed = 0;
	lrMoveSpeed = 0;
	MoveSpeed = WalkMoveSpeed;

	if (CursorControl)
		CursorControl->grab();

	allKeysUp();

	// create key map
	if (!keyMapArray || !keyMapSize)
	{
		// create default key map
		KeyMap.push_back(SKeyMap(EKA_MOVE_FORWARD, irr::KEY_KEY_W));
		KeyMap.push_back(SKeyMap(EKA_MOVE_BACKWARD, irr::KEY_KEY_S));
		KeyMap.push_back(SKeyMap(EKA_STRAFE_LEFT, irr::KEY_KEY_A));
		KeyMap.push_back(SKeyMap(EKA_STRAFE_RIGHT, irr::KEY_KEY_D));
		KeyMap.push_back(SKeyMap(EKA_JUMP_UP, irr::KEY_SPACE));
	}
	else
	{
		// create custom key map
		setKeyMap(keyMapArray, keyMapSize);
	}
}

MyFPSCameraAnimator::~MyFPSCameraAnimator()
{
	if (CursorControl)
		CursorControl->drop();
}

bool MyFPSCameraAnimator::OnEvent(const SEvent& evt)
{
	switch(evt.EventType)
	{
	case EET_KEY_INPUT_EVENT:
		for (u32 i=0; i<KeyMap.size(); ++i)
		{
			if (KeyMap[i].KeyCode == evt.KeyInput.Key)
			{
				CursorKeys[KeyMap[i].Action] = evt.KeyInput.PressedDown;
				return true;
			}
		}
		break;

	case EET_MOUSE_INPUT_EVENT:
		if (evt.MouseInput.Event == EMIE_MOUSE_MOVED)
		{
			CursorPos = CursorControl->getRelativePosition();
			return true;
		}
		break;

	default:
		break;
	}

	return false;
}

void MyFPSCameraAnimator::animateNode(ISceneNode* node, u32 timeMs)
{
	if (!node || node->getType() != ESNT_CAMERA)
		return;

	ICameraSceneNode* camera = static_cast<ICameraSceneNode*>(node);

	if (firstUpdate)
	{
		camera->updateAbsolutePosition();
		if (CursorControl )
		{
			CursorControl->setPosition(0.5f, 0.5f);
			CursorPos = CenterCursor = CursorControl->getRelativePosition();
		}

		LastAnimationTime = timeMs;

		firstUpdate = false;
	}

	// If the camera isn't the active camera, and receiving input, then don't process it.
	if(!camera->isInputReceiverEnabled())
	{
		fbMoveSpeed = 0;
		lrMoveSpeed = 0;
		firstInput = true;
		return;
	}

	if ( firstInput )
	{
		allKeysUp();
		firstInput = false;
	}

	scene::ISceneManager * smgr = camera->getSceneManager();
	if(smgr && smgr->getActiveCamera() != camera)
		return;

	// get time
	f32 timeDiff = (f32)(timeMs - LastAnimationTime);

	LastAnimationTime = timeMs;

	// update position
	core::vector3df pos = camera->getPosition();

	// Update rotation
	core::vector3df target = (camera->getTarget() - camera->getAbsolutePosition());
	core::vector3df relativeRotation = target.getHorizontalAngle();


	if (CursorControl)
	{
		if (CursorPos != CenterCursor)
		{
			relativeRotation.Y -= (0.5f - CursorPos.X) * RotateSpeed;
			relativeRotation.X -= (0.5f - CursorPos.Y) * RotateSpeed * MouseYDirection;

			// X < MaxVerticalAngle or X > 360-MaxVerticalAngle

			if (relativeRotation.X > MaxVerticalAngle*2 &&
				relativeRotation.X < 360.0f-MaxVerticalAngle)
			{
				relativeRotation.X = 360.0f-MaxVerticalAngle;
			}
			else
			if (relativeRotation.X > MaxVerticalAngle &&
				relativeRotation.X < 360.0f-MaxVerticalAngle)
			{
				relativeRotation.X = MaxVerticalAngle;
			}

			// Do the fix as normal, special case below
			// reset cursor position to the centre of the window.
			CursorControl->setPosition(0.5f, 0.5f);
			CenterCursor = CursorControl->getRelativePosition();

			// needed to avoid problems when the event receiver is disabled
			CursorPos = CenterCursor;
		}

		// Special case, mouse is whipped outside of window before it can update.
		video::IVideoDriver* driver = smgr->getVideoDriver();
		core::vector2d<u32> mousepos(u32(CursorControl->getPosition().X), u32(CursorControl->getPosition().Y));
		core::rect<u32> screenRect(0, 0, driver->getScreenSize().Width, driver->getScreenSize().Height);

		// Only if we are moving outside quickly.
		bool reset = !screenRect.isPointInside(mousepos);

		if(reset)
		{
			// Force a reset.
			CursorControl->setPosition(0.5f, 0.5f);
			CenterCursor = CursorControl->getRelativePosition();
			CursorPos = CenterCursor;
 		}
	}

	// set target

	target.set(0,0, core::max_(1.f, pos.getLength()));
	core::vector3df movedir = target;

	core::matrix4 mat;
	mat.setRotationDegrees(core::vector3df(relativeRotation.X, relativeRotation.Y, 0));
	mat.transformVect(target);

	if (NoVerticalMovement)
	{
		mat.setRotationDegrees(core::vector3df(0, relativeRotation.Y, 0));
		mat.transformVect(movedir);
	}
	else
	{
		movedir = target;
	}

	movedir.normalize();

	if (fbMoveSpeed != 0.0f && lrMoveSpeed != 0.0f)
	{
		MoveSpeed = WalkMoveSpeed * 0.7; //When moving diagonally
	}
	else
	{
		MoveSpeed = WalkMoveSpeed;
	}

	if (CursorKeys[EKA_MOVE_FORWARD] && !CursorKeys[EKA_MOVE_BACKWARD])
	{
		if (fbMoveSpeed < MoveSpeed)
		{
			fbMoveSpeed += ar * timeDiff;
			pos += (movedir * fbMoveSpeed) * timeDiff;
		}
		else
		{
			pos += (movedir * MoveSpeed) * timeDiff;
		}
	}
	
	if (CursorKeys[EKA_MOVE_BACKWARD] && !CursorKeys[EKA_MOVE_FORWARD])
	{
		if (fbMoveSpeed > -MoveSpeed)
		{
			fbMoveSpeed -= ar * timeDiff;
			pos += (movedir * fbMoveSpeed) * timeDiff;
		}
		else
		{
			pos += (movedir * -MoveSpeed) * timeDiff;
		}
	}
	
	if ((!CursorKeys[EKA_MOVE_FORWARD] && !CursorKeys[EKA_MOVE_BACKWARD]) || (CursorKeys[EKA_MOVE_FORWARD] && CursorKeys[EKA_MOVE_BACKWARD]))
	{
		if (fbMoveSpeed > 0.02f)
		{
			fbMoveSpeed -= ar * timeDiff;
		}
		else if (fbMoveSpeed < -0.02f)
		{
			fbMoveSpeed += ar * timeDiff;
		}
		else
		{
			fbMoveSpeed = 0.0f;
		}
		pos += movedir * timeDiff * fbMoveSpeed;
	}

	//if (CursorKeys[EKA_MOVE_BACKWARD])
		//pos -= movedir * timeDiff * MoveSpeed;

	// strafing

	core::vector3df strafevect = target;
	strafevect = strafevect.crossProduct(camera->getUpVector());

	if (NoVerticalMovement)
		strafevect.Y = 0.0f;

	strafevect.normalize();

	if (CursorKeys[EKA_STRAFE_LEFT] && !CursorKeys[EKA_STRAFE_RIGHT])
	{
		if (lrMoveSpeed < MoveSpeed)
		{
			lrMoveSpeed += ar * timeDiff;
			pos += strafevect * timeDiff * lrMoveSpeed;
		}
		else
		{
			pos += strafevect * timeDiff * MoveSpeed;
		}
	}

	if (CursorKeys[EKA_STRAFE_RIGHT] && !CursorKeys[EKA_STRAFE_LEFT])
	{
		if (lrMoveSpeed > -MoveSpeed)
		{
			lrMoveSpeed -= ar * timeDiff;
			pos += strafevect * timeDiff * lrMoveSpeed;
		}
		else
		{
			pos += strafevect * timeDiff * -MoveSpeed;
		}
	}

		if ((!CursorKeys[EKA_STRAFE_RIGHT] && !CursorKeys[EKA_STRAFE_LEFT]) || (CursorKeys[EKA_STRAFE_RIGHT] && CursorKeys[EKA_STRAFE_LEFT]))
	{
		if (lrMoveSpeed > 0.02f)
		{
			lrMoveSpeed -= ar * timeDiff;
		}
		else if (lrMoveSpeed < -0.02f)
		{
			lrMoveSpeed += ar * timeDiff;
		}
		else
		{
			lrMoveSpeed = 0.0f;
		}
		pos += strafevect * timeDiff * lrMoveSpeed*1.2;
	}

	// For jumping, we find the collision response animator attached to our camera
	// and if it's not falling, we tell it to jump.
	if (CursorKeys[EKA_JUMP_UP])
	{
		const ISceneNodeAnimatorList& animators = camera->getAnimators();
		ISceneNodeAnimatorList::ConstIterator it = animators.begin();
		while(it != animators.end())
		{
			if(ESNAT_COLLISION_RESPONSE == (*it)->getType())
			{
				ISceneNodeAnimatorCollisionResponse * collisionResponse =
					static_cast<ISceneNodeAnimatorCollisionResponse *>(*it);

				if(!collisionResponse->isFalling())
					collisionResponse->jump(JumpSpeed);
			}

			it++;
		}
	}


	// write translation
	camera->setPosition(pos);

	// write right target
	target += pos;
	camera->setTarget(target);
}

void MyFPSCameraAnimator::allKeysUp()
{
	for (u32 i=0; i<EKA_COUNT; ++i)
		CursorKeys[i] = false;
}

void MyFPSCameraAnimator::setRotateSpeed(f32 speed)
{
	RotateSpeed = speed;
}

void MyFPSCameraAnimator::setMoveSpeed(f32 speed)
{
	MoveSpeed = speed;
}

f32 MyFPSCameraAnimator::getRotateSpeed() const
{
	return RotateSpeed;
}

f32 MyFPSCameraAnimator::getMoveSpeed() const
{
	return MoveSpeed;
}

void MyFPSCameraAnimator::setKeyMap(SKeyMap *map, u32 count)
{
	// clear the keymap
	KeyMap.clear();

	// add actions
	for (u32 i=0; i<count; ++i)
	{
		KeyMap.push_back(map[i]);
	}
}

void MyFPSCameraAnimator::setKeyMap(const core::array<SKeyMap>& keymap)
{
	KeyMap=keymap;
}

const core::array<SKeyMap>& MyFPSCameraAnimator::getKeyMap() const
{
	return KeyMap;
}

void MyFPSCameraAnimator::setVerticalMovement(bool allow)
{
	NoVerticalMovement = !allow;
}

void MyFPSCameraAnimator::setInvertMouse(bool invert)
{
	if (invert)
		MouseYDirection = -1.0f;
	else
		MouseYDirection = 1.0f;
}

ISceneNodeAnimator* MyFPSCameraAnimator::createClone(ISceneNode* node, ISceneManager* newManager)
{
	MyFPSCameraAnimator * newAnimator =
		new MyFPSCameraAnimator(CursorControl,	RotateSpeed, MoveSpeed, JumpSpeed,
											0, 0, NoVerticalMovement);
	newAnimator->setKeyMap(KeyMap);
	return newAnimator;
}

}
}