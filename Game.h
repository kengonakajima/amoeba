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

#include <math.h>

using namespace DirectX;
using namespace Microsoft::WRL;
using Microsoft::WRL::ComPtr;


#define MAX_PLAYER_NUM 5

//////////////
// copied from cumino.h

inline float maxf( float a, float b ){
    return (a>b) ? a:b;
}
inline float maxf( float a, float b, float c ) {
    return maxf( maxf(a,b), c );
}
inline float maxf( float a, float b, float c, float d ) {
    return maxf( maxf(a,b), maxf(c,d) );
}
inline double maxd( double a, double b ){
    return  (a>b) ? a:b;
}
inline double mind( double a, double b ){
    return (a<b) ? a:b;
}
inline int maxi( int a, int b ){
    return  (a>b) ? a:b;
}
inline int mini( int a, int b ){
    return (a<b) ? a:b;
}
inline long random() { 
	int top = rand();
	int bottom = rand();
	long out = ( top << 16 ) | bottom;
	return out;
}

inline double range( double a, double b ) {
    long r = random();
    double rate = (double)r / (double)(0x7fffffff);
    double _a = mind(a,b);
    double _b = maxd(a,b);
    return _a + (_b-_a)*rate;
}
inline int irange( int a, int b ) {
    double r = range(a,b);
    return (int)r;
}


void print( const char *fmt, ... );





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




