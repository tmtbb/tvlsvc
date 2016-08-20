// Copyright (c) 2016 The tourism Authors. All rights reserved.
// user_mysql.cc
// Created on: 2016年8月16日.
// Author: Paco.

#include "user/user_mysql.h"

#include <mysql/mysql.h>

#include "pub/storage/data_engine.h"
#include "pub/comm/comm_head.h"

#include "glog/logging.h"

namespace user {

UserMysql::UserMysql(config::FileConfig* config) {
  mysql_engine_ = base_logic::DataEngine::Create(MYSQL_TYPE);
  mysql_engine_->InitParam(config->mysql_db_list_);
  LOG(INFO) << "mysql list size:" << config->mysql_db_list_.size();
}

UserMysql::~UserMysql() {
  if (mysql_engine_) {
    delete mysql_engine_;
  }
  mysql_engine_ = NULL;
}

int32 UserMysql::NearGuideSelect(double* point, DicValue* dic) {
  int32 err = 0;
  bool r = false;
  do {
    std::stringstream ss;
    ss << "call proc_NearGuideSelect(" << point[0] << "," << point[1] <<
        "," << point[2] << "," << point[3] << ")";
    LOG(INFO) << "sql:" << ss.str();
    r = mysql_engine_->ReadData(ss.str(), dic, CallNearGuideSelect);
    LOG(INFO) << "sql exec result:" << r;
    if (!r) {
      err = SQL_EXEC_ERROR;
      break;
    }
    if (dic->empty()) {
      err = NO_GUIDE_NEARBY;
      break;
    }
  } while (0);
  return err;
}

int32 UserMysql::UserLoginSelect(std::string phone, std::string pass,
                                 int32 type, DicValue* dic) {
  int32 err = 0;
  bool r = false;
  do {
    std::stringstream ss;
    ss << "call proc_UserInfoAddrSelect('" << phone << "','" << pass <<
        "'," << type << ")";
    LOG(INFO) << "sql:" << ss.str();
    r = mysql_engine_->ReadData(ss.str(), dic, CallUserLoginSelect);
    LOG(INFO) << "sql exec result:" << r;
    if (!r) {
      err = SQL_EXEC_ERROR;
      break;
    }
    if (dic->empty()) {
      err = PHONE_OR_PASSWD_ERR;
      break;
    }
  } while (0);
  return err;
}

void UserMysql::CallNearGuideSelect(void* param, Value* value) {
  base_storage::DBStorageEngine* engine =
      (base_storage::DBStorageEngine*)(param);
  MYSQL_ROW rows;
  int32 num = engine->RecordCount();
  DicValue* info = reinterpret_cast<DicValue*>(value);
  if (num > 0) {
    ListValue* list = new ListValue();
    while (rows = (*(MYSQL_ROW*)(engine->FetchRows())->proc)) {
      DicValue* dict = new DicValue();
      if (rows[0] != NULL)
        dict->SetBigInteger(L"uid_", atoll(rows[0]));
      if (rows[1] != NULL)
        dict->SetString(L"phone_num_", rows[1]);
      if (rows[2] != NULL)
        dict->SetString(L"nick_name_", rows[2]);
      if (rows[3] != NULL)
        dict->SetBigInteger(L"level_", atoll(rows[3]));
      if (rows[4] != NULL)
        dict->SetString(L"head_url_", rows[4]);
      if (rows[5] != NULL)
        dict->SetString(L"address_", rows[5]);
      if (rows[6] != NULL)
        dict->SetReal(L"longitude_", atof(rows[6]));
      if (rows[7] != NULL)
        dict->SetReal(L"latitude_", atof(rows[7]));
      list->Append(dict);
    }
    info->Set(L"result", list);
  } else {
    LOG(WARNING) << "CallUserLoginSelect count < 0";
  }
}

void UserMysql::CallUserLoginSelect(void* param, Value* value) {
  base_storage::DBStorageEngine* engine =
      (base_storage::DBStorageEngine*)(param);
  MYSQL_ROW rows;
  int32 num = engine->RecordCount();
  DicValue* dict = reinterpret_cast<DicValue*>(value);
  if (num > 0) {
    while (rows = (*(MYSQL_ROW*)(engine->FetchRows())->proc)) {
      if (rows[0] != NULL)
        dict->SetBigInteger(L"uid_", atoll(rows[0]));
      if (rows[1] != NULL)
        dict->SetString(L"phone_num_", rows[1]);
      if (rows[2] != NULL)
        dict->SetString(L"nick_name_", rows[2]);
      if (rows[3] != NULL)
        dict->SetBigInteger(L"level_", atoll(rows[3]));
      if (rows[4] != NULL)
        dict->SetString(L"head_url_", rows[4]);
      if (rows[5] != NULL)
        dict->SetString(L"address_", rows[5]);
      if (rows[6] != NULL)
        dict->SetReal(L"longitude_", atof(rows[6]));
      if (rows[7] != NULL)
        dict->SetReal(L"latitude_", atof(rows[7]));
    }
  } else {
    LOG(WARNING) << "CallUserLoginSelect count < 0";
  }
}



}  // namespace user

