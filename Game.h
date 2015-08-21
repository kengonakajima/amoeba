//
// Game.h
//

#pragma once


#include "chipmunk/chipmunk.h"

#include "StepTimer.h"
#include "SpriteFont.h"
#include "VertexTypes.h"
#include "PrimitiveBatch.h"
#include "Effects.h"
#include "Audio.h"
#include "GamePad.h"

#include "Main.h"
#include "AnimatedTexture.h"
#include "Util.h"

#include <math.h>

using namespace DirectX;
using namespace Microsoft::WRL;
using Microsoft::WRL::ComPtr;


#define MAX_PLAYER_NUM 4
#define HP_CONSUME_SPEED 0.2
#define CELL_RADIUS 10.0f

//////////////

/* One-to-many Player
  
class Player
{
public:
    Player( shinra::PlayerID playerID, std::shared_ptr<SpriteFont> font);
    shinra::PlayerID getPlayerID() { return playerID; }
    void Update();
    void Render(int frameCnt);
    void handleInput(const RAWINPUT& rawInput);
private:
    // Show status logs
    shinra::PlayerID playerID;
    std::unique_ptr<SpriteBatch> m_spriteBatch;
    std::shared_ptr<SpriteFont> m_spriteFont;
    std::unique_ptr<AudioEngine> m_audioEngine;
    std::unique_ptr<SoundEffect> m_soundEffect;
    WCHAR						m_lastKey;

};
*/


struct CreatureForce {
public:
    XMFLOAT2 leftEye, rightEye;
};

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

    void InitCreatureForce() {
        for(int i=0;i<MAX_PLAYER_NUM;i++) SetCreatureForce( 0, XMFLOAT2(0,0), XMFLOAT2(0,0) );
    }
    void SetCreatureForce( int index, XMFLOAT2 leftEye, XMFLOAT2 rightEye );
    void GetCreatureForce( int index, XMFLOAT2 *leftEye, XMFLOAT2 *rightEye );
    cpSpace *GetSpace() { return m_space; };

    void onBodySeparated();
    
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
    int m_cellCounts[MAX_PLAYER_NUM];

	// Show status logs
	SpriteBatch *m_spriteBatch;
	SpriteFont *m_spriteFont;
    PrimitiveBatch<VertexPositionColor> *m_primBatch;
    BasicEffect *m_basicEffect;
    ComPtr<ID3D11InputLayout> m_inputLayout;
    
    // game sprites, textures
    AnimatedTexture *m_cellAnimTex;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_bgTex;

    // chipmunk related
    struct cpSpace *m_space;

	AudioEngine *m_audioEngine;
	SoundEffect *m_brokenSE, *m_damageSE;

    // Movement
    CreatureForce m_forces[MAX_PLAYER_NUM];
    
};






