#include "log.h"
#include "windows-util.h"

#include <d3dcompiler.h>
#include <d3d11.h>
#include <fstream>
#include <string>
#include <thread>

std::string loadFile(const std::string& fileName) {
  std::ifstream ifs(fileName.c_str(),
    std::ios::in | std::ios::binary | std::ios::ate);
  std::ifstream::pos_type fileSize = ifs.tellg();
  ifs.seekg(0, std::ios::beg);
  std::vector<char> bytes(fileSize);
  ifs.read(&bytes[0], fileSize);
  return std::string(&bytes[0], fileSize);
}

LRESULT CALLBACK windowsMessageCallback(HWND hWindow, UINT message,
  WPARAM wParam, LPARAM lParam) {
  switch (message) {
  case WM_CLOSE:
    PostQuitMessage(0);
    return 0;
  default:
    return DefWindowProc(hWindow, message, wParam, lParam);
  }
}

int main()
{
  using namespace sandbox;
  WindowsConsole console;
  Logger log(console, "Main");

  HINSTANCE hInstance = GetModuleHandle(NULL);

  log.verbose() << "Registering window class... ";
  WNDCLASSEX windowClass;
  ZeroMemory(&windowClass, sizeof(WNDCLASSEX));
  windowClass.cbSize = sizeof(WNDCLASSEX);
  windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  windowClass.lpfnWndProc = windowsMessageCallback;
  windowClass.hInstance = hInstance;
  windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
  windowClass.lpszClassName = "sandbox";
  if (!RegisterClassEx(&windowClass)) {
    log.fatal() << WindowsError::last();
    return 0;
  }

  log.verbose() << "Done. ";

  log.verbose() << "Creating window... ";
  HWND hWindow = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW, "sandbox",
    "Sandpiles DirectX", WS_OVERLAPPEDWINDOW, 80, 80,
    1024, 1024, NULL, NULL, hInstance, NULL);

  log.verbose() << "Acquiring device context... ";
  HDC hDeviceContext = GetDC(hWindow);

  IDXGIFactory* pFactory;
  if (FAILED(CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&pFactory))))
  {
    log.fatal() << WindowsError::last();
    return 0;
  }

  IDXGIAdapter* pAdapter;
  if (FAILED(pFactory->EnumAdapters(0, &pAdapter)))
  {
    log.fatal() << WindowsError::last();
    return 0;
  }

  IDXGIOutput* pOutput;
  if (FAILED(pAdapter->EnumOutputs(0, &pOutput))) {
    log.fatal() << WindowsError::last();
    return 0;
  }

  //UINT numModes;
  //if (FAILED(pOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, nullptr)))
  //{
  //  log.fatal() << WindowsError::last();
  //  return 0;
  //}
  //
  //std::vector<DXGI_MODE_DESC> modes(numModes);
  //if (FAILED(pOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, modes.data())))
  //{
  //  log.fatal() << WindowsError::last();
  //  return 0;
  //}

  DXGI_SWAP_CHAIN_DESC swapChainDesc;
  ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
  swapChainDesc.BufferCount = 2;
  swapChainDesc.BufferDesc.Width = 1024;
  swapChainDesc.BufferDesc.Height = 1024;
  swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
  swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
  swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapChainDesc.OutputWindow = hWindow;
  swapChainDesc.SampleDesc.Count = 1;
  swapChainDesc.SampleDesc.Quality = 0;
  swapChainDesc.Windowed = TRUE;
  swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
  swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
  swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
  swapChainDesc.Flags = 0;

  IDXGISwapChain* pSwapChain;
  ID3D11Device* pDevice;
  ID3D11DeviceContext* pContext;
  D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_1;
  if (FAILED(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
    D3D11_CREATE_DEVICE_DEBUG, &featureLevel, 1, D3D11_SDK_VERSION,
    &swapChainDesc, &pSwapChain, &pDevice, NULL, &pContext)))
  {
    log.fatal() << WindowsError::last();
    return 0;
  }

  ID3D11Texture2D* pBackBuffer;
  if (FAILED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*) &pBackBuffer)))
  {
    log.fatal() << WindowsError::last();
    return 0;
  }

  ID3D11RenderTargetView* pRenderTargetView;
  if (FAILED(pDevice->CreateRenderTargetView(pBackBuffer, NULL, &pRenderTargetView)))
  {
    log.fatal() << WindowsError::last();
    return 0;
  }

  constexpr size_t dim = 1024;

  D3D11_TEXTURE2D_DESC colorTexDesc;
  colorTexDesc.Width = dim;
  colorTexDesc.Height = dim;
  colorTexDesc.MipLevels = 0;
  colorTexDesc.ArraySize = 1;
  colorTexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  colorTexDesc.SampleDesc.Count = 1;
  colorTexDesc.SampleDesc.Quality = 0;
  colorTexDesc.Usage = D3D11_USAGE_DEFAULT;
  colorTexDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
  colorTexDesc.CPUAccessFlags = 0;
  colorTexDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

  D3D11_RENDER_TARGET_VIEW_DESC colorFboDesc;
  colorFboDesc.Format = colorTexDesc.Format;
  colorFboDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
  colorFboDesc.Texture2D.MipSlice = 0;

  D3D11_SHADER_RESOURCE_VIEW_DESC colorTexSrvDesc;
  colorTexSrvDesc.Format = colorTexDesc.Format;
  colorTexSrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
  colorTexSrvDesc.Texture2D.MostDetailedMip = 0;
  colorTexSrvDesc.Texture2D.MipLevels = 5;

  ID3D11Texture2D* colorTex;
  if (FAILED(pDevice->CreateTexture2D(&colorTexDesc, NULL, &colorTex)))
  {
    log.fatal() << "Error creating colorize texture. ";
    return 0;
  }

  ID3D11RenderTargetView* colorFbo;
  if (FAILED(pDevice->CreateRenderTargetView(colorTex, &colorFboDesc, &colorFbo)))
  {
    log.fatal() << "Error creating colorize fbo. ";
    return 0;
  }

  ID3D11ShaderResourceView* colorTexSrv;
  if (FAILED(pDevice->CreateShaderResourceView(colorTex, &colorTexSrvDesc, &colorTexSrv)))
  {
    log.fatal() << "Error creating colorize shader resource view. ";
    return 0;
  }

  D3D11_TEXTURE2D_DESC sandTexDesc;
  sandTexDesc.Width = dim;
  sandTexDesc.Height = dim;
  sandTexDesc.MipLevels = 1;
  sandTexDesc.ArraySize = 1;
  sandTexDesc.Format = DXGI_FORMAT_R32_UINT;
  sandTexDesc.SampleDesc.Count = 1;
  sandTexDesc.SampleDesc.Quality = 0;
  sandTexDesc.Usage = D3D11_USAGE_DEFAULT;
  sandTexDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
  sandTexDesc.CPUAccessFlags = 0;
  sandTexDesc.MiscFlags = 0;

  std::vector<unsigned int> sandData(dim * dim);
  for (unsigned int& sand : sandData)
  {
    sand = 0;
  }

  sandData[(dim * dim / 2) + dim / 2] = 4'000'000'000;

  D3D11_SUBRESOURCE_DATA initialSand;
  initialSand.pSysMem = sandData.data();
  initialSand.SysMemPitch = sandData.size() * sizeof(unsigned int) / dim;
  initialSand.SysMemSlicePitch = sandData.size() * sizeof(unsigned int);

  D3D11_RENDER_TARGET_VIEW_DESC sandFboDesc;
  sandFboDesc.Format = sandTexDesc.Format;
  sandFboDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
  sandFboDesc.Texture2D.MipSlice = 0;

  D3D11_SHADER_RESOURCE_VIEW_DESC sandTexSrvDesc;
  sandTexSrvDesc.Format = sandTexDesc.Format;
  sandTexSrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
  sandTexSrvDesc.Texture2D.MostDetailedMip = 0;
  sandTexSrvDesc.Texture2D.MipLevels = 1;

  ID3D11Texture2D* sandTex[2];
  ID3D11RenderTargetView* sandFbo[2];
  ID3D11ShaderResourceView* sandTexSrv[2];
  for (size_t i = 0; i < 2; i++)
  {
    if (FAILED(pDevice->CreateTexture2D(&sandTexDesc, &initialSand, &sandTex[i])))
    {
      log.fatal() << "Error creating sandpile texture " << i << ". ";
      return 0;
    }

    if (FAILED(pDevice->CreateRenderTargetView(sandTex[i], &sandFboDesc, &sandFbo[i])))
    {
      log.fatal() << "Error creating sandpile fbo " << i << ". ";
      return 0;
    }

    if (FAILED(pDevice->CreateShaderResourceView(sandTex[i], &sandTexSrvDesc, &sandTexSrv[i])))
    {
      log.fatal() << "Error creating sandpile shader resource view " << i << ". ";
      return 0;
    }
  }

  D3D11_SAMPLER_DESC samplerDesc;
  samplerDesc.Filter = D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
  samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
  samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
  samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
  samplerDesc.MipLODBias = 0.0f;
  samplerDesc.MaxAnisotropy = 1;
  samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
  samplerDesc.BorderColor[0] = 0;
  samplerDesc.BorderColor[1] = 0;
  samplerDesc.BorderColor[2] = 0;
  samplerDesc.BorderColor[3] = 0;
  samplerDesc.MinLOD = 0;
  samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

  ID3D11SamplerState* pSamplerState;
  if (FAILED(pDevice->CreateSamplerState(&samplerDesc, &pSamplerState)))
  {
    log.fatal() << "Error creating sampler state. ";
    return 0;
  }

  D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
  ZeroMemory(&depthStencilDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
  depthStencilDesc.DepthEnable = false;
  depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
  depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
  depthStencilDesc.StencilEnable = false;
  depthStencilDesc.StencilReadMask = 0xFF;
  depthStencilDesc.StencilWriteMask = 0xFF;
  depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
  depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
  depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
  depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_NEVER;
  depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
  depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
  depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
  depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_NEVER;

  ID3D11DepthStencilState* pDepthStencilState;
  if (FAILED(pDevice->CreateDepthStencilState(&depthStencilDesc, &pDepthStencilState)))
  {
    log.fatal() << WindowsError::last();
    return 0;
  }

  pContext->OMSetDepthStencilState(pDepthStencilState, 1);

  D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
  ZeroMemory(&depthStencilViewDesc, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
  depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
  depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
  depthStencilViewDesc.Texture2D.MipSlice = 0;

  pContext->OMSetRenderTargets(1, &pRenderTargetView, nullptr);

  D3D11_RASTERIZER_DESC rasterDesc;
  rasterDesc.AntialiasedLineEnable = false;
  rasterDesc.CullMode = D3D11_CULL_BACK;
  rasterDesc.DepthBias = 0;
  rasterDesc.DepthBiasClamp = 0.0f;
  rasterDesc.DepthClipEnable = true;
  rasterDesc.FillMode = D3D11_FILL_SOLID;
  rasterDesc.FrontCounterClockwise = true;
  rasterDesc.MultisampleEnable = false;
  rasterDesc.ScissorEnable = false;
  rasterDesc.SlopeScaledDepthBias = 0.0f;

  ID3D11RasterizerState* pRasterState;
  if (FAILED(pDevice->CreateRasterizerState(&rasterDesc, &pRasterState)))
  {
    log.fatal() << WindowsError::last();
    return 0;
  }
  pContext->RSSetState(pRasterState);

  D3D11_VIEWPORT viewport;
  viewport.Width = 1024.0f;
  viewport.Height = 1024.0f;
  viewport.MinDepth = 0.0f;
  viewport.MaxDepth = 1.0f;
  viewport.TopLeftX = 0.0f;
  viewport.TopLeftY = 0.0f;

  D3D11_VIEWPORT sandpileViewport;
  sandpileViewport.Width = dim;
  sandpileViewport.Height = dim;
  sandpileViewport.MinDepth = 0.0f;
  sandpileViewport.MaxDepth = 1.0f;
  sandpileViewport.TopLeftX = 0.0f;
  sandpileViewport.TopLeftY = 0.0f;

  ID3DBlob* pBuffer;
  ID3DBlob* pErrs;
  ID3D11VertexShader* pVertexShader;
  std::string vertexShaderSource = loadFile("display.vs.hlsl");
  if (FAILED(D3DCompile(vertexShaderSource.c_str(), vertexShaderSource.size(), "display.vs.hlsl", nullptr, nullptr, "main", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, &pBuffer, &pErrs)))
  {
    const char* begin = reinterpret_cast<const char*>(pErrs->GetBufferPointer());
    const char* end = begin + pErrs->GetBufferSize();
    log.fatal() << "Vertex shader failed to compile: " << std::string(begin, end);
    return 0;
  }

  if (FAILED(pDevice->CreateVertexShader(pBuffer->GetBufferPointer(), pBuffer->GetBufferSize(), nullptr, &pVertexShader)))
  {
    log.fatal() << WindowsError::last();
    return 0;
  }

  D3D11_INPUT_ELEMENT_DESC layoutDesc[1];
  layoutDesc[0].SemanticName = "POSITION";
  layoutDesc[0].SemanticIndex = 0;
  layoutDesc[0].Format = DXGI_FORMAT_R32G32_FLOAT;
  layoutDesc[0].InputSlot = 0;
  layoutDesc[0].AlignedByteOffset = 0;
  layoutDesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
  layoutDesc[0].InstanceDataStepRate = 0;

  ID3D11InputLayout* pLayout;
  if (FAILED(pDevice->CreateInputLayout(layoutDesc, 1, pBuffer->GetBufferPointer(), pBuffer->GetBufferSize(), &pLayout)))
  {
    log.fatal() << WindowsError::last();
    return 0;
  }

  ID3D11PixelShader* pFragmentShader;
  std::string fragmentShaderSource = loadFile("display.fs.hlsl");
  if (FAILED(D3DCompile(fragmentShaderSource.c_str(), fragmentShaderSource.size(), "display.fs.hlsl", nullptr, nullptr, "main", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, &pBuffer, &pErrs)))
  {
    const char* begin = reinterpret_cast<const char*>(pErrs->GetBufferPointer());
    const char* end = begin + pErrs->GetBufferSize();
    log.fatal() << "Fragment shader failed to compile: " << std::string(begin, end);
    return 0;
  }

  if (FAILED(pDevice->CreatePixelShader(pBuffer->GetBufferPointer(), pBuffer->GetBufferSize(), nullptr, &pFragmentShader)))
  {
    log.fatal() << WindowsError::last();
    return 0;
  }

  ID3D11PixelShader* pSandpileShader;
  std::string sandpileSource = loadFile("sandpile.fs.hlsl");
  if (FAILED(D3DCompile(sandpileSource.c_str(), sandpileSource.size(), "sandpile.fs.hlsl", nullptr, nullptr, "main", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, &pBuffer, &pErrs)))
  {
    const char* begin = reinterpret_cast<const char*>(pErrs->GetBufferPointer());
    const char* end = begin + pErrs->GetBufferSize();
    log.fatal() << "Sandpile shader failed to compile: " << std::string(begin, end);
    return 0;
  }

  if (FAILED(pDevice->CreatePixelShader(pBuffer->GetBufferPointer(), pBuffer->GetBufferSize(), nullptr, &pSandpileShader)))
  {
    log.fatal() << WindowsError::last();
    return 0;
  }

  ID3D11PixelShader* pColorizeShader;
  std::string colorizeSource = loadFile("colorize.fs.hlsl");
  if (FAILED(D3DCompile(colorizeSource.c_str(), colorizeSource.size(), "colorize.fs.hlsl", nullptr, nullptr, "main", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, &pBuffer, &pErrs)))
  {
    const char* begin = reinterpret_cast<const char*>(pErrs->GetBufferPointer());
    const char* end = begin + pErrs->GetBufferSize();
    log.fatal() << "Colorize shader failed to compile: " << std::string(begin, end);
    return 0;
  }

  if (FAILED(pDevice->CreatePixelShader(pBuffer->GetBufferPointer(), pBuffer->GetBufferSize(), nullptr, &pColorizeShader)))
  {
    log.fatal() << WindowsError::last();
    return 0;
  }

  float vertices[12] {
    -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
    -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f,
  };

  D3D11_BUFFER_DESC vboDesc;
  vboDesc.Usage = D3D11_USAGE_DEFAULT;
  vboDesc.ByteWidth = sizeof(vertices);
  vboDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
  vboDesc.CPUAccessFlags = 0;
  vboDesc.MiscFlags = 0;

  D3D11_SUBRESOURCE_DATA vboData;
  vboData.pSysMem = vertices;
  vboData.SysMemPitch = 0;
  vboData.SysMemSlicePitch = 0;

  ID3D11Buffer* pVbo;
  if (FAILED(pDevice->CreateBuffer(&vboDesc, &vboData, &pVbo)))
  {
    log.fatal() << WindowsError::last();
    return 0;
  }

  UINT strides[1] { sizeof(float) * 2, };
  UINT offsets[1] { 0, };
  pContext->IASetInputLayout(pLayout);
  pContext->IASetVertexBuffers(0, 1, &pVbo, strides, offsets);
  pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  ShowWindow(hWindow, SW_SHOWDEFAULT);

  ID3D11ShaderResourceView* nullSrv = nullptr;

  pContext->VSSetShader(pVertexShader, nullptr, 0);

  std::chrono::high_resolution_clock::time_point now;
  std::chrono::high_resolution_clock::time_point then;
  std::chrono::duration<float> avg = std::chrono::seconds(0);
  std::chrono::duration<float> elapsed;
  size_t frames = 0;
  std::chrono::duration<float> logTimer = std::chrono::seconds(0);

  bool running = true;
  size_t pingPongIndex = 0;
  MSG message;
  then = std::chrono::high_resolution_clock::now();
  while (running) {
    if (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
      if (message.message == WM_QUIT) {
        running = false;
      } else {
        TranslateMessage(&message);
        DispatchMessage(&message);
      }
    } else {
      now = std::chrono::high_resolution_clock::now();
      elapsed = now - then;
      avg += elapsed;
      ++frames;
      logTimer += elapsed;
      if (logTimer >= std::chrono::seconds(1))
      {
        log.verbose() << frames / avg.count() << " FPS";
        logTimer = std::chrono::seconds(0);
      }

      // ======== Sand pass ========
      for (size_t i = 0; i < 10'000; i++) {
        pContext->RSSetViewports(1, &sandpileViewport);

        pContext->PSSetShaderResources(0, 1, &nullSrv);
        pContext->OMSetRenderTargets(1, &sandFbo[pingPongIndex], nullptr);
        pContext->PSSetShader(pSandpileShader, nullptr, 0);
        pContext->PSSetShaderResources(0, 1, &sandTexSrv[1 - pingPongIndex]);
        pContext->Draw(6, 0);
        pingPongIndex = (pingPongIndex + 1) % 2;
      }

      // ======== Colorize pass ========
      pContext->OMSetRenderTargets(1, &colorFbo, nullptr);
      pContext->PSSetShader(pColorizeShader, nullptr, 0);
      pContext->PSSetShaderResources(0, 1, &sandTexSrv[pingPongIndex]);
      pContext->Draw(6, 0);
      pContext->GenerateMips(colorTexSrv);

      // ======== Final pass ========
      pContext->RSSetViewports(1, &viewport);
      pContext->OMSetRenderTargets(1, &pRenderTargetView, nullptr);
      pContext->PSSetShader(pFragmentShader, nullptr, 0);
      pContext->PSSetShaderResources(0, 1, &colorTexSrv);
      pContext->PSSetSamplers(0, 1, &pSamplerState);
      pContext->Draw(6, 0);

      // Swap buffers
      pSwapChain->Present(0, DXGI_PRESENT_DO_NOT_WAIT);
      then = now;
    }
  }

  return 0;
}
