#include "Renderer.h"

namespace Silica {

	DrawList Renderer::s_drawList;
	Vec2 Renderer::s_mousePosition;
	std::vector<PopupRecord> Renderer::s_popups;

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

	void DrawList::addThickLine(const Vec2& p0, const Vec2& p1, float thickness, Color color) {
		Vec2 dir = { p1.x - p0.x, p1.y - p0.y };
		float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
		if (len < 0.001f) return;
		dir.x /= len; dir.y /= len;

		// Perpendicular normal vector
		Vec2 normal = { -dir.y, dir.x };
		Vec2 t = { normal.x * (thickness * 0.5f), normal.y * (thickness * 0.5f) };

		uint32_t startIndex = (uint32_t)vertices.size();
		vertices.push_back({ {p0.x - t.x, p0.y - t.y}, {0.0f, 0.0f}, color });
		vertices.push_back({ {p0.x + t.x, p0.y + t.y}, {0.0f, 0.0f}, color });
		vertices.push_back({ {p1.x + t.x, p1.y + t.y}, {0.0f, 0.0f}, color });
		vertices.push_back({ {p1.x - t.x, p1.y - t.y}, {0.0f, 0.0f}, color });

		indices.push_back(startIndex + 0);
		indices.push_back(startIndex + 1);
		indices.push_back(startIndex + 2);
		indices.push_back(startIndex + 0);
		indices.push_back(startIndex + 2);
		indices.push_back(startIndex + 3);

		if (commands.empty()) commands.push_back({ 0, 0, 0 });
		commands.back().indexCount += 6;
	}

	void DrawList::addBezierCurve(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3, float thickness, Color color) {
		int segments = 40;
		std::vector<Vec2> points;
		points.reserve(segments + 1);

		// -- Calculate All The Points Along The Curve --
		for (int i = 0; i <= segments; ++i) {
			float t = (float)i / (float)segments;
			float u = 1.0f - t;
			float tt = t * t;
			float uu = u * u;
			float uuu = uu * u;
			float ttt = tt * t;

			points.push_back({
				uuu * p0.x + 3 * uu * t * p1.x + 3 * u * tt * p2.x + ttt * p3.x,
				uuu * p0.y + 3 * uu * t * p1.y + 3 * u * tt * p2.y + ttt * p3.y
			});
		}

		uint32_t startIndex = (uint32_t)vertices.size();

		// -- Generate A Continuous Ribbon Of Vertices --
		for (size_t i = 0; i < points.size(); ++i) {
			Vec2 dir;
			if (i == 0) {
				dir = { points[1].x - points[0].x, points[1].y - points[0].y };
			}
			else if (i == points.size() - 1) {
				dir = { points[i].x - points[i - 1].x, points[i].y - points[i - 1].y };
			}
			else {
				dir = { points[i + 1].x - points[i - 1].x, points[i + 1].y - points[i - 1].y };
			}

			float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
			if (len > 0.001f) { dir.x /= len; dir.y /= len; }

			Vec2 normal = { -dir.y, dir.x };

			vertices.push_back({ {points[i].x + normal.x * (thickness * 0.5f), points[i].y + normal.y * (thickness * 0.5f)}, {-thickness, 0.0f}, color });
			vertices.push_back({ {points[i].x - normal.x * (thickness * 0.5f), points[i].y - normal.y * (thickness * 0.5f)}, {-thickness, 1.0f}, color });
		}

		// -- Connect Them All With Indices --
		for (int i = 0; i < segments; ++i) {
			uint32_t base = startIndex + (i * 2);
			indices.push_back(base + 0); indices.push_back(base + 1); indices.push_back(base + 2);
			indices.push_back(base + 1); indices.push_back(base + 3); indices.push_back(base + 2);
		}

		if (commands.empty()) commands.push_back({ 0, 0, 0 });
		commands.back().indexCount += segments * 6;
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

		s_popups.clear();

		// -- Main Tree --
		rootWidget->computeDesiredSize();
		rootWidget->arrangeChildren(screenGeo);

		// -- Draw Main Tree --
		rootWidget->onDraw(s_drawList, screenGeo);

		// -- Draw Popup On Top --
		for (const auto& popup : s_popups) {
			s_drawList.pushClipRect(Rect(0, screenWidth, 0, screenHeight));
			popup.widget->onDraw(s_drawList, popup.geometry);
			s_drawList.popClipRect();
		}
	}

	void Renderer::processMouseMove(WidgetPtr rootWidget, float screenWidth, float screenHeight, float mouseX, float mouseY) {
		s_mousePosition = { mouseX, mouseY };
		Platform::setCursor(Platform::Cursor::Arrow);

		if (SWidget::getCapturedWidget()) {
			SWidget* captured = SWidget::getCapturedWidget();
			captured->onMouseMove(captured->getAllocatedGeometry(), { mouseX, mouseY });
			return;
		}

		bool popupHandled = false;

		for (auto it = s_popups.rbegin(); it != s_popups.rend(); ++it) {
			if (!popupHandled && it->geometry.contains({ mouseX, mouseY })) {
				if (it->widget->onMouseMove(it->geometry, { mouseX, mouseY }).isHandled) {
					popupHandled = true;
					continue;
				}
			}

			it->widget->onMouseMove(it->geometry, popupHandled ? Vec2(-9999.0f, -9999.0f) : Vec2(mouseX, mouseY));
		}

		if (rootWidget) {
			Geometry rootGeo = { {0, 0}, {screenWidth, screenHeight} };
			rootWidget->onMouseMove(rootGeo, popupHandled ? Vec2(-9999.0f, -9999.0f) : Vec2(mouseX, mouseY));
		}
	}

	void Renderer::processMouseDown(WidgetPtr rootWidget, float screenWidth, float screenHeight, float mouseX, float mouseY, MouseButton button) {
		SWidget::setFocusedWidget(nullptr);

		bool hitPopup = false;
		for (auto it = s_popups.rbegin(); it != s_popups.rend(); ++it) {
			if (it->geometry.contains({ mouseX, mouseY })) {
				it->widget->onMouseButtonDown(it->geometry, { mouseX, mouseY }, button);
				hitPopup = true;
				break;
			}
		}

		if (!hitPopup && !s_popups.empty()) {
			for (const auto& popup : s_popups) {
				if (popup.closeCallback) popup.closeCallback();
			}
		}

		if (!hitPopup && rootWidget) {
			Geometry rootGeo = { {0, 0}, {screenWidth, screenHeight} };
			rootWidget->onMouseButtonDown(rootGeo, { mouseX, mouseY }, button);
		}
	}

	void Renderer::processMouseUp(WidgetPtr rootWidget, float screenWidth, float screenHeight, float mouseX, float mouseY, MouseButton button) {
		if (SWidget::getCapturedWidget()) {
			SWidget* captured = SWidget::getCapturedWidget();
			captured->onMouseButtonUp(captured->getAllocatedGeometry(), { mouseX, mouseY }, button);
			return;
		}

		for (auto it = s_popups.rbegin(); it != s_popups.rend(); ++it) {
			if (it->geometry.contains({ mouseX, mouseY })) {
				EventReply reply = it->widget->onMouseButtonUp(it->geometry, { mouseX, mouseY }, button);
				if (reply.isHandled) {
					if (it->closeCallback) it->closeCallback();
					return;
				}
			}
		}

		if (rootWidget) {
			Geometry rootGeo = { {0, 0}, {screenWidth, screenHeight} };
			rootWidget->onMouseButtonUp(rootGeo, { mouseX, mouseY }, button);
		}
	}

	void Renderer::processMouseWheel(WidgetPtr rootWidget, float screenWidth, float screenHeight, float mouseX, float mouseY, float scrollDelta) {
		for (auto it = s_popups.rbegin(); it != s_popups.rend(); ++it) {
			if (it->geometry.contains({ mouseX, mouseY })) {
				if (it->widget->onMouseWheel(it->geometry, { mouseX, mouseY }, scrollDelta).isHandled) return;
			}
		}

		if (rootWidget) {
			Geometry rootGeo = { {0, 0}, {screenWidth, screenHeight} };
			rootWidget->onMouseWheel(rootGeo, { mouseX, mouseY }, scrollDelta);
		}
	}

	void Renderer::pushPopup(WidgetPtr widget, const Geometry& geo, std::function<void()> closeCallback) {
		s_popups.push_back({ widget, geo, closeCallback });
	}

}
