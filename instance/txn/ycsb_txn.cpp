#include <cstring>
#include <set>
#include <vector>
#include <thread>
#include <future>
#include "ycsb_txn.h"

#include "common/buffer/record_buffer.h"
#include "common/index/index.h"
#include "common/storage/catalog.h"
#include "common/storage/row.h"
#include "common/storage/table.h"
#include "common/workload/ycsb.h"
#include "common/myhelper.h"
#include "instance/benchmarks/ycsb_query.h"
#include "instance/concurrency_control/row_mvcc.h"
#include "instance/thread.h"
#include "shared_disk_service.h"
#include "global_lock_service.h"
#include "global_lock_service_helper.h"
#include "util/shared_ptr_helper.h"

void ycsb_txn_man::init(thread_t *h_thd, workload *h_wl, uint64_t thd_id) {
    txn_man::init(h_thd, h_wl, thd_id);
    _wl = (ycsb_wl *) h_wl;
}
//
///**
// * zhangrongrong, 2020/6/27
// * for test
// * 该函数生成一个 query，共四个请求，所有请求的 key=1,2,3,4， type=WR，认为制造冲突，方便调试
// */
//ycsb_query *gen_test_query() {
//    ycsb_query *m_query = new ycsb_query();
//    m_query->requests = new ycsb_request[g_req_per_query]();
//    m_query->request_cnt = 4;
//    for (int i = 0; i < 4; i++) {
//        m_query->requests[i].key = i * 203;
//        m_query->requests[i].rtype = WR;
//    }
//    return m_query;
//}
//
///**
// * zhangrongrong, 2020/6/27
// * 对每个 ycsb_query，返回里面涉及到的写 page 集合
// */
//std::set<uint64_t> GetQueryWritePageSet(ycsb_query *m_query, ycsb_txn_man *ycsb) {
//    std::set<uint64_t> write_page_set;
//    for (uint32_t rid = 0; rid < m_query->request_cnt; rid++) {
//        ycsb_request *req = &m_query->requests[rid];
//        dbx1000::IndexItem indexItem;
//#ifdef DB2_WITH_NO_CONFLICT
//        ycsb->h_thd->manager_client_->index()->IndexGet(req->key + g_synth_table_size / MAX_PROCESS_CNT * ycsb->h_thd->manager_client_->instance_id(), &indexItem);
//#else
//        ycsb->h_thd->manager_client_->index()->IndexGet(req->key, &indexItem);
//#endif
//        if (req->rtype == WR) {
//            ycsb->h_thd->manager_client_->stats()._stats[ycsb->get_thd_id()]->count_write_request_++;
//            write_page_set.insert(indexItem.page_id_);
////            cout << "GetQueryWritePageSet:" << indexItem.page_id_ << endl;
//        }
//    }
//
//    /// 调试用
//    auto print_set = [&]() {
//        cout << "instance " << ycsb->h_thd->manager_client_->instance_id() << ", page_id : ";
//        for (auto iter : write_page_set) {
//            cout << iter << " ";
//        }
//        cout << endl;
//    };
////    print_set();
//
//    return write_page_set;
//}
//
//
///**
// * zhangrongrong, 2020/6/27
// * 先检查是否有其他 instance 需要对 write_page_set 内的 page 进行 Invalid, 有的话则等
// * 然后检查本地是否有 write_page_set 所有页的写锁，没有则 RemoteLock 获取，这个函数确保当前事务在后续过程中，对涉及到的页是有写权限的
// * 然后把当前线程 id 记录到 page 内，目的是为了阻塞事务开始后，其他实例的 invalid 请求
// */
//RC GetWritePageLock(std::set<uint64_t> write_page_set, ycsb_txn_man *ycsb) {
//    dbx1000::LockTable *lockTable = ycsb->h_thd->manager_client_->lock_table_i(TABLES::MAIN_TABLE);
//
//    auto has_invalid_req = [&]() {
//        for (auto iter : write_page_set) {
////            if(ycsb->h_thd->manager_client_->lock_table()->lock_table_[iter]->invalid_req == false ) {lockTable->AddThread(iter, ycsb->get_thd_id());}
//            if (ycsb->h_thd->manager_client_->lock_table_i(TABLES::MAIN_TABLE)->lock_table_[iter].first) { return true; }
//        }
//        return false;
//    };
//    while (has_invalid_req()) { std::this_thread::yield(); }
//    for (auto iter : write_page_set) { lockTable->AddThread(iter, ycsb->get_thd_id());}
////    for (auto iter : write_page_set) { assert(false == ycsb->h_thd->manager_client_->lock_table()->lock_table_[iter]->invalid_req); }
//
//
////    cout << "instance " << ycsb->h_thd->manager_client_->instance_id() << ", thread " << ycsb->get_thd_id() << ", txn " << ycsb->txn_id << " get lock." << endl;
//
//    dbx1000::Profiler profiler;
//    profiler.Start();
//
//    for (auto iter : write_page_set) {
//        if (lockTable->lock_table_[iter]->lock_mode == dbx1000::LockMode::O) {
//            ycsb->h_thd->manager_client_->stats()._stats[ycsb->get_thd_id()]->count_remote_lock_++;
//            bool flag = ATOM_CAS(lockTable->lock_table_[iter]->lock_remoting, false, true);
//            if (flag) {
//                assert(true == lockTable->lock_table_[iter]->lock_remoting);
//                char page_buf[MY_PAGE_SIZE];
////                    cout << "instance " << ycsb->h_thd->manager_client_->instance_id() << ", thread " << ycsb->get_thd_id() << ", txn " << ycsb->txn_id << " remote get lock." << endl;
//#ifdef DB2_WITH_NO_CONFLICT
//                RC rc = ycsb->h_thd->manager_client_->global_lock_service_client()->LockRemote(ycsb->h_thd->manager_client_->instance_id(), iter, dbx1000::LockMode::X, nullptr, 0);
//#else
//                RC rc = ycsb->h_thd->manager_client_->global_lock_service_client()->LockRemote(
//                        ycsb->h_thd->manager_client_->instance_id(), iter, dbx1000::LockMode::X, page_buf
//                        , MY_PAGE_SIZE);
//#endif
//                if (RC::Abort == rc || RC::TIME_OUT == rc) {
//                    lockTable->lock_table_[iter]->lock_remoting = false;
//                    lockTable->lock_table_[iter]->remote_locking_abort.store(true);
//                    return RC::Abort;
//                }
//                assert(rc == RC::RCOK);
//                assert(lockTable->lock_table_[iter]->lock_mode == dbx1000::LockMode::O);
//                lockTable->lock_table_[iter]->lock_mode = dbx1000::LockMode::P;
////                    cout << "thread : " << ycsb->get_thd_id() << " change lock : " << iter << " to P" << endl;
//#ifdef DB2_WITH_NO_CONFLICT
//#else
//                ycsb->h_thd->manager_client_->buffer()->BufferPut(iter, page_buf, MY_PAGE_SIZE);
//#endif
//                lockTable->lock_table_[iter]->lock_remoting = false;
//            } else {
//                assert(true == lockTable->lock_table_[iter]->lock_remoting);
//                while (lockTable->lock_table_[iter]->lock_mode == dbx1000::LockMode::O) {
//                    if(true == lockTable->lock_table_[iter]->remote_locking_abort.load()){
//                        return RC::Abort;
//                    }
////                    cout << "thread waiting..." << endl; std::this_thread::yield();
//                }
//                assert(lockTable->lock_table_[iter]->lock_mode != dbx1000::LockMode::O);
//            }
//        }
//    }
//
//    /// 把该线程请求加入 page 锁, 阻塞事务开始后，其他实例的 invalid 请求
//    for (auto iter : write_page_set) {
////        lockTable->AddThread(iter, ycsb->get_thd_id());
//        assert(lockTable->lock_table_[iter]->lock_mode != dbx1000::LockMode::O);
//    }
//    profiler.End();
//    ycsb->h_thd->manager_client_->stats().tmp_stats[ycsb->get_thd_id()]->time_remote_lock_ += profiler.Nanos();
//
//    return RC::RCOK;
//}
//
//RC AsyncGetWritePageLock(std::set<uint64_t> write_page_set, ycsb_txn_man *ycsb) {
//    dbx1000::LockTable *lockTable = ycsb->h_thd->manager_client_->lock_table();
//
//    auto has_invalid_req = [&]() {
//        for (auto iter : write_page_set) {
//            if (ycsb->h_thd->manager_client_->lock_table()->lock_table_[iter]->invalid_req) { return true; }
//        }
//        return false;
//    };
//    /// 事务开始前，等待其他节点 Invalid 完成
//    while (has_invalid_req()) { std::this_thread::yield(); }
//    /// 把该线程请求加入 page 锁, 阻塞事务开始后，其他实例的 invalid 请求
//    for (auto iter : write_page_set) { lockTable->AddThread(iter, ycsb->get_thd_id());}
//
//    dbx1000::Profiler profiler;
//    profiler.Start();
//
//    /// remote_lock_page_set 为需要 remote_lock 的集合，wait_lock_page_set 等待其他线程 remote_lock 的集合
//    std::set<uint64_t> remote_lock_page_set;
//    std::set<uint64_t> wait_lock_page_set;
//    for(auto iter : write_page_set) {
//        if(lockTable->lock_table_[iter]->lock_mode == dbx1000::LockMode::O) {
//            ycsb->h_thd->manager_client_->stats()._stats[ycsb->get_thd_id()]->count_remote_lock_++;
//            bool flag = ATOM_CAS(lockTable->lock_table_[iter]->lock_remoting, false, true);
//            if(flag) {
//                /// 多个线程不能同时对某个 page 去 Remote lock, 要先抢占 lock_remoting
//                remote_lock_page_set.insert(iter);
//            } else {
//                /// 没有抢到 lock_remoting，等待其他线程 Remote lock
//                wait_lock_page_set.insert(iter);
//            }
//        }
//    }
//
//    /// 等待 wait_lock_page_set 完成
//    auto wait_th = [&lockTable, wait_lock_page_set]() {
//        RC rc = RC::RCOK;
//        for (auto iter : wait_lock_page_set) {
////            assert(true == lockTable->lock_table_[iter]->lock_remoting);
//            if(true == lockTable->lock_table_[iter]->lock_remoting) {
//                rc = RC::Abort;
//                return rc;
//            }
//            while (lockTable->lock_table_[iter]->lock_mode == dbx1000::LockMode::O) {
//                if (true == lockTable->lock_table_[iter]->remote_locking_abort.load()) {
//                    rc = RC::Abort;
//                    return rc;
//                }
//                this_thread::yield();
//            }
//            assert(lockTable->lock_table_[iter]->lock_mode != dbx1000::LockMode::O);
//        }
//        return rc;
//    };
//
//    std::future<RC> rc_fut = std::async(wait_th);
//
//    /// async remote locking
//    std::vector<dbx1000::global_lock_service::OnLockRemoteDone*> dones;
//    dones.resize(remote_lock_page_set.size());
//    int count = 0;
//    for(auto iter : remote_lock_page_set) {
//        char* page_buf = new char[MY_PAGE_SIZE];
//        dones[count] = new dbx1000::global_lock_service::OnLockRemoteDone();
//        ycsb->h_thd->manager_client_->global_lock_service_client()->AsyncLockRemote(ycsb->h_thd->manager_client_->instance_id(), iter, dbx1000::LockMode::X, page_buf
//                , MY_PAGE_SIZE, dones[count]);
//        count++;
//    }
//
//    count = 0;
//    /// join remote locking
//    for(auto iter : remote_lock_page_set) {
//        brpc::Join(dones[count]->cntl.call_id());
//        lockTable->lock_table_[iter]->lock_remoting = false;
//        count++;
//    }
//
//    RC rc = rc_fut.get();
//    if(rc == RC::Abort) { return RC::Abort; }
//
//    count = 0;
//    for(auto iter : remote_lock_page_set) {
//        rc = dbx1000::global_lock_service::GlobalLockServiceHelper::DeSerializeRC(dones[count]->reply.rc());
//        /// remote lock 失败
//        if(rc == RC::TIME_OUT || rc == RC::Abort) {
//            lockTable->lock_table_[iter]->remote_locking_abort.store(true);
//            rc = RC::Abort;
//            break;
//        }
//        /// remote lock 成功
//        assert(rc == RC::RCOK);
//        assert(lockTable->lock_table_[iter]->lock_mode == dbx1000::LockMode::O);
//        lockTable->lock_table_[iter]->lock_mode = dbx1000::LockMode::P;
////                    cout << "thread : " << ycsb->get_thd_id() << " change lock : " << iter << " to P" << endl;
//#ifdef DB2_WITH_NO_CONFLICT
//#else
//        ycsb->h_thd->manager_client_->buffer()->BufferPut(iter, dones[count]->page_buf, MY_PAGE_SIZE);
//        delete dones[count]->page_buf;
//#endif
//        count++;
//    }
//
//    count = 0;
//    for(auto iter : remote_lock_page_set) {
//        delete dones[count];
//        count++;
//    }
//
//    if(rc == RC::Abort) { return rc;}
//
//    /// 把该线程请求加入 page 锁, 阻塞事务开始后，其他实例的 invalid 请求
//    for (auto iter : write_page_set) {
//        assert(lockTable->lock_table_[iter]->lock_mode != dbx1000::LockMode::O);
//    }
//    profiler.End();
//    ycsb->h_thd->manager_client_->stats().tmp_stats[ycsb->get_thd_id()]->time_remote_lock_ += profiler.Nanos();
//
//    return RC::RCOK;
//}

RC ycsb_txn_man::run_txn(base_query *query) {
    if(txn_id % 1000 == 0) {
        cout << "instance id: " << h_thd->manager_client_->instance_id_ << ", txn id : " << txn_id << endl;
    }
//    cout << "txn id : " << txn_id << endl;
//    cout << "instance " << h_thd->manager_client_->instance_id() << ", thread " << this->get_thd_id() << ", txn " << this->txn_id << " start." << endl;
    RC rc;
    ycsb_query *m_query = (ycsb_query *) query;

    GetLockTableSharedPtrs(m_query);
    /// 事务开始前，记录下写 page 集合，同时确保这些写page的锁在本地（若不在本地，则通过 RemoteLock 获取）
    /// ,然后把线程 id 记录到 page 内，目的是为了阻塞事务开始后，其他实例的 invalid 请求
    std::set<uint64_t> write_record_set = GetWriteRecordSet(m_query);
    rc = GetWriteRecordLock(write_record_set, m_query);
    if (rc == RC::Abort) {
        for (auto iter : write_record_set) { lock_node_maps_[TABLES::MAIN_TABLE][iter]->RemoveThread(this->get_thd_id()); }
        lock_node_maps_.clear();
        return rc;
    }

    /* ycsb_wl * wl = (ycsb_wl *) h_wl;
    itemid_t * m_item = NULL; */
    row_cnt = 0;


    for (uint32_t rid = 0; rid < m_query->request_cnt; rid++) {
        h_thd->manager_client_->stats_._stats[get_thd_id()]->count_total_request_++;
        ycsb_request *req = &m_query->requests[rid];
        /* int part_id = wl->key_to_part( req->key ); */      //! 分区数为 1，part_id == 0
        //! finish_req、iteration 是为 req->rtype==SCAN 准备的，扫描需要读 SCAN_LEN 个 item，
        //! while 虽然为 SCAN 提供了 SCAN_LEN 次读，但是每次请求的 key 是一样的，并没有对操作 [key, key + SCAN_LEN]
        bool finish_req = false;
        uint32_t iteration = 0;
        while (!finish_req) {
            /*
            if (iteration == 0) {
                m_item = index_read(_wl->the_index, req->key, part_id);
            }
#if INDEX_STRUCT == IDX_BTREE
            else {
                _wl->the_index->index_next(get_thd_id(), m_item);
                if (m_item == NULL)
                    break;
            }
#endif
            row_t * row = ((row_t *)m_item->location);
             */
            row_t *row_local;
            access_t type = req->rtype;

            /**
             * 为了实现多个进程访问的数据没有重叠，没有冲突时，每个进程只访问 key:[g_synth_table_size/64*this->process_id, g_synth_table_size/MAX_PROCESS_CNT*(this->process_id+1)] 范围内的数据
             * 但是，query 模块没有对应进程的 id, 所以产生的 key 范围只在 [0, g_synth_table_size/64]
             * 在事务执行时，应该加上 : g_synth_table_size / MAX_PROCESS_CNT * process_id;
             *
             * 为什么是 MAX_PROCESS_CNT，而不是 PROCESS_CNT？
             * PROCESS_CNT 下，随着 PROCESS_CNT 增大， key 范围缩小，会导致每个节点内事务同时访问的 page_id  的概率增大
             * MAX_PROCESS_CNT 是预估最大达到的实例数
             */
#ifdef DB2_WITH_NO_CONFLICT
            assert(h_thd->manager_client_->instance_id() < MAX_PROCESS_CNT);
            assert(req->key < g_synth_table_size / MAX_PROCESS_CNT);
            uint64_t real_key = req->key + g_synth_table_size / MAX_PROCESS_CNT * h_thd->manager_client_->instance_id();
            assert(real_key < g_synth_table_size);
            row_local = get_row(real_key , type);
#else
            GetMvccSharedPtr(TABLES::MAIN_TABLE, req->key);
            row_local = get_row(TABLES::MAIN_TABLE, req->key, type);
//			cout << "key: " << req->key << ", type:" << (req->rtype == access_t::WR ? "WR" : "RD") << endl;
#endif
            if (row_local == NULL) {
                rc = RC::Abort;
                goto final;
            }
            size_t size = _wl->the_table->get_schema()->tuple_size;
            assert(row_local->get_tuple_size() == size);

            // Computation //
            // Only do computation when there are more than 1 requests.
            if (m_query->request_cnt > 1) {
                if (req->rtype == RD || req->rtype == SCAN) {
//                  for (int fid = 0; fid < schema->get_field_cnt(); fid++) {
                    int fid = 0;
                    char *data = row_local->get_data();
                    __attribute__((unused)) uint64_t fval = *(uint64_t *) (&data[fid * 10]);
//                  }
                } else {
                    assert(req->rtype == WR);
//					for (int fid = 0; fid < schema->get_field_cnt(); fid++) {
                    int fid = 0;
                    char *data = row_local->get_data();
                    /* *(uint64_t *)(&data[fid * 10]) = 0; */
                    /// 写入事务 id, 当做版本号
                    memcpy(&data[size - sizeof(uint64_t)], &timestamp, sizeof(uint64_t));
//					}
                }
            }
            iteration++;
            if (req->rtype == RD || req->rtype == WR || iteration == req->scan_len)
                finish_req = true;
        }
    }
    rc = RC::RCOK;

    final:
    rc = finish(rc);

#if defined(B_M_L_R)
    this_thread::sleep_for(chrono::microseconds(105) * write_record_set.size());
#elif defined(B_P_L_R)
    this_thread::sleep_for(chrono::microseconds(130) * write_record_set.size());
#endif

    /// 线程结束后，把对应 page 锁内的相关信息清除，通知 invalid 函数可以执行
    for (auto iter : write_record_set) { lock_node_maps_[TABLES::MAIN_TABLE][iter]->RemoveThread(this->get_thd_id()); }
    lock_node_maps_.clear();
//    cout << "instance " << h_thd->manager_client_->instance_id() << ", thread " << this->get_thd_id() << ", txn " << this->txn_id << " end." << endl;

    return rc;
}



void ycsb_txn_man::GetLockTableSharedPtrs(ycsb_query *m_query) {
    auto lock_table = this->h_thd->manager_client_->lock_table_[TABLES::MAIN_TABLE];
    for (uint32_t rid = 0; rid < m_query->request_cnt; rid++) {
        ycsb_request *req = &m_query->requests[rid];
#if defined(B_M_L_P) || defined(B_P_L_P)
        uint64_t page_id = req->key/(16384 / ((ycsb_wl*)this->h_wl)->the_table->get_schema()->get_tuple_size());
        this->lock_node_maps_[TABLES::MAIN_TABLE].insert(make_pair(page_id, GetOrCreateSharedPtr<dbx1000::LockNode>(lock_table->lock_table_, page_id)));
#else
        this->lock_node_maps_[TABLES::MAIN_TABLE].insert(make_pair(req->key, GetOrCreateSharedPtr<dbx1000::LockNode>(lock_table->lock_table_, req->key)));
#endif

    }
}

std::set<uint64_t> ycsb_txn_man::GetWriteRecordSet(ycsb_query *m_query) {
    std::set<uint64_t> write_record_set;
    for (uint32_t rid = 0; rid < m_query->request_cnt; rid++) {
        ycsb_request *req = &m_query->requests[rid];
        if (req->rtype == WR) {
#if defined(B_M_L_P) || defined(B_P_L_P)
            write_record_set.insert(req->key/(16384 / ((ycsb_wl*)this->h_wl)->the_table->get_schema()->get_tuple_size()));
#else
            write_record_set.insert(req->key);
#endif
        }
    }
    this->h_thd->manager_client_->stats_._stats[this->get_thd_id()]->count_write_request_ += write_record_set.size();
    return write_record_set;
}

RC ycsb_txn_man::GetWriteRecordLock(std::set<uint64_t> &write_record_set, ycsb_query *m_query) {
    dbx1000::LockTable *lockTable = this->h_thd->manager_client_->lock_table_[TABLES::MAIN_TABLE];

    /// 等待其他节点 RemoteInvalid 完成
    auto has_invalid_req = [&]() {
        for (auto iter : write_record_set) {
            if (lock_node_maps_[TABLES::MAIN_TABLE][iter]->invalid_req) { return true; }
        }
        return false;
    };
    while (has_invalid_req()) { std::this_thread::yield(); }
    for (auto iter : write_record_set) { lock_node_maps_[TABLES::MAIN_TABLE][iter]->AddThread(this->get_thd_id());}
    /// 当前线程开始后，阻塞其他节点的 RemoteInvalid

    dbx1000::Profiler profiler;
    profiler.Start();

    for (auto iter : write_record_set) {
        std::shared_ptr<dbx1000::LockNode> lockNode = lock_node_maps_[TABLES::MAIN_TABLE][iter];

        /// 本地没有锁权限
        if (!lockTable->IsValid(iter)) {
            this->h_thd->manager_client_->stats_._stats[this->get_thd_id()]->count_remote_lock_++;

            bool flag = ATOM_CAS(lockNode->lock_remoting, false, true);
            if (flag) {
                /// 当前线程去 RemoteLock
                assert(true == lockNode->lock_remoting);
                uint32_t tuple_size = ((ycsb_wl *) this->h_thd->manager_client_->m_workload_)->the_table->get_schema()->tuple_size;
                char record_buf[tuple_size];
                RC rc = this->h_thd->manager_client_->global_lock_service_client_->LockRemote(
                        this->h_thd->manager_client_->instance_id_, TABLES::MAIN_TABLE, iter, dbx1000::LockMode::X, record_buf , tuple_size);

                if (RC::Abort == rc || RC::TIME_OUT == rc) {
                    lockNode->lock_remoting = false;
                    lockNode->remote_locking_abort.store(true);
                    return RC::Abort;
                }
                assert(rc == RC::RCOK);
                lockTable->Valid(iter);
//                assert(lockTable->IsValid(iter));
                row_t* temp_row = new row_t();
                ycsb_wl* wl = (ycsb_wl*) this->h_thd->manager_client_->m_workload_;
                temp_row->init(wl->the_table);
                temp_row->set_primary_key(iter);
                memcpy(temp_row->data, record_buf, tuple_size);
                /// TODO，拿回来的最新值写入缓存
                this->h_thd->manager_client_->m_workload_->buffers_[TABLES::MAIN_TABLE]->BufferPut(iter, temp_row->data, temp_row->get_tuple_size());
                lockNode->lock_remoting = false;
            } else {
                /// 其他线线程去 RemoteLock，要么成功拿到锁，要么此次调用失败 remote_locking_abort==true
//                assert(true == lockNode->lock_remoting);
                while (!lockTable->IsValid(iter)) {
                    if(lockNode->remote_locking_abort.load()){
                        return RC::Abort;
                    }
                }
//                assert(lockTable->IsValid(iter));
            }
        }
    }

    /// 把该线程请求加入 page 锁, 阻塞事务开始后，其他实例的 invalid 请求
//    for (auto iter : write_record_set) { assert(lockTable->IsValid(iter)); }
    profiler.End();
    this->h_thd->manager_client_->stats_.tmp_stats[this->get_thd_id()]->time_remote_lock_ += profiler.Nanos();

    return RC::RCOK;
}
