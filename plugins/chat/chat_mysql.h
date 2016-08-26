// Copyright (c) 2016 The tvlsvc Authors. All rights reserved.
// chat_mysql.h
// Created on: 2016年8月24日.
// Author: Paco.
#ifndef PLUGINS_CHAT_CHAT_MYSQL_H_
#define PLUGINS_CHAT_CHAT_MYSQL_H_

#include "public/config/config.h"
#include "public/basic/basictypes.h"

#include "pub/storage/data_engine.h"

#include "pub/net/typedef.h"

namespace chat {

class ChatMysql {
 public:
  ChatMysql(config::FileConfig* config);
  ~ChatMysql();

  int32 ChatRecordInsert(int64 from, int64 to, std::string msg, int64 time);
  int32 ChatRecordInsert(std::list<std::string> sqls);
 private:
  base_logic::DataEngine* mysql_engine_;
};

}  // namespace chat



#endif  // PLUGINS_CHAT_CHAT_MYSQL_H_