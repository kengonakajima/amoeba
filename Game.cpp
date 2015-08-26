//
// Game.cpp -
//
#include "pch.h"

#include "DDSTextureLoader.h"
#include "CommonStates.h"
#include <assert.h>

#include "MMDeviceapi.h"



#include "Phys.h"
#include "Game.h"




///////////////////
// Utils from cumino

void print( const char *fmt, ... ){
    char dest[1024*16];
    va_list argptr;
    va_start( argptr, fmt );
    vsnprintf( dest, sizeof(dest), fmt, argptr );
    va_end( argptr );
    fprintf( stderr, "%s\n", dest );
#ifdef WIN32
	OutputDebugStringA(dest);
	OutputDebugStringA("\n");
#endif
}




// Constructor.
Game::Game() :
    m_window(0),
    m_featureLevel( D3D_FEATURE_LEVEL_11_1 ),
	m_framecnt(0)
{
    for(int i=0;i<MAX_PLAYER_NUM;i++) m_players[i] = nullptr;
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window)
{
    m_window = window;

	
    CreateDevice();

    CreateResources();

    // TODO: Change the timer settings if you want something other than the default variable timestep mode.
    // e.g. for 60 FPS fixed timestep update logic, call:
    /*
    m_timer.SetFixedTimeStep(true);
    m_timer.SetTargetElapsedSeconds(1.0 / 60);
    */
    
    // chipmunk
    InitGameWorld();
}

void Game::InitGameWorld() {

    m_space = cpSpaceNew();

	cpSpaceSetIterations(m_space, 10);
	cpSpaceSetGravity(m_space, cpv(0, -10));
	cpSpaceSetCollisionSlop(m_space, 2.0);

	cpBody *staticBody = cpSpaceGetStaticBody( m_space);
	cpShape *shape;

	// Create segments around the edge of the screen.
    size_t w,h;
    GetDefaultSize(w,h);

    float mgn=10;
    float minx = -(float)w/2+mgn, maxx = w/2-mgn, miny = -(float)h/2+mgn, maxy = h/2-mgn;
	shape = cpSpaceAddShape(m_space, cpSegmentShapeNew(staticBody, cpv(minx,miny), cpv(minx, maxy), 20.0f)); // left
	cpShapeSetElasticity(shape, 1.0f);
	cpShapeSetFriction(shape, 1.0f);
	cpShapeSetFilter(shape, NOT_GRABBABLE_FILTER);

	shape = cpSpaceAddShape(m_space, cpSegmentShapeNew(staticBody, cpv( maxx,miny), cpv( maxx, maxy), 20.0f)); // right
	cpShapeSetElasticity(shape, 1.0f);
	cpShapeSetFriction(shape, 1.0f);
	cpShapeSetFilter(shape, NOT_GRABBABLE_FILTER);

	shape = cpSpaceAddShape(m_space, cpSegmentShapeNew(staticBody, cpv(minx,miny), cpv( maxx,miny), 20.0f)); // bottom
	cpShapeSetElasticity(shape, 1.0f);
	cpShapeSetFriction(shape, 1.0f);
	cpShapeSetFilter(shape, NOT_GRABBABLE_FILTER);
	
	shape = cpSpaceAddShape(m_space, cpSegmentShapeNew(staticBody, cpv(minx, maxy), cpv( maxx, maxy), 20.0f)); // top
	cpShapeSetElasticity(shape, 1.0f);
	cpShapeSetFriction(shape, 1.0f);
	cpShapeSetFilter(shape, NOT_GRABBABLE_FILTER);


	
	cpCollisionHandler *handler = cpSpaceAddWildcardHandler(m_space, COLLISION_TYPE_STICKY);
	handler->preSolveFunc = StickyPreSolve;
	handler->separateFunc = StickySeparate;
}
cpBody *Game::CreateCellBody( cpVect pos, BodyState *bs, bool is_eye ) {
    cpFloat mass = 0.1f, eye_mass = 10.0f;
    cpFloat radius = CELL_RADIUS;

    if( is_eye ) mass = eye_mass;

    
    cpBody *body = cpSpaceAddBody(m_space, cpBodyNew( mass, cpMomentForCircle(mass, 0.0f, radius, cpvzero)));

    cpBodySetPosition(body, pos);
    cpBodySetUserData(body, bs);

    cpShape *shape = cpSpaceAddShape(m_space, cpCircleShapeNew(body, radius + STICK_SENSOR_THICKNESS, cpvzero));
    cpShapeSetFriction(shape, 0.95f);
    cpShapeSetCollisionType(shape, COLLISION_TYPE_STICKY);

    bs->radius = radius;
    bs->hp = BODY_MAXHP * cpflerp( 0.6, 1.0, frand() );

    return body;
}

// Executes basic game loop.
void Game::Tick()
{
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();
}



//////////////////////

static void ShapeFreeWrap(cpSpace *space, cpShape *shape, void *unused){
	cpSpaceRemoveShape(space, shape);
	cpShapeFree(shape);
}

static void PostShapeFree(cpShape *shape, cpSpace *space){
	cpSpaceAddPostStepCallback(space, (cpPostStepFunc)ShapeFreeWrap, shape, NULL);
}

static void ConstraintFreeWrap(cpSpace *space, cpConstraint *constraint, void *unused){
	cpSpaceRemoveConstraint(space, constraint);
	cpConstraintFree(constraint);
}

static void PostConstraintFree(cpConstraint *constraint, cpSpace *space){
	cpSpaceAddPostStepCallback(space, (cpPostStepFunc)ConstraintFreeWrap, constraint, NULL);
}

static void BodyFreeWrap(cpSpace *space, cpBody *body, void *unused){
	cpSpaceRemoveBody(space, body);
    BodyState *bs = (BodyState*) cpBodyGetUserData(body);
    delete bs;
    cpBodySetUserData(body,NULL);
	cpBodyFree(body);
}

static void PostBodyFree(cpBody *body, cpSpace *space){
	cpSpaceAddPostStepCallback(space, (cpPostStepFunc)BodyFreeWrap, body, NULL);
}


void eachShapeDeleteCallback( cpBody *body, cpShape *shape, void *data ) {
    //    print("eachShapeDeleteCallback shape:%p body:%p", shape, body );
    Game *game = (Game*) data;
    PostShapeFree( shape, game->GetSpace());
}
void eachConstraintDeleteCallback( cpBody *body, cpConstraint *ct, void *data ) {
    //    print("eachConstraintDeleteCallback: ct:%p body:%p", ct, body );
    Game *game = (Game*) data;
    PostConstraintFree( ct, game->GetSpace() );
}


void eachBodyGameUpdateCallback( cpBody *body, void *data ) {
    BodyState *bs = (BodyState*) cpBodyGetUserData(body);
    Game *game = (Game*) data;
    Player *pl = game->GetPlayer( bs->group_id );
    if(!pl)return; 
    
    pl->IncrementCellCount();
    
    if( bs->eye_id < 0 ) {
        bs->hp -= HP_CONSUME_SPEED;
        if( bs->hp < 0 ) {
            //            cpSpaceAddPostStepCallback( game->GetSpace(), postStepRemoveBodyCallback, body, game );
            print("eachBodyGameUpdateCallback: removing body:%p", body );
            cpBodyEachShape(body, eachShapeDeleteCallback, data );
            cpBodyEachConstraint(body, eachConstraintDeleteCallback, data );
            PostBodyFree( body, game->GetSpace());
            return;
        }
    } else {
        float scl = 10000;
        cpVect f;
        if( bs->eye_id == 0 ) {
            XMFLOAT2 lf = pl->GetLeftForce();
            f = cpv( lf.x * scl, lf.y * scl );
            bs->force = len( lf.x, lf.y );
            cpBodySetForce( body, f );        
        } else {
            XMFLOAT2 rf = pl->GetRightForce();            
            f = cpv( rf.x * scl, rf.y * scl );
            bs->force = len( rf.x, rf.y );
            cpBodySetForce( body, f );        
        }

        // Eyes keep max HP 
        bs->hp = BODY_MAXHP;
        //    cpVect v = cpBodyGetVelocity( body );
        //    cpBodySetVelocity( body, cpv( v.x + f.x/1000, v.y + f.y/1000) );        
    }

}

void eachConstraintGameUpdateCallback( cpConstraint *ct, void *data )  {
    cpBody *bodyA = cpConstraintGetBodyA(ct);
    cpBody *bodyB = cpConstraintGetBodyB(ct);

    BodyState *bsA = (BodyState*) cpBodyGetUserData(bodyA);
    BodyState *bsB = (BodyState*) cpBodyGetUserData(bodyB);
    {
        if( bsA->group_id == bsB->group_id ) {
            // exchange hp
            float total = bsA->hp + bsB->hp;
            float average = total / 2.0f;
            bsA->hp = bsB->hp = average;
        }
    }
}

// Updates the world
void Game::Update(DX::StepTimer const& timer)
{
    float elapsedTime = float(timer.GetElapsedSeconds());

    // TODO: Add your game logic here
    elapsedTime;

    for(int i=0;i<MAX_PLAYER_NUM;i++) {
        Player *pl = GetPlayer(i);
        if(pl) pl->Update(elapsedTime);
    }

    PhysUpdateSpace( m_space, elapsedTime );

    // read joystick
#if 0    
    print( "js: %.1f %.1f %.1f %.1f %d %d %d %d",
           state.thumbSticks.leftX, state.thumbSticks.leftY,
           state.thumbSticks.rightX, state.thumbSticks.rightY,           
           state.IsAPressed(), state.IsBPressed(), state.IsXPressed(), state.IsYPressed() );
#endif
    for(int i=0;i<MAX_PLAYER_NUM;i++) {
        auto state = GetGamePadState(i);      
        if( state.IsConnected()) {
            Player *pl = GetPlayer(i);
            if(pl) pl->SetForce( XMFLOAT2( state.thumbSticks.leftX, state.thumbSticks.leftY),
                                 XMFLOAT2( state.thumbSticks.rightX, state.thumbSticks.rightY)
                                 );
        }
    }
    for(int i=0;i<MAX_PLAYER_NUM;i++) {
        Player *pl = GetPlayer(i);
        if( pl ) pl->ResetCellCount();
    }
    cpSpaceEachConstraint( m_space, eachConstraintGameUpdateCallback, this );
    cpSpaceEachBody( m_space, eachBodyGameUpdateCallback, (void*) this );
    for(int i=0;i<MAX_PLAYER_NUM;i++) {
        Player *pl = GetPlayer(i);
        if( pl && pl->GetCellCount() == 2 ) {
            pl->ResetCells();
        }
    }

}

void Player::ResetCells() {
    cpBody *le = m_eyes[0];
    cpBody *re = m_eyes[1];
    cpVect lv = cpBodyGetPosition(le);
    cpVect rv = cpBodyGetPosition(re);
    float dia;
    cpVect center = Game::GetPlayerDefaultPosition( group_id, &dia );
    for(int i=0;i<BODY_CELL_NUM_PER_PLAYER;i++) {
        cpVect p = cpv( cpflerp( center.x-dia, center.x+dia, frand() ), cpflerp( center.y-dia, center.y+dia, frand() ) );
        BodyState *bs = new BodyState( CELL_PRIO_LOW, group_id, -1, game );
        cpBody *body = game->CreateCellBody( p, bs, false );
        cpBodySetUserData(body,bs);
    }
}
void Player::CleanCells() {
    game->CleanGroup( group_id );
}


// Draws the scene
void DrawCircle( PrimitiveBatch<VertexPositionColor> *pb, XMFLOAT3 pos, float dia, XMFLOAT4 col ) {
    const int N = 32;
    VertexPositionColor vertices[N+1];

    for(int i=0;i<N+1;i++) {
        float rad = 2.0f * M_PI * (float)i / (float)N;
        if(i%2==0) {
            vertices[N-i] = VertexPositionColor( XMFLOAT3( pos.x+cos(rad)*dia, pos.y+sin(rad)*dia, 0 ), col );
        } else {
            vertices[N-i] = VertexPositionColor( pos, col );
        }
    }
    pb->Draw( D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, vertices, N+1 );
}


void Player::DrawBody( cpBody *body ) {
    BodyState *bs = (BodyState*) cpBodyGetUserData(body);

    cpVect cppos = cpBodyGetPosition(body);


    // draw
    size_t scrw, scrh;
    game->GetDefaultSize(scrw,scrh);
    XMFLOAT3 dxtkPos( cppos.x+scrw/2, - cppos.y + scrh/2, 0 );
    XMFLOAT4 groupCol = Game::GetPlayerColor( bs->group_id );
    if( bs->eye_id >= 0 ) {
        XMFLOAT4 eyeCol = XMFLOAT4( bs->force,0.2,0.2,1);
        DrawCircle( GetPrimBatch(), dxtkPos, bs->radius, groupCol );
        DrawCircle( GetPrimBatch(), dxtkPos, bs->radius-3, eyeCol );
    } else {        
        float hprate = bs->GetHPRate();
        XMFLOAT4 cellCol( groupCol.x * hprate, groupCol.y * hprate, groupCol.z * hprate, 1.0f );
        
        DrawCircle( GetPrimBatch(), dxtkPos, bs->radius, cellCol );
    }
    
    
}
void eachBodyPushCallback( cpBody *body, void *data ) {
    std::vector<cpBody*> *v = (std::vector<cpBody*>*)data;
    v->push_back(body);
}
void SortBodiesById( std::vector<cpBody*> &vec ) {
    SorterEntry ents[1024];
    assert( vec.size() <= 1024 );
    for(unsigned int i=0; i<vec.size();i++ ) {
        ents[i].ptr = vec[i];
        BodyState *bs = (BodyState*) cpBodyGetUserData( vec[i] );
        ents[i].val = bs->draw_priority;
    }
    quickSortF( ents, 0, vec.size()-1);    
    for(unsigned int i=0;i<vec.size();i++) {
        vec[i] = (cpBody*) ents[i].ptr;
    }    
}
void Game::Render()
{
	m_framecnt++;
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
        return;

    for(int i=0;i<MAX_PLAYER_NUM;i++) {
        Player *pl = GetPlayer(i);
        if(pl) pl->Render();
    }

    Present();    
}

void Player::Render() {
    
    Clear();

    CommonStates states(game->GetD3DDevice().Get());
    // Background
	m_spriteBatch->Begin( SpriteSortMode_Deferred, states.NonPremultiplied() );
    RECT bgrect;
    bgrect.left = 0;
    bgrect.top = 0;
    size_t w,h;
    game->GetDefaultSize( w,h);
    bgrect.right = w;
    bgrect.bottom = h;
    
    m_spriteBatch->Draw( m_bgTex.Get(), XMFLOAT2(0,0), &bgrect, DirectX::Colors::Gray,
                         0, /* rot*/
                         XMFLOAT2(0,0), /*origin*/
                         XMFLOAT2(2,2), /*scale*/
                         DirectX::SpriteEffects_None,
                         0 /*depth*/
                         );
    //        batch->Draw( mTexture.Get(), screenPos, &sourceRect, DirectX::Colors::White,
    //                     mRotation, mOrigin, mScale, DirectX::SpriteEffects_None, mDepth );
    
    m_spriteBatch->End();
    
    
    // Prims

    m_d3dContext->OMSetBlendState( states.Opaque(), nullptr, 0xffffffff );
    m_d3dContext->OMSetDepthStencilState( states.DepthNone(), 0 );
    m_d3dContext->RSSetState( states.CullCounterClockwise() );

    m_basicEffect->Apply( m_d3dContext );
    m_d3dContext->IASetInputLayout( m_inputLayout.Get() );

    m_primBatch->Begin();
#if 0    
    m_primBatch->DrawLine( VertexPositionColor( XMFLOAT3(10,10,0), XMFLOAT4(1,0,1,1)),
                           VertexPositionColor( XMFLOAT3(100,200,0), XMFLOAT4(1,1,0,1)) );
#endif
#if 0    
    XMFLOAT4 col = GetPlayerColor( irange(0,MAX_PLAYER_NUM) );
    XMFLOAT3 cirpos( range(0,500), range(0,300),0 );
    
    DrawCircle( m_primBatch, cirpos, range(10,50), col );
    DrawCircle( m_primBatch, XMFLOAT3(cirpos.x+50,cirpos.y+50,0), range(10,50), col );    
#endif

    // cells
    std::vector<cpBody*> to_sort_body;
    cpSpaceEachBody( game->GetSpace(), eachBodyPushCallback, (void*) &to_sort_body );
    SortBodiesById( to_sort_body );
    for(unsigned int i=0;i<to_sort_body.size();i++){
        cpBody *body = to_sort_body[i];
        DrawBody(body);
    }
    m_primBatch->End();


    // Sprites
    
	m_spriteBatch->Begin( SpriteSortMode_Deferred, states.NonPremultiplied() );

	TCHAR statmsg[100];
	wsprintf(statmsg, L"Frame: %d", game->GetFrameCnt());
	m_spriteFont->DrawString(m_spriteBatch, statmsg, XMFLOAT2(10, 10));

    for(int i=0;i<MAX_PLAYER_NUM;i++) {
        Player *pl = game->GetPlayer(i);
        if(pl) {
            TCHAR nummsg[100];
            wsprintf( nummsg, L"P%d:%d", i, pl->GetCellCount() - 2 );
            DirectX::FXMVECTOR color = DirectX::Colors::White;
            m_spriteFont->DrawString( m_spriteBatch, nummsg, XMFLOAT2(300 + i * 150 ,10), color );
        }
    }
#if 0
	m_spriteFont->DrawString(m_spriteBatch, L"Skeleton code for 1:1 games", XMFLOAT2(100, 100) );
	m_spriteFont->DrawString(m_spriteBatch, L"Press P to play sound effect", XMFLOAT2(130, 160));
	m_spriteFont->DrawString(m_spriteBatch, L"Press Q to quit", XMFLOAT2(130, 200));
#endif

#if 0    
    XMFLOAT2 pos(100,100);
    m_cellAnimTex->Draw( m_spriteBatch, pos );
#endif    
                         
	m_spriteBatch->End();
    
}

// Helper method to clear the backbuffers
void Game::Clear()
{
    for(int i=0;i<MAX_PLAYER_NUM;i++) {
        Player *pl = GetPlayer(i);
        if(pl) pl->Clear();
    }
}
void Player::Clear() {
    // Clear the views
	float d = 100.0f;
	int m = 25;
	float r = (rand() % m) / d, g = (rand() % m) /d, b = (rand() % m) / d;
	float col[4] = { r,g,b,1 };
    m_d3dContext->ClearRenderTargetView( game->GetRenderTargetView().Get(), col );
    m_d3dContext->ClearDepthStencilView( game->GetDepthStencilView().Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
    m_d3dContext->OMSetRenderTargets(1, game->GetRenderTargetView().GetAddressOf(), game->GetDepthStencilView().Get());    
}

// Presents the backbuffer contents to the screen
void Game::Present()
{
    // The first argument instructs DXGI to block until VSync, putting the application
    // to sleep until the next VSync. This ensures we don't waste any cycles rendering
    // frames that will never be displayed to the screen.
    HRESULT hr = m_swapChain->Present(1, 0);

    // If the device was reset we must completely reinitialize the renderer.
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        OnDeviceLost();
    }
    else
    {
        DX::ThrowIfFailed(hr);
    }
}

// Message handlers
void Game::OnActivated()
{
    // TODO: Game is becoming active window
}

void Game::OnDeactivated()
{
    // TODO: Game is becoming background window
}

void Game::OnSuspending()
{
    // TODO: Game is being power-suspended (or minimized)
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();

    // TODO: Game is being power-resumed (or returning from minimize)
}

void Game::OnWindowSizeChanged()
{
    CreateResources();

    // TODO: Game window is being resized
}

// Properties
void Game::GetDefaultSize(size_t& width, size_t& height)
{
    // TODO: Change to desired default window size (note minimum size is 320x200)
    width = 800;
    height = 600;
}

// These are the resources that depend on the device.
void Game::CreateDevice()
{
    // This flag adds support for surfaces with a different color channel ordering than the API default.
    UINT creationFlags = 0;

#ifdef _DEBUG
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    static const D3D_FEATURE_LEVEL featureLevels [] =
    {
        // TODO: Modify for supported Direct3D feature levels (see code below related to 11.1 fallback handling)
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1,
    };

    // Create the DX11 API device object, and get a corresponding context.
    HRESULT hr = D3D11CreateDevice(
        nullptr,                                // specify null to use the default adapter
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,                                // leave as nullptr unless software device
        creationFlags,                          // optionally set debug and Direct2D compatibility flags
        featureLevels,                          // list of feature levels this app can support
        _countof(featureLevels),                // number of entries in above list
        D3D11_SDK_VERSION,                      // always set this to D3D11_SDK_VERSION
        m_d3dDevice.ReleaseAndGetAddressOf(),   // returns the Direct3D device created
        &m_featureLevel,                        // returns feature level of device created
        m_d3dContext.ReleaseAndGetAddressOf()   // returns the device immediate context
        );

    if ( hr == E_INVALIDARG )
    {
        // DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
        hr = D3D11CreateDevice( nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
                                creationFlags, &featureLevels[1], _countof(featureLevels) - 1,
                                D3D11_SDK_VERSION, m_d3dDevice.ReleaseAndGetAddressOf(),
                                &m_featureLevel, m_d3dContext.ReleaseAndGetAddressOf() );
    }

    DX::ThrowIfFailed(hr);

#ifndef NDEBUG
    ComPtr<ID3D11Debug> d3dDebug;
    hr = m_d3dDevice.As(&d3dDebug);
    if (SUCCEEDED(hr))
    {
        ComPtr<ID3D11InfoQueue> d3dInfoQueue;
        hr = d3dDebug.As(&d3dInfoQueue);
        if (SUCCEEDED(hr))
        {
#ifdef _DEBUG
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
#endif
            D3D11_MESSAGE_ID hide [] =
            {
                D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
                // TODO: Add more message IDs here as needed 
            };
            D3D11_INFO_QUEUE_FILTER filter;
            memset(&filter, 0, sizeof(filter));
            filter.DenyList.NumIDs = _countof(hide);
            filter.DenyList.pIDList = hide;
            d3dInfoQueue->AddStorageFilterEntries(&filter);
        }
    }
#endif

    // TODO: Initialize device dependent objects here (independent of window size)
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateResources()
{
    // Clear the previous window size specific context.
    ID3D11RenderTargetView* nullViews [] = { nullptr };
    m_d3dContext->OMSetRenderTargets(_countof(nullViews), nullViews, nullptr);
    m_renderTargetView.Reset();
    m_depthStencilView.Reset();
    m_d3dContext->Flush();

    RECT rc;
    GetWindowRect( m_window, &rc );

    UINT backBufferWidth = std::max<UINT>( rc.right - rc.left, 1 );
    UINT backBufferHeight = std::max<UINT>( rc.bottom - rc.top, 1);
    DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
    DXGI_FORMAT depthBufferFormat = (m_featureLevel >= D3D_FEATURE_LEVEL_10_0) ? DXGI_FORMAT_D32_FLOAT : DXGI_FORMAT_D16_UNORM;

    // If the swap chain already exists, resize it, otherwise create one.
    if (m_swapChain)
    {
        HRESULT hr = m_swapChain->ResizeBuffers(2, backBufferWidth, backBufferHeight, backBufferFormat, 0);

        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            // If the device was removed for any reason, a new device and swap chain will need to be created.
            OnDeviceLost();

            // Everything is set up now. Do not continue execution of this method. OnDeviceLost will reenter this method 
            // and correctly set up the new device.
            return;
        }
        else
        {
            DX::ThrowIfFailed(hr);
        }
    }
    else
    {
        // First, retrieve the underlying DXGI Device from the D3D Device
        ComPtr<IDXGIDevice1> dxgiDevice;
        DX::ThrowIfFailed(m_d3dDevice.As(&dxgiDevice));

        // Identify the physical adapter (GPU or card) this device is running on.
        ComPtr<IDXGIAdapter> dxgiAdapter;
        DX::ThrowIfFailed(dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf()));

        // And obtain the factory object that created it.
        ComPtr<IDXGIFactory1> dxgiFactory;
        DX::ThrowIfFailed(dxgiAdapter->GetParent(__uuidof(IDXGIFactory1), &dxgiFactory));

        ComPtr<IDXGIFactory2> dxgiFactory2;
        HRESULT hr = dxgiFactory.As(&dxgiFactory2);
        if (SUCCEEDED(hr))
        {
            // DirectX 11.1 or later
            m_d3dDevice.As( &m_d3dDevice1 );
            m_d3dContext.As( &m_d3dContext1 );

            // Create a descriptor for the swap chain.
            DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
            swapChainDesc.Width = backBufferWidth;
            swapChainDesc.Height = backBufferHeight;
            swapChainDesc.Format = backBufferFormat;
            swapChainDesc.SampleDesc.Count = 1;
            swapChainDesc.SampleDesc.Quality = 0;
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDesc.BufferCount = 2;

            DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = { 0 };
            fsSwapChainDesc.Windowed = TRUE;

            // Create a SwapChain from a CoreWindow.
            DX::ThrowIfFailed( dxgiFactory2->CreateSwapChainForHwnd(
                m_d3dDevice.Get(), m_window, &swapChainDesc,
                &fsSwapChainDesc,
                nullptr, m_swapChain1.ReleaseAndGetAddressOf() ) );

            m_swapChain1.As( &m_swapChain );
        }
        else
        {
            DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };
            swapChainDesc.BufferCount = 2;
            swapChainDesc.BufferDesc.Width = backBufferWidth;
            swapChainDesc.BufferDesc.Height = backBufferHeight;
            swapChainDesc.BufferDesc.Format = backBufferFormat;
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDesc.OutputWindow = m_window;
            swapChainDesc.SampleDesc.Count = 1;
            swapChainDesc.SampleDesc.Quality = 0;
            swapChainDesc.Windowed = TRUE;

            DX::ThrowIfFailed( dxgiFactory->CreateSwapChain( m_d3dDevice.Get(), &swapChainDesc, m_swapChain.ReleaseAndGetAddressOf() ) );
        }

        // This template does not support 'full-screen' mode and prevents the ALT+ENTER shortcut from working
        dxgiFactory->MakeWindowAssociation(m_window, DXGI_MWA_NO_ALT_ENTER);
    }

    // Obtain the backbuffer for this window which will be the final 3D rendertarget.
    ComPtr<ID3D11Texture2D> backBuffer;
    DX::ThrowIfFailed(m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBuffer));

    // Create a view interface on the rendertarget to use on bind.
    DX::ThrowIfFailed(m_d3dDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, m_renderTargetView.ReleaseAndGetAddressOf()));

    // Allocate a 2-D surface as the depth/stencil buffer and
    // create a DepthStencil view on this surface to use on bind.
    CD3D11_TEXTURE2D_DESC depthStencilDesc(depthBufferFormat, backBufferWidth, backBufferHeight, 1, 1, D3D11_BIND_DEPTH_STENCIL);

    ComPtr<ID3D11Texture2D> depthStencil;
    DX::ThrowIfFailed(m_d3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, depthStencil.GetAddressOf()));

    CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
    DX::ThrowIfFailed(m_d3dDevice->CreateDepthStencilView(depthStencil.Get(), &depthStencilViewDesc, m_depthStencilView.ReleaseAndGetAddressOf()));

    // Create a viewport descriptor of the full window size.
    CD3D11_VIEWPORT viewPort(0.0f, 0.0f, static_cast<float>(backBufferWidth), static_cast<float>(backBufferHeight));

    // Set the current viewport using the descriptor.
    m_d3dContext->RSSetViewports(1, &viewPort);

    // TODO: Initialize windows-size dependent objects here
}

void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here

    m_depthStencil.Reset();
    m_depthStencilView.Reset();
    m_renderTargetView.Reset();
    m_swapChain1.Reset();
    m_swapChain.Reset();
    m_d3dContext1.Reset();
    m_d3dContext.Reset();
    m_d3dDevice1.Reset();
    m_d3dDevice.Reset();

    CreateDevice();

    CreateResources();
}
void Game::OnKeydown(int keycode) {
	TCHAR s[100];
	wsprintf(s, L"Key: %d", keycode);

	OutputDebugString(s);

	if (keycode == 'Q' ) exit(0);
#ifndef USE_SHINRA_API    
	if (keycode == 'P') {
        AddPlayer(0);
    }
    if( keycode == 'U' ) {
        for(int i=0;i<MAX_PLAYER_NUM;i++) {
            if(m_players[i]) {
                m_players[i]->CleanCells();
                delete m_players[i];
                m_players[i] = nullptr;
                break;
            }
        }
    }
#endif    
}


XMFLOAT4 g_playerColors[MAX_PLAYER_NUM] = {
    { 0, 0.63137254f, 0.79607843f, 1 }, // blue
    { 0.38039215f, 0.6823529f, 0.1411764f, 1 }, // green
    { 0.84313725f, 0, 0.3764f, 1 }, // red
    { 0.94509803f, 0.552941f, 0.019607f, 1 }, // orange
    //    { 0.38039215f, 0.3803921f, 0.380392f, 1 } // gray
};

XMFLOAT4 Game::GetPlayerColor( int index ) {
    assert( index >= 0 && index < MAX_PLAYER_NUM );
    return g_playerColors[index];
}


void Game::onBodySeparated( cpBody *bodyA, cpBody *bodyB ) {
}

void Game::onBodyJointed( cpBody *bodyA, cpBody *bodyB ) {
}
void Game::PlaySEForAll( SE_ID se_id ) {
    for(int i=0;i<MAX_PLAYER_NUM;i++) {
        Player *pl = GetPlayer(i);
        if(pl)pl->PlaySE(se_id);
    }
}
void Game::onBodyCollide( cpBody *bodyA, cpBody *bodyB ) {
    cpVect veloA = cpBodyGetVelocity(bodyA);
    cpVect veloB = cpBodyGetVelocity(bodyB);
    float relvelo = len( veloA.x, veloA.y, veloB.x, veloB.y );
    //    print("onBodyCollide: %.2f", relvelo );
    if( relvelo > 400 ) {
        PlaySEForAll( SE_JOIN );
    }    
}
// return false when max player
bool Game::AddPlayer( shinra::PlayerID playerID ) {
    Player *pl = nullptr;
    for(int i=0;i<MAX_PLAYER_NUM;i++) {
        if( m_players[i] == nullptr ) {
            m_players[i] = new Player( playerID, this, i );
            pl = m_players[i];
            print("AddPlayer: new player %d",i);
            break;
        }
    }
    if(pl==nullptr) return false;

    float dia;
    cpVect center = GetPlayerDefaultPosition( pl->GetGroupId(), &dia );
    
    int n = CELL_NUM_PER_PLAYER;
	for(int i=0; i<n; i++){
        int prio, eye_id=-1;
        if(i==0 ||i==1) {
            eye_id = i;
            prio = CELL_PRIO_HIGH;
        } else {
            prio = CELL_PRIO_LOW; // draw eyes always on other cells
        }
        
        BodyState *bs = new BodyState(prio,pl->GetGroupId(),eye_id, this);

        cpVect p = cpv( cpflerp(center.x-dia, center.x+dia, frand()), cpflerp(center.y-dia, center.y+dia, frand() ) );
        cpBody *body = CreateCellBody(p, bs, eye_id >= 0 );

        if( eye_id == 0 ) pl->SetLeftEye(body);
        else if( eye_id == 1 ) pl->SetRightEye(body);

	}
    // add springs between eyes
    cpSpaceAddConstraint( m_space, new_spring( pl->GetLeftEye(), pl->GetRightEye(), cpv(0,0),cpv(0,0), 70, 110, 0.1 ) );
    return true;
}

void Game::RemovePlayer( shinra::PlayerID playerID ) {
    for(int i=0;i<MAX_PLAYER_NUM;i++){
        if( m_players[i]->getPlayerID() == playerID ) {
            print("RemovePlayer: id:%d found. removing.", playerID );
            Player *pl = m_players[i];
            delete pl;
            m_players[i] = nullptr;
            break;
        }
    }
}

Player *Game::GetPlayer( int group_id ) {
    assert( group_id >= 0 && group_id < MAX_PLAYER_NUM );
    return m_players[group_id];
}
cpVect Game::GetPlayerDefaultPosition( int index, float *dia ) {
    size_t w,h;
    GetDefaultSize(w,h);
    float wf = (float)w, hf = (float)h;

    *dia = wf/6.0f;
    
    /*
            |
         0  |  1
            |   
       -----O----
            |
         2  |  3
            |

       O: (0,0) in ChipMunk
       
     */
    index = index % 4;
    switch(index) {
    case 0: return cpv( -wf/4, hf/4);
    case 1: return cpv( wf/4, hf/4 );
    case 2: return cpv( -wf/4, -hf/4 );
    case 3: return cpv( wf/4, -hf/4 );        
    }
    return cpv(0,0);
}

struct CleanCallbackOpts {
    Game *game;
    int group_id;
};
void eachBodyCleanCallback( cpBody *body, void *data ) {
    CleanCallbackOpts *opts = (CleanCallbackOpts*)data;
    BodyState *bs = (BodyState*) cpBodyGetUserData(body);
    if( bs->group_id == opts->group_id ) {
        cpBodyEachShape(body, eachShapeDeleteCallback, opts->game );
        cpBodyEachConstraint(body, eachConstraintDeleteCallback, opts->game );        
        PostBodyFree( body, opts->game->GetSpace() );
    }
}
void Game::CleanGroup( int groupid ) {
    CleanCallbackOpts opts;
    opts.game = this;
    opts.group_id = groupid;
    cpSpaceEachBody( m_space, eachBodyCleanCallback, (void*) & opts );
}

Player::Player( shinra::PlayerID playerID, Game *game, int group_id ) : m_playerID(playerID), game(game), group_id(group_id), m_cellCount(0) {        
    for(int i=0;i<2;i++) {
        m_eyes[i]=nullptr;
        m_forces[i]=XMFLOAT2(0,0);
    }

    // Audio
    AUDIO_ENGINE_FLAGS eflags = AudioEngine_Default;
    /* TODO: debug engine is not currently support in Shinra Audio Layer.
#ifdef _DEBUG
    eflags = eflags | AudioEngine_Debug;
#endif
    */
    auto audioDevice = shinra::GetPlayerAudioDevice(playerID);
    if( audioDevice == nullptr ) {
        // Don't use ShinraGame API
        m_audioEngine = new AudioEngine(eflags);
    } else {
        // Use ShinraGame API
        LPWSTR audioId = nullptr;
        audioDevice->GetId(&audioId);
        m_audioEngine = new AudioEngine(eflags, nullptr, audioId);
        CoTaskMemFree(audioId);
        audioDevice->Release();        
    }


    m_brokenSE = new SoundEffect(m_audioEngine, L"assets\\hagareta.wav");
    m_joinSE = new SoundEffect(m_audioEngine, L"assets\\kuttsuki.wav");
    m_bgm = new SoundEffect( m_audioEngine, L"assets\\BGM_AmoebasBattle.wav");


    // Graphics

    m_d3dContext = shinra::GetPlayerRenderingContext(playerID);
    if(m_d3dContext==nullptr) {
        // Don't use ShinraGame API
        m_d3dContext = game->GetD3DContext().Get();
    } else {
        // Use ShinraGame API
    }
    m_spriteBatch = new SpriteBatch(m_d3dContext);
    
	m_spriteBatch = new SpriteBatch(m_d3dContext);
	m_spriteFont = new SpriteFont( game->GetD3DDevice().Get(), L"assets\\tahoma9.spritefont");
    m_primBatch = new PrimitiveBatch<VertexPositionColor>(m_d3dContext);

    m_basicEffect = new BasicEffect( game->GetD3DDevice().Get() );
    size_t scrw, scrh;
    game->GetDefaultSize(scrw,scrh);    
    m_basicEffect->SetProjection( XMMatrixOrthographicOffCenterRH(0, (float)scrw, (float)scrh, 0,0,1 ) );
    m_basicEffect->SetVertexColorEnabled(true);

    void const* shaderByteCode;
    size_t byteCodeLength ;
    m_basicEffect->GetVertexShaderBytecode( &shaderByteCode, &byteCodeLength );
    game->GetD3DDevice()->CreateInputLayout( VertexPositionColor::InputElements,
                                             VertexPositionColor::InputElementCount,
                                             shaderByteCode, byteCodeLength,
                                             m_inputLayout.GetAddressOf() );

    
    //
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cell;
    enum DDS_ALPHA_MODE alphamode = DDS_ALPHA_MODE_STRAIGHT;
    HRESULT hr = CreateDDSTextureFromFile( game->GetD3DDevice().Get(), L"assets\\basic_cell.dds", nullptr, cell.GetAddressOf(), alphamode );
    DX::ThrowIfFailed(hr);

    // Create an AnimatedTexture helper class instance and set it to use our texture
    // which is assumed to have 4 frames of animation with a FPS of 2 seconds
    m_cellAnimTex = new AnimatedTexture( XMFLOAT2(0,0), 0.f, 2.f, 0.5f );
    m_cellAnimTex->Load( cell.Get(), 4, 2 );

    hr = CreateDDSTextureFromFile( game->GetD3DDevice().Get(), L"assets\\wave.dds", nullptr, m_bgTex.GetAddressOf(), alphamode );
    DX::ThrowIfFailed(hr);    
}
Player::~Player() {
    CleanCells();
    delete m_spriteBatch;
    delete m_spriteFont;
    delete m_primBatch;
    delete m_basicEffect;
    //    delete m_inputLayout;
    delete m_cellAnimTex;
    //    delete m_bgTex;
    //    delete m_d3dContext;

    delete m_audioEngine;
    delete m_brokenSE, *m_joinSE, *m_bgm;    
}
void Player::Update( float elapsedTime ) {
	m_audioEngine->Update();
    if (m_audioEngine->IsCriticalError()) {
        OutputDebugString(L"AudioEngine error!");
	}
    
    // keep BGM playing (couldn't use DXTK loop feature don't know why)
    if( m_bgm->IsInUse()==false) {
        m_bgm->Play();
    }

    // graphics update
    m_cellAnimTex->Update(elapsedTime);    
}
void Player::PlaySE( SE_ID se_id ) {
    switch(se_id) {
    case SE_JOIN: m_joinSE->Play(); break;
    case SE_BROKEN: m_brokenSE->Play(); break;
    }
}


