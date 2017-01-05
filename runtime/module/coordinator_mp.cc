#include "coordinator_mp.h"
#include "../actor/coordinator_messages.h"

#include <glog/logging.h>


void coordinator_mp::customized_init(coordinator* coordinator_actor){
  RegisterTask(nullptr);
  coordinator_actor_ = coordinator_actor;
  num_to_send = 100000000;
  successful_send = 0;
  unsuccessful_send= 0;
  send_end_flag = false;
}

struct task_result coordinator_mp::RunTask(void *arg){
  struct task_result ret;
  ret = (struct task_result){
      .packets = 0, .bits = 0,
  };

  if(coordinator_actor_->migration_target_rt_id_ != -1 && num_to_send>0){

    if(start_time == 0){
      start_time = ctx.current_ns();
    }

    ping_cstruct cstruct;
    cstruct.val = num_to_send;
    for(int i=0; i<32; i++){
      if(num_to_send==0){
        break;
      }

      bool flag = coordinator_actor_->reliables_.find(coordinator_actor_->migration_target_rt_id_)->second
                                    .reliable_send(77363, 1, 1, ping_t::value, &cstruct);
      num_to_send-=1;

      if(flag==false){
        unsuccessful_send+=1;
      }
      else{
        successful_send+=1;
      }
    }
  }

  if(coordinator_actor_->migration_target_rt_id_ != -1 && num_to_send==0 && send_end_flag==false){
    send_end_flag = true;
    end_time = ctx.current_ns();
    LOG(INFO)<<"Unsuccessful send "<<unsuccessful_send;
    LOG(INFO)<<"Successful send "<<successful_send;
    LOG(INFO)<<"The rtt is "
             <<coordinator_actor_->reliables_.find(coordinator_actor_->migration_target_rt_id_)->second.peek_rtt()
             <<"ns";
    uint64_t second = (end_time-start_time)/(1000*1000*1000);
    LOG(INFO)<<successful_send<<" messages sent in "<<second<<"s";
    LOG(INFO)<<"Reliable packet throughput : "<<(successful_send/second)<<"pkts/s";
  }

  return ret;
}

ADD_MODULE(coordinator_mp, "coordinator_mp", "send messages to another mp")
