
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "Utils.h"
#include "FileMonitor.h"
#include "SpreadMonitor.h"
#include "SrvSockMonitor.h"

#include "Config.h"
#include "Pumping.h"
#include "PGSQLloader.h"

int main(int argc, char** argv){
	
    int c;
	int bdaemon=0;
    while(1) {
        c = getopt(argc, argv, "D");
          
        if (c == -1)    break;
        switch (c){
        case 'D':
            bdaemon = 1;
            break;
        default: 
            exit(APP_SUCCESS);
        }
    }

	Utils& utils = Utils::getInstance();
	if(bdaemon){	
		utils.daemonize();
	}
	if(Config::getInstance().init() < 0){
		log(LOG_CRIT, "Config init failed\n");
		exit(-1);
	}
	if (utils.init()<0){
		log(LOG_CRIT, "Utils init failed\n");
		exit(-1);
	}

	AppState::set_state(STATE_INIT);

	while(AppState::get_state()>= STATE_INIT){
		log(LOG_DEBUG, "in while STATE_INIT");			
		SQLloader* sqlloader = new PGSQLloader();
		sqlloader->init(Config::getInstance());
		//sqlloader->debug();
		
		Pumping pump;
		
		Monitor* fm = new FileMonitor(sqlloader);
		fm->init(&pump);
		pump.add_monitor(fm);

		fm = new SrvSockMonitor();
		fm->init(&pump);
		pump.add_monitor(fm);

		AppState::set_state(STATE_ADMINUP);
		while(AppState::get_state()>= STATE_ADMINUP){
			fm = new SpreadMonitor(sqlloader);
			if( fm->init(&pump) < 0){
				log(LOG_CRIT, "SPread init failed, program exiting");
				AppState::set_state(STATE_CLOSED);
				break;
			}
			pump.add_monitor(fm);
			AppState::set_state(STATE_LOOP);
			pump.run();
		}
		delete sqlloader;
	}
    return 0;
}
