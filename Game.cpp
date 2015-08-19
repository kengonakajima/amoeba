//
// Game.cpp -
//
#include "pch.h"

#include "DDSTextureLoader.h"
#include "CommonStates.h"
#include <assert.h>




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
	m_spriteBatch = new SpriteBatch(m_d3dContext.Get());
	m_spriteFont = new SpriteFont( m_d3dDevice.Get(), L"assets\\tahoma32.spritefont");
    m_primBatch = new PrimitiveBatch<VertexPositionColor>(m_d3dContext.Get());

    m_basicEffect = new BasicEffect(m_d3dDevice.Get() );
    size_t scrw, scrh;
    GetDefaultSize(scrw,scrh);    
    m_basicEffect->SetProjection( XMMatrixOrthographicOffCenterRH(0, scrw, scrh, 0,0,1 ) );
    m_basicEffect->SetVertexColorEnabled(true);

    void const* shaderByteCode;
    size_t byteCodeLength ;
    m_basicEffect->GetVertexShaderBytecode( &shaderByteCode, &byteCodeLength );
    m_d3dDevice->CreateInputLayout( VertexPositionColor::InputElements,
                                    VertexPositionColor::InputElementCount,
                                    shaderByteCode, byteCodeLength,
                                    m_inputLayout.GetAddressOf() );

    
    //
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cell;
    enum DDS_ALPHA_MODE alphamode = DDS_ALPHA_MODE_STRAIGHT;
    HRESULT hr = CreateDDSTextureFromFile( m_d3dDevice.Get(), L"assets\\basic_cell.dds", nullptr, cell.GetAddressOf(), alphamode );
    DX::ThrowIfFailed(hr);

    // Create an AnimatedTexture helper class instance and set it to use our texture
    // which is assumed to have 4 frames of animation with a FPS of 2 seconds
    m_cellAnimTex = new AnimatedTexture( XMFLOAT2(0,0), 0.f, 2.f, 0.5f );
    m_cellAnimTex->Load( cell.Get(), 4, 2 );

	// This is only needed in Win32 desktop apps
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	AUDIO_ENGINE_FLAGS eflags = AudioEngine_Default;
#ifdef _DEBUG
	eflags = eflags | AudioEngine_Debug;
#endif
	m_audioEngine = new AudioEngine(eflags);
	m_soundEffect = new SoundEffect(m_audioEngine, L"assets\\coinget.wav");
	m_soundEffect->Play();


    // chipmunk
    InitGameWorld();
}

void Game::InitGameWorld() {

    m_space = cpSpaceNew();

	cpSpaceSetIterations(m_space, 10);
	cpSpaceSetGravity(m_space, cpv(0, -300));
	cpSpaceSetCollisionSlop(m_space, 2.0);

	cpBody *staticBody = cpSpaceGetStaticBody( m_space);
	cpShape *shape;

	// Create segments around the edge of the screen.
    size_t w,h;
    GetDefaultSize(w,h);

    float minx = -(float)w/2, maxx = w/2, miny = -(float)h/2, maxy = h/2;
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
	
	for(int i=0; i<200; i++){
		cpFloat mass = 0.15f;
		cpFloat radius = 10.0f;
        int group_id = i % 2;
        int eye_id = -1;
        float pcminx,pcmaxx;
        if( group_id == 0 ) {
            pcminx = minx+20;
            pcmaxx = -50;
        } else {
            pcminx = 50;
            pcmaxx = maxx-20;
        }
        
        // player 0 eyes
        if( i == 196 ) eye_id = 0;
        if( i == 198 ) eye_id = 1;
        // player 1 eyes
        if( i == 197 ) eye_id = 0;
        if( i == 199 ) eye_id = 1;
        
        BodyState *bs = new BodyState(i,group_id,eye_id, this);

		cpBody *body = cpSpaceAddBody(m_space, cpBodyNew(mass, cpMomentForCircle(mass, 0.0f, radius, cpvzero)));
        cpVect p = cpv( cpflerp(pcminx, pcmaxx, frand()), cpflerp(miny, maxy, frand() ) );
        print("%d: %f,%f   minmax:%f,%f", i, p.x, p.y, pcminx, pcmaxx );
		cpBodySetPosition(body, p);
        cpBodySetUserData(body, bs);
        
		cpShape *shape = cpSpaceAddShape(m_space, cpCircleShapeNew(body, radius + STICK_SENSOR_THICKNESS, cpvzero));
		cpShapeSetFriction(shape, 0.9f);
		cpShapeSetCollisionType(shape, COLLISION_TYPE_STICKY);
	}
	
	cpCollisionHandler *handler = cpSpaceAddWildcardHandler(m_space, COLLISION_TYPE_STICKY);
	handler->preSolveFunc = StickyPreSolve;
	handler->separateFunc = StickySeparate;
    
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

// Updates the world
void Game::Update(DX::StepTimer const& timer)
{
    float elapsedTime = float(timer.GetElapsedSeconds());

    // TODO: Add your game logic here
    elapsedTime;

	m_audioEngine->Update();
	if (m_audioEngine->IsCriticalError()) {
		OutputDebugString(L"AudioEngine error!");
	}
    m_cellAnimTex->Update(elapsedTime);
    PhysUpdateSpace( m_space, elapsedTime );
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


void eachBodyDrawCallback( cpBody *body, void *data ) {
    BodyState *bs = (BodyState*) cpBodyGetUserData(body);
    Game *game = (Game*) data;

    cpVect cppos = cpBodyGetPosition(body);


    // draw
    size_t scrw, scrh;
    game->GetDefaultSize(scrw,scrh);
    XMFLOAT3 dxtkPos( cppos.x+scrw/2, - cppos.y + scrh/2, 0 );
    XMFLOAT4 dxtkCol = Game::GetPlayerColor( bs->group_id );
    //    print( "id:%d g:%d xy:%f,%f", bs->id, bs->group_id, dxtkPos.x, dxtkPos.y );
    if( bs->eye_id >= 0 ) {
        dxtkCol = XMFLOAT4( 0.2,0.2,0.2,1);
    }
    DrawCircle( game->GetPrimBatch(), dxtkPos, 20, dxtkCol );
    
}

void Game::Render()
{
	m_framecnt++;
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
        return;

    Clear();

    // Sprites
    CommonStates states(m_d3dDevice.Get());
	m_spriteBatch->Begin( SpriteSortMode_Deferred, states.NonPremultiplied() );

	TCHAR statmsg[100];
	wsprintf(statmsg, L"Frame: %d", m_framecnt);
	m_spriteFont->DrawString(m_spriteBatch, statmsg, XMFLOAT2(10, 10));

	m_spriteFont->DrawString(m_spriteBatch, L"Skeleton code for 1:1 games", XMFLOAT2(100, 100));
	m_spriteFont->DrawString(m_spriteBatch, L"Press P to play sound effect", XMFLOAT2(130, 160));
	m_spriteFont->DrawString(m_spriteBatch, L"Press Q to quit", XMFLOAT2(130, 200));


    XMFLOAT2 pos(100,100);
    m_cellAnimTex->Draw( m_spriteBatch, pos );


	m_spriteBatch->End();

    // Prims

    m_d3dContext->OMSetBlendState( states.Opaque(), nullptr, 0xffffffff );
    m_d3dContext->OMSetDepthStencilState( states.DepthNone(), 0 );
    m_d3dContext->RSSetState( states.CullCounterClockwise() );

    m_basicEffect->Apply( m_d3dContext.Get() );
    m_d3dContext->IASetInputLayout( m_inputLayout.Get() );

    m_primBatch->Begin();
    m_primBatch->DrawLine( VertexPositionColor( XMFLOAT3(10,10,0), XMFLOAT4(1,0,1,1)),
                           VertexPositionColor( XMFLOAT3(100,200,0), XMFLOAT4(1,1,0,1)) );

    XMFLOAT4 col = GetPlayerColor( irange(0,5) );
    XMFLOAT3 cirpos( range(0,500), range(0,300),0 );
    
    DrawCircle( m_primBatch, cirpos, range(10,50), col );
    DrawCircle( m_primBatch, XMFLOAT3(cirpos.x+50,cirpos.y+50,0), range(10,50), col );    

    cpSpaceEachBody( m_space, eachBodyDrawCallback, (void*) this );
    
    m_primBatch->End();
    

    Present();
}

// Helper method to clear the backbuffers
void Game::Clear()
{
    // Clear the views
	float d = 100.0f;
	int m = 25;
	float r = (rand() % m) / d, g = (rand() % m) /d, b = (rand() % m) / d;
	float col[4] = { r,g,b,1 };
    m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), col );
    m_d3dContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
    m_d3dContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());
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
void Game::GetDefaultSize(size_t& width, size_t& height) const
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
	if (keycode == 'P') m_soundEffect->Play();
}


XMFLOAT4 g_playerColors[MAX_PLAYER_NUM] = {
    { 0, 0.63137254f, 0.79607843f, 1 }, // blue
    { 0.38039215f, 0.6823529f, 0.1411764f, 1 }, // green
    { 0.84313725f, 0, 0.3764f, 1 }, // red
    { 0.94509803f, 0.552941f, 0.019607f, 1 }, // orange
    { 0.38039215f, 0.3803921f, 0.380392f, 1 } // gray
};

XMFLOAT4 Game::GetPlayerColor( int index ) {
    assert( index >= 0 && index < MAX_PLAYER_NUM );
    return g_playerColors[index];
}


////////////////////