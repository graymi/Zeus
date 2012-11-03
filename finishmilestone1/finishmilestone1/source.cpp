// include the basic windows header files and the Direct3D header files
#include <windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dx10.h>
#include <vector>
#include <math.h>
#include <d3dx9math.h>
// include the Direct3D Library file
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dx11.lib")
#pragma comment (lib, "d3dx10.lib")

// define the screen resolution
#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600

using namespace std;

// global declarations
IDXGISwapChain *swapchain;             // the pointer to the swap chain interface
ID3D11Device *dev;                     // the pointer to our Direct3D device interface
ID3D11DeviceContext *devcon;           // the pointer to our Direct3D device context
ID3D11RenderTargetView *backbuffer;    // the pointer to our back buffer
ID3D11InputLayout *pLayout;            // the pointer to the input layout
ID3D11VertexShader *pVS;               // the pointer to the vertex shader
ID3D11PixelShader *pPS;                // the pointer to the pixel shader
ID3D11Buffer *pVBuffer;                // the pointer to the vertex buffer
ID3D11Buffer *pIBuffer;
ID3D11Buffer *pCBuffer;                // the pointer to the constant buffer
ID3D11RasterizerState* noCull;

// various buffer structs
struct VERTEX{D3DXVECTOR3 pos; D3DXCOLOR color;};
struct PERFRAME{D3DXCOLOR Color; FLOAT X, Y, Z;};

vector<VERTEX> vertices;
vector<DWORD> indices;

// function prototypes
void InitD3D(HWND hWnd);    // sets up and initializes Direct3D
void RenderFrame(void);     // renders a single frame
void CleanD3D(void);        // closes Direct3D and releases memory
void InitGraphics(void);    // creates the shape to render
void InitPipeline(void);    // loads and prepares the shaders

// the WindowProc function prototype
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


// the entry point for any Windows program
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
    HWND hWnd;
    WNDCLASSEX wc;

    ZeroMemory(&wc, sizeof(WNDCLASSEX));

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "WindowClass";

    RegisterClassEx(&wc);

    RECT wr = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    hWnd = CreateWindowEx(NULL,
                          "WindowClass",
                          "Our First Direct3D Program",
                          WS_OVERLAPPEDWINDOW,
                          300,
                          300,
                          wr.right - wr.left,
                          wr.bottom - wr.top,
                          NULL,
                          NULL,
                          hInstance,
                          NULL);

    ShowWindow(hWnd, nCmdShow);

    // set up and initialize Direct3D
    InitD3D(hWnd);

    // enter the main loop:

    MSG msg;

    while(TRUE)
    {
        if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if(msg.message == WM_QUIT)
                break;
        }

        RenderFrame();
    }

    // clean up DirectX and COM
    CleanD3D();

    return msg.wParam;
}


// this is the main message handler for the program
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
        case WM_DESTROY:
            {
                PostQuitMessage(0);
                return 0;
            } break;
    }

    return DefWindowProc (hWnd, message, wParam, lParam);
}


// this function initializes and prepares Direct3D for use
void InitD3D(HWND hWnd)
{
    // create a struct to hold information about the swap chain
    DXGI_SWAP_CHAIN_DESC scd;

    // clear out the struct for use
    ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

    // fill the swap chain description struct
    scd.BufferCount = 1;                                   // one back buffer
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;    // use 32-bit color
    scd.BufferDesc.Width = SCREEN_WIDTH;                   // set the back buffer width
    scd.BufferDesc.Height = SCREEN_HEIGHT;                 // set the back buffer height
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;     // how swap chain is to be used
    scd.OutputWindow = hWnd;                               // the window to be used
    scd.SampleDesc.Count = 4;                              // how many multisamples
    scd.Windowed = TRUE;                                   // windowed/full-screen mode
    scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;    // allow full-screen switching

    // create a device, device context and swap chain using the information in the scd struct
    D3D11CreateDeviceAndSwapChain(NULL,
                                  D3D_DRIVER_TYPE_HARDWARE,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  D3D11_SDK_VERSION,
                                  &scd,
                                  &swapchain,
                                  &dev,
                                  NULL,
                                  &devcon);


    // get the address of the back buffer
    ID3D11Texture2D *pBackBuffer;
    swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

    // use the back buffer address to create the render target
    dev->CreateRenderTargetView(pBackBuffer, NULL, &backbuffer);
    pBackBuffer->Release();

    // set the render target as the back buffer
    devcon->OMSetRenderTargets(1, &backbuffer, NULL);


    // Set the viewport
    D3D11_VIEWPORT viewport;
    ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = SCREEN_WIDTH;
    viewport.Height = SCREEN_HEIGHT;

    devcon->RSSetViewports(1, &viewport);

    InitPipeline();
    InitGraphics();
}


// this is the function used to render a single frame
void RenderFrame(void)
{
	//turn off backface culling
	D3D11_RASTERIZER_DESC rastDesc;
	ZeroMemory(&rastDesc, sizeof(D3D11_RASTERIZER_DESC));
	rastDesc.FillMode = D3D11_FILL_SOLID;
	rastDesc.CullMode = D3D11_CULL_NONE;
	dev->CreateRasterizerState(&rastDesc, &noCull);
	devcon->RSSetState(NULL);
	devcon->RSSetState(noCull);


    D3DXMATRIX matRotate, matView, matProjection, matFinal;

    static float Time = 0.0f; Time += 0.001f;

    // create a rotation matrix
    D3DXMatrixRotationY(&matRotate, Time);

    // create a view matrix
    D3DXMatrixLookAtLH(&matView,
                       &D3DXVECTOR3(1.5f, 0.5f, 1.5f),    // the camera position
                       &D3DXVECTOR3(0.0f, 0.0f, 0.0f),    // the look-at position
                       &D3DXVECTOR3(0.0f, 1.0f, 0.0f));   // the up direction

    // create a projection matrix
    D3DXMatrixPerspectiveFovLH(&matProjection,
                               (FLOAT)D3DXToRadian(45),                    // field of view
                               (FLOAT)SCREEN_WIDTH / (FLOAT)SCREEN_HEIGHT, // aspect ratio
                               1.0f,                                       // near view-plane
                               100.0f);                                    // far view-plane

    // create the final transform
    matFinal = matRotate * matView * matProjection;

    // set the new values for the constant buffer
    devcon->UpdateSubresource(pCBuffer, 0, 0, &matFinal, 0, 0);


    // clear the back buffer to a deep blue
    devcon->ClearRenderTargetView(backbuffer, D3DXCOLOR(0.0f, 0.0f, 0.0f, 1.0f));

        // select which vertex buffer to display
        UINT stride = sizeof(VERTEX);
        UINT offset = 0;
        devcon->IASetVertexBuffers(0, 1, &pVBuffer, &stride, &offset);
		devcon->IASetIndexBuffer(pIBuffer, DXGI_FORMAT_R32_UINT, 0);
        // select which primtive type we are using
        devcon->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // draw the vertex buffer to the back buffer
        devcon->DrawIndexed(indices.size(), 0, 0);

    // switch the back buffer and the front buffer
    swapchain->Present(0, 0);
}


// this is the function that cleans up Direct3D and COM
void CleanD3D(void)
{
    swapchain->SetFullscreenState(FALSE, NULL);    // switch to windowed mode

    // close and release all existing COM objects
    pLayout->Release();
    pVS->Release();
    pPS->Release();
    pVBuffer->Release();
    pCBuffer->Release();
    swapchain->Release();
    backbuffer->Release();
    dev->Release();
    devcon->Release();
}


// this is the function that creates the shape to render
void InitGraphics()
{
	VERTEX temp;
	// Setup 3x2 Grid for use with triangle and box
	// 0
	temp.pos = D3DXVECTOR3( -1.0f, 0.0f, 0.0f );
	temp.color =  D3DXCOLOR(1.0f, 0.0f, 0.0f, 1.0f);
	vertices.push_back( temp );
	// 1
	temp.pos = D3DXVECTOR3( -1.0f, 1.0f, 0.0f );
	temp.color = D3DXCOLOR(0.0f, 1.0f, 0.0f, 1.0f);
	vertices.push_back( temp );
	// 2
	temp.pos = D3DXVECTOR3( 0.0f, 1.0f, 0.0f );
	temp.color = D3DXCOLOR(0.0f, 0.0f, 1.0f, 1.0f);
	vertices.push_back( temp );
	// 3
	temp.pos = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
	temp.color = D3DXCOLOR(0.1f, 0.5f, 1.0f, 1.0f);
	vertices.push_back( temp );
	// 4
	temp.pos = D3DXVECTOR3( 1.0f, 1.0f, 0.0f );
	temp.color = D3DXCOLOR(1.0f, 0.0f, 0.0f, 1.0f);
	vertices.push_back( temp );
	// 5
	temp.pos = D3DXVECTOR3( 1.0f, 0.0f, 0.0f );
	temp.color = D3DXCOLOR(0.0f, 1.0f, 0.0f, 1.0f);
	vertices.push_back( temp );

		// top left triangle
	indices.push_back( 0 );
	indices.push_back( 1 );
	indices.push_back( 2 );

	// top right square
	indices.push_back( 3 );
	indices.push_back( 2 );
	indices.push_back( 4 );

	indices.push_back( 3 );
	indices.push_back( 4 );
	indices.push_back( 5 );

			// creating the circle of life/circle of vertices
	float radius = 0.5f;
	int circle_vert = 50;
	float cx = 0.5f;
	float cy = -0.5f;
	float cz = 0.0f;
	float x;
	float y;
	float pi = 3.14159f;
	float theta = 0.0f;

	temp.color = D3DXCOLOR( 1.0f, 0.0f, 0.0f, 0.0f );
	temp.pos = D3DXVECTOR3( cx, cy, cz );
	vertices.push_back( temp );

	D3DXVECTOR3 normal = D3DXVECTOR3( 5.0f, 5.0f, 5.0f );


	//something that works -- jacked from zander
	D3DXVECTOR4 temp2;

	// Add the circle's points
	float rotate_x = atan2(normal.y, normal.z);
	float rotate_y = atan2(normal.x, normal.z);
	D3DXMATRIX transform_x;
	D3DXMatrixRotationX(&transform_x, rotate_x);
	D3DXMATRIX transform_y;
	D3DXMatrixRotationY(&transform_y, rotate_y);
	D3DXMATRIX transform_inv = transform_x * transform_y;
	D3DXMatrixInverse(&transform_inv, NULL, &transform_inv);

	temp.color = D3DXCOLOR( 0.0f, 0.0f, 0.2f, 1.0f );
	for( int i = 0; i < circle_vert; i++ ){

		theta = 2 * pi / circle_vert * i;

		x = cos( theta ) * radius;
		y = sin( theta ) * radius;

		temp.pos = D3DXVECTOR3( x, y, cz );
		D3DXVec3Transform( &temp2, &temp.pos, &transform_inv );
		temp.pos.x = temp2.x + cx; temp.pos.y = temp2.y + cy; temp.pos.z = temp2.z;
		vertices.push_back( temp );
	}

	for( int i = 8; i < 8 + circle_vert - 1; i++ ){
		indices.push_back( 6 );
		indices.push_back( i );
		indices.push_back( i-1 );
	}

	indices.push_back( 6 );
	indices.push_back( 7 );
	indices.push_back( 6 + circle_vert );

		// creating the sphere
	int sphere_start_vec = vertices.size() + 1;
	radius = .25f;
	int max_slice = 50;
	int max_stack = 50;
	float z;
	cx = 0;
	cy = 0;
	cz = 0;


	temp.pos = D3DXVECTOR3( cx+radius, cy+radius, cz+radius );
	temp.color = D3DXCOLOR( 0.0f , 1.0f, 0.0f, 1.0f );

	for( int slices = 1; slices < max_slice; slices++ ){
		for( int stacks = 1; stacks < max_stack; stacks++ ){
			x = sin(pi * slices/max_slice) * cos(2*pi * stacks/max_stack)*radius;
			y = sin(pi * slices/max_slice) * sin(2*pi * stacks/max_stack)*radius;
			z = cos(pi * slices/max_slice)*radius;

			temp.pos = D3DXVECTOR3( x,y,z );
			if( slices % 4 == 0 )
				temp.color = D3DXCOLOR( 0.0f, 0.0f, 1.0f, 1.0f );
			else
				temp.color = D3DXCOLOR( 0.0f, 0.0f, 0.0f, 1.0f );
			vertices.push_back( temp );
		}
	}

	int sph_index = indices.size();

	for( int j = 0; j < max_stack; j++ ) {
		for( int i = -4; i < max_slice-6; i++ ){
		
			indices.push_back( sph_index + i + j*max_slice );
			indices.push_back( sph_index + i + 1 +j*max_slice );
			indices.push_back( sph_index + i + max_slice +j*max_slice );

			indices.push_back( sph_index + i + 1 +j*max_slice );
			indices.push_back( sph_index + i + max_slice + 1 +j*max_slice );
			indices.push_back( sph_index + i + max_slice +j*max_slice );

		}

		indices.push_back( sph_index + max_slice-1 );
		indices.push_back( sph_index + max_slice );
		indices.push_back( sph_index );
	}









    // create the vertex buffer
    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));

    bd.Usage = D3D11_USAGE_DYNAMIC;                // write access access by CPU and GPU
    bd.ByteWidth = sizeof(VERTEX) * vertices.size();             // size is the VERTEX struct * 3
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;       // use as a vertex buffer
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;    // allow CPU to write in buffer

    dev->CreateBuffer(&bd, NULL, &pVBuffer);       // create the buffer


    // copy the vertices into the buffer
    D3D11_MAPPED_SUBRESOURCE ms;
    devcon->Map(pVBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);    // map the buffer
	memcpy(ms.pData, &vertices[0], sizeof(VERTEX) * vertices.size());                 // copy the data
    devcon->Unmap(pVBuffer, NULL);                                      // unmap the buffer

	D3D11_BUFFER_DESC ibd;
	ZeroMemory(&ibd, sizeof(ibd));
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.ByteWidth = sizeof(DWORD) * indices.size();
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA iinitData;
    iinitData.pSysMem = &indices[0];
    dev->CreateBuffer(&ibd, &iinitData, &pIBuffer);
}


// this function loads and prepares the shaders
void InitPipeline()
{
    // load and compile the two shaders
    ID3D10Blob *VS, *PS;
    D3DX11CompileFromFile("shaders.hlsl", 0, 0, "VShader", "vs_5_0", 0, 0, 0, &VS, 0, 0);
    D3DX11CompileFromFile("shaders.hlsl", 0, 0, "PShader", "ps_5_0", 0, 0, 0, &PS, 0, 0);

    // encapsulate both shaders into shader objects
    dev->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), NULL, &pVS);
    dev->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), NULL, &pPS);

    // set the shader objects
    devcon->VSSetShader(pVS, 0, 0);
    devcon->PSSetShader(pPS, 0, 0);

    // create the input layout object
    D3D11_INPUT_ELEMENT_DESC ied[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    dev->CreateInputLayout(ied, 2, VS->GetBufferPointer(), VS->GetBufferSize(), &pLayout);
    devcon->IASetInputLayout(pLayout);

    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = 64;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    dev->CreateBuffer(&bd, NULL, &pCBuffer);
    devcon->VSSetConstantBuffers(0, 1, &pCBuffer);
}