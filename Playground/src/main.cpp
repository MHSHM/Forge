#include <iostream>

#include <Swapchain.h>

#include <Forge.h>
#include <ForgeLogger.h>

int main()
{
	auto forge = forge::forge_new();

	forge_destroy(forge);

	return 0;
}