#include "query.h"
#include "ycsb.h"
#include "ycsb_query.h"

#include "global.h"
#include "helper.h"
#include "wl.h"
#include "thread.h"

#include "table.h"
#include "row.h"
#include "index_hash.h"
//#include "index_btree.h"
#include "catalog.h"
#include "manager.h"
//#include "row_lock.h"
//#include "row_ts.h"
#include "row_mvcc.h"
#include "mem_alloc.h"

void ycsb_txn_man::init(thread_t * h_thd, workload * h_wl, uint64_t thd_id) {
	txn_man::init(h_thd, h_wl, thd_id);
	_wl = (ycsb_wl *) h_wl;
}

RC ycsb_txn_man::run_txn(base_query * query) {
	RC rc;
	ycsb_query * m_query = (ycsb_query *) query;
	ycsb_wl * wl = (ycsb_wl *) h_wl;
	itemid_t * m_item = NULL;
  	row_cnt = 0;

	for (uint32_t rid = 0; rid < m_query->request_cnt; rid ++) {
		ycsb_request * req = &m_query->requests[rid];
		int part_id = wl->key_to_part( req->key );      //! 分区数为 1，part_id == 0
		//! finish_req、iteration 是为 req->rtype==SCAN 准备的，扫描需要读 SCAN_LEN 个 item，
		//! while 虽然为 SCAN 提供了 SCAN_LEN 次读，但是每次请求的 key 是一样的，并没有对操作 [key, key + SCAN_LEN]
		bool finish_req = false;
		uint32_t iteration = 0;
		while ( !finish_req ) {
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
			row_t * row_local; 
			access_t type = req->rtype;
			
			row_local = get_row(row, type);
			if (row_local == NULL) {
				rc = Abort;
				goto final;
			}

			// Computation //
			// Only do computation when there are more than 1 requests.
            if (m_query->request_cnt > 1) {
                if (req->rtype == RD || req->rtype == SCAN) {
//                  for (int fid = 0; fid < schema->get_field_cnt(); fid++) {
						int fid = 0;
						char * data = row_local->get_data();
						__attribute__((unused)) uint64_t fval = *(uint64_t *)(&data[fid * 10]);
//                  }
                } else {
                    assert(req->rtype == WR);
//					for (int fid = 0; fid < schema->get_field_cnt(); fid++) {
						int fid = 0;
						char * data = row->get_data();
						*(uint64_t *)(&data[fid * 10]) = 0;
//					}
                } 
            }


			iteration ++;
			if (req->rtype == RD || req->rtype == WR || iteration == req->scan_len)
				finish_req = true;
		}
	}
	rc = RCOK;
final:
	rc = finish(rc);
	return rc;
}

