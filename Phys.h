
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

#define STICK_SENSOR_THICKNESS 2.0f


#define GRABBABLE_MASK_BIT (1<<31)
extern cpShapeFilter GRAB_FILTER;
extern cpShapeFilter NOT_GRABBABLE_FILTER;


class Game;

class BodyState {
public:
#define CELL_PRIO_HIGH 1
#define CELL_PRIO_LOW 0
	int draw_priority;
    int group_id;
    int eye_id; // -1 for normal, 0,1 for eyes (left/right joystick)
    float force;
    float hp;
#define BODY_MAXHP 100
    Game *game;
    float radius;
	BodyState(int prio, int gid, int eye_id, Game *game) : draw_priority(prio), group_id(gid), eye_id(eye_id), force(0), hp(BODY_MAXHP), game(game), radius(0) {};
    float GetHPRate() { return hp / (float)BODY_MAXHP; }
};


////

cpConstraint *new_spring(cpBody *a, cpBody *b, cpVect anchorA, cpVect anchorB, cpFloat restLength, cpFloat stiff, cpFloat damp);
