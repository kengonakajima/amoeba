//
// Game.h
//

#pragma once


#include "StepTimer.h"
#include "SpriteFont.h"
#include "VertexTypes.h"
#include "PrimitiveBatch.h"
#include "Effects.h"
#include "Audio.h"
#include "GamePad.h"

#include "AnimatedTexture.h"
#include "Util.h"

#include <math.h>

using namespace DirectX;
using namespace Microsoft::WRL;
using Microsoft::WRL::ComPtr;


#define MAX_PLAYER_NUM 5

//////////////



// A basic game implementation that creates a D3D11 device and
// provides a game loop
class Game
{
public:

    Game();

    // Initialization and management
    void Initialize(HWND window);

    // Basic game loop
    void Tick();
    void Render();

    // Rendering helpers
    void Clear();
    void Present();

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowSizeChanged();
	void OnKeydown(int keycode);

    // Properites
    void GetDefaultSize( size_t& width, size_t& height ) const;

    static XMFLOAT4 GetPlayerColor( int index );

    PrimitiveBatch<VertexPositionColor> *GetPrimBatch() { return m_primBatch; }
    
private:

    void Update(DX::StepTimer const& timer);

    void CreateDevice();
    void CreateResources();
    
    void OnDeviceLost();

    void InitGameWorld();
    

    // Application state
    HWND                                            m_window;

    // Direct3D Objects
    D3D_FEATURE_LEVEL                               m_featureLevel;
    Microsoft::WRL::ComPtr<ID3D11Device>            m_d3dDevice;
    Microsoft::WRL::ComPtr<ID3D11Device1>           m_d3dDevice1;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext>     m_d3dContext;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext1>    m_d3dContext1;

    // Rendering resources
    Microsoft::WRL::ComPtr<IDXGISwapChain>          m_swapChain;
    Microsoft::WRL::ComPtr<IDXGISwapChain1>         m_swapChain1;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>  m_renderTargetView;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView>  m_depthStencilView;
    Microsoft::WRL::ComPtr<ID3D11Texture2D>         m_depthStencil;

    // Game state
    DX::StepTimer                                   m_timer;
	int m_framecnt;

	// Show status logs
	SpriteBatch *m_spriteBatch;
	SpriteFont *m_spriteFont;
    PrimitiveBatch<VertexPositionColor> *m_primBatch;
    BasicEffect *m_basicEffect;
    ComPtr<ID3D11InputLayout> m_inputLayout;
    
    // game sprites
    AnimatedTexture *m_cellAnimTex;

    // chipmunk related
    struct cpSpace *m_space;

	AudioEngine *m_audioEngine;
	SoundEffect *m_soundEffect;


    
    
};




