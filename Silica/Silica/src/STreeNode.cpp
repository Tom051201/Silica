#include "silicapch.h"
#include "STreeNode.h"

#include "Renderer.h"

namespace Silica {

	void STreeNode::construct(const Args& args) {
		m_label = args.label;
		m_font = args.font ? args.font : GetTheme().Font_Default;
		m_yOffset = args.yTextOffset;
		m_isOpen = args.initiallyOpen;
		m_isSelected = args.isSelected;
		m_isLeaf = args.isLeaf;
		m_isEmpty = args.isEmpty;
		m_isDragged = args.isDragged;
		m_leadingWidget = args.leadingWidget;
		m_children = args.children;
		m_onClicked = args.onClicked;
		m_onDragStart = args.onDragStart;
		m_onDragOver = args.onDragOver;
		m_onDrop = args.onDrop;
		m_onToggleOpen = args.onToggleOpen;
	}

	void STreeNode::computeDesiredSize() {
		float scaledHeaderHeight = m_headerHeight * m_renderScale;
		float scaledIndentSize = m_indentSize * m_renderScale;

		m_desiredSize = Vec2(100.0f * m_renderScale, scaledHeaderHeight);

		if (m_leadingWidget) {
			m_leadingWidget->computeDesiredSize();
		}

		if (m_isOpen) {
			for (auto& child : m_children) {
				if (child) {
					child->computeDesiredSize();
					m_desiredSize.y += child->getDesiredSize().y;
					m_desiredSize.x = std::max(m_desiredSize.x, child->getDesiredSize().x + scaledIndentSize);
				}
			}
		}
	}

	void STreeNode::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);

		float scaledHeaderHeight = m_headerHeight * m_renderScale;
		float scaledIndentSize = m_indentSize * m_renderScale;

		// -- Layout The Icon --
		if (m_leadingWidget) {
			float textIndent = 20.0f * m_renderScale;
			Vec2 lwSize = m_leadingWidget->getDesiredSize();

			float lwY = allocatedGeometry.position.y + (scaledHeaderHeight - lwSize.y) * 0.5f;
			Geometry lwGeo = {
				{allocatedGeometry.position.x + textIndent, lwY},
				lwSize
			};
			m_leadingWidget->arrangeChildren(lwGeo);
		}

		if (m_isOpen) {
			float currentY = allocatedGeometry.position.y + scaledHeaderHeight;

			for (auto& child : m_children) {
				if (child) {
					float childHeight = child->getDesiredSize().y;
					Geometry childGeo = {
						{allocatedGeometry.position.x + scaledIndentSize, currentY},
						{allocatedGeometry.size.x - scaledIndentSize, childHeight}
					};
					child->arrangeChildren(childGeo);
					currentY += childHeight;
				}
			}
		}
	}

	void STreeNode::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		float scaledHeaderHeight = m_headerHeight * m_renderScale;
		Geometry headerGeo = { allocatedGeometry.position, {allocatedGeometry.size.x, scaledHeaderHeight} };

		// -- Draw Selection / Hover Background
		if (m_isDragged && m_isDragged()) outDrawList.addRect(headerGeo, Color(150, 150, 150, 100));
		else if (m_isSelected) outDrawList.addRect(headerGeo, GetTheme().Accent_Primary);
		else if (m_isHovered) outDrawList.addRect(headerGeo, Color(60, 60, 60, 255));

		// -- Draw Drop Zone Highlight --
		if (SWidget::getDragHoveredWidget() == this) {
			Color overlayColor = Silica::GetTheme().Accent_Primary;
			overlayColor.setAlpha(60);
			outDrawList.addRect(headerGeo, overlayColor);

			Color highlightBorder = Silica::GetTheme().Border_Selected;
			float t = Silica::GetTheme().Border_Thickness * m_renderScale;
			float x = headerGeo.position.x;
			float y = headerGeo.position.y;
			float w = headerGeo.size.x;
			float h = headerGeo.size.y;

			outDrawList.addRect({ {x, y}, {w, t} }, highlightBorder);
			outDrawList.addRect({ {x, y + h - t}, {w, t} }, highlightBorder);
			outDrawList.addRect({ {x, y + t}, {t, h - (t * 2)} }, highlightBorder);
			outDrawList.addRect({ {x + w - t, y + t}, {t, h - (t * 2)} }, highlightBorder);
		}

		// -- Draw Chevron And Text --
		if (m_font) {
			float textIndent = 20.0f * m_renderScale;

			if (!m_isLeaf && !m_isEmpty) {
				Vec2 arrowCenter = {
					headerGeo.position.x + (10.0f * m_renderScale),
					headerGeo.position.y + (scaledHeaderHeight * 0.5f)
				};
				drawTriangle(outDrawList, arrowCenter, 4.0f * m_renderScale, Color::white(), m_isOpen);
			}

			if (m_leadingWidget) {
				m_leadingWidget->onDraw(outDrawList, m_leadingWidget->getAllocatedGeometry());
				textIndent += m_leadingWidget->getDesiredSize().x + (6.0f * m_renderScale);
			}

			Vec2 textPos = { headerGeo.position.x + textIndent, headerGeo.position.y + (m_yOffset * m_renderScale) };
			outDrawList.addText(m_font, m_label, textPos, Color::white(), m_renderScale);
		}

		// -- Draw Children --
		if (m_isOpen) {
			for (auto& child : m_children) {
				if (child) child->onDraw(outDrawList, child->getAllocatedGeometry());
			}
		}
	}

	void STreeNode::setRenderScale(float scale) {
		m_renderScale = scale;
		if (m_leadingWidget) m_leadingWidget->setRenderScale(scale);
		for (auto& child : m_children) {
			if (child) child->setRenderScale(scale);
		}
	}

	EventReply STreeNode::onMouseMove(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		float scaledHeaderHeight = m_headerHeight * m_renderScale;
		Geometry headerGeo = { allocatedGeometry.position, {allocatedGeometry.size.x, scaledHeaderHeight} };
		m_isHovered = headerGeo.contains(mousePos);

		if (m_isLeftMouseDown && m_onDragStart) {
			m_onDragStart();
			m_isLeftMouseDown = false;
			SWidget::setCapturedWidget(nullptr);
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

		float scaledHeaderHeight = m_headerHeight * m_renderScale;
		float scaledIndentSize = m_indentSize * m_renderScale;
		Geometry headerGeo = { allocatedGeometry.position, {allocatedGeometry.size.x, scaledHeaderHeight} };

		if (headerGeo.contains(mousePos)) {
			if (button == MouseButton::Left) {
				if (mousePos.x < headerGeo.position.x + (20.0f * m_renderScale)) {
					if (!m_isLeaf && !m_isEmpty) {
						m_isOpen = !m_isOpen;
						if (m_onToggleOpen) m_onToggleOpen(m_isOpen);
					}
					return EventReply::handled();
				}
				else {
					m_isLeftMouseDown = true;
					SWidget::setCapturedWidget(this);
				}
				return EventReply::handled();
			}
		}
		return EventReply::unhandled();
	}

	EventReply STreeNode::onMouseButtonUp(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		bool wasClicked = m_isLeftMouseDown;

		if (m_isLeftMouseDown) {
			m_isLeftMouseDown = false;
			SWidget::setCapturedWidget(nullptr);
		}

		if (button != MouseButton::Left) return EventReply::unhandled();

		float scaledHeaderHeight = m_headerHeight * m_renderScale;
		Geometry headerGeo = { allocatedGeometry.position, {allocatedGeometry.size.x, scaledHeaderHeight} };

		if (wasClicked && headerGeo.contains(mousePos)) {
			if (m_onClicked) m_onClicked();
			return EventReply::handled();
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

	EventReply STreeNode::onDragOver(const Geometry& allocatedGeometry, const Vec2& mousePos, const DragDropPayload& payload) {
		// -- Route To Children If Open --
		if (m_isOpen) {
			for (auto& child : m_children) {
				if (child && child->getAllocatedGeometry().contains(mousePos)) {
					EventReply reply = child->onDragOver(child->getAllocatedGeometry(), mousePos, payload);
					if (reply.isHandled) return reply;
				}
			}
		}

		// -- This Header --
		float scaledHeaderHeight = m_headerHeight * m_renderScale;
		Geometry headerGeo = { allocatedGeometry.position, {allocatedGeometry.size.x, scaledHeaderHeight} };
		if (headerGeo.contains(mousePos)) {
			if (m_onDragOver) {
				EventReply reply = m_onDragOver(payload);
				if (reply.isHandled) {
					SWidget::setDragHoveredWidget(this);
				}
			}
			return EventReply::handled();
		}

		return EventReply::unhandled();
	}

	EventReply STreeNode::onDrop(const Geometry& allocatedGeometry, const Vec2& mousePos, const DragDropPayload& payload) {
		// -- Route To Children If Open --
		if (m_isOpen) {
			for (auto& child : m_children) {
				if (child && child->getAllocatedGeometry().contains(mousePos)) {
					EventReply reply = child->onDrop(child->getAllocatedGeometry(), mousePos, payload);
					if (reply.isHandled) return reply;
				}
			}
		}

		// -- This Header --
		float scaledHeaderHeight = m_headerHeight * m_renderScale;
		Geometry headerGeo = { allocatedGeometry.position, {allocatedGeometry.size.x, scaledHeaderHeight} };
		if (headerGeo.contains(mousePos) && m_onDrop) {
			return m_onDrop(payload);
		}

		return EventReply::unhandled();
	}

}
