#pragma once


#include "rpc.h"
#include "types.h"
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

/* Dependency: RPC
 */
class Central_manager {
    private:
        std::thread t1;
    private:
        /* Registration APIs handler
         */
        pType::node_ID reg_node(uint32_t _ip)
        {
            pType::node_ID __nID = m_node_id_helper.get_id();

            if (__nID != INVALID_ID) {
                m_node_map.emplace(std::piecewise_construct,
                        std::forward_as_tuple(__nID),
                        std::forward_as_tuple(__nID, _ip));
                //m_node_map[__nID] = node_db_map{__nID, _ip};
            }
            return __nID;
        }

        pType::db_ID reg_db(pType::node_ID _nID)
        {
            pType::db_ID __dbID = m_db_id_helper.get_id();
            node_db_map* _map = nullptr;


            if (__dbID != INVALID_ID) {
                auto iter  = m_node_map.find(_nID);
                if (iter != m_node_map.end()) {
                    _map = &iter->second;
                    _map->add_db(__dbID);
                    m_db_map[__dbID] = _map;

                    return __dbID;
                }
            }

            return false;
        }

        bool rem_node(pType::node_ID _nID)
        {
            /* Assuming vector dtor deletes/frees all db_metas
             */
            return m_node_map.erase(_nID);
        }
        bool rem_db(pType::db_ID _dbID)
        {
            auto iter = m_db_map.find(_dbID);
            node_db_map* _map = nullptr;

            if (iter != m_db_map.end()) {
                _map = iter->second;
                _map->rem_db(_dbID);

                m_db_map.erase(iter);

                return true;
            }

            return false;
        }

        bool register_event_handler(db_snapshot_t& _db_snap, action_t _action) 
        {
            switch (_action) {
                case NODE_ADD:
                    _db_snap.m_node = {reg_node(_db_snap.m_node.m_ip_addr), _db_snap.m_node.m_ip_addr};
                    break;
                case NODE_REM:
                    return rem_node(_db_snap.m_node.m_id);
                case DB_ADD:
                    _db_snap.m_db = {reg_db(_db_snap.m_node.m_id)};
                    break;
                case DB_REM:
                    return rem_db(_db_snap.m_db.m_id);
                default:
                    std::cout << "Event not supported\n";
            } 

            m_db_cbs[0](_db_snap.m_db, true);
            return true;
        }
        void dump_all_meta()
        {
            for (;;) {
                std::cout << "Dumping node map\n";
                for (auto& n_map: m_node_map) {
                    std::cout << "Node ID:" << n_map.first << "\n";
                    std::cout << n_map.second << "\n";
                }
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
        void periodic_meta_dump()
        {
            //t1 = std::thread(&Central_manager::dump_all_meta, this);
        }

    public:
        Central_manager(RPC_helper& m_rpc_helper, bool debug = false)
        {
            m_rpc_helper.register_push_cbs(std::bind(&Central_manager::register_event_handler, this, std::placeholders::_1, std::placeholders::_2));
            if (debug) {
                periodic_meta_dump();
            }
        }
        ~Central_manager()
        {
            std::cout << "Central manager destroyed\n";
            t1.join();
        }
        bool register_db_update_cb(std::function<bool(db_meta_t& _db, bool _add)>&& _cb)
        {
            m_db_cbs.push_back(std::move(_cb));

            return true;
        }
        db_snapshot_t get_db_snap(pType::db_ID& _dbID)
        {
            auto it = m_db_map.find(_dbID);
            if (it == m_db_map.end()) {
                return db_snapshot_t{};
            }

            return db_snapshot_t{it->second->m_node, db_meta_t{_dbID}};
        }

    private:
        std::unordered_map<pType::node_ID, node_db_map> m_node_map;
        std::unordered_map<pType::db_ID, node_db_map*> m_db_map;
        ID_helper<pType::node_ID> m_node_id_helper{50};
        ID_helper<pType::db_ID> m_db_id_helper{500};
        std::vector<std::function<bool(db_meta_t& _db, bool _add)>> m_db_cbs;
};
