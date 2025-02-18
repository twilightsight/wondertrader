/*!
 * \file WtBtRunner.cpp
 * \project	WonderTrader
 *
 * \author Wesley
 * \date 2020/03/30
 * 
 * \brief 
 */
#include "../WtBtCore/HisDataReplayer.h"
#include "../WtBtCore/CtaMocker.h"
#include "../WtBtCore/ExecMocker.h"
#include "../WtBtCore/HftMocker.h"
#include "../WtBtCore/SelMocker.h"
#include "../WtBtCore/UftMocker.h"
#include "../WtBtCore/WtHelper.h"

#include "../WTSTools/WTSLogger.h"
#include "../WTSUtils/SignalHook.hpp"

#include "../WTSUtils/WTSCfgLoader.h"
#include "../Includes/WTSVariant.hpp"
#include "../Share/StdUtils.hpp"

#ifdef _MSC_VER
#include "../Common/mdump.h"
#endif

int main()
{
#ifdef _MSC_VER
    CMiniDumper::Enable("WtBtRunner.exe", true, WtHelper::getCWD().c_str());
#endif

	std::string filename = "logcfgbt.json";
	if (!StdFile::exists(filename.c_str()))
		filename = "logcfgbt.yaml";

	WTSLogger::init(filename.c_str());

#if _WIN32
#pragma message("Signal hooks disabled in WIN32")
#else
#pragma message("Signal hooks enabled in UNIX")
	install_signal_hooks([](const char* message) {
		WTSLogger::error(message);
	});
#endif

	filename = "configbt.json";
	if(!StdFile::exists(filename.c_str()))
		filename = "configbt.yaml";

	WTSVariant* cfg = WTSCfgLoader::load_from_file(filename.c_str());
	if (cfg == NULL)
	{
		WTSLogger::info("Loading configuration file {} failed", filename);
		return -1;
	}

	HisDataReplayer replayer;
	replayer.init(cfg->get("replayer"));

	WTSVariant* cfgEnv = cfg->get("env");
	const char* mode = cfgEnv->getCString("mocker");
	int32_t slippage = cfgEnv->getInt32("slippage");
	if (strcmp(mode, "cta") == 0)
	{
		CtaMocker* mocker = new CtaMocker(&replayer, "cta", slippage);
		mocker->init_cta_factory(cfg->get("cta"));
		const char* stra_id = cfg->get("cta")->get("strategy")->getCString("id");
		// 加载增量回测的基础历史回测数据
		const char* incremental_backtest_base = cfg->get("env")->getCString("incremental_backtest_base");
		if (strlen(incremental_backtest_base) > 0)
		{
			mocker->load_incremental_data(incremental_backtest_base);
		}
		replayer.register_sink(mocker, stra_id);
	}
	else if (strcmp(mode, "hft") == 0)
	{
		HftMocker* mocker = new HftMocker(&replayer, "hft");
		mocker->init_hft_factory(cfg->get("hft"));
		const char* stra_id = cfg->get("hft")->get("strategy")->getCString("id");
		replayer.register_sink(mocker, stra_id);
	}
	else if (strcmp(mode, "sel") == 0)
	{
		SelMocker* mocker = new SelMocker(&replayer, "sel", slippage);
		mocker->init_sel_factory(cfg->get("sel"));
		const char* stra_id = cfg->get("sel")->get("strategy")->getCString("id");
		replayer.register_sink(mocker, stra_id);
	}
	else if (strcmp(mode, "exec") == 0)
	{
		ExecMocker* mocker = new ExecMocker(&replayer);
		mocker->init(cfg->get("exec"));
		replayer.register_sink(mocker, "exec");
	}
	else if (strcmp(mode, "uft") == 0)
	{
		UftMocker* mocker = new UftMocker(&replayer, "uft");
		mocker->init_uft_factory(cfg->get("uft"));
		const char* stra_id = cfg->get("uft")->get("strategy")->getCString("id");
		replayer.register_sink(mocker, stra_id);
	}

	replayer.prepare();

	replayer.run(true);

	printf("press enter key to exit\r\n");
	getchar();

	WTSLogger::stop();
}
