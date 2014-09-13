//  Copyright © 2014 Wei Wang, Anh. All Rights Reserved.
//  2014-08-07 12:53

#ifndef COORDINATOR_H_
#define COORDINATOR_H_
#include <unordered_set>
#include <vector>
#include <map>
#include "net/net.h"
#include "core/global-table.h"
#include "utils/global_context.h"
#include "utils/network_thread.h"
#include "proto/model.pb.h"
#include "model_controller/model.h"


namespace lapis {
// represent the (table, shard) tuple
struct TaskId {
  int table;
  int shard;

  TaskId(int t, int s): table(t), shard(s) {}
};

//  each memory server has a set of (table,shard) partitions
//  assigned to it. shardId is the same for all partitions
struct ServerState {
  int server_id;
  int shard_id;
  std::unordered_set<TaskId *> local_shards;

  ServerState(int id): server_id(id), shard_id(-1) {}
};

class Coordinator {
 public:
  Coordinator();
  void Run(bool load_data, bool do_run);
  ~Coordinator();
 private:
  void InitDistributedStorage(bool load_data, bool do_run, const ModelProto& model);
  void InitTableServers(const std::map<int, GlobalTable*>& tables);
  void RunStandalone(const ModelProto& model);
  void RunOnCluster(const ModelProto& model);
  void LoadData(const DataSourceProtos& sources,
                const std::map<std::string, int>& stores);
  const StringIntMap CreateDataStores(const DataSourceProtos& sources, int fixed_server_id=-1);
  const ParamStorageConfig CreateParamStorage();
  const DataStorageConfig CreateDataStorage(const DataProto& data);
  bool DoValidationOn(int worker_id);
  void Shutdown();
 private:
  //  keep track of the table assignments, only to the memory servers
  std::vector<ServerState *> server_states_;
  shared_ptr<GlobalContext> context_;
  std::shared_ptr<NetworkThread> mpi_;
  ModelController mc_;
};
}  // namespace lapis
#endif  // COORDINATOR_H_
