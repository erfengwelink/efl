#include<iostream>
using namespace std;
#include"sgregion.h"
#include"sgmatrix.h"
#include"sgpath.h"
#include"ftraster_p.h"
#include"sgelapsedtimer.h"
#include"sgdebug.h"
#include "sgjson.h"
#include<iostream>
#include<fstream>
#include<sstream>


int main()
{
    initialize(GuaranteedLogger(), "/tmp/", "ssglog", 1);
    set_log_level(LogLevel::INFO);
    //SGJson json;
    std::ifstream file;
    file.open("/home/subhransu/openSourceEfl/ssgrepo/ssg/build/example/LightBulb.json");
    std::stringstream buffer;
    buffer << file.rdbuf();
    //std::cout <<"file size = "<< buffer.str().size()<<std::endl;
    //std::cout << buffer.str().data();
    SGJson json(buffer.str().data());
    file.close();
    return 0;
}
