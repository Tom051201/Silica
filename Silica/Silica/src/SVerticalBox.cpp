#include "SVerticalBox.h"

namespace Silica {

	void SVerticalBox::computeDesiredSize() {
		m_desiredSize = Vec2::zero();

		for (const Slot& slot : m_slots) {
			if (!slot.child) continue;

			slot.child->computeDesiredSize();
			Vec2 childSize = slot.child->getDesiredSize();

			m_desiredSize.y += childSize.y + (slot.padding.y * 2.0f);

			float childWidthWithPadding = childSize.x + (slot.padding.x * 2.0f);
			if (childWidthWithPadding > m_desiredSize.x) {
				m_desiredSize.x = childWidthWithPadding;
			}
		}
	}

	void SVerticalBox::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);

		float currentY = allocatedGeometry.position.y;

		for (const Slot& slot : m_slots) {
			if (!slot.child) continue;

			Vec2 childDesired = slot.child->getDesiredSize();

			Geometry childGeo;
			childGeo.position.x = allocatedGeometry.position.x + slot.padding.x;
			childGeo.position.y = currentY + slot.padding.y;

			childGeo.size.x = childDesired.x;
			childGeo.size.y = childDesired.y;

			float maxAvailableY = (allocatedGeometry.position.y + allocatedGeometry.size.y) - childGeo.position.y - slot.padding.y;

			if (childGeo.size.y > maxAvailableY) {
				childGeo.size.y = maxAvailableY > 0 ? maxAvailableY : 0.0f;
			}

			float maxAvailableX = allocatedGeometry.size.x - (slot.padding.x * 2.0f);
			if (childGeo.size.x > maxAvailableX) {
				childGeo.size.x = maxAvailableX > 0 ? maxAvailableX : 0.0f;
			}

			slot.child->arrangeChildren(childGeo);

			currentY += childGeo.size.y + (slot.padding.y * 2.0f);
		}
	}

	void SVerticalBox::onDraw(DrawList& outDrawList, const Geometry& allotedGeometry) const {
		for (const Slot& slot : m_slots) {
			if (slot.child) slot.child->onDraw(outDrawList, slot.child->getAllocatedGeometry());
		}
	}

	EventReply SVerticalBox::onMouseMove(const Geometry& allotedGeometry, const Vec2& mousePos) {
		EventReply finalReply = EventReply::unhandled();

		for (const Slot& slot : m_slots) {
			if (slot.child) {
				EventReply reply = slot.child->onMouseMove(slot.child->getAllocatedGeometry(), mousePos);
				if (reply.isHandled) {
					finalReply = reply;
				}
			}
		}

		return finalReply;
	}

	EventReply SVerticalBox::onMouseButtonDown(const Geometry& allotedGeometry, const Vec2& mousePos) {
		for (const Slot& slot : m_slots) {
			if (slot.child && slot.child->getAllocatedGeometry().contains(mousePos)) {
				EventReply reply = slot.child->onMouseButtonDown(slot.child->getAllocatedGeometry(), mousePos);
				if (reply.isHandled) return reply;
			}
		}

		return EventReply::unhandled();
	}

	EventReply SVerticalBox::onMouseButtonUp(const Geometry& allotedGeometry, const Vec2& mousePos) {
		for (const Slot& slot : m_slots) {
			if (slot.child && slot.child->getAllocatedGeometry().contains(mousePos)) {
				EventReply reply = slot.child->onMouseButtonUp(slot.child->getAllocatedGeometry(), mousePos);
				if (reply.isHandled) return reply;
			}
		}

		return EventReply::unhandled();
	}

	EventReply SVerticalBox::onMouseWheel(const Geometry& allotedGeometry, const Vec2& mousePos, float scrollDelta) {
		for (const Slot& slot : m_slots) {
			if (slot.child && slot.child->getAllocatedGeometry().contains(mousePos)) {
				EventReply reply = slot.child->onMouseWheel(slot.child->getAllocatedGeometry(), mousePos, scrollDelta);
				if (reply.isHandled) return reply;
			}
		}
		return EventReply::unhandled();
	}

}
