#include "silicapch.h"
#include "SVerticalBox.h"

namespace Silica {

	void SVerticalBox::construct(const Args& args) {
		m_slots = args.slots;
		m_spacing = args.spacing;
	}

	void SVerticalBox::computeDesiredSize() {
		m_desiredSize = Vec2::zero();
		bool isFirstChild = true;

		for (const Slot& slot : m_slots) {
			if (!slot.child) continue;

			slot.child->computeDesiredSize();
			Vec2 childSize = slot.child->getDesiredSize();

			if (!isFirstChild) m_desiredSize.y += m_spacing;
			isFirstChild = false;

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
		bool isFirstChild = true;
		float scaledSpacing = m_spacing * m_renderScale;

		for (const Slot& slot : m_slots) {
			if (!slot.child) continue;

			if (!isFirstChild) currentY += scaledSpacing;
			isFirstChild = false;

			float scaledPadX = slot.padding.x * m_renderScale;
			float scaledPadY = slot.padding.y * m_renderScale;

			Vec2 childDesired = slot.child->getDesiredSize();
			Geometry childGeo;
			childGeo.position.x = allocatedGeometry.position.x + scaledPadX;
			childGeo.position.y = currentY + scaledPadY;

			childGeo.size.x = allocatedGeometry.size.x - (scaledPadX * 2.0f);
			if (childGeo.size.x < 0.0f) childGeo.size.x = 0.0f;
			childGeo.size.y = childDesired.y * m_renderScale;

			float maxAvailableY = (allocatedGeometry.position.y + allocatedGeometry.size.y) - childGeo.position.y - scaledPadY;
			if (childGeo.size.y > maxAvailableY) {
				childGeo.size.y = maxAvailableY > 0 ? maxAvailableY : 0.0f;
			}

			slot.child->arrangeChildren(childGeo);
			currentY += childGeo.size.y + (scaledPadY * 2.0f);
		}
	}

	void SVerticalBox::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		for (const Slot& slot : m_slots) {
			if (slot.child) {
				slot.child->onDraw(outDrawList, slot.child->getAllocatedGeometry());
			}
		}
	}

	void SVerticalBox::setRenderScale(float scale) {
		m_renderScale = scale;
		for (auto& slot : m_slots) {
			if (slot.child) slot.child->setRenderScale(scale);
		}
	}

	EventReply SVerticalBox::onMouseMove(const Geometry& allocatedGeometry, const Vec2& mousePos) {
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

	EventReply SVerticalBox::onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		for (const Slot& slot : m_slots) {
			if (slot.child) {
				EventReply reply = slot.child->onMouseButtonDown(slot.child->getAllocatedGeometry(), mousePos, button);
				if (reply.isHandled) return reply;
			}
		}

		return EventReply::unhandled();
	}

	EventReply SVerticalBox::onMouseButtonUp(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		for (const Slot& slot : m_slots) {
			if (slot.child) {
				EventReply reply = slot.child->onMouseButtonUp(slot.child->getAllocatedGeometry(), mousePos, button);
				if (reply.isHandled) return reply;
			}
		}

		return EventReply::unhandled();
	}

	EventReply SVerticalBox::onMouseWheel(const Geometry& allocatedGeometry, const Vec2& mousePos, float scrollDelta) {
		for (const Slot& slot : m_slots) {
			if (slot.child) {
				EventReply reply = slot.child->onMouseWheel(slot.child->getAllocatedGeometry(), mousePos, scrollDelta);
				if (reply.isHandled) return reply;
			}
		}

		return EventReply::unhandled();
	}

	void SVerticalBox::addSlot(const Slot& slot) {
		m_slots.push_back(slot);
	}

	void SVerticalBox::clearSlots() {
		m_slots.clear();
	}

	const std::vector<Slot>& SVerticalBox::getSlots() const {
		return m_slots;
	}

	EventReply SVerticalBox::onDragOver(const Geometry& allocatedGeometry, const Vec2& mousePos, const DragDropPayload& payload) {
		for (const Slot& slot : m_slots) {
			if (slot.child) {
				EventReply reply = slot.child->onDragOver(slot.child->getAllocatedGeometry(), mousePos, payload);
				if (reply.isHandled) return reply;
			}
		}
		return EventReply::unhandled();
	}

	EventReply SVerticalBox::onDrop(const Geometry& allocatedGeometry, const Vec2& mousePos, const DragDropPayload& payload) {
		for (const Slot& slot : m_slots) {
			if (slot.child) {
				EventReply reply = slot.child->onDrop(slot.child->getAllocatedGeometry(), mousePos, payload);
				if (reply.isHandled) return reply;
			}
		}
		return EventReply::unhandled();
	}

}
