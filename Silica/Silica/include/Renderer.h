#pragma once

#include <stdint.h>
#include <vector>

#include "MathTypes.h"

namespace Silica {

	using TextureID = uint32_t;

	struct Vertex {
		Vec2 position;
		Vec2 uv;
		uint32_t color;
	};

	struct DrawCommand {
		uint32_t indexCount;
		uint32_t startIndex;
		int32_t vertexOffset;
		Rect clipRect;
		TextureID textureID;
	};

	struct DrawList {
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		std::vector<DrawCommand> commands;
		std::vector<Rect> clipRectStack;
		std::vector<TextureID> textureIDStack;

		void addDrawCommand();

		Rect getCurrentClipRect() const;
		void pushClipRect(const Rect& rect);
		void popClipRect();

		TextureID getCurrentTextureID() const;
		void pushTextureID(TextureID id);
		void popTextureID();


	};

}



// ----- UI Renderer -----
#include "SWidget.h"

namespace Silica {

	class Renderer {
	public:

		static DrawList s_drawList;

		static void render(WidgetPtr rootWidget, float screenWidth, float screenHeight);

		static void processMouseMove(WidgetPtr rootWidget, float screenWidth, float screenHeight, float mouseX, float mouseY);
		static void processMouseClick(WidgetPtr rootWidget, float screenWidth, float screenHeight, float mouseX, float mouseY);
		static void processMouseUp(WidgetPtr rootWidget, float screenWidth, float screenHeight, float mouseX, float mouseY);
		static void processMouseWheel(WidgetPtr rootWidget, float screenWidth, float screenHeight, float mouseX, float mouseY, float scrollDelta);

		static const DrawList* getDrawData() { return &s_drawList; }

	};

}
