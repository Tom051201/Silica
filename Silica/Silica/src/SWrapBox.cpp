#include "SWrapBox.h"

namespace Silica {

	void SWrapBox::construct(const Args& args) {
		m_spacing = args.spacing;
		m_children = args.children;
	}

	void SWrapBox::computeDesiredSize() {
		float cachedHeight = m_desiredSize.y;
		float maxChildWidth = 0.0f;

		for (auto& child : m_children) {
			if (child) {
				child->computeDesiredSize();
				maxChildWidth = std::max(maxChildWidth, child->getDesiredSize().x);
			}
		}

		m_desiredSize = { maxChildWidth, cachedHeight };
	}

	void SWrapBox::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);

		float scaledSpacing = m_spacing * m_renderScale;
		float currentX = scaledSpacing;
		float currentY = scaledSpacing;
		float rowHeight = 0.0f;

		for (auto& child : m_children) {
			if (!child) continue;
			Vec2 size = child->getDesiredSize();

			if (currentX + size.x > allocatedGeometry.size.x && currentX > scaledSpacing) {
				currentX = scaledSpacing;
				currentY += rowHeight + scaledSpacing;
				rowHeight = 0.0f;
			}

			child->arrangeChildren({ allocatedGeometry.position + Vec2(currentX, currentY), size });
			currentX += size.x + scaledSpacing;
			rowHeight = std::max(rowHeight, size.y);
		}

		m_desiredSize.y = currentY + rowHeight + scaledSpacing;
	}

	void SWrapBox::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		for (auto& child : m_children) {
			if (child) child->onDraw(outDrawList, child->getAllocatedGeometry());
		}
	}

	void SWrapBox::setRenderScale(float scale) {
		m_renderScale = scale;
		for (auto& child : m_children) {
			if (child) child->setRenderScale(scale);
		}
	}

	EventReply SWrapBox::onMouseMove(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		bool eventHandled = false;

		for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
			if (*it) {
				if (!eventHandled) {
					EventReply reply = (*it)->onMouseMove((*it)->getAllocatedGeometry(), mousePos);
					if (reply.isHandled) {
						eventHandled = true;
					}
				}
				else {
					(*it)->onMouseMove((*it)->getAllocatedGeometry(), Vec2(-9999.0f, -9999.0f));
				}
			}
		}

		return eventHandled ? EventReply::handled() : EventReply::unhandled();
	}

	EventReply SWrapBox::onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		for (auto& child : m_children) {
			if (child && child->onMouseButtonDown(child->getAllocatedGeometry(), mousePos, button).isHandled) return EventReply::handled();
		}

		return EventReply::unhandled();
	}

	EventReply SWrapBox::onMouseButtonUp(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		for (auto& child : m_children) {
			if (child && child->onMouseButtonUp(child->getAllocatedGeometry(), mousePos, button).isHandled) return EventReply::handled();
		}

		return EventReply::unhandled();
	}

	EventReply SWrapBox::onMouseWheel(const Geometry& allocatedGeometry, const Vec2& mousePos, float scrollDelta) {
		for (auto& child : m_children) {
			if (child && child->onMouseWheel(child->getAllocatedGeometry(), mousePos, scrollDelta).isHandled) return EventReply::handled();
		}

		return EventReply::unhandled();
	}

	void SWrapBox::addChild(WidgetPtr child) {
		m_children.push_back(child);
	}

	EventReply SWrapBox::onDragOver(const Geometry& allocatedGeometry, const Vec2& mousePos, const DragDropPayload& payload) {
		for (auto& child : m_children) {
			if (child) {
				EventReply reply = child->onDragOver(child->getAllocatedGeometry(), mousePos, payload);
				if (reply.isHandled) return reply;
			}
		}
		return EventReply::unhandled();
	}

	EventReply SWrapBox::onDrop(const Geometry& allocatedGeometry, const Vec2& mousePos, const DragDropPayload& payload) {
		for (auto& child : m_children) {
			if (child) {
				EventReply reply = child->onDrop(child->getAllocatedGeometry(), mousePos, payload);
				if (reply.isHandled) return reply;
			}
		}
		return EventReply::unhandled();
	}

}
