#pragma once
#include <httplib.h>
#include "api/ApiContext.h"

void registerAuthRoutes(httplib::Server& svr, ApiContext& ctx);
void registerSecurityRoutes(httplib::Server& svr, ApiContext& ctx);
void registerDashboardRoutes(httplib::Server& svr, ApiContext& ctx);
