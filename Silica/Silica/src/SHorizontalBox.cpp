#include "silicapch.h"
#include "SHorizontalBox.h"

namespace Silica {

	void SHorizontalBox::construct(const Args& args) {
		m_spacing = args.spacing;
		m_slots = args.slots;
	}

	void SHorizontalBox::computeDesiredSize() {
		m_desiredSize = Vec2::zero();
		bool isFirstChild = true;

		for (const Slot& slot : m_slots) {
			if (!slot.child) continue;

			slot.child->computeDesiredSize();
			Vec2 childSize = slot.child->getDesiredSize();

			if (!isFirstChild) m_desiredSize.x += m_spacing;
			isFirstChild = false;

			m_desiredSize.x += childSize.x + (slot.padding.x * 2.0f);

			float childHeightWithPadding = childSize.y + (slot.padding.y * 2.0f);
			if (childHeightWithPadding > m_desiredSize.y) {
				m_desiredSize.y = childHeightWithPadding;
			}
		}
	}

	void SHorizontalBox::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);

		float scaledSpacing = m_spacing * m_renderScale;
		float currentX = allocatedGeometry.position.x;
		bool isFirstChild = true;

		for (const Slot& slot : m_slots) {
			if (!slot.child) continue;

			if (!isFirstChild) currentX += scaledSpacing;
			isFirstChild = false;

			float padX = slot.padding.x * m_renderScale;
			float padY = slot.padding.y * m_renderScale;

			Vec2 childDesired = slot.child->getDesiredSize();
			Geometry childGeo;
			childGeo.position.x = currentX + padX;
			childGeo.position.y = allocatedGeometry.position.y + padY;
			childGeo.size.x = childDesired.x * m_renderScale;
			childGeo.size.y = allocatedGeometry.size.y - (padY * 2.0f);

			if (childGeo.size.y < 0.0f) childGeo.size.y = 0.0f;

			float maxAvailableX = (allocatedGeometry.position.x + allocatedGeometry.size.x) - childGeo.position.x - padX;
			if (childGeo.size.x > maxAvailableX) {
				childGeo.size.x = maxAvailableX > 0 ? maxAvailableX : 0.0f;
			}

			slot.child->arrangeChildren(childGeo);
			currentX += childGeo.size.x + (padX * 2.0f);
		}
	}

	void SHorizontalBox::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		for (const Slot& slot : m_slots) {
			if (slot.child) {
				slot.child->onDraw(outDrawList, slot.child->getAllocatedGeometry());
			}
		}
	}

	void SHorizontalBox::setRenderScale(float scale) {
		m_renderScale = scale;
		for (auto& slot : m_slots) {
			if (slot.child) slot.child->setRenderScale(scale);
		}
	}

	EventReply SHorizontalBox::onMouseMove(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		bool eventHandled = false;

		for (auto it = m_slots.rbegin(); it != m_slots.rend(); ++it) {
			if (it->child) {
				if (!eventHandled) {
					EventReply reply = it->child->onMouseMove(it->child->getAllocatedGeometry(), mousePos);
					if (reply.isHandled) {
						eventHandled = true;
					}
				}
				else {
					it->child->onMouseMove(it->child->getAllocatedGeometry(), Vec2(-9999.0f, -9999.0f));
				}
			}
		}

		return eventHandled ? EventReply::handled() : EventReply::unhandled();
	}

	EventReply SHorizontalBox::onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		for (const Slot& slot : m_slots) {
			if (slot.child) {
				EventReply reply = slot.child->onMouseButtonDown(slot.child->getAllocatedGeometry(), mousePos, button);
				if (reply.isHandled) return reply;
			}
		}

		return EventReply::unhandled();
	}

	EventReply SHorizontalBox::onMouseButtonUp(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		for (const Slot& slot : m_slots) {
			if (slot.child) {
				EventReply reply = slot.child->onMouseButtonUp(slot.child->getAllocatedGeometry(), mousePos, button);
				if (reply.isHandled) return reply;
			}
		}

		return EventReply::unhandled();
	}

	EventReply SHorizontalBox::onMouseWheel(const Geometry& allocatedGeometry, const Vec2& mousePos, float scrollDelta) {
		for (const Slot& slot : m_slots) {
			if (slot.child) {
				EventReply reply = slot.child->onMouseWheel(slot.child->getAllocatedGeometry(), mousePos, scrollDelta);
				if (reply.isHandled) return reply;
			}
		}

		return EventReply::unhandled();
	}

	void SHorizontalBox::addSlot(const Slot& slot) {
		m_slots.push_back(slot);
	}

	void SHorizontalBox::clearSlots() {
		m_slots.clear();
	}

	const std::vector<Slot>& SHorizontalBox::getSlots() const {
		return m_slots;
	}

	EventReply SHorizontalBox::onDragOver(const Geometry& allocatedGeometry, const Vec2& mousePos, const DragDropPayload& payload) {
		for (const Slot& slot : m_slots) {
			if (slot.child) {
				EventReply reply = slot.child->onDragOver(slot.child->getAllocatedGeometry(), mousePos, payload);
				if (reply.isHandled) return reply;
			}
		}
		return EventReply::unhandled();
	}

	EventReply SHorizontalBox::onDrop(const Geometry& allocatedGeometry, const Vec2& mousePos, const DragDropPayload& payload) {
		for (const Slot& slot : m_slots) {
			if (slot.child) {
				EventReply reply = slot.child->onDrop(slot.child->getAllocatedGeometry(), mousePos, payload);
				if (reply.isHandled) return reply;
			}
		}
		return EventReply::unhandled();
	}

}
