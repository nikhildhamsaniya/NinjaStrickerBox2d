//
//  HelloWorldScene.cpp
//  NinjaStrickerBox2d
//
//  Created by NgocDu on 13/09/09.
//  Copyright __MyCompanyName__ 2013年. All rights reserved.
//
#include "HelloWorldScene.h"
#include "SimpleAudioEngine.h"
#include "GB2ShapeCache-x.h"
#include <math.h>
using namespace cocos2d;
using namespace CocosDenshion;

#define PTM_RATIO 32

enum {
    kTagParentNode = 1,
};

HelloWorld::HelloWorld()
{
    // load physics shapes
    GB2ShapeCache::sharedGB2ShapeCache()->addShapesWithFile("ninjaS.plist");
    
    CCSize s = CCDirector::sharedDirector()->getWinSize();
    delta = 3.14f;
    setTouchEnabled( true );
    setAccelerometerEnabled( true );
    // init physics
    this->initPhysics();
    GameManager::sharedGameManager()->setNumberActionPlayer(2);
    //---------------------------------
    _tileMap = new CCTMXTiledMap();
    _tileMap->initWithTMXFile("TileMap.tmx");
    _background = _tileMap->layerNamed("Background");
    _coin = _tileMap->layerNamed("Coin");
    _snake = _tileMap->layerNamed("Snake");
    _scorpion = _tileMap->layerNamed("Scorpion");
    _arrow = _tileMap->layerNamed("Arrow");
    _meta = _tileMap->layerNamed("Meta");
    _meta->setVisible(false);
    
    CCLOG("size tiled: %f , %f",_tileMap->getTileSize().width, _tileMap->getTileSize().height);
    withTileMap = _tileMap->getMapSize().width * _tileMap->getTileSize().width;
    heightTileMap = _tileMap->getMapSize().height * _tileMap->getTileSize().height;
    CCLog("size map with %f", withTileMap);

    this->addChild(_tileMap);
    
    CCTMXObjectGroup *objectGroup = _tileMap->objectGroupNamed("Objects");
    
    if(objectGroup == NULL){
        CCLog("tile map has no objects object layer");
//        return false;
    }

    this->prepareLayers();
    
    addNewSpriteAtPosition(ccp(s.width/2, s.height/4));
    this->setViewPointCenter(_player->getPosition());
    //----------------------------------
    _arrayCoin = new CCArray();
    _arraySnake = new CCArray();
    _arrayScorpion = new CCArray();
    _arrayArrow = new CCArray();
    
    this->addCoins();
    this->addSnakes();
    this->addScorpions();
    
    this->schedule(schedule_selector(HelloWorld::updateLocation_Direction), 0.1f);
    scheduleUpdate();
}

HelloWorld::~HelloWorld()
{
    delete world;
    world = NULL;
    
    delete m_debugDraw;
}

void HelloWorld::initPhysics()
{

    
    CCSize s = CCDirector::sharedDirector()->getWinSize();

    b2Vec2 gravity;
    gravity.Set(0.0f, -15.0f);
    world = new b2World(gravity);
    _contactListener = new MyContactListener();
    _contactListener->setNumberBegin(0);
    world->SetContactListener(_contactListener);
    // Do we want to let bodies sleep?
    world->SetAllowSleeping(true);

    world->SetContinuousPhysics(true);

   
     m_debugDraw = new GLESDebugDraw( PTM_RATIO );
     world->SetDebugDraw(m_debugDraw);

    uint32 flags = 0;
    flags += b2Draw::e_shapeBit;
    //        flags += b2Draw::e_jointBit;
    //        flags += b2Draw::e_aabbBit;
    //        flags += b2Draw::e_pairBit;
    //        flags += b2Draw::e_centerOfMassBit;
    m_debugDraw->SetFlags(flags);


    // Define the ground body.
    b2BodyDef groundBodyDef;
    groundBodyDef.position.Set(0, 0); // bottom-left corner

    // Call the body factory which allocates memory for the ground body
    // from a pool and creates the ground box shape (also from a pool).
    // The body is also added to the world.
    b2Body* groundBody = world->CreateBody(&groundBodyDef);

    // Define the ground box shape.
    b2EdgeShape groundBox;

    // bottom

    groundBox.Set(b2Vec2(0,0), b2Vec2(withTileMap/PTM_RATIO,0));
    groundBody->CreateFixture(&groundBox,0);

    // top
    groundBox.Set(b2Vec2(0, heightTileMap/PTM_RATIO), b2Vec2(withTileMap/PTM_RATIO, heightTileMap/PTM_RATIO));
    groundBody->CreateFixture(&groundBox,0);

    // left
    groundBox.Set(b2Vec2(0,s.height/PTM_RATIO), b2Vec2(0 ,0));
    groundBody->CreateFixture(&groundBox,0);

    // right
    
    groundBox.Set(b2Vec2(withTileMap/PTM_RATIO, s.height/PTM_RATIO), b2Vec2(withTileMap/PTM_RATIO,0));
    groundBody->CreateFixture(&groundBox,0);
}

void HelloWorld::draw()
{
    //
    // IMPORTANT:
    // This is only for debug purposes
    // It is recommend to disable it
    //
    CCLayer::draw();

    ccGLEnableVertexAttribs( kCCVertexAttribFlag_Position );

    kmGLPushMatrix();

    world->DrawDebugData();

    kmGLPopMatrix();
    
}

void HelloWorld::addNewSpriteAtPosition(CCPoint p)
{
    CCSprite *sp = CCSprite::create("ninja.png");
    
    _player = new Ninja();
        
    _player->setAttack(0);
//    _player->initWithFile("ninja.png");
    _player->init();
//    _player->autorelease();
    this->addChild(_player, 1000);
    _player->addChild(sp);
    _player->setImage(sp);
    
    _player->setPosition( CCPointMake( p.x, p.y) );
    // Define the dynamic body.
    //Set up a 1m squared box in the physics world
    b2BodyDef bodyDef;
    bodyDef.type = b2_dynamicBody;
    bodyDef.position.Set(p.x/PTM_RATIO, p.y/PTM_RATIO);
    bodyDef.userData = _player;
    b2Body *body = world->CreateBody(&bodyDef);
    
    // Define another box shape for our dynamic body.
    b2PolygonShape dynamicBox;
    
    b2Vec2 vertices[4];
    vertices[0].Set(-1.2f, -1.2f);    
    vertices[1].Set(1.2f, -1.2f);    
    vertices[2].Set(1.2f, 1.2f);
    vertices[3].Set(-1.2f, 1.2f);
    
    int32 count = 4;

    b2PolygonShape polygon;
    
    dynamicBox.Set(vertices, count);

    // Define the dynamic body fixture.
    b2FixtureDef fixtureDef;
    fixtureDef.shape = &dynamicBox;    
    fixtureDef.density = 4.0f; // trong luong
    fixtureDef.friction = 1.0f; //ma sat
    fixtureDef.restitution = 0;
    body->CreateFixture(&fixtureDef);
    
    _player->setMpBody(body);
    
//    GB2ShapeCache *sc = GB2ShapeCache::sharedGB2ShapeCache();
//    sc->addFixturesToBody(body, "ninja");
//    _player->setAnchorPoint(sc->anchorPointForShape("ninja"));
//    _player->setScale(0.5f);
    
    
//     _player = new PhysicsSprite();
//    _player->initWithFile("ninja_attack.png");
//    
//    _player->setPosition(p);
//    
//    this->addChild(_player);
//    
//	b2BodyDef bodyDef;
//	bodyDef.type = b2_dynamicBody;
//    
//	bodyDef.position.Set(p.x/PTM_RATIO, p.y/PTM_RATIO);
//	bodyDef.userData = _player;
//	b2Body *body = world->CreateBody(&bodyDef);
//    
//    GB2ShapeCache *sc = GB2ShapeCache::sharedGB2ShapeCache();
//    sc->addFixturesToBody(body, "ninja_attack");
//    _player->setAnchorPoint(sc->anchorPointForShape("ninja_attack"));
//    body->GetFixtureList()->SetDensity(5.0f);
//    body->GetFixtureList()->SetFriction(1.0f);
//    _player->setMpBody(body);
//    //khong co tac dung gi 
//    _player->setScale(0.1f);
//    CCScaleTo *scale = CCScaleTo::create(0, 0.5f);
//    _player->runAction(scale);
    
//    CCAnimation *anim=CCAnimation::create();
//    for (int i = 1; i <= 5; i++) {
//        char strname[20] = {0};
//        sprintf(strname, "%i.png", i);
//        string file_name = strname;
//        anim->addSpriteFrameWithFileName(file_name.c_str());
//    }
//    anim->setDelayPerUnit(2.8f / 19.0f);
//    anim->setRestoreOriginalFrame(true);
//    CCAnimate * animet=CCAnimate::create(anim);
//    CCRepeatForever * rep=CCRepeatForever::create(animet);
//    rep->setTag(123456);
////    _player->runAction(rep);
//    
//    CCSprite * attack = CCSprite::create("ninja_attack.png");
//    CCRotateTo * rotete = CCRotateTo::create(1, 180);
//    CCRotateTo * rotete2 = CCRotateTo::create(1, 360);
//    CCSequence * sq = CCSequence::create(rotete, rotete2, NULL);
//    CCRepeatForever * rep2=CCRepeatForever::create(sq);
//    attack->runAction(rep2);
////    _player->addChild(attack);
}

#pragma mark - update
void HelloWorld::update(float dt)
{
    //It is recommended that a fixed time step is used with Box2D for stability
    //of the simulation, however, we are using a variable time step here.
    //You need to make an informed choice, the following URL is useful
    //http://gafferongames.com/game-physics/fix-your-timestep/
    
    if (_player->getMpBody()) {
        _player->setPositionX(_player->getMpBody()->GetPosition().x * PTM_RATIO);
        _player->setPositionY(_player->getMpBody()->GetPosition().y * PTM_RATIO);
        //true thi _player khong xoay false thi xoay
        _player->getMpBody()->SetFixedRotation(true);
//        _player->getMpBody()->SetFixedRotation(false);
        
        if (_player->getMpBody()->GetPosition().y * PTM_RATIO > touchLocation.y &&
            giamVanToc == true && isTouchTop == true) {
            _player->setAttack(2);
            b2Vec2 v = _player->getMpBody()->GetLinearVelocity();
            CCLog("velocity x: %f", v.x);
            CCLog("velocity y: %f", v.y);
            float diffx = abs(_player->getMpBody()->GetPosition().x * PTM_RATIO - touchLocation.x);
            float diffy = abs(_player->getMpBody()->GetPosition().y * PTM_RATIO - touchLocation.y);
            CCLog("diff y / diff x: %f", v.y / v.x);
            if (abs(v.y / v.x) > 1 && abs(v.x) > 40) {
                if (abs(v.x) > 20) {
                    _player->getMpBody()->SetLinearVelocity(b2Vec2(v.x  /(diffx/15 + 1), v.y  /(diffy/50 + 1)));
                }else if (abs(v.y) > 12) {
                    _player->getMpBody()->SetLinearVelocity(b2Vec2(v.x  /(diffx/20 + 1), v.y  /(diffy/30 + 1)));
                }else {
                    _player->getMpBody()->SetLinearVelocity(b2Vec2(v.x  /(diffx/20 + 1), v.y  /(diffy/50 + 1)));
                }

            }else if (abs(v.y / v.x) < 1 && abs(v.x) < 20) {
                if (abs(v.x) > 20) {
                    _player->getMpBody()->SetLinearVelocity(b2Vec2(v.x  /(diffx/150 + 1), v.y  /(diffy/50 + 1)));
                }else if (abs(v.y) > 12) {
                    _player->getMpBody()->SetLinearVelocity(b2Vec2(v.x  /(diffx/200 + 1), v.y  /(diffy/30 + 1)));
                }else {
                    _player->getMpBody()->SetLinearVelocity(b2Vec2(v.x  /(diffx/200 + 1), v.y  /(diffy/50 + 1)));
                }
            }else if (abs(v.y / v.x) > 1  || abs(v.x) > 20){
                if (abs(v.x) > 20) {
                    _player->getMpBody()->SetLinearVelocity(b2Vec2(v.x  /(diffx/50 + 1), v.y  /(diffy/50 + 1)));
                }else if (abs(v.y) > 12) {
                    _player->getMpBody()->SetLinearVelocity(b2Vec2(v.x  /(diffx/100 + 1), v.y  /(diffy/30 + 1)));
                }else {
                    _player->getMpBody()->SetLinearVelocity(b2Vec2(v.x  /(diffx/100 + 1), v.y  /(diffy/50 + 1)));
                }
            }
            
            
            
            //dung yen khong quay ************ cam xoa doan nay 
//            _player->getMpBody()->SetLinearVelocity(b2Vec2(0, 0));
//            _player->getMpBody()->SetGravityScale(0);
//            
//            _player->getMpBody()->SetAngularVelocity(0);
//            //giam soc
//            _player->getMpBody()->SetLinearDamping(0);
//            _player->getMpBody()->se
            
            
            giamVanToc = false;
        }else if (_player->getMpBody()->GetPosition().y * PTM_RATIO < touchLocation.y &&
                  giamVanToc == true && isTouchTop == false) {
            _player->setAttack(2);
            b2Vec2 v = _player->getMpBody()->GetLinearVelocity();
            CCLog("velocity x: %f", v.x);
            CCLog("velocity y: %f", v.y);
            float diffx = abs(_player->getMpBody()->GetPosition().x * PTM_RATIO - touchLocation.x);
            float diffy = abs(_player->getMpBody()->GetPosition().y * PTM_RATIO - touchLocation.y);
            _player->getMpBody()->SetLinearVelocity(b2Vec2(v.x /(diffx/100 + 1), v.y /(diffy/30 + 1)));
            giamVanToc = false;
        }
        
    }
    
    this->setViewPointCenter(ccp(_player->getPosition().x ,
                                 _player->getPosition().y ));
    int velocityIterations = 8;
    int positionIterations = 1;
    
    // Instruct the world to perform a single step of simulation. It is
    // generally best to keep the time step and iterations fixed.
    world->Step(dt, velocityIterations, positionIterations);
    
    //Iterate over the bodies in the physics world
//    for (b2Body* b = world->GetBodyList(); b; b = b->GetNext())
//    {
//        if (b->GetUserData() != NULL) {
//            //Synchronize the AtlasSprites position and rotation with the corresponding body
//            CCSprite* myActor = (CCSprite*)b->GetUserData();
//            myActor->setPosition( CCPointMake( b->GetPosition().x * PTM_RATIO, b->GetPosition().y * PTM_RATIO) );
////            myActor->setRotation( -1 * CC_RADIANS_TO_DEGREES(b->GetAngle()) );
//            
//            if (b->GetPosition().y * PTM_RATIO > touchLocation.y) {
//                
//            }
//        }
//    }
    
    // Collision Detection
//    std::vector<MyContact>::iterator pos;
//    for(pos = _contactListener->_contacts.begin();
//        pos != _contactListener->_contacts.end(); ++pos) {
//        MyContact contact = *pos;
//        GameManager::sharedGameManager()->setNumberActionPlayer(GameManager::sharedGameManager()->getNumberActionPlayer() - 1);
//    }

    // Collision Detection ----------- contact wall ----------------------------
    //begin contact
    if (GameManager::sharedGameManager()->getBeginContact() == true) {
        CCAnimation *anim=CCAnimation::create();
        //top
        if (GameManager::sharedGameManager()->getDirectionContact() == 1) {
            anim->addSpriteFrameWithFileName("ninja_attack.png");
            anim->addSpriteFrameWithFileName("ninja.png");
        //bottom
        }else if (GameManager::sharedGameManager()->getDirectionContact() == 2) {
            anim->addSpriteFrameWithFileName("ninja_attack.png");
            anim->addSpriteFrameWithFileName("ninja.png");
            anim->addSpriteFrameWithFileName("ninja_attack.png");
            anim->addSpriteFrameWithFileName("ninja.png");
            _player->setFlipY(true);
            _player->getMpBody()->SetLinearVelocity(b2Vec2(0, 0));
            _player->getMpBody()->SetGravityScale(0);
            _player->getMpBody()->SetAngularVelocity(0);
            //giam soc
            _player->getMpBody()->SetLinearDamping(0);
        //left
        }else if (GameManager::sharedGameManager()->getDirectionContact() == 3) {
            anim->addSpriteFrameWithFileName("ninja2_bam_tuong.png");
            _player->setFlipX(true);
            _player->getMpBody()->SetLinearVelocity(b2Vec2(0, 0));
            _player->getMpBody()->SetGravityScale(0);
            _player->getMpBody()->SetAngularVelocity(0);
            //giam soc
            _player->getMpBody()->SetLinearDamping(0);
        //right
        }else if (GameManager::sharedGameManager()->getDirectionContact() == 4) {
            anim->addSpriteFrameWithFileName("ninja2_bam_tuong.png");
//            _player->setFlipX(true);
            _player->getMpBody()->SetLinearVelocity(b2Vec2(0, 0));
            _player->getMpBody()->SetGravityScale(0);
            _player->getMpBody()->SetAngularVelocity(0);
            //giam soc
            _player->getMpBody()->SetLinearDamping(0);
        }

        anim->setDelayPerUnit(2.8f / 4.0f);
        anim->setRestoreOriginalFrame(true);
        CCAnimate * animet=CCAnimate::create(anim);
        CCRepeatForever * rep=CCRepeatForever::create(animet);
        rep->setTag(123456);
        _player->getImage()->runAction(animet);
        GameManager::sharedGameManager()->setBeginContact(false);
    }
    //end contact
    if (GameManager::sharedGameManager()->getEndContact() == true) {
        _player->getMpBody()->SetGravityScale(1);
        GameManager::sharedGameManager()->setEndContact(false);
        CCAnimation *anim=CCAnimation::create();
        anim->addSpriteFrameWithFileName("ninja.png");
        anim->setDelayPerUnit(2.8f / 6.0f);
        anim->setRestoreOriginalFrame(true);
        CCAnimate * animet=CCAnimate::create(anim);
        _player->getImage()->runAction(animet);
        _player->getImage()->setFlipX(false);
        _player->getImage()->setFlipY(false);
    }
    
    
    //---------------------change direction ------------------------------------
    CCObject * i1;
    CCARRAY_FOREACH(_arraySnake, i1) {
        Snake *snake = (Snake*)i1;
        if (snake->getPosition().x > snake->getLocation().x) {
            snake->setDirection(1);
            snake->setFlipX(true);
        }else if (snake->getPosition().x < snake->getLocation().x){
            snake->setDirection(0);
            snake->setFlipX(false);
        }
    }
    
    CCObject * i2;
    CCARRAY_FOREACH(_arrayScorpion, i2) {
        Scorpion *scorpion = (Scorpion*)i2;
        if (scorpion->getPosition().x > scorpion->getLocation().x) {
            scorpion->setDirection(1);
            scorpion->setFlipX(true);
        }else if (scorpion->getPosition().x < scorpion->getLocation().x){
            scorpion->setDirection(0);
            scorpion->setFlipX(false);
        }

    }
    
    //--------------------contact with coins------------------------------------
    CCObject *i3 ;
    CCARRAY_FOREACH(_arrayCoin, i3) {
        Coin *coin = (Coin*)i3;
        float kc1 = _player->getImage()->getContentSize().width/2 + coin->getContentSize().width/2;
        float kc2 = ccpDistance(_player->getPosition(), coin->getPosition());
        if (kc2 <= kc1) {
            coin->actionMoveTop();
        }
    }
    //--------------------contact with snake------------------------------------
    CCObject *i4 ;
    CCARRAY_FOREACH(_arraySnake, i4) {
        Coin *snake = (Coin*)i4;
        float kc1 = _player->getImage()->getContentSize().width/2 + snake->getContentSize().width/2;
        float kc2 = ccpDistance(_player->getPosition(), snake->getPosition());
        if (kc2 <= kc1 && _player->getAttack() == 2) {
            _player->getMpBody()->SetLinearVelocity(b2Vec2(0, 5));
            _player->setAttack(0);
        }else if (kc2 <= kc1 && _player->getAttack() == 1) {
            
        }
    }
    //--------------------contact with Scorpion---------------------------------
    CCObject *i5 ;
    CCARRAY_FOREACH(_arrayScorpion, i5) {
        Scorpion *scorpion = (Scorpion*)i5;
        float kc1 = _player->getImage()->getContentSize().width/2 + scorpion->getContentSize().width/2;
        float kc2 = ccpDistance(_player->getPosition(), scorpion->getPosition());
        if (kc2 <= kc1 && _player->getAttack() == 2) {
            _player->getMpBody()->SetLinearVelocity(b2Vec2(0, 5));
            _player->setAttack(0);
        }else if (kc2 <= kc1 && _player->getAttack() == 1) {
            
        }
    }
    
    //-----------------phantom--------------------------------------------------
    if (_player->getAttack() == 1) {
        CCSprite * sprite = CCSprite::create("ninja.png");
        sprite->setPosition(_player->getPosition());
        CCFadeOut *fo = CCFadeOut::create(0.4f);
        CCCallFuncN *remove = CCCallFuncN::create(this,callfuncN_selector(HelloWorld::removeSprite));
        CCSequence * sq = CCSequence::create(fo, remove, NULL);
        sprite->runAction(sq);
        this->addChild(sprite, 1000);
    }

}
void HelloWorld::updateLocation_Direction(float dt) {
    CCObject * i1;
    CCARRAY_FOREACH(_arraySnake, i1) {
        Snake *snake = (Snake*)i1;
        snake->setLocation(snake->getPosition());
    }
    
    CCObject * i2;
    CCARRAY_FOREACH(_arrayScorpion, i2) {
        Scorpion *scorpion = (Scorpion*)i2;
        scorpion->setLocation(scorpion->getPosition());
    }
    
    
}
bool HelloWorld::ccTouchBegan(CCTouch *touch, CCEvent *event)
{
    if (GameManager::sharedGameManager()->getNumberActionPlayer() > 0)
    {
        _contactting = false;
        giamVanToc = true;
        touchLocation = touch->getLocationInView();
        touchLocation = CCDirector::sharedDirector()->convertToGL(touchLocation);
        touchLocation = this->convertToNodeSpace(touchLocation);
        if (_player->getMpBody()->GetPosition().y * PTM_RATIO < touchLocation.y )
            isTouchTop = true;
        else isTouchTop = false;
        CCSprite * t = CCSprite::create("Icon-72.png");
        CCScaleBy *scale = CCScaleBy::create(2, 0);
        CCHide *hide = CCHide::create();
        CCSequence * sq = CCSequence::create(scale, hide, NULL);
        t->runAction(sq);
        t->setPosition(touchLocation);
        this->addChild(t, 10);
        _player->setAttack(true);
        _player->getMpBody()->SetLinearVelocity(b2Vec2(0, 0));
        this->touch(touchLocation);
        
        GameManager::sharedGameManager()->setNumberActionPlayer(
                GameManager::sharedGameManager()->getNumberActionPlayer() - 1);
    }
    return true;
}

void HelloWorld::ccTouchEnded(CCSet* touches, CCEvent* event)
{
    //Add a new body/atlas sprite at the touched location
    CCSetIterator it;
    CCTouch* touch;
    
    for( it = touches->begin(); it != touches->end(); it++) 
    {
        touch = (CCTouch*)(*it);
        
        if(!touch)
            break;
        
        CCPoint location = touch->getLocationInView();
        
        location = CCDirector::sharedDirector()->convertToGL(location);
        
//        addNewSpriteAtPosition( location );
        this->touch(location);
    }
}
void HelloWorld::touch( CCPoint location)
{
    CCSize winSize = CCDirector::sharedDirector()->getWinSize();
    b2Vec2 currentVelocity = _player->getMpBody()->GetLinearVelocity();
    b2Vec2 impulse(0.0f,0.0f);
    
    {
        // apply impulse
//        impulse.y = 1300.0f * delta;
//        impulse.x = 30.0f * delta;
//        impulse.y = 16.0f;
//        impulse.x = 10.0f;
//        CCLOG("location x : %f", location.x);
//        CCLOG("location y : %f", location.y);
//        CCLOG("point x : %f", _player->getMpBody()->GetPosition().x * PTM_RATIO);
//        CCLOG("point y : %f", _player->getMpBody()->GetPosition().y * PTM_RATIO);
//        impulse.y = (location.y - _player->getMpBody()->GetPosition().y * PTM_RATIO)*4;
//        impulse.x = (location.x - _player->getMpBody()->GetPosition().x * PTM_RATIO)*3;
       
//        float k = 10.0f / abs(impulse.y - impulse.x);
//        impulse.y = impulse.y * k;
//        impulse.x = impulse.x * k;
        impulse.y = (location.y - _player->getMpBody()->GetPosition().y * PTM_RATIO) * 1.5f + 10;
        impulse.x = (location.x - _player->getMpBody()->GetPosition().x * PTM_RATIO) * 1.5f;
//        CCLog("im x %f", impulse.x);
//        CCLog("im y %f", impulse.y);
//        CCLog(" leght  %f", impulse.LengthSquared());
//        CCLog(" diff y / diff x  %f", impulse.y / impulse.x);
        if (abs(impulse.y / impulse.x) < 0.9f) {
            if (impulse.LengthSquared() > 100000 && impulse.LengthSquared() < 350000) {
                impulse.y = (location.y - _player->getMpBody()->GetPosition().y * PTM_RATIO) * 1.5f + 150 * (1-abs(impulse.y / impulse.x));
                impulse.x = (location.x - _player->getMpBody()->GetPosition().x * PTM_RATIO) * 1.5f;
            }else if (impulse.LengthSquared() < 100000) {
                impulse.y = (location.y - _player->getMpBody()->GetPosition().y * PTM_RATIO) * 2.0f + 150 * (1-abs(impulse.y / impulse.x));
                impulse.x = (location.x - _player->getMpBody()->GetPosition().x * PTM_RATIO) * 2.0f;
            }else if (impulse.LengthSquared() > 350000) {
                impulse.y = (location.y - _player->getMpBody()->GetPosition().y * PTM_RATIO) * 1.2f + 150 * (1-abs(impulse.y / impulse.x));
                impulse.x = (location.x - _player->getMpBody()->GetPosition().x * PTM_RATIO) * 1.2f;
            }
        }else {
            if (impulse.LengthSquared() > 100000 && impulse.LengthSquared() < 350000) {
                impulse.y = (location.y - _player->getMpBody()->GetPosition().y * PTM_RATIO) * 1.5f + 30;
                impulse.x = (location.x - _player->getMpBody()->GetPosition().x * PTM_RATIO) * 1.5f;
            }else if (impulse.LengthSquared() < 100000) {
                impulse.y = (location.y - _player->getMpBody()->GetPosition().y * PTM_RATIO) * 2.0f + 30;
                impulse.x = (location.x - _player->getMpBody()->GetPosition().x * PTM_RATIO) * 2.0f;
            }else if (impulse.LengthSquared() > 350000) {
                impulse.y = (location.y - _player->getMpBody()->GetPosition().y * PTM_RATIO) * 1.2f + 10;
                impulse.x = (location.x - _player->getMpBody()->GetPosition().x * PTM_RATIO) * 1.2f;
            }
        }
        
        
        if( location.x < (winSize.width * 0.5f) )
//            impulse.x = -impulse.x;
        b2Vec2 point((location.x - _player->getPositionX())/10, (location.y - _player->getPositionY())/10);
        _player->getMpBody()->ApplyLinearImpulse(impulse, _player->getMpBody()->GetWorldCenter());
        
//        world->GetGravity();
//        _player->getMpBody()->ApplyForce(impulse, _player->getMpBody()->GetWorldCenter());
//        _player->getMpBody()->ApplyForceToCenter(impulse);
//        _player->getMpBody()->ApplyLinearImpulse(impulse, point);
        
    }
}
#pragma mark - create map 
void HelloWorld::prepareLayers()
{
//    CCObject *object;
//    CCARRAY_FOREACH(this->_tileMap->getChildren(), object)
//    {
//        // is this map child a tile layer?
//        CCTMXLayer* layer = dynamic_cast<CCTMXLayer*>(object);
//        if( layer != NULL )
//            this->createFixtures(layer);
//    }
    this->createFixtures(_meta);
}

void HelloWorld::createFixtures(CCTMXLayer* layer)
{
    // create all the rectangular fixtures for each tile in the level
    CCSize layerSize = layer->getLayerSize();
    for( int y=0; y < layerSize.height; y++ )
    {
        for( int x=0; x < layerSize.width; x++ )
        {
            // create a fixture if this tile has a sprite
            CCSprite* tileSprite = layer->tileAt(ccp(x, y));
            if( tileSprite )
                this->createRectangularFixture(layer, x, y, 1.0f, 1.0f);
        }
    }
}

void HelloWorld::createRectangularFixture(CCTMXLayer* layer, int x, int y,
                                     float width, float height)
{
    // get position & size
    CCPoint p = layer->positionAt(ccp(x,y));
    CCSize tileSize = this->_tileMap->getTileSize();
    const float pixelsPerMeter = 32.0f;
    
    // create the body
    b2BodyDef bodyDef;
    bodyDef.type = b2_staticBody;
    bodyDef.position.Set((p.x + (tileSize.width / 2.0f)) / pixelsPerMeter,
                         (p.y + (tileSize.height / 2.0f)) / pixelsPerMeter);
    b2Body* body = world->CreateBody(&bodyDef);
    
    // define the shape
    b2PolygonShape shape;
    shape.SetAsBox((tileSize.width / pixelsPerMeter) * 0.5f * width,
                   (tileSize.width / pixelsPerMeter) * 0.5f * height);
    
    // create the fixture
    b2FixtureDef fixtureDef;
    fixtureDef.shape = &shape;
    fixtureDef.density = 1.0f;
    fixtureDef.friction = 1.0f;
    fixtureDef.restitution = 0.0f;
//    fixtureDef.filter.categoryBits = kFilterCategoryLevel;
    fixtureDef.filter.maskBits = 0xffff;
    body->CreateFixture(&fixtureDef);
}
#pragma mark - handle touches

void HelloWorld::registerWithTouchDispatcher()
{
//    CCDirector* pDirector = CCDirector::sharedDirector();
//    pDirector->getTouchDispatcher()->addTargetedDelegate(this, -10, true);
    CCDirector::sharedDirector()->getTouchDispatcher()->addTargetedDelegate(this, 0, true);
}
void HelloWorld::setViewPointCenter(CCPoint position)
{
    
//    CCSize winSize = CCDirector::sharedDirector()->getWinSize();
//    
//    int x = MAX(position.x, winSize.width/2);
//    int y = MAX(position.y, winSize.height/2);
//    x = MIN(x, (_tileMap->getMapSize().width * this->_tileMap->getTileSize().width) - winSize.width / 2);
//    y = MIN(y, (_tileMap->getMapSize().height * _tileMap->getTileSize().height) - winSize.height/2);
//    CCPoint actualPosition = ccp(x, y);
//    
//    CCPoint centerOfView = ccp(winSize.width/2, winSize.height/2);
//    CCPoint viewPoint = ccpSub(centerOfView, actualPosition);
//    if (this->getPositionX() != 0 || this->getPositionY() != 160) {
////        CCLog("this x: %f", this->getPositionX());
////        CCLog("this v: %f", this->getPositionY());
//    }
//    
//    this->setPosition(viewPoint);
    
    
    CCSize winSize = CCDirector::sharedDirector()->getWinSize();
    
    int x = MAX(position.x, winSize.width/2);
    int y = MAX(position.y, winSize.height/2);
    x = MIN(x, (_tileMap->getMapSize().width * this->_tileMap->getTileSize().width) - winSize.width / 2);
    y = MIN(y, (_tileMap->getMapSize().height * _tileMap->getTileSize().height) - winSize.height/2);
    CCPoint actualPosition = ccp(x, y);
    
    CCPoint centerOfView = ccp(winSize.width/2, winSize.height/2);
    CCPoint viewPoint = ccpSub(centerOfView, actualPosition);
    this->setPosition(viewPoint);
}
CCPoint HelloWorld::tileCoordForPosition(CCPoint position)
{
    int x = position.x / _tileMap->getTileSize().width;
    int y = ((_tileMap->getMapSize().height * _tileMap->getTileSize().height) - position.y) / _tileMap->getTileSize().height;
    return ccp(x, y);
}
CCScene* HelloWorld::scene()
{
    // 'scene' is an autorelease object
    CCScene *scene = CCScene::create();
    
    // add layer as a child to scene
    CCLayer* layer = new HelloWorld();
    scene->addChild(layer);
    layer->release();
    
    return scene;
}
#pragma mark - add object 
void HelloWorld::addCoins() {
    CCSize layerSize = _coin->getLayerSize();
    for( int y=0; y < layerSize.height; y++ )
    {
        for( int x=0; x < layerSize.width; x++ )
        {
            // create a fixture if this tile has a sprite
            CCSprite* tileSprite = _coin->tileAt(ccp(x, y));
            if( tileSprite ) {
                CCPoint p = convertPoitMapToPixel(ccp(tileSprite->getPosition().x / PTM_RATIO,
                                                      tileSprite->getPosition().y / PTM_RATIO));
                Coin * coin =  new Coin();
                coin->initWithFile("coin.png");
                coin->setPosition(p);
                coin->action();
                _arrayCoin->addObject(coin);
                this->addChild(coin, 10000);
            }
                
        }
    }
}
void HelloWorld::addSnakes() {
    CCSize layerSize = _snake->getLayerSize();
    for( int y=0; y < layerSize.height; y++ )
    {
        for( int x=0; x < layerSize.width; x++ )
        {
            // create a fixture if this tile has a sprite
            CCSprite* tileSprite = _snake->tileAt(ccp(x, y));
            if( tileSprite ) {
                CCPoint p = convertPoitMapToPixel(ccp(tileSprite->getPosition().x / PTM_RATIO,
                                                      tileSprite->getPosition().y / PTM_RATIO));
                Snake * snake =  new Snake();
                snake->initWithFile("monter01.png");
                snake->setPosition(p);
                snake->actionMoveToPoint(CCPoint(p.x + 200, p.y));
                _arraySnake->addObject(snake);
                this->addChild(snake, 10000);
            }
            
        }
    }
}
void HelloWorld::addScorpions() {
    CCSize layerSize = _scorpion->getLayerSize();
    for( int y=0; y < layerSize.height; y++ )
    {
        for( int x=0; x < layerSize.width; x++ )
        {
            // create a fixture if this tile has a sprite
            CCSprite* tileSprite = _scorpion->tileAt(ccp(x, y));
            if( tileSprite ) {
                CCPoint p = convertPoitMapToPixel(ccp(tileSprite->getPosition().x / PTM_RATIO,
                                                      tileSprite->getPosition().y / PTM_RATIO));
                Scorpion * scorpion =  new Scorpion();
                scorpion->initWithFile("monter02.png");
                scorpion->setPosition(p);
                int i = rand() % 2;
                if (i == 0) {
                    scorpion->fluctuating(30);
                }
                if(i == 1)
                    scorpion->fluctuatingAndMove(30, 200);
                
                _arrayScorpion->addObject(scorpion);
                this->addChild(scorpion, 10000);
            }
            
        }
    }
}
#pragma mark - convert point
CCPoint HelloWorld::convertMetterToPixel(CCPoint p) {
    return CCPoint(p.x * PTM_RATIO, p.y * PTM_RATIO);
}
CCPoint HelloWorld::convertPixelToMetter(cocos2d::CCPoint p) {
    return CCPoint(p.x / PTM_RATIO, p.y / PTM_RATIO);
}
CCPoint HelloWorld::convertPoitMapToPixel(cocos2d::CCPoint pointMap) {
    float W = CCDirector::sharedDirector()->getWinSize().width;
    float H = CCDirector::sharedDirector()->getWinSize().height;
    float X = _tileMap->getMapSize().width;
    float Y = _tileMap->getMapSize().height;
    float x = _tileMap->getTileSize().width;
    float y = _tileMap->getTileSize().height;
    return CCPoint(x * 0.5f + x * pointMap.x,H - ((Y - pointMap.y) * y - y * 0.5f));
}
void HelloWorld::removeSprite(cocos2d::CCNode *node) {
    CCSprite * sp = (CCSprite*)node;
    sp->removeFromParentAndCleanup(true);
}