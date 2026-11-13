#pragma once
#include <httplib.h>
#include "api/ApiContext.h"

class ApiServer {
public:
    explicit ApiServer(ApiContext& ctx);
    bool run();   // blocks until stop()
    void stop();
private:
    void configure();
    ApiContext& ctx_;
    bool hasFrontend_ = false;
    httplib::Server svr_;
};
