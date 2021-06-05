#include "robot.h"
#include <cmath>


using namespace mini;
using namespace gk2;
using namespace DirectX;
using namespace std;

#pragma region Constants
const unsigned int Robot::BS_MASK = 0xffffffff;

const XMFLOAT4 Robot::GREEN_LIGHT_POS = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
const XMFLOAT4 Robot::RED_LIGHT_POS = XMFLOAT4(-1.0f, -1.0f, -1.0f, 1.0f);

const float Robot::ARM_SPEED = 2.0f;
const float Robot::CIRCLE_RADIUS = 0.5f;

const float Robot::SHEET_ANGLE = DirectX::XM_PIDIV4;
const float Robot::SHEET_SIZE = 2.0f;
const XMFLOAT3 Robot::SHEET_POS = XMFLOAT3(0.0f, -0.05f, 0.0f);
const XMFLOAT4 Robot::SHEET_COLOR = XMFLOAT4(1.0f, 1.0f, 1.0f, 255.0f / 255.0f);
const float KACZOR_SIZE = 0.0015f;

const float Robot::WALL_SIZE = 2.0f;
const XMFLOAT3 Robot::WALLS_POS = XMFLOAT3(0.0f, 0.0f, 0.0f);
const XMFLOAT4 LightPos = XMFLOAT4(0.0f, 0.5f, 1.0f, 1.0f);

#pragma endregion


#pragma region Initalization
Robot::Robot(HINSTANCE hInstance)
	: Base(hInstance, 1280, 720, L"Kaczor"),
	m_cbWorld(m_device.CreateConstantBuffer<XMFLOAT4X4>()),
	m_cbView(m_device.CreateConstantBuffer<XMFLOAT4X4, 2>()),
	m_cbLighting(m_device.CreateConstantBuffer<Lighting>()),
	m_cbSurfaceColor(m_device.CreateConstantBuffer<XMFLOAT4>()),
	m_cbdivisions(m_device.CreateConstantBuffer<XMFLOAT4>())
{
	//Projection matrix
	auto s = m_window.getClientSize();
	auto ar = static_cast<float>(s.cx) / s.cy;

	XMStoreFloat4x4(&m_projMtx, XMMatrixPerspectiveFovLH(XM_PIDIV4, ar, 0.01f, 100.0f));
	m_cbProj = m_device.CreateConstantBuffer<XMFLOAT4X4>();
	UpdateBuffer(m_cbProj, m_projMtx);
	XMFLOAT4X4 cameraMtx;
	XMStoreFloat4x4(&cameraMtx, m_camera.getViewMatrix());
	UpdateCameraCB(cameraMtx);
	UpdateBuffer(m_cbdivisions, XMFLOAT4(set_divisions,0,0,0));
	//Sampler States
	SamplerDescription sd;
	sd.Filter = D3D11_FILTER_ANISOTROPIC;
	sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.MaxAnisotropy = 16;
	m_samplerWrap = m_device.CreateSamplerState(sd);

	//Regular shaders
	auto vsCode = m_device.LoadByteCode(L"vs.cso");
	auto psCode = m_device.LoadByteCode(L"ps.cso");
	m_vs = m_device.CreateVertexShader(vsCode);
	m_ps = m_device.CreatePixelShader(psCode);
	m_il = m_device.CreateInputLayout(VertexPositionNormal::Layout, vsCode);


	// texture shaders
	vsCode = m_device.LoadByteCode(L"textureVS.cso");
	psCode = m_device.LoadByteCode(L"texturePS.cso");
	m_textureVS = m_device.CreateVertexShader(vsCode);
	m_texturePS = m_device.CreatePixelShader(psCode);
	m_textureIL = m_device.CreateInputLayout(VertexPositionNormal::Layout, vsCode);

	// texture shaders
	vsCode = m_device.LoadByteCode(L"kaczorVS.cso");
	psCode = m_device.LoadByteCode(L"kaczorPS.cso");
	m_kaczorVS = m_device.CreateVertexShader(vsCode);
	m_kaczorPS = m_device.CreatePixelShader(psCode);
	m_kaczorIL = m_device.CreateInputLayout(VertexPositionNormalTex::Layout, vsCode);

	vsCode = m_device.LoadByteCode(L"bezierVS.cso");
	psCode = m_device.LoadByteCode(L"bezierPS.cso");
	auto dsCode = m_device.LoadByteCode(L"bezierDS.cso");
	auto hsCode = m_device.LoadByteCode(L"bezierHS.cso");
	m_bezierVS = m_device.CreateVertexShader(vsCode);
	m_bezierPS = m_device.CreatePixelShader(psCode);
	m_bezierPS = m_device.CreatePixelShader(psCode);
	m_bezierHS = m_device.CreateHullShader(hsCode);
	m_bezierDS = m_device.CreateDomainShader(dsCode);
	m_bezierIL = m_device.CreateInputLayout(VertexPositionNormalTex::Layout, vsCode);

	psCode = m_device.LoadByteCode(L"bezierTexPS.cso");
	dsCode = m_device.LoadByteCode(L"bezierTexDS.cso");
	m_bezierTexPS = m_device.CreatePixelShader(psCode);
	m_bezierTexDS = m_device.CreateDomainShader(dsCode);

	RasterizerDescription rsDesc;
	rsDesc.CullMode = D3D11_CULL_NONE;
	rsDesc.FillMode = D3D11_FILL_SOLID;
	m_rsCullNon = m_device.CreateRasterizerState(rsDesc);

	rsDesc.FillMode = D3D11_FILL_WIREFRAME;
	m_rsCullNonWire = m_device.CreateRasterizerState(rsDesc);

	//Render states
	CreateRenderStates();

	m_wall = Mesh::Rectangle(m_device);
	m_sheet = Mesh::Rectangle(m_device);
	m_duck = Mesh::LoadMesh(m_device, L"resources/duck/duck.txt");

	m_patch = Mesh::Bezier(m_device,4,4);
	m_patch_lines = Mesh::BezierLines(m_device,4,4);
	m_patch2 = Mesh::Bezier(m_device,4,4,1);
	m_patch2_lines = Mesh::BezierLines(m_device, 4, 4,1);

	SetShaders();
	ID3D11Buffer* vsb[] = { m_cbWorld.get(),  m_cbView.get(), m_cbProj.get() };
	m_device.context()->VSSetConstantBuffers(0, 3, vsb);
	ID3D11Buffer* dsb[] = { m_cbWorld.get(),  m_cbView.get(), m_cbProj.get() };
	m_device.context()->DSSetConstantBuffers(0, 3, dsb);
	ID3D11Buffer* hsb[] = { m_cbWorld.get(),  m_cbView.get(), m_cbProj.get(), m_cbdivisions.get() };
	m_device.context()->HSSetConstantBuffers(0, 4, hsb);
	ID3D11Buffer* psb[] = { m_cbSurfaceColor.get(), m_cbLighting.get() };
	m_device.context()->PSSetConstantBuffers(0, 2, psb);

	m_heightTexture = m_device.CreateShaderResourceView(L"resources/textures/height.dds");
	m_normalTexture = m_device.CreateShaderResourceView(L"resources/textures/normals.dds");
	m_colorTexture = m_device.CreateShaderResourceView(L"resources/textures/diffuse.dds");
}

void Robot::CreateRenderStates()
//Setup render states used in various stages of the scene rendering
{
}

#pragma endregion

#pragma region Per-Frame Update
void Robot::Update(const Clock& c)
{
	Base::Update(c);
	double dt = c.getFrameTime();
	
	if (HandleCameraInput(dt))
	{
		XMFLOAT4X4 cameraMtx;
		XMStoreFloat4x4(&cameraMtx, m_camera.getViewMatrix());
		UpdateCameraCB(cameraMtx);
	}
	if (HandleKeyboard()) {
		UpdateBuffer(m_cbdivisions, XMFLOAT4(set_divisions, 0, 0, 0));
	}
	if (dt < 0.016)
		Sleep(16 - (dt * 1000));
}


void Robot::UpdateCameraCB(DirectX::XMFLOAT4X4 cameraMtx)
{
	XMMATRIX mtx = XMLoadFloat4x4(&cameraMtx);
	XMVECTOR det;
	auto invvmtx = XMMatrixInverse(&det, mtx);
	XMFLOAT4X4 view[2] = { cameraMtx };
	XMStoreFloat4x4(view + 1, invvmtx);
	UpdateBuffer(m_cbView, view);
}

#pragma endregion
#pragma region Frame Rendering Setup
void Robot::SetShaders()
{
	m_device.context()->IASetInputLayout(m_il.get());
	m_device.context()->VSSetShader(m_vs.get(), 0, 0);
	m_device.context()->PSSetShader(m_ps.get(), 0, 0);
	m_device.context()->GSSetShader(nullptr, nullptr, 0);
	m_device.context()->DSSetShader(nullptr, nullptr, 0);
	m_device.context()->HSSetShader(nullptr, nullptr, 0);
	m_device.context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void Robot::SetShaders(const dx_ptr<ID3D11VertexShader>& vs, const dx_ptr<ID3D11PixelShader>& ps)
{
	m_device.context()->VSSetShader(vs.get(), nullptr, 0);
	m_device.context()->PSSetShader(ps.get(), nullptr, 0);
	m_device.context()->DSSetShader(nullptr, nullptr, 0);
	m_device.context()->HSSetShader(nullptr, nullptr, 0);
}

void Robot::SetShaders(const dx_ptr<ID3D11VertexShader>& vs, const dx_ptr<ID3D11PixelShader>& ps, const dx_ptr<ID3D11InputLayout>& il)
{
	m_device.context()->IASetInputLayout(il.get());
	m_device.context()->VSSetShader(vs.get(), nullptr, 0);
	m_device.context()->PSSetShader(ps.get(), nullptr, 0);
	m_device.context()->DSSetShader(nullptr, nullptr, 0);
	m_device.context()->HSSetShader(nullptr, nullptr, 0);
}

void Robot::SetShaders(const dx_ptr<ID3D11VertexShader>& vs, const dx_ptr<ID3D11PixelShader>& ps, const dx_ptr<ID3D11HullShader>& hs, const dx_ptr<ID3D11DomainShader>& ds, const dx_ptr<ID3D11InputLayout>& il)
{
	m_device.context()->IASetInputLayout(il.get());
	m_device.context()->VSSetShader(vs.get(), nullptr, 0);
	m_device.context()->PSSetShader(ps.get(), nullptr, 0);
	m_device.context()->DSSetShader(ds.get(), nullptr, 0);
	m_device.context()->HSSetShader(hs.get(), nullptr, 0);
	m_device.context()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST);
}

void mini::gk2::Robot::KaczorowyDeBoor()
{
	float N[5] = { 0,1,0,0,0 };

	float A[5], B[5];
	int i = (int)kaczordt + 1;
	for (int j = 1; j <= 3; j++) {
		A[j] = T[i+j] - kaczordt;
		B[j] = kaczordt - T[i+1-j];
		float saved = 0;
		for (int k = 1; k <= j; k++) {
			float term = N[k] / (A[k] + B[j +1 - k]);
			N[k] = saved + A[k] * term;
			saved = B[j +1- k] * term;
		}
		N[j + 1] = saved;
	}
	XMFLOAT3 oldPos = kaczorPosition;
	kaczorPosition.x = N[1] * deBoorPoints[i-3].x + N[2] * deBoorPoints[i-2].x + N[3] * deBoorPoints[i-1].x + N[4] * deBoorPoints[i].x;
	kaczorPosition.y = N[1] * deBoorPoints[i-3].y + N[2] * deBoorPoints[i-2].y + N[3] * deBoorPoints[i-1].y + N[4] * deBoorPoints[i].y;
	kaczorPosition.z = N[1] * deBoorPoints[i-3].z + N[2] * deBoorPoints[i-2].z + N[3] * deBoorPoints[i-1].z + N[4] * deBoorPoints[i].z;
	kaczordt += kaczorSpeed;
	XMStoreFloat3(&kaczorDirection, XMVector3Normalize(XMLoadFloat3(&kaczorPosition)-XMLoadFloat3(&oldPos)));
}

void mini::gk2::Robot::CreateKaczorMtx()
{
	m_kaczorMtx = XMMatrixScaling(KACZOR_SIZE, KACZOR_SIZE, KACZOR_SIZE)* XMMatrixRotationAxis(XMVECTOR{ 0, -1, 0 }, std::atan2f(kaczorDirection.x, kaczorDirection.z) + g_XMPi.f[0]) * XMMatrixTranslation(kaczorPosition.z, kaczorPosition.y, kaczorPosition.x);
}

void Robot::SetTextures(std::initializer_list<ID3D11ShaderResourceView*> resList, const dx_ptr<ID3D11SamplerState>& sampler)
{
	m_device.context()->PSSetShaderResources(0, resList.size(), resList.begin());
	auto s_ptr = sampler.get();
	m_device.context()->PSSetSamplers(0, 1, &s_ptr);
}

void Robot::SetTexturesDS(std::initializer_list<ID3D11ShaderResourceView*> resList, const dx_ptr<ID3D11SamplerState>& sampler)
{
	m_device.context()->DSSetShaderResources(0, resList.size(), resList.begin());
	auto s_ptr = sampler.get();
	m_device.context()->DSSetSamplers(0, 1, &s_ptr);
}

void Robot::Set1Light(XMFLOAT4 poition)
//Setup one positional light at the camera
{
	Lighting l{
		/*.ambientColor = */ XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
		/*.surface = */ XMFLOAT4(0.2f, 0.8f, 0.8f, 200.0f),
		/*.lights =*/ {
			{ /*.position =*/ poition, /*.color =*/ XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) }
			//other 2 lights set to 0
		}
	};
	ZeroMemory(&l.lights[1], sizeof(Light) * 2);
	UpdateBuffer(m_cbLighting, l);
}

void Robot::Set3Lights()
//Setup one white positional light at the camera
{
	Lighting l{
		/*.ambientColor = */ XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
		/*.surface = */ XMFLOAT4(0.2f, 0.8f, 0.8f, 200.0f),
		/*.lights =*/ {
			{ /*.position =*/ XMFLOAT4(0.0f,0.5f,0.0f,1.0f), /*.color =*/ XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) }
			//other 2 lights set to 0
		}
	};

	//comment the following line when structure is filled
	ZeroMemory(&l.lights[1], sizeof(Light) * 2);

	UpdateBuffer(m_cbLighting, l);
}
#pragma endregion

#pragma region Drawing
void Robot::SetWorldMtx(DirectX::XMFLOAT4X4 mtx)
{
	UpdateBuffer(m_cbWorld, mtx);
}

void Robot::DrawMesh(const Mesh& m, DirectX::XMFLOAT4X4 worldMtx)
{
	SetWorldMtx(worldMtx);
	m.Render(m_device.context());
}

void Robot::CreateWallsMtx()
{
	XMVECTOR xRot = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR yRot = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR zRot = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);


	float a = WALL_SIZE;
	float scale = WALL_SIZE;

	m_wallsMtx[0] = XMMatrixScaling(scale, scale, scale) * XMMatrixTranslation(0.0f, 0.0f, a / 2);

	for (int i = 1; i < 4; ++i)
	{
		m_wallsMtx[i] = m_wallsMtx[i - 1] * XMMatrixRotationAxis(yRot, DirectX::XM_PIDIV2);
	}
	m_wallsMtx[4] = m_wallsMtx[3] * XMMatrixRotationAxis(zRot, DirectX::XM_PIDIV2);
	m_wallsMtx[5] = m_wallsMtx[4] * XMMatrixRotationAxis(zRot, DirectX::XM_PI);


	for (int i = 0; i < 6; ++i)
	{
		m_wallsMtx[i] = m_wallsMtx[i] * XMMatrixTranslation(WALLS_POS.x, WALLS_POS.y, WALLS_POS.z);
	}
}

void Robot::DrawWalls()
{
	for (int i = 0; i < 6; ++i)
	{
		UpdateBuffer(m_cbWorld, m_wallsMtx[i]);
		UpdateBuffer(m_cbSurfaceColor, XMFLOAT4(1.0f, 0.1f, 0.1f, 0.5f));
		m_wall.Render(m_device.context());
	}
}

void Robot::DrawSheet(bool colors)
{

	UpdateBuffer(m_cbSurfaceColor, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	UpdateBuffer(m_cbWorld, m_sheetMtx);
	m_sheet.Render(m_device.context());

	UpdateBuffer(m_cbSurfaceColor, XMFLOAT4(1.0f, -1.0f, 1.0f, 1.0f));

	UpdateBuffer(m_cbWorld, m_revSheetMtx);
	m_sheet.Render(m_device.context());
}

void Robot::DrawBezier()
{
	
}

bool mini::gk2::Robot::HandleKeyboard()
{
	bool moved = false;
	bool keyboard = true;
	KeyboardState kstate;
	if (!m_keyboard.GetState(kstate))
		keyboard = false;
	if (keyboard) {
		if (kstate.isKeyDown(DIK_Z))// Z
		{
			show_lines = true;
		}
		if (kstate.isKeyDown(DIK_Z) && kstate.isKeyDown(DIK_LCONTROL))// Z
		{
			show_lines = false;
		}
		if (kstate.isKeyDown(DIK_X))// X
		{
			is_solid = true;
		}
		if (kstate.isKeyDown(DIK_X) && kstate.isKeyDown(DIK_LCONTROL))// X
		{
			is_solid = false;
		}
		if (kstate.isKeyDown(DIK_C))// C
		{
			show_second_type = true;
		}
		if (kstate.isKeyDown(DIK_C) && kstate.isKeyDown(DIK_LCONTROL))// C
		{
			show_second_type = false;
		}
		if (kstate.isKeyDown(DIK_T))// T
		{
			is_textured = true;
		}
		if (kstate.isKeyDown(DIK_T) && kstate.isKeyDown(DIK_LCONTROL))// T
		{
			is_textured = false;
		}
		if (kstate.isKeyDown(DIK_W))// W
		{
			set_divisions += 0.01f;
			set_divisions > 3.0f ? set_divisions = 3.0f : set_divisions = set_divisions;
			moved = true;
		}
		if (kstate.isKeyDown(DIK_S))// S
		{
			set_divisions -= 0.01f;
			set_divisions < 0.1f ? set_divisions = 0.1f : set_divisions = set_divisions;
			moved = true;
		}
	}
	return moved;
}

void Robot::CreateSheetMtx()
{
	m_sheetMtx = XMMatrixScaling(SHEET_SIZE, SHEET_SIZE, 1.0f) * XMMatrixRotationX(DirectX::XM_PIDIV2) * XMMatrixTranslationFromVector(XMLoadFloat3(&SHEET_POS));
	m_revSheetMtx = XMMatrixRotationY(-DirectX::XM_PI) * m_sheetMtx;
}

void Robot::GenerateHeightMap()
{
}

void Robot::DrawKaczor()
{
	UpdateBuffer (m_cbWorld, m_kaczorMtx);

	m_duck.Render(m_device.context());
}

void Robot::Render()
{
	Base::Render();
	//KaczorowyDeBoor();
	//GenerateHeightMap();
	//SetShaders(m_textureVS, m_texturePS);
	//SetTextures({ m_waterTexture.get(), m_cubeTexture.get() }, m_samplerWrap);
	//UpdateBuffer(m_cbSurfaceColor, XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f));
	//DrawSheet(true);
	if(is_solid)
		m_device.context()->RSSetState(m_rsCullNon.get());
	else
		m_device.context()->RSSetState(m_rsCullNonWire.get());

	if (is_textured) {
		SetShaders(m_bezierVS, m_bezierTexPS, m_bezierHS, m_bezierTexDS, m_bezierIL);
		SetTexturesDS({ m_heightTexture.get() }, m_samplerWrap);
		SetTextures({ m_colorTexture.get(), m_normalTexture.get() }, m_samplerWrap);
	}
	else {
		SetShaders(m_bezierVS, m_bezierPS, m_bezierHS, m_bezierDS, m_bezierIL);
	}
	UpdateBuffer(m_cbWorld, XMMatrixScaling(1, 1, 1));

	show_second_type? m_patch2.Render(m_device.context()) : m_patch.Render(m_device.context());
	if (show_lines) {
		SetShaders(m_vs, m_ps, m_il);
		show_second_type ? m_patch2_lines.Render(m_device.context()) : m_patch_lines.Render(m_device.context());
	}
	//SetShaders(m_kaczorVS, m_kaczorPS, m_kaczorIL);
	//SetTextures({ m_kaczorTexture.get() }, m_samplerWrap);
	//CreateKaczorMtx();
	//DrawKaczor();

	//SetShaders();
	//SetTextures({ m_cubeTexture.get() }, m_samplerWrap);
	//Set1Light(LightPos);
	//UpdateBuffer(m_cbSurfaceColor, XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f));
	//DrawWalls();
}
#pragma endregion