#pragma once

#include "Common.h"
#include "Overlay.h"

#include "imgui/imgui.h"


class ImguiObject;
class ImguiOverlay
{
public:
	using Ptr = std::shared_ptr<ImguiOverlay>;
public:
	ImguiOverlay(Renderer::Ptr renderer, Input::Ptr input);
	~ImguiOverlay();

	void init(HWND hwnd, int width, int height);

	void update();
private:
	void initImGui(HWND hwnd, int width, int height);
	void initRendering(int width, int height);
	void initFonts();
	void draw(ImDrawData* data);
private:
	Renderer::Ptr mRenderer;
	Input::Ptr mInput;
	Renderer::Layout::Ptr mLayout;
	Renderer::Buffer::Ptr mConstants;
	Renderer::Rasterizer::Ptr mRasterizer;
	Renderer::DepthStencilState::Weak mDepthStencilState;
	Renderer::Buffer::Ptr mVertexBuffer;
	Renderer::Buffer::Ptr mIndexBuffer;
	Renderer::Effect::Ptr mEffect;
	Renderer::Texture2D::Ptr mFonts;


	size_t mVertexBufferSize = 0;
	size_t mIndexBufferSize = 0;
};


class ImguiObject
{
public:
	virtual void update() {
		for (auto& i : children)
			i->update();
	};
	template<class T, class ... Args>
	T* createChild(const Args& ... args)
	{
		auto c = new T(args...);
		c->parent = this;
		children.push_back(c);
		return c;
	}

	virtual ~ImguiObject()
	{
		destroy();
	}

	void destroy()
	{
		std::vector<ImguiObject*> temp;
		temp.swap(children);
		for (auto& i : temp)
		{
			i->destroy();
		}

		if (parent)
		{
			auto endi = parent->children.end();
			for (auto i = parent->children.begin(); i != endi; ++i)
			{
				if (*i == this)
				{
					parent->children.erase(i);
					parent = nullptr;
					return;
				}
			}
		}
	}

	ImguiObject* operator[](size_t index)const 
	{
		return children[index];
	}



	std::vector<ImguiObject*> children;
	ImguiObject* parent = nullptr;


	static ImguiObject* root() 
	{
		static ImguiObject r;
		return &r;
	}

	using Cmd = std::function<void(void)>;

	void setCommand(int index, Cmd cmd)
	{
		mCmdMaps[index] = cmd;
	}
protected:
	std::map<int, std::function<void(void)>> mCmdMaps;

};

struct ImguiWindow: public ImguiObject
{
	std::string text;
	bool visible;
	ImGuiWindowFlags flags;

	ImguiWindow(const char* t = "", bool b = true, ImGuiWindowFlags f = 0): 
		text(t), visible(b), flags(f)
	{}
	void update() {
		if (!visible)
			return;

		for (auto& i : mCmdMaps)
			i.second();
		mCmdMaps.clear();

		ImGui::Begin(text.c_str(),&visible, flags);
		ImguiObject::update();
		ImGui::End();
	}

	enum {
		POSITION,
		SIZE,
	};

	void setSize(float x, float y)
	{
		setCommand(SIZE, [=]() {
			ImGui::SetNextWindowSize({ x, y });
			});
	}

	void setPosition(float x, float y)
	{
		setCommand(POSITION, [=]() {
			ImGui::SetNextWindowPos({ x, y });
			});
	}
private:

};

struct ImguiText :public ImguiObject
{
	std::string text;

	void update() {
		ImGui::Text(text.c_str());
		ImguiObject::update();
	}

	ImguiText(const char* c = "") : text(c)
	{
	}
};

struct ImguiMenuItem : public ImguiObject
{
	std::string text;
	bool selected = true;
	bool enabled;

	using Callback = std::function<void(ImguiMenuItem*)>;
	Callback callback;

	ImguiMenuItem(const char* t, Callback cb, bool e = true):
		text(t), callback(cb), enabled(e)
	{
	}

	void update()
	{
		if (ImGui::MenuItem(text.c_str(), nullptr, &selected, enabled) && callback)
			callback(this);

		ImguiObject::update();
	}

};

struct ImguiMenu :public ImguiObject
{
	std::string text;
	bool enabled = true;
	
	void update()
	{
		if (ImGui::BeginMenu(text.c_str(), enabled))
		{
			ImguiObject::update();
			ImGui::EndMenu();
		}
	}


	ImguiMenu(const char* t, bool e):
		text(t), enabled(e)
	{
	}

	ImguiMenuItem* createMenuItem(const char* t, ImguiMenuItem::Callback c, bool enable = true)
	{
		return createChild<ImguiMenuItem>(t, c, enable);
	}

};

struct ImguiMenuBar: public ImguiObject
{
	bool main = false;
	void update() {
		if (main)
		{
			if (ImGui::BeginMainMenuBar())
			{
				ImguiObject::update();
				ImGui::EndMainMenuBar();
			}
		}
		else
		{
			if (ImGui::BeginMenuBar())
			{
				ImguiObject::update();
				ImGui::EndMenuBar();
			}
		}
	}

	ImguiMenuBar(bool m = false) :main(m)
	{
	}

	ImguiMenu* createMenu(const char*  text, bool enable)
	{
		return createChild<ImguiMenu>(text, enable);
	}
};

struct ImguiSlider :public ImguiObject
{
	std::string text;
	float value;
	float valMin;
	float valMax;
	std::string display = "%.3f";
	using Callback = std::function<void(ImguiSlider*)>;
	Callback callback;


	void update()
	{
		if (ImGui::SliderFloat(text.c_str(), &value, valMin, valMax, display.c_str()) && callback)
		{
			callback(this);
		}
	}
	ImguiSlider(const char* t, float v, float vmin, float vmax, Callback cb):
		text(t), value(v),valMin(vmin), valMax(vmax),callback(cb)
	{

	}
};