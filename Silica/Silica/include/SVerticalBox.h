#pragma once

#include "SWidget.h"

namespace Silica {

	class SVerticalBox : public SWidget {
	public:

		struct Args {
			std::vector<Slot> slots;
		};

		void construct(const Args& args) {
			m_slots = args.slots;
		}

		void computeDesiredSize() override;
		void arrangeChildren(const Geometry& allocatedGeometry) override;
		void onDraw(DrawList& outDrawList, const Geometry& allotedGeometry) const override;

		EventReply onMouseMove(const Geometry& allotedGeometry, const Vec2& mousePos) override;
		EventReply onMouseButtonDown(const Geometry& allotedGeometry, const Vec2& mousePos) override;
		EventReply onMouseButtonUp(const Geometry& allotedGeometry, const Vec2& mousePos) override;
		EventReply onMouseWheel(const Geometry& allotedGeometry, const Vec2& mousePos, float scrollDelta) override;

	private:

		std::vector<Slot> m_slots;

	};

}
