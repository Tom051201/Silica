#pragma once

#include <string>

#include "SWidget.h"
#include "FontAtlas.h"
#include "MathTypes.h"

namespace Silica {

	class STextBlock : public SWidget {
	public:
		struct Args {
			std::string text = "";
			Color color = Color::white();
			FontAtlas* font = nullptr;
		};

		void construct(const Args& args);

		void computeDesiredSize() override;
		void arrangeChildren(const Geometry& allocatedGeometry) override;
		void onDraw(DrawList& outDrawList, const Geometry& allotedGeometry) const override;

	private:

		std::string m_text;
		Color m_color;
		FontAtlas* m_font;

	};

}
