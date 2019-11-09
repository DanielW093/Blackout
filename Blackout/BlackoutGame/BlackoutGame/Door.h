#pragma once

#include <irrlicht.h>
using namespace irr;

class Door
{
public:
	Door(void);
	Door(scene::ISceneManager* sm, scene::ISceneNode* dr, const core::vector3df& ds, const core::vector3df& dv);
	~Door(void);
	void buttonPress(float timeToOpen);
	void collisionResponse(float timeToOpen);
	core::aabbox3d<f32> getTransformedBoundingBox() const {return door->getTransformedBoundingBox();}
private:
	// door animation
	scene::ISceneNodeAnimator* dooranim;

	// door closed
	bool doorclosed;

	// door start
	core::vector3df doorstart;

	// door dist - 
	core::vector3df doorvector;

	//it's scene node
	scene::ISceneNode* door;

	// smgr
	scene::ISceneManager* smgr;
};

