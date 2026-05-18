#pragma once

#include <vector>

#include "SWidget.h"

namespace Silica {


	class SOverlay : public SWidget {
	public:

		struct Args {
			std::vector<WidgetPtr> children;
		};

		void construct(const Args& args) {
			m_children = args.children;
		}

		void computeDesiredSize() override {
			m_desiredSize = Vec2::zero();

			for (auto& child : m_children) {
				if (child) {
					child->computeDesiredSize();
				}
			}
		}

		void arrangeChildren(const Geometry& allocatedGeometry) override {
			SWidget::arrangeChildren(allocatedGeometry);
			for (auto& child : m_children) {
				if (child) child->arrangeChildren(allocatedGeometry);
			}
		}

		void onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const override {
			for (auto& child : m_children) {
				if (child) child->onDraw(outDrawList, child->getAllocatedGeometry());
			}
		}

		EventReply onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos) override {
			for (auto it = m_children.rbegin(); it != m_children.rend(); it++) {
				if (*it && (*it)->onMouseButtonDown((*it)->getAllocatedGeometry(), mousePos).isHandled) {
					return EventReply::handled();
				}
			}
			return EventReply::unhandled();
		}

		EventReply onMouseMove(const Geometry& allocatedGeometry, const Vec2& mousePos) override {
			EventReply finalReply = EventReply::unhandled();
			for (auto it = m_children.rbegin(); it != m_children.rend(); it++) {
				if (*it) {
					EventReply reply = (*it)->onMouseMove((*it)->getAllocatedGeometry(), mousePos);
					if (reply.isHandled) finalReply = EventReply::handled();
				}
			}
			return finalReply;
		}

		EventReply onMouseButtonUp(const Geometry& allocatedGeometry, const Vec2& mousePos) override {
			for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
				if (*it && (*it)->onMouseButtonUp((*it)->getAllocatedGeometry(), mousePos).isHandled) return EventReply::handled();
			}
			return EventReply::unhandled();
		}

		EventReply onMouseWheel(const Geometry& allocatedGeometry, const Vec2& mousePos, float scrollDelta) override {
			for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
				if (*it && (*it)->onMouseWheel((*it)->getAllocatedGeometry(), mousePos, scrollDelta).isHandled) return EventReply::handled();
			}
			return EventReply::unhandled();
		}

	private:

		std::vector<WidgetPtr> m_children;

	};

}