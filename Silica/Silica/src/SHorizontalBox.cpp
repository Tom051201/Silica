#include "SHorizontalBox.h"

namespace Silica {

	void SHorizontalBox::computeDesiredSize() {
		m_desiredSize = Vec2::zero();

		for (const Slot& slot : m_slots) {
			if (!slot.child) continue;

			slot.child->computeDesiredSize();
			Vec2 childSize = slot.child->getDesiredSize();

			m_desiredSize.x += childSize.x + (slot.padding.x * 2.0f);

			float childHeightWithPadding = childSize.y + (slot.padding.y * 2.0f);
			if (childHeightWithPadding > m_desiredSize.y) {
				m_desiredSize.y = childHeightWithPadding;
			}
		}
	}

	void SHorizontalBox::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);

		float currentX = allocatedGeometry.position.x;

		for (const Slot& slot : m_slots) {
			if (!slot.child) continue;

			Vec2 childDesired = slot.child->getDesiredSize();

			Geometry childGeo;
			childGeo.position.x = currentX + slot.padding.x;
			childGeo.position.y = allocatedGeometry.position.y + slot.padding.y;

			childGeo.size.x = childDesired.x;
			childGeo.size.y = childDesired.y;

			float maxAvailableX = (allocatedGeometry.position.x + allocatedGeometry.size.x) - childGeo.position.x - slot.padding.x;

			if (childGeo.size.x > maxAvailableX) {
				childGeo.size.x = maxAvailableX > 0 ? maxAvailableX : 0.0f;
			}

			float maxAvailableY = allocatedGeometry.size.y - (slot.padding.y * 2.0f);
			if (childGeo.size.y > maxAvailableY) {
				childGeo.size.y = maxAvailableY > 0 ? maxAvailableY : 0.0f;
			}

			slot.child->arrangeChildren(childGeo);

			currentX += childGeo.size.x + (slot.padding.x * 2.0f);
		}
	}

	void SHorizontalBox::onDraw(DrawList& outDrawList, const Geometry& allotedGeometry) const {
		for (const Slot& slot : m_slots) {
			if (slot.child) slot.child->onDraw(outDrawList, slot.child->getAllocatedGeometry());
		}
	}

	EventReply SHorizontalBox::onMouseMove(const Geometry& allotedGeometry, const Vec2& mousePos) {
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

	EventReply SHorizontalBox::onMouseButtonDown(const Geometry& allotedGeometry, const Vec2& mousePos) {
		for (const Slot& slot : m_slots) {
			if (slot.child && slot.child->getAllocatedGeometry().contains(mousePos)) {
				EventReply reply = slot.child->onMouseButtonDown(slot.child->getAllocatedGeometry(), mousePos);
				if (reply.isHandled) return reply;
			}
		}

		return EventReply::unhandled();
	}

	EventReply SHorizontalBox::onMouseButtonUp(const Geometry& allotedGeometry, const Vec2& mousePos) {
		for (const Slot& slot : m_slots) {
			if (slot.child && slot.child->getAllocatedGeometry().contains(mousePos)) {
				EventReply reply = slot.child->onMouseButtonUp(slot.child->getAllocatedGeometry(), mousePos);
				if (reply.isHandled) return reply;
			}
		}

		return EventReply::unhandled();
	}

	EventReply SHorizontalBox::onMouseWheel(const Geometry& allotedGeometry, const Vec2& mousePos, float scrollDelta) {
		for (const Slot& slot : m_slots) {
			if (slot.child && slot.child->getAllocatedGeometry().contains(mousePos)) {
				EventReply reply = slot.child->onMouseWheel(slot.child->getAllocatedGeometry(), mousePos, scrollDelta);
				if (reply.isHandled) return reply;
			}
		}
		return EventReply::unhandled();
	}

}
