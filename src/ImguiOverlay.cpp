#include "ImguiOverlay.h"

ImguiOverlay::ImguiOverlay(Renderer::Ptr r, Input::Ptr input) :
	mRenderer(r), mInput(input)
{

}

ImguiOverlay::~ImguiOverlay()
{

}


void ImguiOverlay::init(HWND hwnd,int width, int height)
{

	initImGui( hwnd,  width,  height);
	initRendering(width, height);
	initFonts();


}

void ImguiOverlay::initImGui(HWND hwnd, int width, int height)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsClassic();

	auto& io = ImGui::GetIO();
	io.DisplaySize = { (float)width, (float)height };
	io.ImeWindowHandle = hwnd;


	mInput->listen([&io](const Input::Mouse& mouse, const Input::Keyboard& keyboard) {
		io.MousePos = { (float)mouse.x, (float)mouse.y };
		io.MouseDown[0] = mouse.leftButton;
		io.MouseDown[1] = mouse.rightButton;
		io.MouseDown[2] = mouse.middleButton;
		//io.MouseWheel = mouse.scrollWheelValue;
		
		return io.WantCaptureMouse;
		}, 2);

}

void ImguiOverlay::initRendering(int width, int height)
{
	// Create the input layout
	D3D11_INPUT_ELEMENT_DESC local_layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (size_t)(&((ImDrawVert*)0)->pos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (size_t)(&((ImDrawVert*)0)->uv),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (size_t)(&((ImDrawVert*)0)->col), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	mLayout = mRenderer->createLayout(local_layout, ARRAYSIZE(local_layout));
	mConstants = mRenderer->createConstantBuffer(sizeof(4 * 4 * 4));

	{
		D3D11_RASTERIZER_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_NONE;
		desc.ScissorEnable = true;
		desc.DepthClipEnable = true;
		mRasterizer = mRenderer->createOrGetRasterizer(desc);
	}

	{
		D3D11_DEPTH_STENCIL_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.DepthEnable = false;
		desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
		desc.StencilEnable = false;
		desc.FrontFace.StencilFailOp = desc.FrontFace.StencilDepthFailOp = desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		desc.BackFace = desc.FrontFace;
		mDepthStencilState = mRenderer->createDepthStencilState("imgui",desc);
	}


	mEffect = mRenderer->createEffect("hlsl/imgui.fx");
}

void ImguiOverlay::draw(ImDrawData* data)
{
	if (data->DisplaySize.x <= 0.0f || data->DisplaySize.y <= 0.0f || data->TotalIdxCount <= 0)
		return;

	mRenderer->setRenderTarget(mRenderer->getBackbuffer());

	if (mVertexBuffer.expired() || mVertexBufferSize < data->TotalVtxCount )
	{
		if (!mVertexBuffer.expired())
			mRenderer->destroyBuffer(mVertexBuffer);

		mVertexBufferSize = data->TotalIdxCount * 2;
		mVertexBuffer = mRenderer->createBuffer(mVertexBufferSize * sizeof(ImDrawVert),D3D11_BIND_VERTEX_BUFFER,0,D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
	}

	if (mIndexBuffer.expired() || mIndexBufferSize< data->TotalIdxCount)
	{
		if (!mIndexBuffer.expired())
			mRenderer->destroyBuffer(mIndexBuffer);

		mIndexBufferSize = data->TotalIdxCount * 2;
		mIndexBuffer = mRenderer->createBuffer(mIndexBufferSize * sizeof(ImDrawIdx), D3D11_BIND_INDEX_BUFFER, 0, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
	}

	mVertexBuffer->map();
	mIndexBuffer->map();
	for (int n = 0; n < data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = data->CmdLists[n];
		mVertexBuffer->write(cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
		mIndexBuffer->write(cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
	}

	mVertexBuffer->unmap();
	mIndexBuffer->unmap();

	float L = data->DisplayPos.x;
	float R = data->DisplayPos.x + data->DisplaySize.x;
	float T = data->DisplayPos.y;
	float B = data->DisplayPos.y + data->DisplaySize.y;
	float mvp[4][4] =
	{
		{ 2.0f / (R - L),   0.0f,           0.0f,       0.0f },
		{ 0.0f,         2.0f / (T - B),     0.0f,       0.0f },
		{ 0.0f,         0.0f,           0.5f,       0.0f },
		{ (R + L) / (L - R),  (T + B) / (B - T),    0.5f,       1.0f },
	};
	mEffect.lock()->getVariable("ProjectionMatrix")->AsMatrix()->SetMatrix((const float*)mvp);

	D3D11_VIEWPORT vp = {0};
	vp.Width = data->DisplaySize.x;
	vp.Height = data->DisplaySize.y;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = vp.TopLeftX = 0;
	mRenderer->setViewport(vp);

	mRenderer->setIndexBuffer(mIndexBuffer, sizeof(ImDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);
	mRenderer->setVertexBuffer(mVertexBuffer, mLayout.lock()->getSize(), 0);

	mEffect.lock()->render(mRenderer, [this, &data](ID3DX11EffectPass* pass)
		{
			mRenderer->setLayout(mLayout.lock()->bind(pass));
			auto context = mRenderer->getContext();
			int global_idx_offset = 0;
			int global_vtx_offset = 0;
			ImVec2 clip_off = data->DisplayPos;
			for (int n = 0; n < data->CmdListsCount; n++)
			{
				const ImDrawList* cmd_list = data->CmdLists[n];
				for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
				{
					const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
					if (pcmd->UserCallback != NULL)
					{

					}
					else
					{
						const D3D11_RECT r = { (LONG)(pcmd->ClipRect.x - clip_off.x), (LONG)(pcmd->ClipRect.y - clip_off.y), (LONG)(pcmd->ClipRect.z - clip_off.x), (LONG)(pcmd->ClipRect.w - clip_off.y) };
						context->RSSetScissorRects(1, &r);

						// Bind texture, Draw
						ID3D11ShaderResourceView* texture_srv = (ID3D11ShaderResourceView*)pcmd->TextureId;
						context->PSSetShaderResources(0, 1, &texture_srv);
						context->DrawIndexed(pcmd->ElemCount, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset);
					}
				}
				global_idx_offset += cmd_list->IdxBuffer.Size;
				global_vtx_offset += cmd_list->VtxBuffer.Size;
			}
		});
}

void ImguiOverlay::update()
{
	ImGui::NewFrame();

	ImguiObject::root()->update();

	ImGui::Render();

	auto data = ImGui::GetDrawData();
	draw(data);
	
}


void ImguiOverlay::initFonts()
{
	// Build texture atlas
	ImGuiIO& io = ImGui::GetIO();
	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;

	mFonts = mRenderer->createTexture(desc);
	mFonts->blit(pixels,0);

	io.Fonts->TexID = (ImTextureID)mFonts->Renderer::ShaderResource::getView();
}
