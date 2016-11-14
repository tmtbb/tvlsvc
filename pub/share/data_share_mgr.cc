// Copyright (c) 2016 The tourism Authors. All rights reserved.
// data_share_mgr.cc
// Created on: 2016年8月16日.
// Author: Paco.

#include "pub/share/data_share_mgr.h"

#include <sys/unistd.h>
#include "glog/logging.h"

#include "base/logic/logic_comm.h"

share::DataShareMgr *GetDataShareMgr(void) {
  return share::DataShareMgr::GetInstance();
}

namespace share {
DataShareMgr* DataShareMgr::instance_ = NULL;

DataShareMgr::DataShareMgr() {
  InitThreadrw(&lock_);
  token_time_ = 0;
}

DataShareMgr::~DataShareMgr() {
  DeinitThreadrw(lock_);
}

__attribute__((visibility("default")))
      DataShareMgr* DataShareMgr::GetInstance() {
  if (instance_ == NULL) {
    instance_ = new DataShareMgr();
  }
  return instance_;
}

UserInfo* DataShareMgr::GetUser(int64 uid) {
  LOG(ERROR)<< "DataShareMgr GetUser lock";
  base_logic::RLockGd lk(lock_);
  UserMap::iterator it1 = user_map_.find(uid);
  if (it1 != user_map_.end()) {
    LOG(ERROR) << "DataShareMgr GetUser unlock";
    return it1->second;
  }
  LOG(ERROR) << "DataShareMgr GetUser unlock";
  return NULL;
}

UserInfo* DataShareMgr::GetFreeCoordinator() {
  LOG(ERROR)<< "DataShareMgr GetFreeCoordinator lock";
  base_logic::RLockGd lk(lock_);
  LOG(ERROR)<< "DataShareMgr GetFreeCoordinator locked";
  Coordinator* info = NULL;
  if (coordinator_map_.empty())
    return info;
  CoordinatorMap::iterator it = coordinator_map_.begin();
  info = it->second;
  ++it;
  for (; it != coordinator_map_.end(); ++it) {
    if (it->second != NULL) {
      if (it->second->customers_num() < info->customers_num()) {
        info = it->second;
      }
    }
  }
  LOG(ERROR)<< "DataShareMgr GetFreeCoordinator unlocked";
  return info;
}

void DataShareMgr::AddUser(UserInfo* user) {
  AddNick(user->uid(), user->nickname());
  LOG(ERROR)<< "DataShareMgr AddUser lock";
  base_logic::WLockGd lk(lock_);
  LOG(ERROR)<< "DataShareMgr AddUser locked";
  user_map_[user->uid()] = user;
  if (user->user_type() == 1)
    visitor_map_[user->uid()] = reinterpret_cast<Visitor*>(user);
  else if (user->user_type() == 2)
    guide_map_[user->uid()] = reinterpret_cast<Guide*>(user);
  else if (user->user_type() == 3)
    coordinator_map_[user->uid()] = reinterpret_cast<Coordinator*>(user);
  LOG(ERROR)<< "DataShareMgr AddUser unlock";
}

void DataShareMgr::DelUser(int64 uid) {
  UserInfo* p_user = NULL;
  DelGuide(uid);
  DelVisitor(uid);
  DelCoordinator(uid);
  {
    LOG(ERROR)<< "DataShareMgr DelUser lock";
    base_logic::WLockGd lk(lock_);
    LOG(ERROR) << "DataShareMgr DelUser locked";
    UserMap::iterator it1 = user_map_.find(uid);
    if (it1 != user_map_.end()) {
      p_user = it1->second;
      user_map_.erase(it1);
    }
  }
  // 清理完3个map 最后释放
  if (p_user != NULL) {
    delete p_user;
    p_user = NULL;
  }
  LOG(ERROR)<< "DataShareMgr DelUser unlock";
}

void DataShareMgr::UserOffline(int fd) {
  LOG(ERROR)<< "DataShareMgr UserOffline lock";
  base_logic::WLockGd lk(lock_);
  LOG(ERROR) << "DataShareMgr UserOffline locked";
  UserMap::iterator it1 = user_map_.begin();
  for (; it1 != user_map_.end(); ) {
    UserInfo* p = it1->second;
    if (p == NULL) {
      LOG(ERROR) << "UserOffline user NULL";
      user_map_.erase(it1++);
      continue;
    }
    if (p->socket_fd() == fd) {
      user_map_.erase(it1++);
      GuideMap::iterator itg = guide_map_.find(p->uid());
      if (itg != guide_map_.end()) {
        Guide* p = itg->second;
        guide_map_.erase(itg);
      }
      VisitorMap::iterator itv = visitor_map_.find(p->uid());
      if (itv != visitor_map_.end()) {
        Visitor* p = itv->second;
        visitor_map_.erase(itv);
      }
      CoordinatorMap::iterator itc = coordinator_map_.find(p->uid());
      if (itc != coordinator_map_.end()) {
        Coordinator* p = itc->second;
        coordinator_map_.erase(itc);
      }
      delete p;
      p = NULL;
      break;
    } else {
      it1++;
      continue;
    }
  }
  LOG(ERROR) << "DataShareMgr UserOffline unlock";
}

void DataShareMgr::CheckHeartLoss() {
  LOG(ERROR)<< "DataShareMgr CheckHeartLoss lock";
  base_logic::WLockGd lk(lock_);
  LOG(ERROR) << "DataShareMgr CheckHeartLoss locked";
  UserMap::iterator it1 = user_map_.begin();
  for (; it1 != user_map_.end(); ) {
    UserInfo* p = it1->second;
    if (p == NULL) {
      ++it1;
      LOG(ERROR) << "CheckHeartLoss UserInfo NULL";
      continue;
    }
    p->set_heart_loss(p->heart_loss() + 1);
    if (p->heart_loss() < 3) {
      ++it1;
      continue;
    } else {
      LOG(WARNING) << "user lost connect uid:" << p->uid();
      user_map_.erase(it1++);
      GuideMap::iterator itg = guide_map_.find(p->uid());
      if (itg != guide_map_.end()) {
        Guide* p = itg->second;
        guide_map_.erase(itg);
      }
      VisitorMap::iterator itv = visitor_map_.find(p->uid());
      if (itv != visitor_map_.end()) {
        Visitor* p = itv->second;
        visitor_map_.erase(itv);
      }
      CoordinatorMap::iterator itc = coordinator_map_.find(p->uid());
      if (itc != coordinator_map_.end()) {
        Coordinator* p = itc->second;
        coordinator_map_.erase(itc);
      }
      close(p->socket_fd());
      delete p;
      p = NULL;
    }
  }
  LOG(ERROR) << "DataShareMgr CheckHeartLoss unlock";
}

void DataShareMgr::UserHeart(int64 uid) {
  LOG(ERROR)<< "DataShareMgr UserHeart lock";
  base_logic::WLockGd lk(lock_);
  LOG(ERROR) << "DataShareMgr UserHeart locked";
  UserMap::iterator it1 = user_map_.find(uid);
  if (it1 != user_map_.end()) {
    UserInfo* p = it1->second;
    if (p != NULL) {
      p->set_heart_loss(0);
    }
  }
  LOG(ERROR) << "DataShareMgr UserHeart unlock";
}

//只清理map,内存由user管理
void DataShareMgr::DelGuide(int64 uid) {
  LOG(ERROR)<< "DataShareMgr DelGuide lock";
  base_logic::WLockGd lk(lock_);
  LOG(ERROR) << "DataShareMgr DelGuide locked";
  GuideMap::iterator it1 = guide_map_.find(uid);
  if (it1 != guide_map_.end()) {
    Guide* p = it1->second;
    guide_map_.erase(it1);
  }
  LOG(ERROR) << "DataShareMgr DelGuide unlock";
}

void DataShareMgr::DelVisitor(int64 uid) {
  LOG(ERROR)<< "DataShareMgr DelVisitor lock";
  base_logic::WLockGd lk(lock_);
  LOG(ERROR) << "DataShareMgr DelVisitor locked";
  VisitorMap::iterator it1 = visitor_map_.find(uid);
  if (it1 != visitor_map_.end()) {
    Visitor* p = it1->second;
    visitor_map_.erase(it1);
  }
  LOG(ERROR) << "DataShareMgr DelVisitor unlock";
}

void DataShareMgr::DelCoordinator(int64 uid) {
  LOG(ERROR)<< "DataShareMgr DelCoordinator lock";
  base_logic::WLockGd lk(lock_);
  LOG(ERROR) << "DataShareMgr DelCoordinator locked";
  CoordinatorMap::iterator it1 = coordinator_map_.find(uid);
  if (it1 != coordinator_map_.end()) {
    Coordinator* p = it1->second;
    coordinator_map_.erase(it1);
  }
  LOG(ERROR) << "DataShareMgr DelCoordinator unlock";
}

int32 DataShareMgr::AddDeviceToken(int64 uid, std::string token) {
  int32 result = 0;
  LOG(ERROR)<< "DataShareMgr AddDeviceToken lock";
  base_logic::WLockGd lk(lock_);
  LOG(ERROR)<< "DataShareMgr AddDeviceToken locked";
  DeviceTokenMap::iterator it = dt_map_.find(uid);
  do {
    if (it != dt_map_.end()) {
      if (it->second == token) {
        result = -1;
        break;
      } else {
        dt_map_[uid] = token;
        result = 1;
        break;
      }
    } else {
      dt_map_[uid] = token;
      result = 0;
    }
  } while (0);
  LOG(ERROR)<< "DataShareMgr AddDeviceToken unlock";
  return result;
}

std::string DataShareMgr::GetDeviceToken(int64 uid) {
  LOG(ERROR)<< "DataShareMgr GetDeviceToken lock";
  base_logic::RLockGd lk(lock_);
  LOG(ERROR) << "DataShareMgr GetDeviceToken locked";
  DeviceTokenMap::iterator it = dt_map_.find(uid);
  LOG(ERROR) << "DataShareMgr GetDeviceToken find";
  if (it != dt_map_.end()) {
    LOG(ERROR) << "DataShareMgr GetDeviceToken unlock";
    return it->second;
  } else {
    LOG(ERROR) << "DataShareMgr GetDeviceToken unlock";
    return "";
  }
}

int32 DataShareMgr::AddUnReadCount(int64 uid) {
  int32 count = 0;
  LOG(ERROR)<< "DataShareMgr AddUnReadCount lock";
  base_logic::WLockGd lk(lock_);
  LOG(ERROR)<< "DataShareMgr AddUnReadCount locked";
  UnReadMap::iterator it = unread_map_.find(uid);
  if (it != unread_map_.end()) {
    count = it->second + 1;
    unread_map_[uid] = count;
  } else {
    count = 1;
    unread_map_[uid] = count;
  }
  LOG(ERROR)<< "DataShareMgr AddUnReadCount unlock";
  return count;
}

void DataShareMgr::DelUnReadCount(int64 uid, int32 count) {
  int32 left = 0;
  LOG(ERROR)<< "DataShareMgr DelUnReadCount lock";
  base_logic::WLockGd lk(lock_);
  LOG(ERROR)<< "DataShareMgr DelUnReadCount locked";
  do {
    if (count == -1) {
      left = 0;
      break;
    }
    UnReadMap::iterator it = unread_map_.find(uid);
    if (it != unread_map_.end()) {
      left = it->second - count;
    } else {
      left = 0;
    }
  } while (0);
  if (count < 0)
    count = 0;
  unread_map_[uid] = count;
  LOG(ERROR)<< "DataShareMgr DelUnReadCount unlock";
}

void DataShareMgr::AddNick(int64 uid, std::string nick) {
  LOG(ERROR)<< "DataShareMgr AddNick lock";
  base_logic::WLockGd lk(lock_);
  LOG(ERROR) << "DataShareMgr AddNick locked";
  nick_map_[uid] = nick;
  LOG(ERROR) << "DataShareMgr AddNick unlock";
}

std::string DataShareMgr::GetNick(int64 uid) {
  LOG(ERROR)<<"DataShareMgr GetNick lock";
  base_logic::RLockGd lk(lock_);
  LOG(ERROR) << "DataShareMgr GetNick locked";
  NickMap::iterator it = nick_map_.find(uid);
  if (it != nick_map_.end()) {
    LOG(ERROR) << "DataShareMgr GetNick unlock";
    return it->second;
  } else {
    LOG(ERROR) << "DataShareMgr GetNick unlock";
    return "";
  }
}

void DataShareMgr::InitShareType(ListValue* list) {
  LOG(ERROR) << "DataShareMgr InitShareType lock";
  base_logic::WLockGd lk(lock_);
  LOG(ERROR) << "DataShareMgr InitShareType locked";
  ShareTypeMap::iterator sit = share_type_map_.begin();
  for (; sit != share_type_map_.end();) {
    ShareIdMap s_map = sit->second;
    s_map.clear();
    share_type_map_.erase(sit++);
  }
  ListValue::iterator lit = list->begin();
  for (; lit != list->end(); ++lit) {
    DicValue* dic = (DicValue*) (*lit);
    int64 type;
    dic->GetBigInteger(L"type_id_", &type);
    ShareIdMap map;
    share_type_map_[type] = map;
  }
  LOG(ERROR) << "DataShareMgr InitShareType unlock";
}

void DataShareMgr::ClearShareTourismMap() {
  LOG(ERROR) << "DataShareMgr ClearShareTourismMap lock";
  base_logic::WLockGd lk(lock_);
  LOG(ERROR) << "DataShareMgr ClearShareTourismMap locked";
  ShareTourismMap::iterator it = share_tourism_map_.begin();
  for (; it != share_tourism_map_.end();) {
    ShareTourism* share = it->second;
    share_tourism_map_.erase(it++);
    if (share != NULL) {
      delete share;
      share = NULL;
    }
  }
  recommend_share_map_.clear();
  LOG(ERROR) << "DataShareMgr ClearShareTourismMap unlock";
}

void DataShareMgr::ClearShareSkillMap() {
  LOG(ERROR) << "DataShareMgr ClearShareSkillMap lock";
  base_logic::WLockGd lk(lock_);
  LOG(ERROR) << "DataShareMgr ClearShareSkillMap locked";
  ShareSkillMap::iterator it = share_skill_map_.begin();
  for (; it != share_skill_map_.end();) {
    ShareSkill* share = it->second;
    share_skill_map_.erase(it++);
    if (share != NULL) {
      delete share;
      share = NULL;
    }
  }
  banner_share_map_.clear();
  LOG(ERROR) << "DataShareMgr ClearShareSkillMap unlock";
}

void DataShareMgr::InitTourismShare(ListValue* list) {
  //清空share_tourism_map_
  ClearShareTourismMap();
  LOG(ERROR) << "DataShareMgr InitTourismShare lock";
  base_logic::WLockGd lk(lock_);
  LOG(ERROR) << "DataShareMgr InitTourismShare locked";
  ListValue::iterator lit = list->begin();
  for (; lit != list->end(); ++lit) {
    DicValue* dic = (DicValue*) (*lit);
    if (dic != NULL) {
      ShareTourism* share = new ShareTourism();
      share->Serialization(dic);
      share_tourism_map_[share->share_id()] = share;
      if (share->is_recommend()) {
        recommend_share_map_[share->share_id()] = share->share_id();
      }
      ShareIdMap* tmp_map = &share_type_map_[share->share_type()];
      tmp_map->insert(
          ShareIdMap::value_type(share->share_id(), share->share_id()));
    }
  }
  LOG(ERROR) << "DataShareMgr InitTourismShare unlock";
}

void DataShareMgr::InitSkillShare(ListValue* list) {
  ClearShareSkillMap();
  LOG(ERROR) << "DataShareMgr InitSkillShare lock";
  base_logic::WLockGd lk(lock_);
  LOG(ERROR) << "DataShareMgr InitSkillShare locked";
  ListValue::iterator lit = list->begin();
    for (; lit != list->end(); ++lit) {
      DicValue* dic = (DicValue*) (*lit);
      if (dic != NULL) {
        ShareSkill* share = new ShareSkill();
        share->Serialization(dic);
        share_skill_map_[share->share_id()] = share;
        if (share->is_banner()) {
          banner_share_map_[share->share_id()] = share->share_id();
        }
      }
    }
  LOG(ERROR) << "DataShareMgr InitSkillShare unlock";
}

void DataShareMgr::test() {

}

//id 大的在前面   id 为上一页的最小id
int32 DataShareMgr::QueryRecommendShare(int64 id, int64 count, int64 type,
                                        DicValue* info) {
  LOG(ERROR) << "DataShareMgr QueryRecommendShare lock";
  base_logic::RLockGd lk(lock_);
  LOG(ERROR) << "DataShareMgr QueryRecommendShare locked";
  int32 err = 0;
  ListValue* list = new ListValue();
  ShareIdMap tem_map;
  if (type == 0)
    tem_map = recommend_share_map_;
  else
    tem_map = share_type_map_[type];
  do {
    ShareIdMap::reverse_iterator rit;
    if (id == 0) {
      rit = tem_map.rbegin();
    } else {
      ShareIdMap::iterator it = tem_map.find(id);
      ShareIdMap::reverse_iterator rfindit(it);
      if (it == tem_map.end())
        rit = tem_map.rend();
      else
        rit = rfindit++;
    }
    for (; rit != tem_map.rend() && count > 0; ++rit) {
      ShareTourismMap::iterator share_it = share_tourism_map_.find(rit->second);
      if (share_it != share_tourism_map_.end()) {
        ShareTourism* share = share_it->second;
        DicValue* dic = new DicValue();
        share->SetBriefSerialization(dic);
        list->Append(dic);
        count--;
      }
    }
  } while (0);
  info->Set(L"data_list", list);
  LOG(ERROR) << "DataShareMgr QueryRecommendShare unlock";
  return err;
}

int32 DataShareMgr::QuerySkillShare(int64 id, int64 count, DicValue* info) {
  LOG(ERROR) << "DataShareMgr QuerySkillShare lock";
  base_logic::WLockGd lk(lock_);
  LOG(ERROR) << "DataShareMgr QuerySkillShare locked";
  do {
    if (id == 0) {
      ListValue* banner_list = new ListValue();
      ShareIdMap::reverse_iterator banner_it = banner_share_map_.rbegin();
      for (; banner_it != banner_share_map_.rend(); ++banner_it) {
        int64 share_id = banner_it->second;
        ShareSkillMap::iterator sit = share_skill_map_.find(share_id);
        if (sit != share_skill_map_.end()) {
          ShareSkill* share = sit->second;
          if (share != NULL) {
            DicValue* banner_dic = new DicValue();
            share->SetBannerSerialization(banner_dic);
            banner_list->Append(banner_dic);
          }
        }
      }
      info->Set(L"banner_list", banner_list);
    }
    ShareSkillMap::reverse_iterator rit;
    if (id == 0) {
      rit = share_skill_map_.rbegin();
    } else {
      ShareSkillMap::iterator it = share_skill_map_.find(id);
      ShareSkillMap::reverse_iterator rfindit(it);
      if (it == share_skill_map_.end())
        rit = share_skill_map_.rend();
      else
        rit = rfindit++;
    }
    ListValue* data_list = new ListValue();
    for (; rit != share_skill_map_.rend() && count > 0; ++rit) {
      ShareSkill* share = rit->second;
      if (share != NULL) {
        DicValue* data_dic = new DicValue();
        share->SetBriefSerialization(data_dic);
        data_list->Append(data_dic);
        count--;
      }
    }
    info->Set(L"data_list", data_list);
  } while (0);

  LOG(ERROR) << "DataShareMgr QuerySkillShare unlock";
}

int32 DataShareMgr::QueryShareTourismDetail(int64 id, DicValue* dic) {
  LOG(ERROR) << "DataShareMgr QueryShareTourismDetail lock";
  base_logic::RLockGd lk(lock_);
  LOG(ERROR) << "DataShareMgr QueryShareTourismDetail locked";
  int32 err = 0;
  ShareTourismMap::iterator share_it = share_tourism_map_.find(id);
  if (share_it != share_tourism_map_.end()) {
    ShareTourism* share = share_it->second;
    share->SetDetailSerialization(dic);
  }
  LOG(ERROR) << "DataShareMgr QueryShareTourismDetail unlocked";
  return err;
}

int32 DataShareMgr::QueryShareSkillDetail(int64 id, DicValue* dic) {
  LOG(ERROR) << "DataShareMgr QueryShareSkillDetail lock";
  base_logic::RLockGd lk(lock_);
  LOG(ERROR) << "DataShareMgr QueryShareSkillDetail locked";
  int32 err = 0;
  DicValue * in = new DicValue();
  ShareSkillMap::iterator share_it = share_skill_map_.find(id);
  if (share_it != share_skill_map_.end()) {
    ShareSkill* share = share_it->second;
    share->SetDetailSerialization(in);
  }
  dic->Set(L"detail", in);
  LOG(ERROR) << "DataShareMgr QueryShareSkillDetail unlock";
  return err;
}

void DataShareMgr::SetImgToken(std::string token, int64 time) {
  img_token_ = token;
  token_time_ = time;
}

void DataShareMgr::GetImgToken(std::string*token, int64* time) {
  *token = img_token_;
  *time = token_time_;
}

}
// namespace share
