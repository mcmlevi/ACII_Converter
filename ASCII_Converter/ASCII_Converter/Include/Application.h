#pragma once
#include "Image.h"
namespace ASCII
{
	class Application
	{
	public:
		Application();

		void InitializeTable();

		~Application() = default;

	private:
		Image mImageMap;
	};

}

