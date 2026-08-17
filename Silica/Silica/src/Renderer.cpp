#include "silicapch.h"
#include "Renderer.h"

#include "SWidget.h"
#include "FontAtlas.h"
#include "DragDrop.h"

namespace Silica {

	// ----- SWidget Implementation -----
	SWidget* SWidget::s_focusedWidget = nullptr;
	SWidget* SWidget::s_capturedWidget = nullptr;
	SWidget* SWidget::s_dragHoveredWidget = nullptr;



	DrawList Renderer::s_drawList;
	Vec2 Renderer::s_mousePosition;
	std::vector<PopupRecord> Renderer::s_popups;
	std::string Renderer::s_tooltipText = "";
	FontAtlas* Renderer::s_tooltipFont = nullptr;

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

	void DrawList::addRect(const Geometry& geo, Color color) {
		Rect clip = getCurrentClipRect();
		if ((int)clip.right <= (int)clip.left || (int)clip.bottom <= (int)clip.top) return;

		uint32_t startIndex = (uint32_t)vertices.size();

		vertices.push_back({ {geo.position.x, geo.position.y}, {0.0f, 0.0f}, color }); // TL
		vertices.push_back({ {geo.position.x + geo.size.x, geo.position.y}, {0.0f, 0.0f}, color }); // TR
		vertices.push_back({ {geo.position.x + geo.size.x, geo.position.y + geo.size.y}, {0.0f, 0.0f}, color }); // BR
		vertices.push_back({ {geo.position.x, geo.position.y + geo.size.y}, {0.0f, 0.0f}, color }); // BL

		indices.push_back(startIndex + 0);
		indices.push_back(startIndex + 1);
		indices.push_back(startIndex + 2);
		indices.push_back(startIndex + 0);
		indices.push_back(startIndex + 2);
		indices.push_back(startIndex + 3);

		if (commands.empty()) {
			commands.push_back({ 0, 0, 0 });
		}
		commands.back().indexCount += 6;
	}

	void DrawList::addGradientRect(const Geometry& geo, Color tl, Color tr, Color br, Color bl) {
		Rect clip = getCurrentClipRect();
		if ((int)clip.right <= (int)clip.left || (int)clip.bottom <= (int)clip.top) return;

		uint32_t startIndex = (uint32_t)vertices.size();

		vertices.push_back({ {geo.position.x, geo.position.y}, {0.0f, 0.0f}, tl });
		vertices.push_back({ {geo.position.x + geo.size.x, geo.position.y}, {0.0f, 0.0f}, tr });
		vertices.push_back({ {geo.position.x + geo.size.x, geo.position.y + geo.size.y}, {0.0f, 0.0f}, br });
		vertices.push_back({ {geo.position.x, geo.position.y + geo.size.y}, {0.0f, 0.0f}, bl });

		indices.push_back(startIndex + 0); indices.push_back(startIndex + 1); indices.push_back(startIndex + 2);
		indices.push_back(startIndex + 0); indices.push_back(startIndex + 2); indices.push_back(startIndex + 3);

		if (commands.empty()) commands.push_back({ 0, 0, 0 });
		commands.back().indexCount += 6;
	}

	void DrawList::addThickLine(const Vec2& p0, const Vec2& p1, float thickness, Color color) {
		Rect clip = getCurrentClipRect();
		if ((int)clip.right <= (int)clip.left || (int)clip.bottom <= (int)clip.top) return;

		Vec2 dir = { p1.x - p0.x, p1.y - p0.y };
		float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
		if (len < 0.001f) return;
		dir.x /= len; dir.y /= len;

		// -- Perpendicular Normal Vector --
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
		Rect clip = getCurrentClipRect();
		if ((int)clip.right <= (int)clip.left || (int)clip.bottom <= (int)clip.top) return;

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

	void DrawList::addText(FontAtlas* font, const std::string& text, Vec2 position, Color color, float scale, float lineHeight) {
		Rect clip = getCurrentClipRect();
		if ((int)clip.right <= (int)clip.left || (int)clip.bottom <= (int)clip.top) return;
		if (!font || text.empty() || color.a() == 0) return;

		float cursorX = position.x;
		float baselineY = std::round(position.y);

		for (char c : text) {
			if (c == '\n') {
				cursorX = position.x;
				baselineY += std::round(lineHeight * scale);
				continue;
			}

			const Glyph& g = font->getGlyph(c);

			if (g.size.x > 0 && g.size.y > 0) {
				float x0 = std::round(cursorX + (g.offset.x * scale));
				float y0 = baselineY + std::round(g.offset.y * scale);
				float x1 = x0 + std::round(g.size.x * scale);
				float y1 = y0 + std::round(g.size.y * scale);

				uint32_t startIndex = (uint32_t)vertices.size();

				vertices.push_back({ {x0, y0}, {g.uvMin.x, g.uvMin.y}, color }); // TL
				vertices.push_back({ {x1, y0}, {g.uvMax.x, g.uvMin.y}, color }); // TR
				vertices.push_back({ {x1, y1}, {g.uvMax.x, g.uvMax.y}, color }); // BR
				vertices.push_back({ {x0, y1}, {g.uvMin.x, g.uvMax.y}, color }); // BL

				indices.push_back(startIndex + 0);
				indices.push_back(startIndex + 1);
				indices.push_back(startIndex + 2);
				indices.push_back(startIndex + 0);
				indices.push_back(startIndex + 2);
				indices.push_back(startIndex + 3);

				if (commands.empty()) {
					commands.push_back({ 0, 0, 0 });
				}
				commands.back().indexCount += 6;
			}

			cursorX += (g.advanceX * scale);
		}
	}

	Rect DrawList::getCurrentClipRect() const {
		if (clipRectStack.empty()) return Rect(0, 8192, 0, 8192);
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
		for (size_t i = 0; i < s_popups.size(); ++i) {
//			s_popups[i].widget->computeDesiredSize();
//			s_popups[i].geometry.size = s_popups[i].widget->getDesiredSize();
			s_popups[i].widget->arrangeChildren(s_popups[i].geometry);

			s_drawList.pushClipRect(Rect(0, screenWidth, 0, screenHeight));
			s_popups[i].widget->onDraw(s_drawList, s_popups[i].geometry);
			s_drawList.popClipRect();
		}

		// --Draw Global Floating Tooltip --
		std::string activeTooltip = "";
		FontAtlas* activeFont = nullptr;

		// -- Drag-And-Drop Tooltips --
		if (DragDrop::isDragging() && !DragDrop::getPayload().tooltip.empty()) {
			activeTooltip = DragDrop::getPayload().tooltip;
			activeFont = DragDrop::getPayload().font;
		}
		// -- Normal Tooltips --
		else if (!s_tooltipText.empty()) {
			activeTooltip = s_tooltipText;
			activeFont = s_tooltipFont;
		}

		// -- Draw Tooltip --
		if (!activeTooltip.empty() && activeFont) {
			float textWidth = 0.0f;
			for (char c : activeTooltip) {
				textWidth += activeFont->getGlyph(c).advanceX;
			}

			Vec2 tooltipPos = { s_mousePosition.x + 15.0f, s_mousePosition.y + 15.0f };
			if (tooltipPos.x + textWidth + 16.0f > screenWidth) tooltipPos.x = screenWidth - textWidth - 16.0f;
			if (tooltipPos.y + 26.0f > screenHeight) tooltipPos.y = screenHeight - 26.0f;

			Geometry bgGeo = { tooltipPos, {textWidth + 16.0f, 26.0f} };

			Color bgColor = Silica::GetTheme().Background_Popup;
			Color textColor = Silica::GetTheme().Text_Main;

			s_drawList.pushClipRect(Rect(0, screenWidth, 0, screenHeight));

			uint32_t startIndex = (uint32_t)s_drawList.vertices.size();
			s_drawList.vertices.push_back({ {bgGeo.position.x, bgGeo.position.y}, {0.0f, 0.0f}, bgColor });
			s_drawList.vertices.push_back({ {bgGeo.position.x + bgGeo.size.x, bgGeo.position.y}, {0.0f, 0.0f}, bgColor });
			s_drawList.vertices.push_back({ {bgGeo.position.x + bgGeo.size.x, bgGeo.position.y + bgGeo.size.y}, {0.0f, 0.0f}, bgColor });
			s_drawList.vertices.push_back({ {bgGeo.position.x, bgGeo.position.y + bgGeo.size.y}, {0.0f, 0.0f}, bgColor });
			s_drawList.indices.push_back(startIndex + 0); s_drawList.indices.push_back(startIndex + 1); s_drawList.indices.push_back(startIndex + 2);
			s_drawList.indices.push_back(startIndex + 0); s_drawList.indices.push_back(startIndex + 2); s_drawList.indices.push_back(startIndex + 3);
			if (s_drawList.commands.empty()) s_drawList.commands.push_back({ 0, 0, 0 });
			s_drawList.commands.back().indexCount += 6;

			s_drawList.addText(activeFont, activeTooltip, { tooltipPos.x + 8.0f, tooltipPos.y + 18.0f }, textColor);

			s_drawList.popClipRect();
		}

		s_drawList.commands.erase(
			std::remove_if(s_drawList.commands.begin(), s_drawList.commands.end(), [](const DrawCommand& cmd) {
				return cmd.indexCount == 0 ||
					(int)cmd.clipRect.right <= (int)cmd.clipRect.left ||
					(int)cmd.clipRect.bottom <= (int)cmd.clipRect.top;
				}),
			s_drawList.commands.end()
		);
	}

	void Renderer::processMouseMove(WidgetPtr rootWidget, float screenWidth, float screenHeight, float mouseX, float mouseY) {
		s_mousePosition = { mouseX, mouseY };

		SWidget::setDragHoveredWidget(nullptr);

		if (DragDrop::isDragging()) {
			Platform::setCursor(Platform::Cursor::Hand);
		}
		else {
			Platform::setCursor(Platform::Cursor::Arrow);
		}

		s_tooltipText.clear();
		s_tooltipFont = nullptr;

		if (SWidget::getCapturedWidget() && !DragDrop::isDragging()) {
			SWidget* captured = SWidget::getCapturedWidget();
			captured->onMouseMove(captured->getAllocatedGeometry(), { mouseX, mouseY });
			return;
		}

		bool popupHandled = false;

		for (int i = (int)s_popups.size() - 1; i >= 0; --i) {
			if (i >= s_popups.size()) continue;

			if (!popupHandled && s_popups[i].geometry.contains({ mouseX, mouseY })) {
				if (s_popups[i].widget->onMouseMove(s_popups[i].geometry, { mouseX, mouseY }).isHandled) {
					popupHandled = true;
					continue;
				}
			}

			s_popups[i].widget->onMouseMove(s_popups[i].geometry, popupHandled ? Vec2(-9999.0f, -9999.0f) : Vec2(mouseX, mouseY));
		}

		if (rootWidget) {
			Geometry rootGeo = { {0, 0}, {screenWidth, screenHeight} };
			rootWidget->onMouseMove(rootGeo, popupHandled ? Vec2(-9999.0f, -9999.0f) : Vec2(mouseX, mouseY));
		}

		// -- Route Drag Hover Events --
		if (DragDrop::isDragging() && rootWidget) {
			Geometry rootGeo = { {0, 0}, {screenWidth, screenHeight} };
			rootWidget->onDragOver(rootGeo, popupHandled ? Vec2(-9999.0f, -9999.0f) : Vec2(mouseX, mouseY), DragDrop::getPayload());
		}
	}

	void Renderer::processMouseDown(WidgetPtr rootWidget, float screenWidth, float screenHeight, float mouseX, float mouseY, MouseButton button) {
		SWidget::setFocusedWidget(nullptr);

		bool hitPopup = false;
		for (int i = (int)s_popups.size() - 1; i >= 0; --i) {
			if (i >= s_popups.size()) continue;

			if (s_popups[i].geometry.contains({ mouseX, mouseY })) {
				s_popups[i].widget->onMouseButtonDown(s_popups[i].geometry, { mouseX, mouseY }, button);
				hitPopup = true;
				break;
			}
		}

		if (!hitPopup && !s_popups.empty()) {
			std::vector<std::function<void()>> closeCallbacks;
			for (const auto& popup : s_popups) {
				if (popup.closeCallback) closeCallbacks.push_back(popup.closeCallback);
			}
			for (auto& cb : closeCallbacks) cb();
			return;
		}

		if (!hitPopup && rootWidget) {
			Geometry rootGeo = { {0, 0}, {screenWidth, screenHeight} };
			rootWidget->onMouseButtonDown(rootGeo, { mouseX, mouseY }, button);
		}
	}

	void Renderer::processMouseUp(WidgetPtr rootWidget, float screenWidth, float screenHeight, float mouseX, float mouseY, MouseButton button) {
		if (button == MouseButton::Left && DragDrop::isDragging()) {
			bool hitPopup = false;

			for (int i = (int)s_popups.size() - 1; i >= 0; --i) {
				if (s_popups[i].geometry.contains({ mouseX, mouseY })) {
					if (s_popups[i].widget->onDrop(s_popups[i].geometry, { mouseX, mouseY }, DragDrop::getPayload()).isHandled) {
						hitPopup = true;
						break;
					}
				}
			}

			if (!hitPopup && rootWidget) {
				Geometry rootGeo = { {0, 0}, {screenWidth, screenHeight} };
				rootWidget->onDrop(rootGeo, { mouseX, mouseY }, DragDrop::getPayload());
			}

			DragDrop::endDrag();
			SWidget::setDragHoveredWidget(nullptr);
			SWidget::setCapturedWidget(nullptr);
			return;
		}

		if (SWidget::getCapturedWidget()) {
			SWidget* captured = SWidget::getCapturedWidget();
			captured->onMouseButtonUp(captured->getAllocatedGeometry(), { mouseX, mouseY }, button);
			return;
		}

		for (int i = (int)s_popups.size() - 1; i >= 0; --i) {
			if (i >= s_popups.size()) continue;

			if (s_popups[i].geometry.contains({ mouseX, mouseY })) {
				auto cachedCloseCb = s_popups[i].closeCallback;

				EventReply reply = s_popups[i].widget->onMouseButtonUp(s_popups[i].geometry, { mouseX, mouseY }, button);

				if (reply.isHandled) {
					if (cachedCloseCb) cachedCloseCb();
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
		for (int i = (int)s_popups.size() - 1; i >= 0; --i) {
			if (i >= s_popups.size()) continue;

			if (s_popups[i].geometry.contains({ mouseX, mouseY })) {
				if (s_popups[i].widget->onMouseWheel(s_popups[i].geometry, { mouseX, mouseY }, scrollDelta).isHandled) return;
			}
		}

		if (rootWidget) {
			Geometry rootGeo = { {0, 0}, {screenWidth, screenHeight} };
			rootWidget->onMouseWheel(rootGeo, { mouseX, mouseY }, scrollDelta);
		}
	}

	const DrawList* Renderer::getDrawData() {
		return &s_drawList;
	}

	const Vec2& Renderer::getMousePosition() {
		return s_mousePosition;
	}

	void Renderer::pushPopup(WidgetPtr widget, const Geometry& geo, std::function<void()> closeCallback) {
		s_popups.push_back({ widget, geo, closeCallback });
	}

	void Renderer::closeAllPopups() {
		s_popups.clear();
	}

}
