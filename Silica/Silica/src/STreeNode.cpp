#include "STreeNode.h"
#include "Theme.h"
#include "Renderer.h"

namespace Silica {

	void STreeNode::construct(const Args& args) {
		m_label = args.label;
		m_font = args.font;
		m_yOffset = args.yTextOffset;
		m_isOpen = args.initiallyOpen;
		m_isSelected = args.isSelected;
		m_isDragged = args.isDragged;
		m_children = args.children;
		m_onClicked = args.onClicked;
		m_onDragStart = args.onDragStart;
		m_onDrop = args.onDrop;
	}

	void STreeNode::computeDesiredSize() {
		m_desiredSize = Vec2(100.0f, m_headerHeight);

		if (m_isOpen) {
			for (auto& child : m_children) {
				if (child) {
					child->computeDesiredSize();
					m_desiredSize.y += child->getDesiredSize().y;
					m_desiredSize.x = std::max(m_desiredSize.x, child->getDesiredSize().x + m_indentSize);
				}
			}
		}
	}

	void STreeNode::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);

		if (m_isOpen) {
			float currentY = allocatedGeometry.position.y + m_headerHeight;

			for (auto& child : m_children) {
				if (child) {
					float childHeight = child->getDesiredSize().y;
					Geometry childGeo = {
						{allocatedGeometry.position.x + m_indentSize, currentY},
						{allocatedGeometry.size.x - m_indentSize, childHeight}
					};
					child->arrangeChildren(childGeo);
					currentY += childHeight;
				}
			}
		}
	}

	void STreeNode::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		Geometry headerGeo = { allocatedGeometry.position, {allocatedGeometry.size.x, m_headerHeight} };

		// -- Draw Selection / Hover Background
		if (m_isDragged && m_isDragged()) {
			addRectToDrawList(outDrawList, headerGeo, Color(150, 150, 150, 100));
		}
		else if (m_isSelected) {
			addRectToDrawList(outDrawList, headerGeo, GetTheme().accentPrimary);
		}
		else if (m_isHovered) {
			addRectToDrawList(outDrawList, headerGeo, Color(60, 60, 60, 255));
		}

		// -- Draw Chevron And Text --
		if (m_font) {
			float textIndent = 5.0f;

			if (!m_children.empty()) {
				Vec2 arrowCenter = {
					headerGeo.position.x + 10.0f,
					headerGeo.position.y + (m_headerHeight * 0.5f)
				};

				drawTriangle(outDrawList, arrowCenter, 4.0f, Color::white(), m_isOpen);

				textIndent = 20.0f;
			}
			else {
				textIndent = 20.0f;
			}

			drawText(outDrawList, m_label, { headerGeo.position.x + textIndent, headerGeo.position.y }, Color::white(), m_yOffset);
		}

		// -- Draw Children --
		if (m_isOpen) {
			for (auto& child : m_children) {
				if (child) child->onDraw(outDrawList, child->getAllocatedGeometry());
			}
		}
	}

	EventReply STreeNode::onMouseMove(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		Geometry headerGeo = { allocatedGeometry.position, {allocatedGeometry.size.x, m_headerHeight} };
		m_isHovered = headerGeo.contains(mousePos);

		if (m_isLeftMouseDown && m_isHovered && m_onDragStart) {
			m_onDragStart();
		}

		if (m_isOpen) {
			for (auto& child : m_children) {
				if (child) child->onMouseMove(child->getAllocatedGeometry(), mousePos);
			}
		}

		return m_isHovered ? EventReply::handled() : EventReply::unhandled();
	}

	EventReply STreeNode::onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {

		if (m_isOpen) {
			for (auto& child : m_children) {
				if (child && child->getAllocatedGeometry().contains(mousePos)) {
					EventReply reply = child->onMouseButtonDown(child->getAllocatedGeometry(), mousePos, button);
					if (reply.isHandled) return reply;
				}
			}
		}

		Geometry headerGeo = { allocatedGeometry.position, {allocatedGeometry.size.x, m_headerHeight} };

		if (headerGeo.contains(mousePos)) {
			if (button == MouseButton::Left) {
				m_isLeftMouseDown = true;

				if (mousePos.x < headerGeo.position.x + m_indentSize) {
					if (!m_children.empty()) m_isOpen = !m_isOpen;
				}
				else {
					if (m_onClicked) m_onClicked();
				}
				return EventReply::handled();
			}
		}

		return EventReply::unhandled();
	}

	EventReply STreeNode::onMouseButtonUp(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		m_isLeftMouseDown = false;

		if (button != MouseButton::Left) return EventReply::unhandled();

		Geometry headerGeo = { allocatedGeometry.position, {allocatedGeometry.size.x, m_headerHeight} };

		if (headerGeo.contains(mousePos) && m_onDrop) {
			EventReply reply = m_onDrop();
			if (reply.isHandled) return reply;
		}

		// -- Route To Children If Open --
		if (m_isOpen) {
			for (auto& child : m_children) {
				if (child) {
					EventReply reply = child->onMouseButtonUp(child->getAllocatedGeometry(), mousePos, button);
					if (reply.isHandled) return reply;
				}
			}
		}

		return EventReply::unhandled();
	}

	void STreeNode::addChild(WidgetPtr child) {
		m_children.push_back(child);
	}

	void STreeNode::clearChildren() {
		m_children.clear();
	}

	bool STreeNode::isOpen() const {
		return m_isOpen;
	}

	void STreeNode::setOpen(bool open) {
		m_isOpen = open;
	}
	
	void STreeNode::setSelected(bool selected) {
		m_isSelected = selected;
	}

	void STreeNode::drawText(DrawList& drawList, const std::string& text, Vec2 pos, Color color, float yOffset) const {
		if (!m_font || text.empty() || color.a() == 0) return;

		float cursorX = pos.x;
		float baselineY = pos.y + yOffset;

		for (char c : text) {
			const Glyph& g = m_font->getGlyph(c);

			if (g.size.x > 0 && g.size.y > 0) {
				float x0 = cursorX + g.offset.x;
				float y0 = baselineY + g.offset.y;
				float x1 = x0 + g.size.x;
				float y1 = y0 + g.size.y;

				uint32_t startIndex = (uint32_t)drawList.vertices.size();

				drawList.vertices.push_back({ {x0, y0}, {g.uvMin.x, g.uvMin.y}, color }); // TL
				drawList.vertices.push_back({ {x1, y0}, {g.uvMax.x, g.uvMin.y}, color }); // TR
				drawList.vertices.push_back({ {x1, y1}, {g.uvMax.x, g.uvMax.y}, color }); // BR
				drawList.vertices.push_back({ {x0, y1}, {g.uvMin.x, g.uvMax.y}, color }); // BL

				drawList.indices.push_back(startIndex + 0);
				drawList.indices.push_back(startIndex + 1);
				drawList.indices.push_back(startIndex + 2);
				drawList.indices.push_back(startIndex + 0);
				drawList.indices.push_back(startIndex + 2);
				drawList.indices.push_back(startIndex + 3);

				if (drawList.commands.empty()) {
					drawList.commands.push_back({ 0, 0, 0 });
				}
				drawList.commands.back().indexCount += 6;
			}
			cursorX += g.advanceX;
		}
	}

	void STreeNode::addRectToDrawList(DrawList& drawList, const Geometry& geo, Color color) const {
		uint32_t startIndex = (uint32_t)drawList.vertices.size();

		drawList.vertices.push_back({ {geo.position.x, geo.position.y}, {0.0f, 0.0f}, color }); // TL
		drawList.vertices.push_back({ {geo.position.x + geo.size.x, geo.position.y}, {0.0f, 0.0f}, color }); // TR
		drawList.vertices.push_back({ {geo.position.x + geo.size.x, geo.position.y + geo.size.y}, {0.0f, 0.0f}, color }); // BR
		drawList.vertices.push_back({ {geo.position.x, geo.position.y + geo.size.y}, {0.0f, 0.0f}, color }); // BL

		drawList.indices.push_back(startIndex + 0); drawList.indices.push_back(startIndex + 1); drawList.indices.push_back(startIndex + 2);
		drawList.indices.push_back(startIndex + 0); drawList.indices.push_back(startIndex + 2); drawList.indices.push_back(startIndex + 3);

		if (drawList.commands.empty()) {
			drawList.commands.push_back({ 0, 0, 0 });
		}
		drawList.commands.back().indexCount += 6;
	}

	void STreeNode::drawTriangle(DrawList& drawList, const Vec2& center, float radius, Color color, bool pointDown) const {
		uint32_t startIndex = (uint32_t)drawList.vertices.size();

		Vec2 p0, p1, p2;

		if (pointDown) {
			p0 = { center.x - radius,	center.y - (radius * 0.5f) };
			p1 = { center.x + radius,	center.y - (radius * 0.5f) };
			p2 = { center.x,			center.y + (radius * 0.5f) };
		}
		else {
			p0 = { center.x - (radius * 0.5f), center.y - radius };
			p1 = { center.x - (radius * 0.5f), center.y + radius };
			p2 = { center.x + (radius * 0.5f), center.y };
		}

		drawList.vertices.push_back({ p0, {0.0f, 0.0f}, color });
		drawList.vertices.push_back({ p1, {0.0f, 0.0f}, color });
		drawList.vertices.push_back({ p2, {0.0f, 0.0f}, color });

		drawList.indices.push_back(startIndex + 0);
		drawList.indices.push_back(startIndex + 1);
		drawList.indices.push_back(startIndex + 2);

		if (drawList.commands.empty()) drawList.commands.push_back({ 0, 0, 0 });
		drawList.commands.back().indexCount += 3;
	}

}
