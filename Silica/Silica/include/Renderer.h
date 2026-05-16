#pragma once

#include <stdint.h>
#include <vector>

#include "MathTypes.h"

namespace Silica {

	struct Vertex {
		Vec2 position;
		Vec2 uv;
		uint32_t color;
	};

	struct DrawCommand {
		uint32_t indexCount;
		uint32_t startIndex;
		int32_t vertexOffset;
	};

	struct DrawList {
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		std::vector<DrawCommand> commands;
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

		static const DrawList* getDrawData() { return &s_drawList; }

	};

}
