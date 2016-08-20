// Copyright (c) 2016 The tourism Authors. All rights reserved.
// user_manager.cc
// Created on: 2016年8月10日.
// Author: Paco.

#include "user/user_manager.h"

#include <stdio.h>

#include "user/user_opcode.h"
#include "user/user_interface.h"

namespace user {
UserManager* UserManager::instance_;

UserManager* UserManager::GetInstance() {
  if (instance_ == NULL)
    instance_ = new UserManager();
  return instance_;
}

void UserManager::FreeInstance() {
  if (instance_ == NULL) {
    delete instance_;
    instance_ = NULL;
  }
}

UserManager::UserManager() {

}

UserManager::~UserManager() {

}

int32 UserManager::AssignPacket(const int32 socket, PacketHead* packet) {
  int32 err = 0;
  UserInterface* interface = UserInterface::GetInstance();
  switch (packet->operate_code()) {
    case USER_LOGIN_REQ: {
      interface->UserLogin(socket, packet);
      break;
    }
    case NEARBY_WAITER_REQ: {
      interface->NearbyGuide(socket, packet);
      break;
    }

  }
  return err;
}

}  // namespace user

