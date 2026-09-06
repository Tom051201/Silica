#include "silicapch.h"
#include "SBox.h"

namespace Silica {

	void SBox::construct(const Args& args) {
		m_padding = args.padding;
		m_explicitSize = args.explicitSize.value_or(Vec2::zero());
		m_borderThickness = args.borderThickness;
		m_consumePointerEvents = args.consumePointerEvents;
		m_backgroundColor = args.backgroundColor.value_or(GetTheme().Background_Panel);
		m_borderColor = args.borderColor.value_or(GetTheme().Border_Primary);
		m_onDragOver = args.onDragOver;
		m_onDrop = args.onDrop;
		m_child = args.child;
	}

	void SBox::computeDesiredSize() {
		m_desiredSize = Vec2::zero();

		if (m_child) {
			m_child->computeDesiredSize();
			m_desiredSize = m_child->getDesiredSize();
		}

		m_desiredSize.x += (m_padding.x + m_borderThickness) * 2.0f;
		m_desiredSize.y += (m_padding.y + m_borderThickness) * 2.0f;

		if (m_explicitSize.x > 0.0f) m_desiredSize.x = m_explicitSize.x;
		if (m_explicitSize.y > 0.0f) m_desiredSize.y = m_explicitSize.y;
	}

	void SBox::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);
		if (m_child) {
			m_child->arrangeChildren(getChildGeometry(allocatedGeometry));
		}
	}

	void SBox::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		if (m_backgroundColor.a() > 0) {
			outDrawList.addRect(allocatedGeometry, m_backgroundColor);
		}

		if (m_child) {
			m_child->onDraw(outDrawList, getChildGeometry(allocatedGeometry));
		}

		if (m_borderThickness > 0.0f && m_borderColor.a() > 0) {
			float t = m_borderThickness * m_renderScale;
			float x = allocatedGeometry.position.x;
			float y = allocatedGeometry.position.y;
			float w = allocatedGeometry.size.x;
			float h = allocatedGeometry.size.y;

			outDrawList.addRect({ {x, y}, {w, t} }, m_borderColor);
			outDrawList.addRect({ {x, y + h - t}, {w, t} }, m_borderColor);
			outDrawList.addRect({ {x, y + t}, {t, h - (t * 2)} }, m_borderColor);
			outDrawList.addRect({ {x + w - t, y + t}, {t, h - (t * 2)} }, m_borderColor);
		}

		if (SWidget::getDragHoveredWidget() == this) {
			Color overlayColor = Silica::GetTheme().Accent_Primary;
			overlayColor.setAlpha(60);
			outDrawList.addRect(allocatedGeometry, overlayColor);

			Color highlightBorder = Silica::GetTheme().Border_Selected;
			float t = std::max(2.0f, Silica::GetTheme().Border_Thickness * m_renderScale);
			float x = allocatedGeometry.position.x;
			float y = allocatedGeometry.position.y;
			float w = allocatedGeometry.size.x;
			float h = allocatedGeometry.size.y;

			outDrawList.addRect({ {x, y}, {w, t} }, highlightBorder);
			outDrawList.addRect({ {x, y + h - t}, {w, t} }, highlightBorder);
			outDrawList.addRect({ {x, y + t}, {t, h - (t * 2)} }, highlightBorder);
			outDrawList.addRect({ {x + w - t, y + t}, {t, h - (t * 2)} }, highlightBorder);
		}
	}

	void SBox::setRenderScale(float scale) {
		m_renderScale = scale;
		if (m_child) m_child->setRenderScale(scale);
	}

	EventReply SBox::onMouseMove(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		EventReply childReply = EventReply::unhandled();

		if (m_child) {
			childReply = m_child->onMouseMove(getChildGeometry(allocatedGeometry), mousePos);
		}

		m_isHovered = allocatedGeometry.contains(mousePos);

		if (childReply.isHandled) return childReply;

		if (m_isHovered && m_consumePointerEvents) return EventReply::handled();
		return EventReply::unhandled();
	}

	EventReply SBox::onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		if (m_child) {
			EventReply reply = m_child->onMouseButtonDown(getChildGeometry(allocatedGeometry), mousePos, button);
			if (reply.isHandled) return reply;
		}

		m_isHovered = allocatedGeometry.contains(mousePos);
		if (m_isHovered && m_consumePointerEvents) return EventReply::handled();
		return EventReply::unhandled();
	}

	EventReply SBox::onMouseButtonUp(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		if (m_child) {
			EventReply reply = m_child->onMouseButtonUp(getChildGeometry(allocatedGeometry), mousePos, button);
			if (reply.isHandled) return reply;
		}

		m_isHovered = allocatedGeometry.contains(mousePos);
		if (m_isHovered && m_consumePointerEvents) return EventReply::handled();
		return EventReply::unhandled();
	}

	EventReply SBox::onMouseWheel(const Geometry& allocatedGeometry, const Vec2& mousePos, float scrollDelta) {
		if (m_child) {
			EventReply reply = m_child->onMouseWheel(getChildGeometry(allocatedGeometry), mousePos, scrollDelta);
			if (reply.isHandled) return reply;
		}

		m_isHovered = allocatedGeometry.contains(mousePos);
		if (m_isHovered && m_consumePointerEvents) return EventReply::handled();
		return EventReply::unhandled();
	}

	void SBox::setChild(WidgetPtr child) {
		m_child = child;
	}

	EventReply SBox::onDragOver(const Geometry& allocatedGeometry, const Vec2& mousePos, const DragDropPayload& payload) {
		if (m_child) {
			EventReply reply = m_child->onDragOver(getChildGeometry(allocatedGeometry), mousePos, payload);
			if (reply.isHandled) return reply;
		}

		if (m_onDragOver && allocatedGeometry.contains(mousePos)) {
			EventReply reply = m_onDragOver(payload);
			if (reply.isHandled) {
				SWidget::setDragHoveredWidget(this);
				return reply;
			}
		}

		m_isHovered = allocatedGeometry.contains(mousePos);
		if (m_isHovered && m_consumePointerEvents) return EventReply::handled();
		return EventReply::unhandled();
	}

	EventReply SBox::onDrop(const Geometry& allocatedGeometry, const Vec2& mousePos, const DragDropPayload& payload) {
		if (m_child) {
			EventReply reply = m_child->onDrop(getChildGeometry(allocatedGeometry), mousePos, payload);
			if (reply.isHandled) return reply;
		}

		if (m_onDrop && allocatedGeometry.contains(mousePos)) {
			EventReply reply = m_onDrop(payload);
			if (reply.isHandled) {
				SWidget::setDragHoveredWidget(this);
				return reply;
			}
		}

		m_isHovered = allocatedGeometry.contains(mousePos);
		if (m_isHovered && m_consumePointerEvents) return EventReply::handled();
		return EventReply::unhandled();
	}

	Geometry SBox::getChildGeometry(const Geometry& allocatedGeometry) const {
		float offsetX = (m_padding.x + m_borderThickness) * m_renderScale;
		float offsetY = (m_padding.y + m_borderThickness) * m_renderScale;

		Geometry childGeo;
		childGeo.position.x = allocatedGeometry.position.x + offsetX;
		childGeo.position.y = allocatedGeometry.position.y + offsetY;
		childGeo.size.x = std::max(0.0f, allocatedGeometry.size.x - (offsetX * 2.0f));
		childGeo.size.y = std::max(0.0f, allocatedGeometry.size.y - (offsetY * 2.0f));
		return childGeo;
	}

	void SBox::setBackgroundColor(const Color& color) {
		m_backgroundColor = color;
	}

}
