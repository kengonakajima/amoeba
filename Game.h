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
#define BODY_CELL_NUM_PER_PLAYER 100
#define CELL_NUM_PER_PLAYER ( 2 + BODY_CELL_NUM_PER_PLAYER )
#define TOTAL_CELL_NUM (CELL_NUM_PER_PLAYER * MAX_PLAYER_NUM )


//////////////
class Game;
  
class Player
{
public:
    //    Player( shinra::PlayerID playerID, std::shared_ptr<SpriteFont> font);
    //    shinra::PlayerID getPlayerID() { return playerID; }
    //    void Update();
    //    void Render(int frameCnt);
    //    void handleInput(const RAWINPUT& rawInput);
    void SetLeftEye(cpBody *bd) { m_eyes[0] = bd; }
    void SetRightEye(cpBody *bd) { m_eyes[1] = bd; }
    cpBody *GetLeftEye() { return m_eyes[0]; }
    cpBody *GetRightEye() { return m_eyes[1]; }    
    void SetForce( XMFLOAT2 lf, XMFLOAT2 rf ) { m_forces[0] = lf; m_forces[1] = rf; };
    XMFLOAT2 GetLeftForce() { return m_forces[0];  };
    XMFLOAT2 GetRightForce() { return m_forces[1]; };    
    void IncrementCellCount() { m_cellCount ++; }
    void ResetCellCount() { m_cellCount=0; }
    int GetCellCount() { return m_cellCount; }
    void ResetCells();
    void CleanCells();
    int GetGroupId() { return group_id; }
    Player( Game *game, int group_id ) : game(game), group_id(group_id), m_cellCount(0) {
        for(int i=0;i<2;i++) {
            m_eyes[i]=nullptr;
            m_forces[i]=XMFLOAT2(0,0);
        }
    }
private:
    Game *game;
    int group_id;
    int m_cellCount;
    cpBody *m_eyes[2]; // 0:Left 1:Right
    XMFLOAT2 m_forces[2]; // 0:Left 1:Right
    
    // Show status logs
    //    shinra::PlayerID playerID;
    //    std::unique_ptr<SpriteBatch> m_spriteBatch;
    //    std::shared_ptr<SpriteFont> m_spriteFont;
    //    std::unique_ptr<AudioEngine> m_audioEngine;
    //    std::unique_ptr<SoundEffect> m_soundEffect;
    //    WCHAR						m_lastKey;

};


class BodyState;

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
    static void GetDefaultSize( size_t& width, size_t& height );

    static XMFLOAT4 GetPlayerColor( int index );

    PrimitiveBatch<VertexPositionColor> *GetPrimBatch() { return m_primBatch; }

    //    void SetCreatureForce( int index, XMFLOAT2 leftEye, XMFLOAT2 rightEye );
    //    void GetCreatureForce( int index, XMFLOAT2 *leftEye, XMFLOAT2 *rightEye );
    cpSpace *GetSpace() { return m_space; };

    void onBodySeparated( cpBody *bodyA, cpBody *bodyB );
    void onBodyJointed( cpBody *bodyA, cpBody *bodyB );
    void onBodyCollide( cpBody *bodyA, cpBody *bodyB );    
    bool AddPlayer();
    Player *GetPlayer( int groupId );
    static cpVect GetPlayerDefaultPosition( int index, float *dia );

    cpBody *CreateCellBody( cpVect pos, BodyState *bs, bool is_eye );

    void CleanGroup( int groupId );
    
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
    Player *m_players[MAX_PLAYER_NUM];

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
	SoundEffect *m_brokenSE, *m_damageSE, *m_joinSE;
    SoundEffect *m_bgm;
    
};






