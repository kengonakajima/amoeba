
#include "chipmunk/chipmunk.h"
#include "ChipmunkDemo.h"

void PostStepAddJoint(cpSpace *space, void *key, void *data);
cpBool StickyPreSolve(cpArbiter *arb, cpSpace *space, void *data);
void PostStepRemoveJoint(cpSpace *space, void *key, void *data);
void StickySeparate(cpArbiter *arb, cpSpace *space, void *data);

void PhysUpdateSpace(cpSpace *space, double dt);




enum {
	COLLISION_TYPE_STICKY = 1,
};

#define STICK_SENSOR_THICKNESS 5.5f


#define GRABBABLE_MASK_BIT (1<<31)
extern cpShapeFilter GRAB_FILTER;
extern cpShapeFilter NOT_GRABBABLE_FILTER;


class Game;

class BodyState {
public:
	int id;
    int group_id;
    Game *game;
	BodyState(int id, int gid, Game *game) : id(id), group_id(gid), game(game) {};
};


////

