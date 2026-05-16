#include "Renderer.h"

namespace Silica {

	DrawList Renderer::s_drawList;

	void Renderer::render(WidgetPtr rootWidget, float screenWidth, float screenHeight) {
		if (!rootWidget || screenWidth <= 0 || screenHeight <= 0) return;

		// -- Clear Previous Frame's Geometry --
		s_drawList.vertices.clear();
		s_drawList.indices.clear();
		s_drawList.commands.clear();

		Geometry screenGeo = { {0.0f, 0.0f}, {screenWidth, screenHeight} };

		// -- Bottom-Up Layout (Compute Sizes) --
		rootWidget->computeDesiredSize();

		// -- Top-Down Layout (Allocate space) --
		rootWidget->arrangeChildren(screenGeo);

		// -- Draw (Generate Vertices) --
		rootWidget->onDraw(s_drawList, screenGeo);
	}

	void Renderer::processMouseMove(WidgetPtr rootWidget, float screenWidth, float screenHeight, float mouseX, float mouseY) {
		if (!rootWidget) return;
		Geometry screenGeo = { {0.0f, 0.0f}, {screenWidth, screenHeight} };
		rootWidget->onMouseMove(screenGeo, { mouseX, mouseY });
	}

	void Renderer::processMouseClick(WidgetPtr rootWidget, float screenWidth, float screenHeight, float mouseX, float mouseY) {
		if (!rootWidget) return;
		Geometry screenGeo = { {0.0f, 0.0f}, {screenWidth, screenHeight} };
		rootWidget->onMouseButtonDown(screenGeo, { mouseX, mouseY });
	}

	void Renderer::processMouseUp(WidgetPtr rootWidget, float screenWidth, float screenHeight, float mouseX, float mouseY) {
		if (!rootWidget) return;
		Geometry screenGeo = { {0.0f, 0.0f}, {screenWidth, screenHeight} };
		rootWidget->onMouseButtonUp(screenGeo, { mouseX, mouseY });
	}

}
