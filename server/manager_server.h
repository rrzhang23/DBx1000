//
// Created by rrzhang on 2020/4/26.
//

#ifndef DBX1000_MANAGER_SERVER_H
#define DBX1000_MANAGER_SERVER_H

#include <atomic>
#include <unordered_map>
#include <mutex>
#include <memory>
#include "api.pb.h"
//#include "global.h"

class Row_mvcc;
//class workload;
class ycsb_wl;

namespace dbx1000 {

    class TxnRowMan;
    class RowItem;
    class Buffer;

    class ManagerServer {
    public:
        ManagerServer();
        void InitWl(ycsb_wl* wl);

        uint64_t get_next_ts(uint64_t thread_id);     /// 获取下个一个时间戳
        void add_ts(uint64_t thread_id, uint64_t ts);
        uint64_t GetMinTs(uint64_t thread_id = 0);

        TxnRowMan* SetTxn(Mess_TxnRowMan messTxnRowMan);

        bool AllTxnReady();

//    private:
        std::unordered_map<uint64_t, Row_mvcc*> row_mvccs_;
        std::mutex row_mvccs_mutex_;                        // TODO : 尝试不用锁

        std::atomic<uint64_t> timestamp_;
//        uint64_t volatile *volatile *volatile all_ts_;
        uint64_t*   all_ts_;
        uint64_t    min_ts_;
        TxnRowMan* all_txns_;
        ycsb_wl* wl_;

        bool* txn_ready_;
        bool init_wl_done_;
    };
}


#endif //DBX1000_MANAGER_SERVER_H
