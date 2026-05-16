#pragma once

#include <cstdint>

#include "SWidget.h"
#include "Renderer.h"

namespace Silica {

	class SBox : public SWidget {
	public:

		struct Args {
			Vec2 padding = Vec2::zero();
			uint32_t backgroundColor = 0x00000000;
			uint32_t hoverColor = 0x00000000;
			std::function<EventReply()> onClick = nullptr;
			WidgetPtr child = nullptr;
		};

		void construct(const Args& args);

		void computeDesiredSize() override;
		void arrangeChildren(const Geometry& allocatedGeometry) override;
		void onDraw(DrawList& outDrawList, const Geometry& allotedGeometry) const override;

		EventReply onMouseMove(const Geometry& allotedGeometry, const Vec2& mousePos) override;
		EventReply onMouseButtonDown(const Geometry& allotedGeometry, const Vec2& mousePos) override;
		EventReply onMouseButtonUp(const Geometry& allotedGeometry, const Vec2& mousePos) override;

	private:

		Vec2 m_padding;
		uint32_t m_backgroundColor;
		uint32_t m_hoverColor;
		std::function<EventReply()> m_onClick;
		WidgetPtr m_child;

		void addRectToDrawList(DrawList& drawList, const Geometry& geo, uint32_t color) const;

	};

}
