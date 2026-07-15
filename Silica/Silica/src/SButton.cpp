#include "SButton.h"

#include "Theme.h"

namespace Silica {

	void SButton::construct(const Args& args) {
		m_padding = args.padding;
		m_isEnabled = args.enabled;
		m_color = args.color.value_or(GetTheme().Element_Normal);
		m_hoverColor = args.hoverColor.value_or(GetTheme().Element_Hover);
		m_pressedColor = args.pressedColor.value_or(GetTheme().Element_Pressed);
		m_disabledColor = args.disabledColor.value_or(GetTheme().Element_Disabled);
		m_onClick = args.onClick;
		m_child = args.child;
	}

	void SButton::computeDesiredSize() {
		m_desiredSize = Vec2::zero();
		if (m_child) {
			m_child->computeDesiredSize();
			m_desiredSize = m_child->getDesiredSize();
		}
		m_desiredSize.x += m_padding.x * 2;
		m_desiredSize.y += m_padding.y * 2;
	}

	void SButton::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);

		if (m_child) {
			Geometry childGeo;
			childGeo.position.x = allocatedGeometry.position.x + m_padding.x;
			childGeo.position.y = allocatedGeometry.position.y + m_padding.y;
			childGeo.size.x = allocatedGeometry.size.x - (m_padding.x * 2.0f);
			childGeo.size.y = allocatedGeometry.size.y - (m_padding.y * 2.0f);
			m_child->arrangeChildren(childGeo);
		}
	}

	void SButton::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		Color drawColor = !m_isEnabled ? m_disabledColor : (m_isPressed ? m_pressedColor : (m_isHovered ? m_hoverColor : m_color));

		if (drawColor.a() > 0) {
			outDrawList.addRect(allocatedGeometry, drawColor);
		}

		if (m_child) {
			Geometry childGeo;
			childGeo.position.x = allocatedGeometry.position.x + m_padding.x;
			childGeo.position.y = allocatedGeometry.position.y + m_padding.y;
			childGeo.size.x = allocatedGeometry.size.x - (m_padding.x * 2.0f);
			childGeo.size.y = allocatedGeometry.size.y - (m_padding.y * 2.0f);
			m_child->onDraw(outDrawList, childGeo);
		}
	}

	EventReply SButton::onMouseMove(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		m_isHovered = allocatedGeometry.contains(mousePos);

		if (!m_isHovered || !m_isEnabled) {
			m_isPressed = false;
		}

		if (m_child) {
			Geometry childGeo;
			childGeo.position.x = allocatedGeometry.position.x + m_padding.x;
			childGeo.position.y = allocatedGeometry.position.y + m_padding.y;
			childGeo.size.x = allocatedGeometry.size.x - (m_padding.x * 2.0f);
			childGeo.size.y = allocatedGeometry.size.y - (m_padding.y * 2.0f);

			m_child->onMouseMove(childGeo, mousePos);
		}

		return m_isHovered ? EventReply::handled() : EventReply::unhandled();
	}

	EventReply SButton::onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		if (button != MouseButton::Left) return EventReply::unhandled();

		if (allocatedGeometry.contains(mousePos)) {
			if (!m_isEnabled) return EventReply::handled();

			m_isPressed = true;
			return EventReply::handled();
		}
		return EventReply::unhandled();
	}

	EventReply SButton::onMouseButtonUp(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		if (button != MouseButton::Left) return EventReply::unhandled();

		if (m_isPressed && allocatedGeometry.contains(mousePos)) {
			m_isPressed = false;
			if (m_isEnabled && m_onClick) {
				return m_onClick();
			}
			return EventReply::handled();
		}

		m_isPressed = false;
		return EventReply::unhandled();
	}

	void SButton::setEnabled(bool enabled) {
		m_isEnabled = enabled;
	}

	bool SButton::isEnabled() const {
		return m_isEnabled;
	}

	EventReply SButton::onDragOver(const Geometry& allocatedGeometry, const Vec2& mousePos, const DragDropPayload& payload) {
		if (m_child) {
			Geometry childGeo;
			childGeo.position.x = allocatedGeometry.position.x + m_padding.x;
			childGeo.position.y = allocatedGeometry.position.y + m_padding.y;
			childGeo.size.x = allocatedGeometry.size.x - (m_padding.x * 2.0f);
			childGeo.size.y = allocatedGeometry.size.y - (m_padding.y * 2.0f);

			EventReply reply = m_child->onDragOver(childGeo, mousePos, payload);
			if (reply.isHandled) return reply;
		}

		return EventReply::unhandled();
	}

	EventReply SButton::onDrop(const Geometry& allocatedGeometry, const Vec2& mousePos, const DragDropPayload& payload) {
		if (m_child) {
			Geometry childGeo;
			childGeo.position.x = allocatedGeometry.position.x + m_padding.x;
			childGeo.position.y = allocatedGeometry.position.y + m_padding.y;
			childGeo.size.x = allocatedGeometry.size.x - (m_padding.x * 2.0f);
			childGeo.size.y = allocatedGeometry.size.y - (m_padding.y * 2.0f);

			EventReply reply = m_child->onDrop(childGeo, mousePos, payload);
			if (reply.isHandled) return reply;
		}

		return EventReply::unhandled();
	}

}
