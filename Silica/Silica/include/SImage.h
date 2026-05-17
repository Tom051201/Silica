#pragma once

#include "SWidget.h"
#include "Renderer.h"

namespace Silica {

	class SImage : public SWidget {
	public:

		struct Args {
			TextureID textureID = 0;
			Color tint = Color::white();
			Vec2 desiredSize = Vec2(100.0f, 100.0f);
		};

		void construct(const Args& args);

		void computeDesiredSize() override {}
		void arrangeChildren(const Geometry& allocatedGeometry) override;
		void onDraw(DrawList& outDrawList, const Geometry& allotedGeometry) const override;

	private:

		TextureID m_textureId;
		Color m_tintColor;

	};

}
