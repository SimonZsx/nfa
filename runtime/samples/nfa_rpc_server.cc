#include <thread>
#include <chrono>

#include <glog/logging.h>

#include "../nfaflags.h"
#include "../nfadpdk.h"
#include "../rpc/server_impl.h"

using namespace std;

int main(int argc, char* argv[]){
  google::ParseCommandLineFlags(&argc, &argv, true);
  FLAGS_logtostderr = 1;
  google::InitGoogleLogging(argv[0]);

  nfa_init_dpdk(argv[0]);

  const int llring_size = 1024;
  int bytes_per_llring = llring_bytes_with_slots(llring_size);
  struct llring* rpc2worker_ring = static_cast<struct llring*>(rte_zmalloc(nullptr, bytes_per_llring, 0));
  struct llring* worker2rpc_ring = static_cast<struct llring*>(rte_zmalloc(nullptr, bytes_per_llring, 0));
  llring_init(rpc2worker_ring, llring_size, 0, 0);
  llring_set_water_mark(rpc2worker_ring, ((llring_size >> 3) * 7));
  llring_init(worker2rpc_ring, llring_size, 0, 0);
  llring_set_water_mark(worker2rpc_ring, ((llring_size >> 3) * 7));

  ServerImpl rpc_server(rpc2worker_ring, worker2rpc_ring);
  rpc_server.Run("127.0.0.1", 50051);

  auto rpc_thread_fun = [](ServerImpl &rpc_server, set<int> cpu_set, int lcore_id,
                           std::atomic<bool>& rpc_server_thread_ready){
    rpc_server.HandleRpcs(std::move(cpu_set), lcore_id, rpc_server_thread_ready);
  };

  set<int> cpu_set = {5};
  std::atomic<bool> rpc_server_thread_ready(false);
  thread rpc_thread(rpc_thread_fun, std::ref(rpc_server), std::move(cpu_set), 2, std::ref(rpc_server_thread_ready));
  rpc_thread.detach();

  while(rpc_server_thread_ready.load() == false){
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  LOG(INFO)<<"server thread runs";

  while(true){
    std::this_thread::sleep_for(std::chrono::seconds(50));
  }
}
