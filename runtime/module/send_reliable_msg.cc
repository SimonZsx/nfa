//
#include "send_reliable_msg.h"

void send_reliable_msg::customized_init(coordinator* coordinator_actor){
  RegisterTask(nullptr);
  coordinator_actor_ = coordinator_actor;
}

struct task_result send_reliable_msg::RunTask(void *arg){
  struct task_result ret;
  ret = (struct task_result){
      .packets = 0, .bits = 0,
  };

  bess::PacketBatch batch;
  batch.clear();
  uint16_t out_gates[bess::PacketBatch::kMaxBurst];

  generic_list_item* first_item = coordinator_actor_->reliable_send_list_.peek_head();

  while( (batch.full() == false) && (first_item!=nullptr)){
    int residual_batch_size = bess::PacketBatch::kMaxBurst - batch.cnt();
    reliable_p2p& r = coordinator_actor_->reliables_.find(first_item->reliable_rtid)->second;

    if(first_item->pkt_num > residual_batch_size){
      bess::PacketBatch send_batch = r.get_send_batch(residual_batch_size);

      for(int i=batch.cnt(); i<batch.cnt()+send_batch.cnt(); i++){
        out_gates[i] = first_item->output_gate;
      }
      batch.CopyAddr(send_batch.pkts(), send_batch.cnt());

      first_item->pkt_num -= residual_batch_size;
    }
    else{
      bess::PacketBatch send_batch = r.get_send_batch(first_item->pkt_num);

      for(int i=batch.cnt(); i<batch.cnt()+send_batch.cnt(); i++){
        out_gates[i] = first_item->output_gate;
      }
      batch.CopyAddr(send_batch.pkts(), send_batch.cnt());

      coordinator_actor_->reliable_send_list_.pop_head();
      coordinator_actor_->get_list_item_allocator()->deallocate(first_item);

      first_item = coordinator_actor_->reliable_send_list_.peek_head();
    }
  }

  RunSplit(out_gates, &batch);

  return ret;
}
