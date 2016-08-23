// Copyright (c) 2016 The tourism Authors. All rights reserved.
// user_opcode.h
// Created on: 2016年8月10日.
// Author: Paco.
#ifndef PLUGINS_USER_USER_OPCODE_H_
#define PLUGINS_USER_USER_OPCODE_H_

#include "pub/comm/comm_head.h"

namespace user {
#define PACKET_HEART_REQ 1000
#define USER_LOGIN_REQ USER_TYPE*1000 + 1
#define USER_LOGIN_RLY USER_TYPE*1000 + 2

#define NEARBY_GUIDE_REQ USER_TYPE*1000 + 3
#define NEARBY_GUIDE_RLY USER_TYPE*1000 + 4

#define GUIDE_DETAIL_REQ USER_TYPE*1000 + 5
#define GUIDE_DETAIL_RLY USER_TYPE*1000 + 6

#define GUIDE_RECOMMEND_REQ USER_TYPE*1000 + 7
#define GUIDE_RECOMMEND_RLY USER_TYPE*1000 + 8

#define SERVICE_CITY_REQ USER_TYPE*1000 + 9
#define SERVICE_CITY_RLY USER_TYPE*1000 + 10
}  // namespace user



#endif  // PLUGINS_USER_USER_OPCODE_H_
