#include "proxyserver.h"
#include <string>

int main()
{
	SWL_Init();
	CProxyServer webser;
	webser.Init(8080);
	webser.Start();

	getchar();

	webser.Stop();
	webser.Quit();

	SWL_Quit();
	return 0;
}
