#include "Renderer.h"

namespace Silica {

	DrawList Renderer::s_drawList;

	// ----- Draw Command -----
	void DrawList::addDrawCommand() {
		if (!commands.empty() && commands.back().indexCount == 0) {
			commands.back().clipRect = getCurrentClipRect();
			commands.back().textureID = getCurrentTextureID();
			return;
		}
		DrawCommand cmd;
		cmd.indexCount = 0;
		cmd.startIndex = (uint32_t)indices.size();
		cmd.vertexOffset = 0;
		cmd.clipRect = getCurrentClipRect();
		cmd.textureID = getCurrentTextureID();
		commands.push_back(cmd);
	}

	Rect DrawList::getCurrentClipRect() const {
		if (clipRectStack.empty()) return Rect(0, 0, 8192, 8192);
		return clipRectStack.back();
	}

	void DrawList::pushClipRect(const Rect& rect) {
		if (clipRectStack.empty()) {
			clipRectStack.push_back(rect);
		}
		else {
			clipRectStack.push_back(rect.intersect(clipRectStack.back()));
		}
		addDrawCommand();
	}

	void DrawList::popClipRect() {
		if (!clipRectStack.empty()) {
			clipRectStack.pop_back();
		}
		addDrawCommand();
	}

	TextureID DrawList::getCurrentTextureID() const {
		if (textureIDStack.empty()) return 0;
		return textureIDStack.back();
	}

	void DrawList::pushTextureID(TextureID id) {
		textureIDStack.push_back(id);
		addDrawCommand();
	}

	void DrawList::popTextureID() {
		if (!textureIDStack.empty()) textureIDStack.pop_back();
		addDrawCommand();
	}



	// ----- Renderer -----
	void Renderer::render(WidgetPtr rootWidget, float screenWidth, float screenHeight) {
		if (!rootWidget || screenWidth <= 0 || screenHeight <= 0) return;

		// -- Clear Previous Frame's Geometry --
		s_drawList.vertices.clear();
		s_drawList.indices.clear();
		s_drawList.commands.clear();
		s_drawList.clipRectStack.clear();
		s_drawList.textureIDStack.clear();

		s_drawList.pushClipRect(Rect(0, screenWidth, 0, screenHeight));

		Geometry screenGeo = { {0.0f, 0.0f}, {screenWidth, screenHeight} };

		// -- Bottom-Up Layout (Compute Sizes) --
		rootWidget->computeDesiredSize();

		// -- Top-Down Layout (Allocate space) --
		rootWidget->arrangeChildren(screenGeo);

		// -- Draw (Generate Vertices) --
		rootWidget->onDraw(s_drawList, screenGeo);
	}

	void Renderer::processMouseMove(WidgetPtr rootWidget, float screenWidth, float screenHeight, float mouseX, float mouseY) {
		Platform::setCursor(Platform::Cursor::Arrow);

		if (SWidget::getCapturedWidget()) {
			SWidget* captured = SWidget::getCapturedWidget();
			captured->onMouseMove(captured->getAllocatedGeometry(), { mouseX, mouseY });
			return;
		}

		if (rootWidget) {
			Geometry rootGeo = { {0, 0}, {screenWidth, screenHeight} };
			rootWidget->onMouseMove(rootGeo, { mouseX, mouseY });
		}
	}

	void Renderer::processMouseClick(WidgetPtr rootWidget, float screenWidth, float screenHeight, float mouseX, float mouseY) {
		SWidget::setFocusedWidget(nullptr);
		if (rootWidget) {
			Geometry rootGeo = { {0, 0}, {screenWidth, screenHeight} };
			rootWidget->onMouseButtonDown(rootGeo, { mouseX, mouseY });
		}
	}

	void Renderer::processMouseUp(WidgetPtr rootWidget, float screenWidth, float screenHeight, float mouseX, float mouseY) {
		if (SWidget::getCapturedWidget()) {
			SWidget* captured = SWidget::getCapturedWidget();
			captured->onMouseButtonUp(captured->getAllocatedGeometry(), { mouseX, mouseY });
			return;
		}

		if (rootWidget) {
			Geometry rootGeo = { {0, 0}, {screenWidth, screenHeight} };
			rootWidget->onMouseButtonUp(rootGeo, { mouseX, mouseY });
		}
	}

	void Renderer::processMouseWheel(WidgetPtr rootWidget, float screenWidth, float screenHeight, float mouseX, float mouseY, float scrollDelta) {
		if (rootWidget) {
			Geometry rootGeo = { {0, 0}, {screenWidth, screenHeight} };
			rootWidget->onMouseWheel(rootGeo, { mouseX, mouseY }, scrollDelta);
		}
	}

}
