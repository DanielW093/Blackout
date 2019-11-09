#include "Door.h"

#include <iostream>
using namespace std;

Door::Door(void)
{	
	dooranim = 0;
	doorclosed = true;
}

Door::Door(scene::ISceneManager* sm, scene::ISceneNode* dr, const core::vector3df& ds, const core::vector3df& dv):smgr(sm),door(dr),doorstart(ds),doorvector(dv)
{
	dooranim = 0;
	doorclosed = true;
}

Door::~Door(void)
{
}

void Door::buttonPress(float timeToOpen)
{
	// The end point is the start point, but a bit higher up
	// hence Y plus a bit
	core::vector3df doorpos = door->getPosition();
	core::vector3df doorend;

	if (dooranim) {
		door->removeAnimator(dooranim);
		dooranim->drop();
		dooranim = 0;

		// now put door into reverse
	}

	// Door must open if it is closed, or close if it is open
	int doortime = 0;

	float fracdist = 0;

	if (doorclosed) {
		doorend = doorstart + doorvector;
	}
	else {
		doorend = doorstart;
	}

	fracdist = (doorend - doorpos).getLength()/doorvector.getLength();

	doortime = (int)((timeToOpen*1000)*fracdist);

	//cout << "fracdist = " << fracdist;
	//cout << "doorend = " << doorend.X << "," << doorend.Y << "," << doorend.Z << endl;
	//cout << "doorend = " << doorend.X << "," << doorend.Y << "," << doorend.Z << endl;


	//cout << "time to close door" << doortime << endl;

		//// if there is not already a door animation, or it has finished
		//if (!dooranim || dooranim->hasFinished()) 
		//{
		//	if (dooranim && dooranim->hasFinished()) 
		//	{
		//		// if finished drop it
		//		dooranim->drop();
		//		dooranim = 0;
		//	}
		//}

	if (dooranim) {
		door->removeAnimator(dooranim);
		dooranim->drop();
		dooranim = 0;
	}

	// make the door open/close
	dooranim =
		smgr->createFlyStraightAnimator(doorpos, doorend, doortime, false, false);		

	// add animator
	door->addAnimator(dooranim);

	// change the door status
	doorclosed = !doorclosed;
}

void Door::collisionResponse(float timeToOpen)
{
	// The end point is the start point, but a bit higher up
	// hence Y plus a bit
	core::vector3df doorpos = door->getPosition();
	core::vector3df doorend;

	if (dooranim) 
	{
		door->removeAnimator(dooranim);
		dooranim->drop();
		dooranim = 0;

		// now put door into reverse
	}

	int doortime = 0;

	float fracdist = 0;

	doorend = doorstart + doorvector;

	fracdist = (doorend - doorpos).getLength()/doorvector.getLength();

	doortime = (int)((timeToOpen*1000)*fracdist);

	if (dooranim) 
	{
		door->removeAnimator(dooranim);
		dooranim->drop();
		dooranim = 0;
	}

	// make the door open
	dooranim =
		smgr->createFlyStraightAnimator(doorpos, doorend, doortime, false, false);		

	// add animator
	door->addAnimator(dooranim);

	// change the door status
	doorclosed = !doorclosed;
}