#include "Theme.h"

namespace Silica {

	static Theme s_globalTheme;

	Theme& GetTheme() {
		return s_globalTheme;
	}

}
